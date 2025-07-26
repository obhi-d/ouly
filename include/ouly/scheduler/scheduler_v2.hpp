// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/detail/worker_v2.hpp"
#include "ouly/scheduler/detail/workgroup_v2.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/type_traits.hpp"
#include <array>
#include <condition_variable>
#include <mutex>
#include <new>
#include <thread>

namespace ouly::inline v2
{
/**
 * @brief A task scheduler with TBB-style workgroup architecture using Chase-Lev work-stealing queues
 *
 * New Architecture Features:
 * - Each workgroup contains Chase-Lev work-stealing queues (one per worker)
 * - Workgroups advertise work availability to the scheduler
 * - Global condition variable for worker notification when work is available
 * - Mailbox system for cross-workgroup work submission
 * - TBB-style scheduler assigns workers to needy workgroups
 *
 * Key improvements over the old architecture:
 * - Better work-stealing performance with Chase-Lev queues
 * - Reduced contention through workgroup-local queues
 * - More efficient cross-workgroup communication via mailboxes
 * - Better load balancing through centralized work advertisement
 *
 * All existing APIs are preserved for backward compatibility:
 * - submit() methods work exactly the same
 * - worker_context API remains unchanged
 * - Workgroup creation and management APIs are identical
 *
 * @note The scheduler must be started with begin_execution() before submitting tasks
 * @note Work group creation is frozen after begin_execution() is called
 * @note Only one scheduler should be active at a time, use take_ownership() if multiple exist
 */
class scheduler
{
public:
  static constexpr uint32_t work_scale = 4;

  OULY_API scheduler() noexcept                  = default;
  OULY_API scheduler(const scheduler&)           = delete;
  scheduler(scheduler&&)                         = delete;
  auto operator=(const scheduler&) -> scheduler& = delete;
  auto operator=(scheduler&&) -> scheduler&      = delete;
  ~scheduler() noexcept;

  /**
   * @brief Submits a coroutine-based task to be executed by the scheduler
   * @param src The ID of the worker submitting the task
   * @param group The workgroup ID that this task belongs to
   * @param task_obj The task object containing the coroutine to be resumed
   */
  template <CoroutineTask C>
  void submit(worker_context const& current, workgroup_id group, C const& task_obj) noexcept
  {
    auto work_fn = [address = task_obj.address()](worker_context const&)
    {
      std::coroutine_handle<>::from_address(address).resume();
    };
    submit_internal(current, group, std::move(work_fn));
  }

  /**
   * @brief Submits a work item to be executed by the scheduler.
   * @tparam Lambda Type of the callable work item
   * @param src ID of the worker submitting the work item
   * @param group ID of the workgroup this item belongs to
   * @param data Callable object to be executed
   */
  template <typename Lambda>
    requires(ouly::detail::Callable<Lambda, ouly::worker_context const&>)
  void submit(worker_context const& current, workgroup_id group, Lambda&& data) noexcept
  {
    submit_internal(current, group, detail::work_item::pbind(std::forward<Lambda>(data), group));
  }

  /**
   * @brief Submits a member function to be executed as a work item in the scheduler
   * @tparam M Member function pointer to be executed
   * @tparam Class Type of the context object
   * @param src ID of the worker submitting the work item
   * @param group Workgroup ID for the work item
   * @param ctx Reference to the context object
   */
  template <auto M, typename Class>
  void submit(worker_context const& current, workgroup_id group, Class& ctx) noexcept
  {
    submit_internal(current, group, detail::work_item::pbind<M>(ctx, group));
  }

  /**
   * @brief Submits a work item to the scheduler bound to a member function.
   * @tparam M Member function pointer to be bound to the work item
   * @param src Source worker ID submitting the work
   * @param group Workgroup ID for the submitted work
   */
  template <auto M>
  void submit(worker_context const& current, workgroup_id group) noexcept
  {
    submit_internal(current, group, detail::work_item::pbind<M>(group));
  }

  /**
   * @brief Submits a task to be executed by the scheduler.
   * @tparam Args Variadic template parameter pack for callable arguments
   * @param src Source worker ID submitting the task
   * @param group Target workgroup ID where the task will be executed
   * @param callable Function pointer to the task to be executed
   * @param args Arguments to be forwarded to the callable
   */
  template <typename... Args>
  void submit(worker_context const& current, workgroup_id group, void (*callable)(worker_context const&, Args...),
              Args&&... args) noexcept
  {
    submit_internal(
     current, group,
     detail::work_item::pbind(callable, std::make_tuple<std::decay_t<Args>...>(std::forward<Args>(args)...), group));
  }

  /**
   * @brief Begin scheduler execution, group creation is frozen after this call.
   * @param entry An entry function can be provided that will be executed on all worker threads upon entry.
   * @param user_context User context pointer passed to worker threads
   */
  OULY_API void begin_execution(scheduler_worker_entry&& entry = {}, void* user_context = nullptr);

  /**
   * @brief Wait for threads to finish executing and end scheduler execution.
   */
  OULY_API void end_execution();

  /**
   * @brief Get worker count in the scheduler
   */
  [[nodiscard]] auto get_worker_count() const noexcept -> uint32_t
  {
    return worker_count_;
  }

  /**
   * @brief Ensure a work-group by id and set a name
   */
  OULY_API void create_group(workgroup_id group, uint32_t thread_offset, uint32_t thread_count, uint32_t priority = 0);

