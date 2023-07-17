
#include "scheduler.hpp"
#include "task.hpp"
#include <acl/math/vml_fcn.hpp>

namespace acl
{

scheduler::~scheduler()
{
  if (!stop.load())
    end_execution();
}

void scheduler::run(worker_id thread)
{
  while (!stop.load(std::memory_order_relaxed))
  {
    work(thread);
  }
}

bool scheduler::should_we_sleep(uint32_t thread) noexcept
{
  auto const& g = worker_groups[thread];
  for (auto group = g.group_ids, end = g.group_ids + g.group_count; group != end; group++)
  {
    auto& wg = work_groups[*group];
    if (wg.work_count.load(std::memory_order_acquire) != 0)
      return false;
  }
  return true;
}

detail::work_item scheduler::get_work(worker_id thread) noexcept
{
  auto work = std::move(immediate_work[thread.get_index()]);
  do
  {
    if (!work)
    {
      auto const& g = worker_groups[thread.get_index()];
      for (auto group = g.group_ids, end = g.group_ids + g.group_count; group != end; group++)
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
              return work;
            }
            wg.locks[q].unlock<std::false_type>();
          }
        }
      }
      if (!work)
      {
        for (auto group = g.group_ids, end = g.group_ids + g.group_count; group != end; group++)
        {
          auto& wg = work_groups[g.group_ids[*group]];
          if (wg.shared_queue.first.try_lock())
          {
            if (!wg.shared_queue.second.empty())
            {
              wg.work_count.fetch_sub(1, std::memory_order_release);
              work = std::move(wg.shared_queue.second.pop_front_unsafe());
              wg.shared_queue.first.unlock<std::false_type>();
              return work;
            }
            wg.shared_queue.first.unlock<std::false_type>();
          }
        }
      }
    }
  }
  while (!should_we_sleep(thread.get_index()));
  return {};
}

void scheduler::work(worker_id thread) noexcept
{
  auto work = get_work(thread);
  if (!work)
  {
    sleep_status[thread.get_index()].store(true, std::memory_order_release);
    wake_events[thread.get_index()].acquire();
  }
  else
  {
    auto [delegate_fn, context, tag] = work.unpack();
    switch (tag)
    {
    case detail::work_type_coroutine:
      std::coroutine_handle<>::from_address(delegate_fn).resume();
      break;
    case detail::work_type_task_functor:
      (*reinterpret_cast<task*>(delegate_fn))(context, thread);
      break;
    case detail::work_type_free_functor:
      reinterpret_cast<task_delegate>(delegate_fn)(context, thread);
      break;
    }
  }
}

void scheduler::work_no_sleep(worker_id thread) noexcept
{
  auto work = get_work(thread);
  if (!work)
    return;
  else
  {
    auto [delegate_fn, context, tag] = work.unpack();
    switch (tag)
    {
    case detail::work_type_coroutine:
      std::coroutine_handle<>::from_address(delegate_fn).resume();
      break;
    case detail::work_type_task_functor:
      (*reinterpret_cast<task*>(delegate_fn))(context, thread);
      break;
    case detail::work_type_free_functor:
      reinterpret_cast<task_delegate>(delegate_fn)(context, thread);
      break;
    }
  }
}

void scheduler::wake_up(uint32_t thread) noexcept
{
  bool sleeping = true;
  if (sleep_status[thread].compare_exchange_strong(sleeping, false))
  {
    wake_events[thread].release();
    return;
  }
}

void scheduler::begin_execution()
{
  immediate_work = std::make_unique<detail::work_item[]>(worker_count);
  group_masks    = std::make_unique<uint32_t[]>(worker_count);
  sleep_status   = std::make_unique<std::atomic_bool[]>(worker_count);
  wake_events    = std::make_unique<detail::wake_event[]>(worker_count);
  worker_groups  = std::make_unique<detail::worker_group_ids[]>(worker_count);

  threads.reserve(worker_count - 1);

  for (auto group = 0; group < 32; ++group)
  {
    auto const& g = work_groups[group];
    for (uint32_t i = g.start_thread_idx; i < g.end_thread_idx; ++i)
    {
      group_masks[i] |= 1u << group;
      worker_groups[i].group_ids[worker_groups[i].group_count++] = group;
    }
  }

  stop = false;
  sleep_status[0].store(false);
  for (uint32_t thread = 1; thread < worker_count; ++thread)
  {
    sleep_status[thread].store(false);
    threads.emplace_back(&scheduler::run, this, worker_id(thread, group_masks[thread]));
  }
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
      work_no_sleep(worker_id(0, group_masks[0]));
  }
}

void scheduler::end_execution()
{
  finish_pending_tasks();
  stop = true;
  for (uint32_t thread = 1; thread < worker_count; ++thread)
  {
    wake_up(thread);
    threads[thread - 1].join();
  }
  threads.clear();
}

void scheduler::submit(detail::work_item work, uint32_t group, worker_id current)
{
  auto& wg = work_groups[group];
  wg.work_count.fetch_add(1, std::memory_order_relaxed);

  while (true)
  {
    for (uint32_t i = wg.start_thread_idx; i != wg.end_thread_idx; ++i)
    {
      bool sleeping = true;
      if (sleep_status[i].compare_exchange_weak(sleeping, false))
      {
        immediate_work[i] = std::move(work);
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

void scheduler::create_group(uint32_t group, std::string name, uint32_t thread_offset, uint32_t thread_count)
{
  assert(group < work_groups.size());
  thread_count = acl::next_pow2(thread_count);
  worker_count = std::max(work_groups[group].create_group(std::move(name), thread_offset, thread_count), worker_count);
}

uint32_t scheduler::create_group(std::string name, uint32_t thread_offset, uint32_t thread_count)
{
  thread_count = acl::next_pow2(thread_count);
  for (uint32_t i = 0; i < work_groups.size(); ++i)
  {
    if (!work_groups[i].thread_count)
    {
      worker_count = std::max(work_groups[i].create_group(std::move(name), thread_offset, thread_count), worker_count);
      return i;
    }
  }
  // no empty group found
  return std::numeric_limits<uint32_t>::max();
}

void scheduler::clear_group(uint32_t group)
{
  work_groups[group].start_thread_idx = 0;
  work_groups[group].end_thread_idx   = 0;
  work_groups[group].thread_count     = 0;
}

uint32_t scheduler::find_group(std::string const& name)
{
  auto it = std::ranges::find(work_groups.begin(), work_groups.end(), name, &detail::work_group::name);
  return it == work_groups.end() ? std::numeric_limits<uint32_t>::max()
                                 : static_cast<uint32_t>(std::distance(work_groups.begin(), it));
}

} // namespace acl