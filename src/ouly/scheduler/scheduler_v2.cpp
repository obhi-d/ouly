// SPDX-License-Identifier: MIT

#include "ouly/scheduler/scheduler_v2.hpp"
#include "ouly/scheduler/detail/pause.hpp"
#include "ouly/scheduler/detail/worker_v2.hpp"
#include "ouly/scheduler/detail/workgroup_v2.hpp"
#include "ouly/scheduler/task_context_v2.hpp"
#include <atomic>
#include <barrier>
#include <latch>
#include <thread>

namespace ouly::inline v2
{

// Bring type aliases into scope
using workgroup_type = ouly::detail::v2::workgroup;
using work_item_type = ouly::detail::v2::work_item;
using worker_type    = ouly::detail::v2::worker;

// Thread-local storage for worker information
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local detail::v2::worker const* g_worker = nullptr;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local ouly::worker_id g_worker_id = {};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local uint32_t g_random_seed = 0;

// LCG constants for fast PRNG
static constexpr uint32_t lcg_multiplier    = 1664525U;
static constexpr uint32_t lcg_increment     = 1013904223U;
static constexpr uint32_t initial_seed_mask = 0xAAAAAAAAU;

static auto update_seed() -> uint32_t;

static auto update_seed() -> uint32_t
{
  return (g_random_seed = (g_random_seed * lcg_multiplier + lcg_increment) & initial_seed_mask);
}

auto task_context::this_context::get() noexcept -> task_context const&
{
  return g_worker->get_context();
}

auto task_context::this_context::get_worker_id() noexcept -> worker_id
{
  return g_worker_id;
}

struct scheduler::tally_publisher
{
  scheduler* owner_ = nullptr;

  void operator()() const noexcept;
};

struct scheduler::worker_synchronizer
{
  std::latch                    job_board_freeze_ack_;
  std::barrier<tally_publisher> published_tally_;
  std::atomic_int64_t           tally_{0};
  void*                         user_context_ = nullptr;