  /**
   * @brief Get the next available group
   */
  OULY_API auto create_group(uint32_t thread_offset, uint32_t thread_count, uint32_t priority = 0) -> workgroup_id;

  /**
   * @brief Clear a group, and re-create it
   */
  OULY_API void clear_group(workgroup_id group);

  /**
   * @brief Get worker count in this group
   */
  [[nodiscard]] auto get_worker_count(workgroup_id g) const noexcept -> uint32_t
  {
    return new_workgroups_[g.get_index()].get_thread_count();
  }

  /**
   * @brief Get worker start index
   */
  [[nodiscard]] auto get_worker_start_idx(workgroup_id g) const noexcept -> uint32_t
  {
    return new_workgroups_[g.get_index()].get_start_thread_idx();
  }

  /**
   * @brief Get logical divisor for workgroup
   */
  [[nodiscard]] auto get_logical_divisor(workgroup_id g) const noexcept -> uint32_t
  {
    return new_workgroups_[g.get_index()].get_thread_count() * work_scale;
  }

  [[nodiscard]] auto get_context(worker_id worker, workgroup_id group) const -> worker_context const&
  {
    return memory_block_.workers_[worker.get_index()].get().contexts_[group.get_index()];
  }

  /**
   * @brief If multiple schedulers are active, this function should be called from main thread before using the
   * scheduler
   */
  OULY_API void take_ownership() noexcept;

  /**
   * @brief Worker busy work loop - called when worker has no immediate work
   */
  OULY_API void busy_work(worker_id /*thread*/) noexcept;

private:
  /**
   * @brief Submit a work for execution - new implementation using mailbox system
   */
  OULY_API void submit_internal(worker_id src, workgroup_id dst, ouly::detail::work_item const& work);

  // New TBB-style scheduler methods
  void run_worker(worker_id worker_id);
  void notify_workers_work_available() noexcept;
  auto find_work_for_worker(worker_id worker_id) noexcept -> bool;
  auto try_get_work_from_workgroup(worker_id worker_id, workgroup_id group_id) noexcept -> bool;
  void execute_work(worker_id worker_id, ouly::detail::work_item& work) noexcept;

  // Legacy methods for compatibility - will be updated to use new architecture
  void assign_priority_order();
  auto compute_group_range(uint32_t worker_index) -> bool;
  void compute_steal_mask(uint32_t worker_index) const;
  void finish_pending_tasks() noexcept;
  void wake_up(worker_id /*thread*/) noexcept;
  void finalize_worker(worker_id /*thread*/) noexcept;

  struct worker_synchronizer;

  uint32_t         worker_count_ = 0;
  std::atomic_bool stop_         = false;

  static constexpr std::size_t cache_line_size = detail::cache_line_size;

  // Cache-aligned wake data to prevent false sharing
  struct wake_data
  {
    std::atomic_bool         status_{false};
    ouly::detail::wake_event event_;
  };

  using aligned_worker    = detail::cache_optimized_data<ouly::detail::worker>;
  using aligned_wake_data = detail::cache_optimized_data<wake_data>;

  // Memory layout optimization: Allocate all scheduler data in a single block
  struct scheduler_memory_block
  {
    std::unique_ptr<aligned_worker[]>            workers_;
    std::unique_ptr<ouly::detail::group_range[]> group_ranges_;
    std::unique_ptr<aligned_wake_data[]>         wake_data_;
  } memory_block_;

  // New workgroups with Chase-Lev architecture
  std::vector<ouly::detail::new_workgroup> new_workgroups_;

  // Global condition variable for work availability notification (TBB-style)
  mutable std::mutex              global_work_mutex_;
  mutable std::condition_variable global_work_cv_;
  std::atomic<bool>               has_global_work_{false};

  std::shared_ptr<worker_synchronizer> synchronizer_ = nullptr;
  std::vector<std::thread>             threads_;

  // Scheduler state and configuration (cold data)
  scheduler_worker_entry entry_fn_;
};

/**
 * @brief Asynchronously submits a task to the scheduler
 *
 * This function forwards the given arguments to the scheduler's submit function,
 * allowing tasks to be queued for asynchronous execution in the specified workgroup.
 *
 * @tparam Args Variadic template parameter pack for forwarded arguments
 * @param current The current worker context from which the task is being submitted
 * @param submit_group The workgroup identifier where the task should be scheduled
 * @param args Arguments to be forwarded to the task
 *
 * @note This is a convenience wrapper around scheduler::submit()
 */
template <typename... Args>
void async(worker_context const& current, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().submit(current.get_worker(), submit_group, std::forward<Args>(args)...);
}

/**
 * @brief Submits a task asynchronously to the scheduler
 *
 * @tparam M Work item type to be submitted
 * @tparam Args Variable template parameter pack for work item arguments
 *
 * @param current Current worker context that will submit the work
 * @param submit_group Target workgroup ID where the work will be submitted
 * @param args Arguments to be forwarded to the work item constructor
 *
 * @note This is a helper function that forwards the submission request to the scheduler
 *       associated with the current worker context
 */
template <auto M, typename... Args>
void async(worker_context const& current, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().submit<M>(current.get_worker(), submit_group, std::forward<Args>(args)...);
}

} // namespace ouly::inline v2
