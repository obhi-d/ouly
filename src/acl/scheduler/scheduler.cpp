
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
  return g_worker->contexts[group.get_index()].get();
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
  switch (work.data.reserved_0)
  {
  case detail::work_type_coroutine:
    std::coroutine_handle<>::from_address((void*)work.delegate_fn).resume();
    break;
  case detail::work_type_task_functor:
    (*reinterpret_cast<task*>(work.delegate_fn))(work.data,
                                                 workers[thread.get_index()].contexts[work.data.reserved_1].get());
    break;
  case detail::work_type_free_functor:
    reinterpret_cast<task_delegate>(work.delegate_fn)(work.data,
                                                      workers[thread.get_index()].contexts[work.data.reserved_1].get());
    break;
  }
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

  entry_fn(worker_desc(thread, group_masks[thread.get_index()]));

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
  auto wrk = try_get_work(thread);
  if (!wrk.delegate_fn)
  {
    wrk = get_work(thread);
    if (!wrk.delegate_fn)
      return false;
  }
  do_work(thread, wrk);
  return true;
}

detail::work_item scheduler::get_work(worker_id thread) noexcept
{
  auto& worker = workers[thread.get_index()];

  // try to get work from own queue

  {
    auto& work_list = global_work[thread.get_index()];
    auto  lck       = std::scoped_lock(work_list.first);
    if (!work_list.second.empty())
      return work_list.second.pop_front_unsafe();
  }

  {
    // try to steal from stealing sources, first from lock free queue
    // read head
    for (uint32_t steal_src = 0; steal_src != worker.friend_worker_count; ++steal_src)
    {
      uint32_t steal_from = (worker.stealing_source++) % worker.friend_worker_count;
      auto&    work_list  = global_work[steal_from];
      auto     lck        = std::scoped_lock(work_list.first);
      if (!work_list.second.empty())
        return work_list.second.pop_front_unsafe();
    }
  }

  return {};
}

