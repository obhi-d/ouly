// SPDX-License-Identifier: MIT

#include "ouly/scheduler/v2/scheduler.hpp"
#include "ouly/scheduler/detail/pause.hpp"
#include "ouly/scheduler/detail/v2/worker.hpp"
#include "ouly/scheduler/detail/v2/workgroup.hpp"
#include "ouly/scheduler/v2/task_context.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include <atomic>
#include <barrier>
#include <cstdint>
#include <latch>
#include <ranges>
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

struct workgroup_desc
{
  uint32_t start_        = 0;
  uint32_t thread_count_ = 0;
  uint32_t priority_     = 0; // Priority of the workgroup
};

struct scheduler::worker_initializer
{
  std::array<workgroup_desc, detail::v2::max_workgroup> workgroup_descriptions_;
};

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

  if (current.get_workgroup() == dst)
  {
    if (target_workgroup.push_work_to_worker(current.get_group_offset(), work))
    {
      // Work was successfully pushed, workgroup will advertise availability
      wake_up_workers(1); // Wake up one worker
      return;
    }
  }

  while (!target_workgroup.submit_to_mailbox(work))
  {
    // Possibly coop wait
    busy_work(current_worker);
  }

  // Add to needy workgroups list for better work distribution
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
      if (worker.get_workgroup())
      {
        auto& workgroup = workgroups_[worker.get_workgroup().get_index()];
        workgroup.exit(static_cast<int>(worker.get_group_offset()));
        worker.set_workgroup_info(0, workgroup_id{});
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

  g_worker    = nullptr;
  g_worker_id = {};

  finished_.fetch_add(1, std::memory_order_release);
}

auto scheduler::enter_context(worker_id wid, workgroup_id needy_wg) noexcept -> bool
{
  auto& worker = workers_[wid.get_index()];

  if (worker.get_workgroup() != needy_wg)
  {
    auto& needy_workgroup = workgroups_[needy_wg.get_index()];
    int   enter_ctx       = needy_workgroup.enter();
    if (enter_ctx < 0)
    {
      return false;
    }

    if (worker.get_workgroup())
    {
      auto& current_group = workgroups_[worker.get_workgroup().get_index()];
      current_group.exit(static_cast<int>(worker.get_group_offset()));
    }

    worker.set_workgroup_info(static_cast<uint32_t>(enter_ctx), needy_wg);
  }
  return true;
}

