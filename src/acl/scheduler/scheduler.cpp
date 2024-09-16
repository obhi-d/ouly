
#include <acl/math/vml_fcn.hpp>
#include <acl/scheduler/scheduler.hpp>
#include <acl/scheduler/task.hpp>
#include <latch>
#include <numeric>

namespace acl
{

thread_local detail::worker const* g_worker = nullptr;

worker_context const& worker_context::get(workgroup_id group) noexcept
{
  return g_worker->contexts[group.get_index()];
}

worker_id const& worker_id::get() noexcept
{
  return g_worker->id;
}

scheduler::~scheduler() noexcept
{
  if (!stop.load())
    end_execution();
}

inline void scheduler::do_work(worker_id thread, detail::work_item const& work) noexcept
{
  work.delegate_fn(work.data, workers[thread.get_index()].contexts[work.data.get_workgroup_id()]);
}

void scheduler::busy_work(worker_id thread) noexcept
{
  {
    auto& lw = local_work[thread.get_index()];
    if (lw.delegate_fn)
    {
      do_work(thread, lw);
      lw.delegate_fn = nullptr;
      return;
    }
  }

  work(thread);
}

void scheduler::run(worker_id thread)
{
  g_worker = &workers[thread.get_index()];

  entry_fn(worker_desc(thread, group_ranges[thread.get_index()].mask));

  while (true)
  {
    {
      auto& lw = local_work[thread.get_index()];
      if (lw.delegate_fn)
      {
        do_work(thread, lw);
        lw.delegate_fn = nullptr;
      }
    }

    while (work(thread))
      ;

    if (stop.load(std::memory_order_seq_cst))
      break;

    wake_status[thread.get_index()].store(false);
    wake_events[thread.get_index()].wait();
  }

  workers[thread.get_index()].quitting.store(true);
}

inline bool scheduler::work(worker_id thread) noexcept
{
  auto wrk = get_work(thread);
  if (!wrk.delegate_fn)
    return false;
  assert(&workers[thread.get_index()].contexts[wrk.data.get_workgroup_id()].get_scheduler() == this);
  do_work(thread, wrk);
  return true;
}

detail::work_item scheduler::get_work(worker_id thread) noexcept
{
  auto const& range = group_ranges[thread.get_index()];

  // try to get work from own queue
  for (uint32_t start = 0; start < range.count; ++start)
  {
    auto  group_id = range.priority_order[start];
    auto& group    = workgroups[group_id];
    for (uint32_t queue_idx = 0, queue_end = group.thread_count - 1; queue_idx <= queue_end; ++queue_idx)
    {
      auto& queue = group.work_queues[(thread.get_index() - group.start_thread_idx + queue_idx) & queue_end];
      if (queue.first.try_lock())
      {
        if (!queue.second.empty())
        {
          auto item = queue.second.pop_front_unsafe();
          queue.first.unlock();
          return item;
        }
        queue.first.unlock();
      }
    }
  }

  // Exclusive
  {
    auto& worker    = workers[thread.get_index()];
    auto& work_list = worker.exlusive_items;
    auto  lck       = std::scoped_lock(work_list.first);
    if (!work_list.second.empty())
      return work_list.second.pop_front_unsafe();
  }

  return {};
}

void scheduler::wake_up(worker_id thread) noexcept
{
  bool sleeping = true;
  if (!wake_status[thread.get_index()].exchange(true))
  {
    wake_events[thread.get_index()].notify();
    return;
  }
}

void scheduler::begin_execution(scheduler_worker_entry&& entry)
{
  local_work   = std::make_unique<detail::work_item[]>(worker_count);
  workers      = std::make_unique<detail::worker[]>(worker_count);
  group_ranges = std::make_unique<detail::group_range[]>(worker_count);
  wake_status  = std::make_unique<std::atomic_bool[]>(worker_count);
  wake_events  = std::make_unique<detail::wake_event[]>(worker_count);
  workers      = std::make_unique<detail::worker[]>(worker_count);

  threads.reserve(worker_count - 1);

  auto wgroup_count = static_cast<uint32_t>(workgroups.size());

  for (uint32_t group = 0; group < wgroup_count; ++group)
  {
    auto const& g = workgroups[group];

    for (uint32_t i = g.start_thread_idx, end = g.thread_count + i; i < end; ++i)
    {
      auto& worker = workers[i];
      auto& range  = group_ranges[i];
      range.mask |= 1u << group;
      range.priority_order[range.count++] = static_cast<uint8_t>(group);
    }
  }

  for (uint32_t w = 0; w < worker_count; ++w)
  {
    auto& worker = workers[w];
    worker.id    = worker_id(w);
    auto& range  = group_ranges[w];

    std::sort(range.priority_order.data(), range.priority_order.data() + range.count,
              [&](uint8_t first, uint8_t second)
              {
                return workgroups[first].priority == workgroups[second].priority
                         ? first < second
                         : workgroups[first].priority > workgroups[second].priority;
              });

    worker.contexts = std::make_unique<worker_context[]>(wgroup_count);
    for (uint32_t g = 0; g < wgroup_count; ++g)
      worker.contexts[g] =
        worker_context(*this, worker_id(w), workgroup_id(g), group_ranges[w].mask, w - workgroups[g].start_thread_idx);
    wake_status[w].store(true);
  }

  stop               = false;
  auto start_counter = std::latch(worker_count);

  entry_fn = [cust_entry = std::move(entry), &start_counter](worker_desc worker)
  {
    if (cust_entry)
      cust_entry(worker);
    start_counter.count_down();
  };

  entry_fn(worker_desc(worker_id(0), group_ranges[0].mask));

  for (uint32_t thread = 1; thread < worker_count; ++thread)
  {
    threads.emplace_back(&scheduler::run, this, worker_id(thread));
  }

  g_worker = &workers[0];
  start_counter.wait();
  entry_fn = {};
}

void scheduler::take_ownership() noexcept
{
  g_worker = &workers[0];
}

void scheduler::finish_pending_tasks() noexcept
{

  bool has_work = false;
  do
  {
    has_work = false;
    for (auto& group : workgroups)
    {
      bool has_items = false;
      for (uint32_t q = 0; q < group.thread_count; ++q)
      {
        auto lck = std::scoped_lock(group.work_queues[q].first);
        has_items |= !group.work_queues[q].second.empty();
      }
      if (has_items)
      {
        for (uint32_t w = group.start_thread_idx, end = w + group.thread_count; w < end; ++w)
          wake_up(worker_id(w));
      }

      has_work |= has_items;
    }

    for (uint32_t w = 0; w < worker_count; ++w)
    {
      auto& worker    = workers[w];
      bool  has_items = false;
      {
        auto lck  = std::scoped_lock(worker.exlusive_items.first);
        has_items = !worker.exlusive_items.second.empty();
      }

      has_work |= has_items;
      if (!has_items)
        wake_up(worker_id(w));
    }
  }
  while (has_work);
}

void scheduler::end_execution()
{
  finish_pending_tasks();
  stop = true;
  for (uint32_t thread = 1; thread < worker_count; ++thread)
  {
    while (!workers[thread].quitting.load())
      wake_up(worker_id(thread));
    threads[thread - 1].join();
  }
  threads.clear();
}

void scheduler::submit(worker_id src, worker_id dst, detail::work_item const& work)
{
  if (src == dst)
    do_work(src, work);
  else
  {
    {
      auto& worker = workers[dst.get_index()];
      auto  lck    = std::scoped_lock(worker.exlusive_items.first);
      worker.exlusive_items.second.emplace_back(std::move(work));
    }
    if (!wake_status[dst.get_index()].exchange(true))
      wake_events[dst.get_index()].notify();
  }
}

void scheduler::submit(worker_id src, workgroup_id dst, detail::work_item const& work)
{
  auto& wg     = workgroups[dst.get_index()];
  auto& worker = workers[src.get_index()];

  for (uint32_t i = wg.start_thread_idx, end = i + wg.thread_count; i != end; ++i)
  {
    bool sleeping = true;
    if (!wake_status[i].exchange(true))
    {
      local_work[i] = work;
      wake_events[i].notify();
      return;
    }
  }

  while (true)
  {
    wg.push_offset++;
    for (uint32_t i = 0, end = wg.thread_count - 1; i <= end; ++i)
    {
      uint32_t q     = (wg.push_offset + i) & end;
      auto&    queue = wg.work_queues[q];
      if (queue.first.try_lock())
      {
        queue.second.emplace_back(std::move(work));
        queue.first.unlock();
        q += wg.start_thread_idx;
        if (!wake_status[q].exchange(true))
          wake_events[q].notify();
        return;
      }
    }
  }
}

void scheduler::create_group(workgroup_id group, uint32_t thread_offset, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= workgroups.size())
    workgroups.resize(group.get_index() + 1);
  // thread_count = acl::next_pow2(thread_count);
  worker_count =
    std::max(workgroups[group.get_index()].create_group(thread_offset, thread_count, priority), worker_count);
}

workgroup_id scheduler::create_group(uint32_t thread_offset, uint32_t thread_count, uint32_t priority)
{
  workgroups.emplace_back();
  // thread_count = acl::next_pow2(thread_count);
  worker_count = std::max(workgroups.back().create_group(thread_offset, thread_count, priority), worker_count);
  // no empty group found
  return workgroup_id(static_cast<uint32_t>(workgroups.size() - 1));
}

void scheduler::clear_group(workgroup_id group)
{
  workgroups[group.get_index()].start_thread_idx = 0;
  workgroups[group.get_index()].thread_count     = 0;
  workgroups[group.get_index()].push_offset      = 0;
  workgroups[group.get_index()].work_queues      = nullptr;
}

} // namespace acl