detail::work_item scheduler::try_get_work(worker_id thread) noexcept
{
  auto& worker = workers[thread.get_index()];

  // try to get work from own queue

  {
    auto& work_list = global_work[thread.get_index()];
    if (work_list.first.try_lock())
    {
      auto lck = std::scoped_lock(std::adopt_lock, work_list.first);
      if (!work_list.second.empty())
        return work_list.second.pop_front_unsafe();
    }
  }

  {
    // try to steal from stealing sources, first from lock free queue
    // read head
    for (uint32_t steal_src = 0; steal_src != worker.friend_worker_count; ++steal_src)
    {
      uint32_t steal_from = (worker.stealing_source++) % worker.friend_worker_count;
      auto&    work_list  = global_work[steal_from];
      if (work_list.first.try_lock())
      {
        auto lck = std::scoped_lock(std::adopt_lock, work_list.first);
        if (!work_list.second.empty())
          return work_list.second.pop_front_unsafe();
      }
    }
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
  local_work  = std::make_unique<detail::work_item[]>(worker_count);
  global_work = std::make_unique<detail::global_work_queue[]>(worker_count);
  group_masks = std::make_unique<uint32_t[]>(worker_count);
  wake_status = std::make_unique<std::atomic_bool[]>(worker_count);
  wake_events = std::make_unique<detail::wake_event[]>(worker_count);
  workers     = std::make_unique<detail::worker[]>(worker_count);

  threads.reserve(worker_count - 1);

  for (uint32_t group = 0; group < detail::max_worker_groups; ++group)
  {
    auto const& g = workgroups[group];

    for (uint32_t i = g.start_thread_idx; i < g.end_thread_idx; ++i)
    {
      auto& worker = workers[i];
      group_masks[i] |= 1u << group;
    }
  }

  for (uint32_t group = 0; group < detail::max_worker_groups; ++group)
  {
    auto const& g = workgroups[group];

    for (uint32_t i = g.start_thread_idx; i < g.end_thread_idx; ++i)
    {
      auto& worker = workers[i];
      worker.contexts[group].emplace(*this, worker_id(i), workgroup_id(group), group_masks[i], i - g.start_thread_idx);
      worker.friend_worker_start = std::min(g.start_thread_idx, worker.friend_worker_start);
      worker.friend_worker_count = std::max(g.end_thread_idx, worker.friend_worker_count);
    }
  }

  for (uint32_t w = 0; w < worker_count; ++w)
  {
    workers[w].friend_worker_count = workers[w].friend_worker_count - workers[w].friend_worker_start;
    workers[w].stealing_source     = workers[w].friend_worker_start + (w + 1) % workers[w].friend_worker_count;
    workers[w].id                  = worker_id(w);
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

  entry_fn(worker_desc(worker_id(0), group_masks[0]));

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

  uint32_t empty_count = 0;
  auto     checked     = std::vector<uint32_t>(worker_count);
  std::iota(checked.begin(), checked.end(), 0);
  auto worker = g_worker;
  while (!checked.empty())
  {
    for (auto it = checked.begin(); it != checked.end();)
    {
      auto w        = *it;
      bool is_empty = true;
      do
      {
        auto lock = std::scoped_lock(global_work[w].first);
        is_empty  = global_work[w].second.empty();
      }
      while (0);
      if (!is_empty)
      {
        wake_up(worker_id(w));
        it++;
      }
      else
        it = checked.erase(it);
      if (worker)
        busy_work(worker->id);
    }
  }
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

void scheduler::submit(detail::work_item work, worker_id current)
{
  auto& wg     = workgroups[work.data.reserved_1];
  auto& worker = workers[current.get_index()];

  for (uint32_t i = wg.start_thread_idx; i != wg.end_thread_idx; ++i)
  {
    bool sleeping = true;
    if (!wake_status[i].exchange(true))
    {
      local_work[i] = std::move(work);
      wake_events[i].notify();
      return;
    }
  }
  while (true)
  {

    auto mask = wg.thread_count - 1;
    for (uint32_t start = worker.push_offset, end = worker.push_offset + wg.thread_count; start != end; ++start)
    {
      uint32_t q = wg.start_thread_idx + start & mask;
      if (global_work[q].first.try_lock())
      {
        global_work[q].second.emplace_back(std::move(work));
        global_work[q].first.unlock();
        if (!wake_status[q].exchange(true))
          wake_events[q].notify();
        return;
      }
    }
  }
}

void scheduler::create_group(workgroup_id group, std::string name, uint32_t thread_offset, uint32_t thread_count)
{
  assert(group.get_index() < workgroups.size());
  thread_count = acl::next_pow2(thread_count);
  worker_count =
    std::max(workgroups[group.get_index()].create_group(std::move(name), thread_offset, thread_count), worker_count);
}

workgroup_id scheduler::create_group(std::string name, uint32_t thread_offset, uint32_t thread_count)
{
  thread_count = acl::next_pow2(thread_count);
  for (uint32_t i = 0; i < workgroups.size(); ++i)
  {
    if (!workgroups[i].thread_count)
    {
      worker_count = std::max(workgroups[i].create_group(std::move(name), thread_offset, thread_count), worker_count);
      return workgroup_id(i);
    }
  }
  // no empty group found
  return workgroup_id(std::numeric_limits<uint32_t>::max());
}

void scheduler::clear_group(workgroup_id group)
{
  workgroups[group.get_index()].start_thread_idx = 0;
  workgroups[group.get_index()].end_thread_idx   = 0;
  workgroups[group.get_index()].thread_count     = 0;
}

workgroup_id scheduler::find_group(std::string const& name)
{
  auto it = std::ranges::find(workgroups.begin(), workgroups.end(), name, &detail::workgroup::name);
  return workgroup_id(it == workgroups.end() ? std::numeric_limits<uint32_t>::max()
                                             : static_cast<uint32_t>(std::distance(workgroups.begin(), it)));
}

} // namespace acl