// NOLINTNEXTLINE
auto scheduler::find_work_for_worker(worker_id wid) noexcept -> bool
{
  // Try to drain work from the worker's own workgroup first
  auto&                 worker        = workers_[wid.get_index()];
  thread_local uint32_t random_victim = update_seed();

  if (worker.get_workgroup())
  {
    auto& workgroup = workgroups_[worker.get_workgroup().get_index()];

    if (workgroup.has_work())
    {
      detail::v2::work_item work{detail::v2::work_item::noinit};

      if (workgroup.pop_work_from_worker(work, worker.get_group_offset()))
      {
        execute_work(wid, work);
        return true;
      }

      // No work in mailbox, try stealing from this workgroup
      if (workgroup.steal_work(work, random_victim))
      {
        execute_work(wid, work);
        return true;
      }

      if (workgroup.receive_from_mailbox(work))
      {
        execute_work(wid, work);
        return true;
      }

      random_victim = update_seed();
    }
  }

  // Priority 3: Work stealing from any workgroup with priority-based selection
  // Start with higher priority workgroups and overloaded workgroups
  uint32_t steal_start_idx = update_seed();

  for (uint32_t attempt = 0; attempt < workgroup_count_; ++attempt)
  {
    uint32_t i         = (steal_start_idx + attempt) % workgroup_count_;
    auto&    workgroup = workgroups_[i];

    if (!workgroup.has_work())
    {
      continue;
    }

    if (!enter_context(wid, workgroup_id(i)))
    {
      continue;
    }

    detail::v2::work_item work{detail::v2::work_item::noinit};
    if (workgroup.steal_work(work, steal_start_idx))
    {
      execute_work(wid, work);
      return true;
    }

    if (workgroup.receive_from_mailbox(work))
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
  // Create a copy since work_item expects mutable reference
  auto const& current_context = worker.get_context();
  work(current_context);

  workgroups_[current_context.get_workgroup().get_index()].sink_one_work();
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, void* user_context)
{

  auto& workgroup_descs = initializer_->workgroup_descriptions_;
  // Initialize worker contexts
  for (uint32_t i = 0; i < detail::v2::max_workgroup; ++i)
  {
    if (workgroup_descs[i].thread_count_ > 0)
    {
      workgroup_count_ = std::max(workgroup_count_, i + 1);
      worker_count_    = std::max(worker_count_, workgroup_descs[i].start_ + workgroup_descs[i].thread_count_);
    }
  }

  workgroups_ = std::make_unique<detail::v2::workgroup[]>(workgroup_count_);

  for (uint32_t i = 0; i < workgroup_count_; ++i)
  {
    if (workgroup_descs[i].thread_count_ > 0)
    {
      workgroups_[i].create_group(workgroup_descs[i].start_, workgroup_descs[i].thread_count_,
                                  workgroup_descs[i].priority_);
    }
  }

  // Initialize workers and workgroups
  workers_ = std::make_unique<detail::v2::worker[]>(worker_count_);

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
  enter_context(worker_id(0), workgroup_id(0));

  g_worker    = &workers_[0];
  g_worker_id = worker_id(0);

  entry_fn_(worker_id(0));
  start_counter.wait();
  entry_fn_ = {}; // Clear entry function after execution starts

  initializer_ = nullptr;
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
}

auto scheduler::has_work() const -> bool
{
  return std::ranges::any_of(std::span(workgroups_.get(), workgroup_count_),
                             [](const auto& group)
                             {
                               return group.has_work_strong();
                             });
}

void scheduler::wait_for_tasks()
{
  // Wake up all workers
  wake_tokens_.release(worker_count_);

  while (has_work())
  {
    wake_up_workers(worker_count_);
    // Busy wait to ensure we don't miss any work
    busy_work(worker_id(0));
  }
}

void scheduler::finish_pending_tasks()
{
  wait_for_tasks();

  // First: Signal stop to prevent new task submissions
  stop_.store(true, std::memory_order_seq_cst);

  auto thread_count = static_cast<uint32_t>(worker_count_ - 1);
  while (finished_.load(std::memory_order_acquire) < thread_count)
  {
    wake_tokens_.release(thread_count);
  }
}

void scheduler::create_group(workgroup_id group, uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= detail::v2::max_workgroup || thread_count == 0)
  {
    return;
  }

  if (!initializer_)
  {
    initializer_ = std::make_shared<worker_initializer>();
  }

  if (group.get_index() >= workgroup_count_)
  {
    workgroup_count_ = group.get_index() + 1;
  }

  auto& wg         = initializer_->workgroup_descriptions_[group.get_index()];
  wg.start_        = start_thread_idx;
  wg.thread_count_ = thread_count;
  wg.priority_     = priority;
  worker_count_    = std::max(worker_count_, wg.start_ + wg.thread_count_);
}

auto scheduler::create_group(uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority) -> workgroup_id
{
  // Find next available group ID
  auto& workgroup_descs = initializer_->workgroup_descriptions_;
  for (uint32_t i = 0; i < detail::v2::max_workgroup; ++i)
  {
    if (workgroup_descs[i].thread_count_ == 0)
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
  enter_context(worker_id(0), workgroup_id(0));
}

void scheduler::busy_work(worker_id thread) noexcept
{
  auto& worker = workers_[thread.get_index()];
  // We need to preserve the worker's context and group information and restore it after busy work
  auto preserve_group = worker.get_workgroup();

  // Try to find work for the worker during busy wait
  // Optimized work stealing with adaptive attempts
  constexpr uint32_t min_attempts      = 1;
  constexpr uint32_t max_attempts      = 3;
  constexpr uint32_t failure_threshold = 5;

  // Use thread_local failure tracking for better performance
  thread_local uint32_t local_recent_failures = 0;
  uint32_t              attempts = (local_recent_failures > failure_threshold) ? min_attempts : max_attempts;

  // If thi

  for (uint32_t attempt = 0; attempt < attempts; ++attempt)
  {
    if (find_work_for_worker(thread)) [[likely]]
    {
      local_recent_failures = std::max(0U, local_recent_failures - 1);
      return; // Found and executed work
    }

    // Shorter pause for first attempts, longer for subsequent ones
    if (attempt < attempts - 1)
    {
      ouly::detail::pause_exec();
    }
  }

  if (worker.get_workgroup() != preserve_group)
  {
    constexpr uint32_t max_context_enter_attempts = 1000;
    uint32_t           enter_attempts             = 0;
    while (worker.get_workgroup() != preserve_group && enter_attempts < max_context_enter_attempts)
    {
      if (enter_context(thread, preserve_group))
      {
        break; // Successfully entered the preserved group context
      };

      ++enter_attempts;
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
