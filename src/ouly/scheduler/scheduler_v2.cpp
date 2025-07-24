// SPDX-License-Identifier: MIT

#include "ouly/scheduler/scheduler_v2.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/worker_context.hpp"
#include "ouly/utility/common.hpp"
#include <atomic>
#include <barrier>
#include <latch>

namespace ouly::inline v2
{

// Thread-local storage for worker information
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local ouly::detail::worker const* g_worker = nullptr;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local ouly::worker_id g_worker_id = {};

auto worker_context::get(workgroup_id group) noexcept -> worker_context const&
{
  return g_worker->contexts_[group.get_index()];
}

auto worker_id::this_worker::get_id() noexcept -> worker_id const&
{
  return g_worker_id;
}

struct scheduler::worker_synchronizer
{
  std::latch          job_board_freeze_ack_;
  std::barrier<>      published_tally_;
  std::atomic_int64_t tally_{0};

  worker_synchronizer(ptrdiff_t worker_count) noexcept
      : job_board_freeze_ack_(worker_count), published_tally_(worker_count)
  {}
};

scheduler::~scheduler() noexcept
{
  if (!stop_.load())
  {
    end_execution();
  }
}

void scheduler::submit_internal(worker_id src, workgroup_id dst, ouly::detail::work_item const& work)
{
  // New implementation using the workgroup mailbox system
  auto& target_workgroup = new_workgroups_[dst.get_index()];

  // Try to submit directly to the mailbox
  if (target_workgroup.submit_to_mailbox(work))
  {
    // Notify workers that work is available
    notify_workers_work_available();
  }
  else
  {
    // Mailbox is full - could implement fallback strategy here
    // For now, just drop the work (in production, might want to retry or use a different strategy)
  }
}

void scheduler::notify_workers_work_available() noexcept
{
  // Set the global work flag and notify all waiting workers
  has_global_work_.store(true, std::memory_order_release);

  std::lock_guard<std::mutex> lock(global_work_mutex_);
  global_work_cv_.notify_all();
}

void scheduler::run_worker(worker_id worker_id)
{
  // Set thread-local storage
  g_worker_id = worker_id;
  g_worker    = &memory_block_.workers_[worker_id.get_index()].get();

  // Execute the entry function if provided
  if (entry_fn_)
  {
    // Create a worker_desc for the entry function - simplified for demo
    worker_desc desc{worker_id, nullptr};
    entry_fn_(desc);
  }

  // Main worker loop
  while (!stop_.load(std::memory_order_relaxed))
  {
    bool found_work = find_work_for_worker(worker_id);

    if (!found_work)
    {
      // No work found, wait for notification
      std::unique_lock<std::mutex> lock(global_work_mutex_);
      global_work_cv_.wait(lock,
                           [this]
                           {
                             return has_global_work_.load(std::memory_order_acquire) ||
                                    stop_.load(std::memory_order_relaxed);
                           });

      // Reset the global work flag - it will be set again when new work arrives
      has_global_work_.store(false, std::memory_order_relaxed);
    }
  }

  finalize_worker(worker_id);
}

auto scheduler::find_work_for_worker(worker_id worker_id) noexcept -> bool
{
  // Check each workgroup for available work, starting with workgroups this worker belongs to
  for (auto& workgroup : new_workgroups_)
  {
    if (workgroup.has_work())
    {
      if (try_get_work_from_workgroup(worker_id, workgroup_id{static_cast<uint32_t>(&workgroup - &new_workgroups_[0])}))
      {
        return true;
      }
    }
  }

  return false;
}

auto scheduler::try_get_work_from_workgroup(worker_id worker_id, workgroup_id group_id) noexcept -> bool
{
  auto& workgroup = new_workgroups_[group_id.get_index()];

  // First, try to get work from the mailbox
  ouly::detail::work_item work;
  if (workgroup.receive_from_mailbox(work))
  {
    execute_work(worker_id, work);
    return true;
  }

  // Then try to get work from the worker's own queue (if this worker belongs to this workgroup)
  uint32_t worker_start = workgroup.get_start_thread_idx();
  uint32_t worker_end   = workgroup.get_end_thread_idx();
  uint32_t worker_index = worker_id.get_index();

  if (worker_index >= worker_start && worker_index < worker_end)
  {
    // This worker belongs to this workgroup, try to pop from own queue
    uint32_t local_offset = worker_index - worker_start;
    auto     work_opt     = workgroup.pop_work_from_worker(local_offset);
    if (work_opt.has_value())
    {
      execute_work(worker_id, work_opt.value());
      return true;
    }
  }

  // Finally, try to steal work from other workers in this workgroup
  uint32_t avoid_offset =
   (worker_index >= worker_start && worker_index < worker_end) ? worker_index - worker_start : UINT32_MAX;

  auto stolen_work = workgroup.steal_work(avoid_offset);
  if (stolen_work.has_value())
  {
    execute_work(worker_id, stolen_work.value());
    return true;
  }

  // No work available in this workgroup
  workgroup.clear_work_available();
  return false;
}

void scheduler::execute_work(worker_id worker_id, ouly::detail::work_item& work) noexcept
{
  // Execute the work item with the appropriate worker context
  auto& worker = memory_block_.workers_[worker_id.get_index()].get();

  // For simplicity, use the default workgroup context
  // In a full implementation, we'd determine the correct workgroup from the work item
  work(worker.contexts_[0]);
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{
  entry_fn_ = std::move(entry);
  stop_.store(false, std::memory_order_relaxed);

  // Create worker threads
  threads_.reserve(worker_count_);
  for (uint32_t i = 0; i < worker_count_; ++i)
  {
    threads_.emplace_back(
     [this, worker_id = worker_id{i}]()
     {
       run_worker(worker_id);
     });
  }
}

void scheduler::end_execution()
{
  stop_.store(true, std::memory_order_release);

  // Wake up all workers
  {
    std::lock_guard<std::mutex> lock(global_work_mutex_);
    global_work_cv_.notify_all();
  }

  // Wait for all threads to finish
  for (auto& thread : threads_)
  {
    if (thread.joinable())
    {
      thread.join();
    }
  }

  threads_.clear();
}

void scheduler::create_group(workgroup_id group, uint32_t thread_offset, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= new_workgroups_.size())
  {
    new_workgroups_.resize(group.get_index() + 1);
  }

  new_workgroups_[group.get_index()].create_group(thread_offset, thread_count, priority);
  worker_count_ = std::max(worker_count_, thread_offset + thread_count);

  // Allocate memory blocks if needed
  if (!memory_block_.workers_)
  {
    memory_block_.workers_      = std::make_unique<aligned_worker[]>(worker_count_);
    memory_block_.group_ranges_ = std::make_unique<ouly::detail::group_range[]>(worker_count_);
    memory_block_.wake_data_    = std::make_unique<aligned_wake_data[]>(worker_count_);
  }
}

auto scheduler::create_group(uint32_t thread_offset, uint32_t thread_count, uint32_t priority) -> workgroup_id
{
  workgroup_id new_group{static_cast<uint32_t>(new_workgroups_.size())};
  create_group(new_group, thread_offset, thread_count, priority);
  return new_group;
}

void scheduler::clear_group(workgroup_id group)
{
  if (group.get_index() < new_workgroups_.size())
  {
    new_workgroups_[group.get_index()] = ouly::detail::new_workgroup{};
  }
}

void scheduler::take_ownership() noexcept
{
  // Implementation for taking ownership when multiple schedulers exist
  // For simplicity, this is a no-op in this demo
}

void scheduler::busy_work(worker_id worker_id) noexcept
{
  // Try to find work for the worker
  find_work_for_worker(worker_id);
}

// Legacy methods - simplified implementations for compatibility
void scheduler::assign_priority_order() {}
auto scheduler::compute_group_range(uint32_t worker_index) -> bool
{
  return true;
}
void scheduler::compute_steal_mask(uint32_t worker_index) const {}
void scheduler::finish_pending_tasks() noexcept {}
void scheduler::wake_up(worker_id /*thread*/) noexcept {}
void scheduler::finalize_worker(worker_id /*thread*/) noexcept {}

} // namespace ouly::inline v2
