
#include "scheduler.hpp"
#include "task.hpp"
#include <acl/math/vml_fcn.hpp>
#include <latch>

namespace acl
{

thread_local detail::worker const* g_worker = nullptr;

worker_context const& worker_context::get_context(work_group_id group)
{
  return g_worker->contexts[group.get_index()].get();
}

scheduler::~scheduler()
{
  if (!stop.load())
    end_execution();
}

void scheduler::run(worker_id worker)
{
  g_worker = &workers[worker.get_index()];
  entry_fn(worker);
  while (true)
  {
    while (work(worker))
      ;

    if (stop.load(std::memory_order_relaxed))
      break;

    sleep_status[worker.get_index()].store(true, std::memory_order_release);
    wake_events[worker.get_index()].acquire();
    sleep_status[worker.get_index()].store(false, std::memory_order_relaxed);
  }
}

bool scheduler::should_we_sleep(worker_id thread) noexcept
{
  auto const& g = workers[thread.get_index()];
  for (auto group = g.group_ids.begin(), end = g.group_ids.begin() + g.group_count; group != end; group++)
  {
    auto& wg = work_groups[*group];
    if (wg.work_count.load(std::memory_order_acquire) != 0)
      return false;
  }
  return true;
}

detail::work scheduler::get_work(worker_id thread) noexcept
{
  auto work = std::move(immediate_work[thread.get_index()]);
  if (work)
  {
    auto group = immediate_work_group[thread.get_index()];
    work_groups[group.get_index()].work_count.fetch_sub(1, std::memory_order_release);
    return std::make_pair(std::move(work), group);
  }
  do
  {
    if (!work)
    {
      auto const& g = workers[thread.get_index()];
      for (auto group = g.group_ids.begin(), end = g.group_ids.begin() + g.group_count; group != end; group++)
      {
        auto&    wg     = work_groups[*group];
        auto     mask   = wg.thread_count - 1;
        uint32_t offset = thread.get_index() - wg.start_thread_idx;
        for (uint32_t i = offset, end = offset + wg.thread_count; i != end; ++i)
        {
          uint32_t q = i & mask;
          if (wg.locks[q].try_lock())
          {
            if (!wg.queues[q].empty())
            {
              wg.work_count.fetch_sub(1, std::memory_order_release);
              work = std::move(wg.queues[q].pop_front_unsafe());
              wg.locks[q].unlock<std::false_type>();
              return std::make_pair(std::move(work), work_group_id(*group));
            }
            wg.locks[q].unlock<std::false_type>();
          }
        }
      }
      if (!work)
      {
        for (auto group = g.group_ids.data(), end = g.group_ids.data() + g.group_count; group != end; group++)
        {
          auto& wg = work_groups[*group];
          if (wg.shared_queue.first.try_lock())
          {
            if (!wg.shared_queue.second.empty())
            {
              wg.work_count.fetch_sub(1, std::memory_order_release);
              work = std::move(wg.shared_queue.second.pop_front_unsafe());
              wg.shared_queue.first.unlock<std::false_type>();
              return std::make_pair(std::move(work), work_group_id(*group));
            }
            wg.shared_queue.first.unlock<std::false_type>();
          }
        }
      }
    }
  }
  while (!should_we_sleep(thread));
  return {};
}

bool scheduler::work(worker_id ctx) noexcept
{
  auto work = get_work(ctx);
  auto idx  = ctx.get_index();
  if (work.first)
  {
    auto [delegate_fn, context, tag] = work.first.unpack();
    switch (tag)
    {
    case detail::work_type_coroutine:
      std::coroutine_handle<>::from_address(delegate_fn).resume();
      break;
    case detail::work_type_task_functor:
      (*reinterpret_cast<task*>(delegate_fn))(context, workers[idx].contexts[work.second.get_index()].get());
      break;
    case detail::work_type_free_functor:
      reinterpret_cast<task_delegate>(delegate_fn)(context, workers[idx].contexts[work.second.get_index()].get());
      break;
    }
    return true;
  }
  return false;
}

void scheduler::wake_up(worker_id thread) noexcept
{
  bool sleeping = true;
  if (sleep_status[thread.get_index()].compare_exchange_strong(sleeping, false))
  {
    wake_events[thread.get_index()].release();
    return;
  }
}

void scheduler::begin_execution(scheduler_worker_entry&& entry)
{
  immediate_work       = std::make_unique<detail::work_item[]>(worker_count);
  immediate_work_group = std::make_unique<work_group_id[]>(worker_count);
  group_masks          = std::make_unique<uint32_t[]>(worker_count);
  sleep_status         = std::make_unique<std::atomic_bool[]>(worker_count);
  wake_events          = std::make_unique<detail::wake_event[]>(worker_count);
  workers              = std::make_unique<detail::worker[]>(worker_count);

  threads.reserve(worker_count - 1);

  for (auto group = 0; group < 32; ++group)
  {
    auto const& g = work_groups[group];

    for (uint32_t i = g.start_thread_idx; i < g.end_thread_idx; ++i)
    {
      auto& worker = workers[i];
      group_masks[i] |= 1u << group;
      worker.group_ids[worker.group_count++] = group;
    }
  }

  for (auto group = 0; group < 32; ++group)
  {
    auto const& g = work_groups[group];

    for (uint32_t i = g.start_thread_idx; i < g.end_thread_idx; ++i)
    {
      auto& worker = workers[i];
      worker.contexts[group].emplace(*this, worker_id(i), work_group_id(group), group_masks[i], i - g.start_thread_idx);
    }
  }

  stop = false;
  sleep_status[0].store(false);
  auto start_counter = std::latch(worker_count);

  entry_fn = [cust_entry = std::move(entry), &start_counter](worker_id worker)
  {
    start_counter.count_down();
    if (cust_entry)
      cust_entry(worker);
  };

  entry_fn(worker_id(0));

  for (uint32_t thread = 1; thread < worker_count; ++thread)
  {
    sleep_status[thread].store(false);
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
  bool has_work = true;
  while (has_work)
  {
    has_work = false;
    for (auto& wg : work_groups)
    {
      if (wg.work_count.load(std::memory_order_acquire) != 0)
      {
        has_work = true;
        break;
      }
    }

    if (has_work)
      work(worker_id(0));
  }
}

void scheduler::end_execution()
{
  finish_pending_tasks();
  stop = true;
  for (uint32_t thread = 1; thread < worker_count; ++thread)
  {
    wake_up(worker_id(thread));
    threads[thread - 1].join();
  }
  threads.clear();
}

void scheduler::submit(detail::work_item work, work_group_id group, worker_id current)
{
  auto& wg = work_groups[group.get_index()];
  wg.work_count.fetch_add(1, std::memory_order_relaxed);

  while (true)
  {
    for (uint32_t i = wg.start_thread_idx; i != wg.end_thread_idx; ++i)
    {
      bool sleeping = true;
      if (sleep_status[i].compare_exchange_weak(sleeping, false))
      {
        immediate_work[i]       = std::move(work);
        immediate_work_group[i] = group;
        wake_events[i].release();
        return;
      }
    }

    uint32_t offset = wg.thread_selection.fetch_add(1, std::memory_order_relaxed);
    auto     mask   = wg.thread_count - 1;
    for (uint32_t i = offset, end = offset + wg.thread_count; i != end; ++i)
    {
      uint32_t q = i & mask;
      if (wg.locks[q].try_lock())
      {
        wg.queues[q].emplace_back(std::move(work));
        wg.locks[q].unlock<std::false_type>();
        return;
      }
    }
    if (wg.shared_queue.first.try_lock())
    {
      wg.shared_queue.second.emplace_back(std::move(work));
      wg.shared_queue.first.unlock<std::false_type>();
      return;
    }
  }
}

void scheduler::create_group(work_group_id group, std::string name, uint32_t thread_offset, uint32_t thread_count)
{
  assert(group.get_index() < work_groups.size());
  thread_count = acl::next_pow2(thread_count);
  worker_count =
    std::max(work_groups[group.get_index()].create_group(std::move(name), thread_offset, thread_count), worker_count);
}

work_group_id scheduler::create_group(std::string name, uint32_t thread_offset, uint32_t thread_count)
{
  thread_count = acl::next_pow2(thread_count);
  for (uint32_t i = 0; i < work_groups.size(); ++i)
  {
    if (!work_groups[i].thread_count)
    {
      worker_count = std::max(work_groups[i].create_group(std::move(name), thread_offset, thread_count), worker_count);
      return work_group_id(i);
    }
  }
  // no empty group found
  return work_group_id(std::numeric_limits<uint32_t>::max());
}

void scheduler::clear_group(work_group_id group)
{
  work_groups[group.get_index()].start_thread_idx = 0;
  work_groups[group.get_index()].end_thread_idx   = 0;
  work_groups[group.get_index()].thread_count     = 0;
}

work_group_id scheduler::find_group(std::string const& name)
{
  auto it = std::ranges::find(work_groups.begin(), work_groups.end(), name, &detail::work_group::name);
  return work_group_id(it == work_groups.end() ? std::numeric_limits<uint32_t>::max()
                                               : static_cast<uint32_t>(std::distance(work_groups.begin(), it)));
}

} // namespace acl