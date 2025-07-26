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
#ifdef __linux__
#include <numa.h>
#include <sched.h>
#endif

namespace ouly::inline v2
{

// Using declarations to resolve namespace conflicts
using workgroup_type                   = ouly::detail::v2::workgroup;
using work_item_type                   = ouly::detail::v2::work_item;
using worker_type                      = ouly::detail::v2::worker;
constexpr uint32_t max_workgroup_count = ouly::detail::v2::max_workgroup;

namespace
{
// Random seed for work distribution
constexpr uint32_t initial_random_seed = 0xAAAAAAAAU;
constexpr uint32_t lcg_multiplier      = 1664525U;
constexpr uint32_t lcg_increment       = 1013904223U;
constexpr uint32_t initial_seed_mask   = 0xAAAAAAAAU;
constexpr uint32_t max_retry_attempts  = 100;
constexpr uint32_t max_backoff_shift   = 8U;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local uint32_t g_random_seed = initial_random_seed;

auto update_seed() -> uint32_t
{
  return (g_random_seed = (g_random_seed * lcg_multiplier + lcg_increment) & initial_seed_mask);
}

} // anonymous namespace

// Thread-local storage for worker information
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local detail::v2::worker const* g_worker = nullptr;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local ouly::worker_id g_worker_id = {};

auto task_context::this_context::get() noexcept -> task_context const&
{
  return g_worker->get_context();
}

auto task_context::this_context::get_worker_id() noexcept -> worker_id
{
  return g_worker_id;
}

struct scheduler::worker_synchronizer
{
  std::latch          job_board_freeze_ack_;
  std::barrier<>      published_tally_;
  std::atomic_int64_t tally_{0};
  void*               user_context_ = nullptr;

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

void scheduler::submit_internal(task_context const& current, workgroup_id dst,
                                ::ouly::detail::v2::work_item const& work)
{
  auto& target_workgroup = workgroups_[dst.get_index()];

  // First try to submit to the worker's own queue if it belongs to this workgroup
  worker_id current_worker = current.get_worker();
  uint32_t  worker_start   = get_worker_start_idx(dst);
  uint32_t  worker_count   = get_worker_count(dst);
  uint32_t  worker_end     = worker_start + worker_count;

  if (current_worker.get_index() >= worker_start && current_worker.get_index() < worker_end)
  {
    uint32_t local_offset = current_worker.get_index() - worker_start;
    if (target_workgroup.push_work_to_worker(local_offset, work))
    {
      // Work was successfully pushed, workgroup will advertise availability
      return;
    }
  }

  // Try to submit to mailbox for cross-workgroup submission
  if (target_workgroup.submit_to_mailbox(work))
  {
    // Add workgroup to needy list if it has work available
    ouly::detail::v2::workgroup* wg_ptr = &target_workgroup;
    needy_workgroups_.emplace(wg_ptr);

    // Notify waiting workers
    wake_tokens_.fetch_add(1, std::memory_order_release);
    work_available_cv_.notify_one();
  }
  else
  {
    // Mailbox is full - implement exponential backoff and retry
    for (uint32_t retry = 0; retry < max_retry_attempts; ++retry)
    {
      // Try random worker queue in the workgroup
      uint32_t random_offset = update_seed() % worker_count;
      if (target_workgroup.push_work_to_worker(random_offset, work))
      {
        return;
      }

      // Exponential backoff
      for (uint32_t i = 0; i < (1U << std::min(retry, max_backoff_shift)); ++i)
      {
        ouly::detail::pause_exec();
      }
    }

    // Final fallback - force push to mailbox (blocking)
    while (!target_workgroup.submit_to_mailbox(work))
    {
      busy_work(current_worker);
    }
  }
}

void scheduler::run_worker(worker_id worker_id)
{
  // Set thread-local storage
  g_worker_id = worker_id;
  g_worker    = &workers_[worker_id.get_index()];

  // Set CPU affinity for NUMA awareness
  set_worker_affinity(worker_id);

  // Execute the entry function if provided
  if (entry_fn_)
  {
    entry_fn_(worker_id);
  }

  // Main worker loop
  while (!stop_.load(std::memory_order_relaxed))
  {
    bool found_work = false;

    // Try to find work
    found_work = find_work_for_worker(worker_id);

    if (!found_work)
    {
      // Enter sleep state
      sleeping_.fetch_add(1, std::memory_order_relaxed);

      std::unique_lock<std::mutex> lock(work_available_mutex_);
      work_available_cv_.wait(lock,
                              [this]
                              {
                                return wake_tokens_.load(std::memory_order_acquire) > 0 ||
                                       stop_.load(std::memory_order_relaxed);
                              });

      // Exit sleep state
      sleeping_.fetch_sub(1, std::memory_order_relaxed);

      // Consume a wake token
      wake_tokens_.fetch_sub(1, std::memory_order_relaxed);
    }
  }
}

auto scheduler::find_work_for_worker(worker_id worker_id) noexcept -> bool
{
  // Priority 1: Check assigned workgroups' own queues
  for (uint32_t i = 0; i < detail::max_workgroup; ++i)
  {
    auto& workgroup = workgroups_[i];
    if (workgroup.get_thread_count() == 0)
    {
      continue;
    }

    uint32_t worker_start = get_worker_start_idx(workgroup_id{i});
    uint32_t worker_count = get_worker_count(workgroup_id{i});
    uint32_t worker_end   = worker_start + worker_count;

    // Check if this worker belongs to this workgroup
    if (worker_id.get_index() >= worker_start && worker_id.get_index() < worker_end)
    {
      uint32_t local_offset = worker_id.get_index() - worker_start;

      detail::work_item work;
      if (workgroup.pop_work_from_worker(work, local_offset))
      {
        execute_work(worker_id, work);
        return true;
      }
    }
  }

  // Priority 2: Check needy workgroups for mailbox work
  detail::v2::workgroup* needy_wg = nullptr;
  if (needy_workgroups_.pop(needy_wg))
  {
    detail::work_item work;
    if (needy_wg->receive_from_mailbox(work))
    {
      execute_work(worker_id, work);
      return true;
    }

    // No work in mailbox, try stealing from this workgroup
    if (needy_wg->steal_work(work))
    {
      execute_work(worker_id, work);
      return true;
    }

    // Clear work available flag since we found no work
    needy_wg->clear_work_available();
  }

  // Priority 3: Work stealing from any workgroup
  for (uint32_t i = 0; i < detail::max_workgroup; ++i)
  {
    auto& workgroup = workgroups_[i];
    if (workgroup.get_thread_count() == 0)
    {
      continue;
    }

    detail::work_item work;
    if (workgroup.steal_work(work))
    {
      execute_work(worker_id, work);
      return true;
    }
  }

  return false;
}

void scheduler::execute_work(worker_id /*worker_id*/, detail::work_item const& work) noexcept
{
  auto& worker = workers_[g_worker_id.get_index()];
  // Create a copy since work_item expects mutable reference
  auto work_copy = work;
  work_copy(worker.get_context());
}

void scheduler::set_worker_affinity(worker_id worker_id) noexcept
{
#ifdef __linux__
  // Set CPU affinity for NUMA awareness
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(worker_id.get_index() % std::thread::hardware_concurrency(), &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

  // Set NUMA policy if available
  if (numa_available() != -1)
  {
    int node = worker_id.get_index() / (std::thread::hardware_concurrency() / numa_num_configured_nodes());
    numa_set_preferred(node);
  }
#else
  // On non-Linux platforms, this is a no-op
  (void)worker_id;
#endif
}

void scheduler::begin_execution(scheduler_worker_entry&& entry, uint32_t worker_count, void* user_context)
{
  entry_fn_ = std::move(entry);

  // Determine worker count
  if (worker_count == 0)
  {
    worker_count_ = std::thread::hardware_concurrency();
  }
  else
  {
    worker_count_ = worker_count;
  }

  // Initialize workers and workgroups
  workers_    = std::make_unique<detail::v2::worker[]>(worker_count_);
  workgroups_ = std::make_unique<detail::v2::workgroup[]>(detail::max_workgroup);

  // Initialize worker contexts
  for (uint32_t i = 0; i < worker_count_; ++i)
  {
    // Workers will be assigned to workgroups when workgroups are created
  }

  // Create synchronizer
  synchronizer_                = std::make_shared<worker_synchronizer>(worker_count_);
  synchronizer_->user_context_ = user_context;

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
    std::lock_guard<std::mutex> lock(work_available_mutex_);
    work_available_cv_.notify_all();
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
  synchronizer_.reset();
}

void scheduler::create_group(workgroup_id group, uint32_t thread_count, uint32_t priority)
{
  if (group.get_index() >= detail::max_workgroup)
  {
    return; // Invalid group ID
  }

  auto& wg = workgroups_[group.get_index()];
  wg.initialize(thread_count, priority, this);

  // Update worker assignments
  update_worker_assignments();
}

auto scheduler::create_group(uint32_t thread_count, uint32_t priority) -> workgroup_id
{
  // Find next available group ID
  for (uint32_t i = 0; i < detail::max_workgroup; ++i)
  {
    if (workgroups_[i].get_thread_count() == 0)
    {
      workgroup_id new_group{i};
      create_group(new_group, thread_count, priority);
      return new_group;
    }
  }

  return workgroup_id{0}; // Fallback to group 0 if all are used
}

void scheduler::clear_group(workgroup_id group)
{
  if (group.get_index() < detail::max_workgroup)
  {
    workgroups_[group.get_index()].clear();
    update_worker_assignments();
  }
}

void scheduler::update_worker_assignments()
{
  uint32_t next_worker = 0;

  for (uint32_t i = 0; i < detail::max_workgroup && next_worker < worker_count_; ++i)
  {
    auto&    wg              = workgroups_[i];
    uint32_t wg_thread_count = wg.get_thread_count();

    if (wg_thread_count > 0)
    {
      uint32_t assigned_workers = std::min(wg_thread_count, worker_count_ - next_worker);

      // Assign workers to this workgroup
      for (uint32_t j = 0; j < assigned_workers; ++j)
      {
        workers_[next_worker + j].set_workgroup_info(j, workgroup_id{i});
      }

      wg.set_worker_range(next_worker, assigned_workers);
      next_worker += assigned_workers;
    }
  }
}

auto scheduler::get_worker_count(workgroup_id g) const noexcept -> uint32_t
{
  if (g.get_index() < detail::max_workgroup)
  {
    return workgroups_[g.get_index()].get_thread_count();
  }
  return 0;
}

auto scheduler::get_worker_start_idx(workgroup_id g) const noexcept -> uint32_t
{
  uint32_t start = 0;
  for (uint32_t i = 0; i < g.get_index() && i < detail::max_workgroup; ++i)
  {
    start += workgroups_[i].get_thread_count();
  }
  return start;
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
  find_work_for_worker(thread);
}

} // namespace ouly::inline v2