  worker_synchronizer(ptrdiff_t worker_count, scheduler* owner) noexcept
      : job_board_freeze_ack_(worker_count), published_tally_(worker_count, tally_publisher{owner})
  {}
};

void scheduler::tally_publisher::operator()() const noexcept
{
  // Cache-friendly sequential access with prefetching
  int64_t total_remaining_tasks = 0;

  // Count tasks
  for (uint32_t thread = 0; thread < owner_->worker_count_; ++thread)
  {
    total_remaining_tasks += owner_->workers_[thread].tally_;
  }
  owner_->synchronizer_->tally_.store(total_remaining_tasks, std::memory_order_release);
}

scheduler::~scheduler() noexcept
{
  if (!stop_.load())
  {
    end_execution();
  }
}

void scheduler::submit_internal(task_context const& current, workgroup_id dst,
                                ::ouly::detail::v2::work_item const& work)
{
  auto& target_workgroup = workgroups_[dst.get_index()];

  // First try to submit to the worker's own queue if it belongs to this workgroup
  worker_id current_worker = current.get_worker();
  workers_[current_worker.get_index()].tally_++;

  if (current.get_workgroup() == dst)
  {
    if (target_workgroup.push_work_to_worker(current.get_group_offset(), work))
    {
      // Work was successfully pushed, workgroup will advertise availability
      wake_up_workers(1); // Wake up one worker
      return;
    }
  }

  target_workgroup.submit_to_mailbox(work);

  // Add to needy workgroups list for better work distribution
  needy_workgroups_.push(&target_workgroup);
  wake_up_workers(1); // Wake up workers to handle the new work
}

void scheduler::wake_up_workers(uint32_t count) noexcept
{
  auto sleeping_count = sleeping_.load(std::memory_order_acquire);
  auto wake_count     = std::min(count, static_cast<uint32_t>(sleeping_count));

  if (wake_count > 0)
  {
    wake_tokens_.release(static_cast<int32_t>(wake_count));
  }
}

void scheduler::run_worker(worker_id wid)
{
  // Set thread-local storage for this worker
  g_worker_id = wid;
  g_worker    = &workers_[wid.get_index()];

  // Execute the entry function if provided
  if (entry_fn_)
  {
    entry_fn_(wid);
  }

  // Main worker loop
  while (!stop_.load(std::memory_order_relaxed))
  {
    bool found_work = false;

    // Try to find work
    found_work = find_work_for_worker(wid);

    if (!found_work)
    {
      // exit context
      auto& worker = workers_[wid.get_index()];
      if (worker.assigned_group_ != nullptr)
      {
        worker.assigned_group_->exit();
        worker.assigned_group_ = nullptr;
      }
      // Enter sleep state
      sleeping_.fetch_add(1, std::memory_order_relaxed);

      wake_tokens_.acquire();

      if (stop_.load(std::memory_order_relaxed))
      {
        break;
      }

      // Exit sleep state
      sleeping_.fetch_sub(1, std::memory_order_relaxed);
    }
  }

  if (synchronizer_ != nullptr)
  {
    synchronizer_->job_board_freeze_ack_.count_down();
    synchronizer_->job_board_freeze_ack_.wait();
  }

  finalize_worker(wid);
}

auto scheduler::enter_context(worker_id wid, detail::v2::workgroup* needy_wg) noexcept -> bool
{
  auto& worker = workers_[wid.get_index()];

  if (worker.assigned_group_ != needy_wg)
  {
    int enter_ctx = needy_wg->enter();
    if (enter_ctx < 0)
    {
      return false;
    }

    if (worker.assigned_group_ != nullptr)
    {
      worker.assigned_group_->exit();
    }

    worker.assigned_group_  = needy_wg;
    worker.assigned_offset_ = static_cast<uint32_t>(enter_ctx);
  }
  return true;
}

auto scheduler::find_work_for_worker(worker_id wid) noexcept -> bool
{
  // Try to drain work from the worker's own workgroup first
  auto& worker = workers_[wid.get_index()];

  if (worker.assigned_group_ != nullptr)
  {
    detail::v2::work_item work;
    if (worker.assigned_group_->pop_work_from_worker(work, worker.assigned_offset_))
    {
      execute_work(wid, work);
      return true;
    }

    if (worker.assigned_group_->has_work() && worker.assigned_group_->receive_from_mailbox(work))
    {
      execute_work(wid, work);
      return true;
    }

    worker.assigned_group_->clear_work_available();
  }

  // Priority 2: Check needy workgroups for mailbox work (limit attempts to avoid starvation)
  static constexpr uint32_t max_needy_attempts = 3;
  uint32_t                  needy_attempts     = 0;
  thread_local uint32_t     random_victim      = update_seed();

  detail::v2::workgroup* needy_wg = nullptr;
  while (needy_attempts < max_needy_attempts && needy_workgroups_.pop(needy_wg))
  {
    ++needy_attempts;

    if (!enter_context(wid, needy_wg))
    {
      continue; // Failed to enter context, try next needy workgroup
    }

    detail::v2::work_item work;
    if (needy_wg->receive_from_mailbox(work))
    {
      execute_work(wid, work);
      return true;
    }

    // No work in mailbox, try stealing from this workgroup
    if (needy_wg->steal_work(work, random_victim))
    {
      execute_work(wid, work);
      return true;
    }

    // Clear work available flag since we found no work
    needy_wg->clear_work_available();
    random_victim = update_seed();
  }

  // Priority 3: Work stealing from any workgroup with priority-based selection
  // Start with higher priority workgroups and overloaded workgroups
  uint32_t steal_start_idx = update_seed();

  for (uint32_t attempt = 0; attempt < workgroup_count_; ++attempt)
  {
    uint32_t i         = (steal_start_idx + attempt) % workgroup_count_;
    auto&    workgroup = workgroups_[i];

    if (!workgroup.has_work() || !enter_context(wid, &workgroup))
    {
      continue;
    }

    detail::v2::work_item work;
    if (workgroup.steal_work(work, steal_start_idx))
    {
      execute_work(wid, work);
      return true;
    }
  }

  return false;
}

void scheduler::execute_work(worker_id wid, detail::v2::work_item& work) noexcept
{
  auto& worker = workers_[wid.get_index()];
  worker.tally_--;
  // Create a copy since work_item expects mutable reference
  work(worker.get_context());
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{
  // Initialize workers and workgroups
  workers_ = std::make_unique<detail::v2::worker[]>(worker_count_);

  // Initialize worker contexts
  for (uint32_t i = 0; i < detail::v2::max_workgroup; ++i)
  {
    if (workgroups_[i].get_thread_count() > 0)
    {
      workgroup_count_ = std::max(workgroup_count_, i + 1);
    }
  }

  stop_.store(false, std::memory_order_relaxed);

  auto start_counter = std::latch(worker_count_);

  entry_fn_ = [cust_entry = std::move(entry), &start_counter](ouly::worker_id worker)
  {
    if (cust_entry)
    {
      cust_entry(worker);
    }
    start_counter.count_down();
  };

  // Create worker threads
  threads_.reserve(worker_count_);

  for (uint32_t thread = 1; thread < worker_count_; ++thread)
  {
    workers_[thread].current_context_.init(*this, user_context, 0, worker_id(thread));
    threads_.emplace_back(&scheduler::run_worker, this, worker_id(thread));
  }

  workers_[0].current_context_.init(*this, user_context, 0, worker_id(0));
  workers_[0].set_workgroup_info(0, workgroup_id(0));

  g_worker    = &workers_[0];
  g_worker_id = worker_id(0);

  entry_fn_(worker_id(0));
  start_counter.wait();
  entry_fn_ = {}; // Clear entry function after execution starts
}

void scheduler::end_execution()
{
  finish_pending_tasks();

  // Wait for all threads to finish
  for (auto& thread : threads_)
  {
    if (thread.joinable())
    {
      thread.join();
    }
  }

  threads_.clear();
  synchronizer_.reset();
}

void scheduler::finish_pending_tasks()
{
  synchronizer_ = std::make_shared<worker_synchronizer>(worker_count_, this);

  // First: Signal stop to prevent new task submissions
  stop_.store(true, std::memory_order_seq_cst);

  // Wake up all workers
  wake_tokens_.release(worker_count_);

  // Third: Wait for all workers to acknowledge the stop signal
  synchronizer_->job_board_freeze_ack_.count_down();
  while (!synchronizer_->job_board_freeze_ack_.try_wait())
  {
    wake_tokens_.release(worker_count_);
    // Wait for all workers to acknowledge freeze
    ouly::detail::pause_exec();
  }

  finalize_worker(worker_id(0));
}

void scheduler::finalize_worker(worker_id wid)
{
  // Implementation for finalizing a worker
  if (synchronizer_ != nullptr)
  {

    auto old_tally = workers_[wid.get_index()].tally_;
    synchronizer_->tally_.fetch_add(old_tally, std::memory_order_acq_rel);

    synchronizer_->published_tally_.arrive_and_wait();

    while (synchronizer_->tally_.load(std::memory_order_acquire) > 0)
    {
      while (find_work_for_worker(wid))
      {
      }

      synchronizer_->published_tally_.arrive_and_wait();
    }

    // Wait to take stock of the remaning work
  }

  g_worker    = nullptr;
  g_worker_id = {};
}

void scheduler::create_group(workgroup_id group, uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= detail::v2::max_workgroup)
  {
    return; // Invalid group ID
  }

  auto& wg = workgroups_[group.get_index()];
  wg.initialize(start_thread_idx, thread_count, priority, this);
  worker_count_ = std::max(worker_count_, wg.get_end_thread_idx());
}

auto scheduler::create_group(uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority) -> workgroup_id
{
  // Find next available group ID
  for (uint32_t i = 0; i < detail::v2::max_workgroup; ++i)
  {
    if (workgroups_[i].get_thread_count() == 0)
    {
      workgroup_id new_group{i};
      create_group(new_group, start_thread_idx, thread_count, priority);
      return new_group;
    }
  }

  return workgroup_id{0}; // Fallback to group 0 if all are used
}

void scheduler::clear_group(workgroup_id group)
{
  if (group.get_index() < detail::v2::max_workgroup)
  {
    workgroups_[group.get_index()].clear();
  }
}

auto scheduler::get_worker_count(workgroup_id g) const noexcept -> uint32_t
{
  return workgroups_[g.get_index()].get_thread_count();
}

auto scheduler::get_worker_start_idx(workgroup_id g) const noexcept -> uint32_t
{
  return workgroups_[g.get_index()].get_start_thread_idx();
}

auto scheduler::get_logical_divisor(workgroup_id g) const noexcept -> uint32_t
{
  auto worker_count = get_worker_count(g);
  return worker_count > 0 ? worker_count : 1;
}

void scheduler::take_ownership() noexcept
{
  // Implementation for taking ownership when multiple schedulers exist
  // This is typically used when multiple scheduler instances exist
}

void scheduler::busy_work(worker_id thread) noexcept
{
  // Try to find work for the worker during busy wait
  // Optimized work stealing with adaptive attempts
  constexpr uint32_t min_attempts      = 1;
  constexpr uint32_t max_attempts      = 3;
  constexpr uint32_t failure_threshold = 5;

  // Use thread_local failure tracking for better performance
  thread_local uint32_t local_recent_failures = 0;
  uint32_t              attempts = (local_recent_failures > failure_threshold) ? min_attempts : max_attempts;

  for (uint32_t attempt = 0; attempt < attempts; ++attempt)
  {
    if (find_work_for_worker(thread)) [[likely]]
    {
      if (thread.get_index() == 0)
      {
        auto& worker = workers_[0];
        // reset the context
        if (worker.assigned_group_ != nullptr)
        {
          worker.assigned_group_->exit();
          worker.assigned_group_ = nullptr;
        }
        worker.set_workgroup_info(0, workgroup_id(0));
      }
      local_recent_failures = std::max(0U, local_recent_failures - 1);
      return; // Found and executed work
    }

    // Shorter pause for first attempts, longer for subsequent ones
    if (attempt < attempts - 1)
    {
      ouly::detail::pause_exec();
    }
  }
}

} // namespace ouly::inline v2

void ouly::v2::task_context::busy_wait(std::binary_semaphore& event) const
{
  while (!event.try_acquire())
  {
    // Use a busy wait loop to avoid blocking the thread
    owner_->busy_work(index_);
  }
  // Wait until the semaphore is signaled
}