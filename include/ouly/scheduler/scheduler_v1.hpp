// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/detail/worker_v1.hpp"
#include "ouly/utility/common.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/type_traits.hpp"
#include <array>
#include <functional>
#include <new>
#include <semaphore>
#include <thread>

namespace ouly::v1
{

static constexpr uint32_t default_logical_task_divisior = 64;
/**
 * @brief A task scheduler that manages concurrent execution across multiple worker threads and workgroups
 *
 * The scheduler allows organizing work into groups and submitting tasks for parallel execution.
 * Tasks can be submitted as coroutines, lambdas, member functions or function pointers.
 *
 * Example usage:
 * @code
 * // Create scheduler and workgroups
 * ouly::scheduler scheduler;
 * scheduler.create_group(ouly::workgroup_id(0), 0, 16);  // 16 workers starting at index 0
 * scheduler.create_group(ouly::workgroup_id(1), 16, 2);  // 2 workers starting at index 16
 *
 * // Begin execution
 * scheduler.begin_execution();
 *
 * // Submit lambda task
 * ouly::async(context, group_id, [](ouly::worker_context const& ctx) {
 *   // Task work here
 * });
 *
 * // Submit coroutine task
 * auto task = continue_string();
 * scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, task);
 *
 * // Parallel for loop
 * ouly::parallel_for([](int a, ouly::worker_context const& ctx) {
 *   // Process element a
 * }, std::span(data), ouly::default_workgroup_id);
 *
 * // Wait for completion
 * scheduler.end_execution();
 * @endcode
 *
 * Key features:
 * - Workgroup organization for logical task grouping
 * - Multiple task submission methods (coroutines, lambdas, etc)
 * - Parallel for loop execution
 * - Worker thread management and work stealing
 * - Priority-based scheduling between workgroups
 * - Thread affinity control via workgroup thread offset/count
 *
 * Common workgroup configurations:
 * - Default group: General purpose work
 * - Game logic group: Game simulation tasks
 * - Render group: Graphics/rendering tasks
 * - IO group: File/network operations
 * - Stream group: Media streaming tasks
 *
 * @note The scheduler must be started with begin_execution() before submitting tasks
 * @note Work group creation is frozen after begin_execution() is called
 * @note Only one scheduler should be active at a time, use take_ownership() if multiple exist
 */
class OULY_API scheduler
{

public:
  static constexpr uint32_t work_scale = 4;

  scheduler() noexcept                           = default;
  scheduler(const scheduler&)                    = delete;
  scheduler(scheduler&&)                         = delete;
  auto operator=(const scheduler&) -> scheduler& = delete;
  auto operator=(scheduler&&) -> scheduler&      = delete;
  ~scheduler() noexcept;

  /**
   * @brief Submits a coroutine-based task to be executed by the scheduler
   *
   * @param src The ID of the worker submitting the task
   * @param group The workgroup ID that this task belongs to
   * @param task_obj The task object containing the coroutine to be resumed
   *
   * @details This overload takes a task object that contains a coroutine and wraps it
   * in a work item that will resume the coroutine when executed. The task is associated
   * with the specified workgroup and submitted from the given worker.
   *
   * @note This function is noexcept and will not throw exceptions
   *
   * @see worker_id
   * @see workgroup_id
   * @see worker_context
   */
  template <CoroutineTask C>
  void submit(worker_context const& src, workgroup_id group, C const& task_obj) noexcept
  {
    submit_internal(src.get_worker(), group,
                    detail::work_item::pbind(
                     [address = task_obj.address()](worker_context const&)
                     {
                       std::coroutine_handle<>::from_address(address).resume();
                     },
                     group));
  }

  /**
   * @brief Submits a work item to be executed by the scheduler.
   *
   * @tparam Lambda Type of the callable work item
   * @param src ID of the worker submitting the work item
   * @param group ID of the workgroup this item belongs to
   * @param data Callable object to be executed
   *
   * @requires Lambda must be callable with ouly::worker_context const& parameter
   *
   * @note This function is noexcept and will forward the lambda to the internal submit implementation
   */
  template <typename Lambda>
    requires(ouly::detail::Callable<Lambda, ouly::worker_context const&>)
  void submit(worker_context const& src, workgroup_id group, Lambda&& data) noexcept
  {
    submit_internal(src.get_worker(), group, detail::work_item::pbind(std::forward<Lambda>(data), group));
  }

  /**
   * @brief Submits a member function to be executed as a work item in the scheduler
   *
   * @tparam M Member function pointer to be executed
   * @tparam Class Type of the context object
   * @param src ID of the worker submitting the work item
   * @param group Workgroup ID for the work item
   * @param ctx Reference to the context object
   *
   * @note This is a convenience overload that automatically binds the member function to the context
   * @note The function is noexcept and will not throw exceptions
   */
  template <auto M, typename Class>
  void submit(worker_context const& src, workgroup_id group, Class& ctx) noexcept
  {
    submit_internal(src.get_worker(), group, detail::work_item::pbind<M>(ctx, group));
  }

  /**
   * @brief Submits a work item to the scheduler bound to a member function.
   *
   * @tparam M Member function pointer to be bound to the work item
   * @param src Source worker ID submitting the work
   * @param group Workgroup ID for the submitted work
   *
   * @note This is a convenience overload that automatically creates a work item
   *       using the member function pointer and workgroup.
   */
  template <auto M>
  void submit(worker_context const& src, workgroup_id group) noexcept
  {
    submit_internal(src.get_worker(), group, detail::work_item::pbind<M>(group));
  }

  /**
   * @brief Submits a task to be executed by the scheduler.
   *
   * @tparam Args Variadic template parameter pack for callable arguments
   * @param src Source worker ID submitting the task
   * @param group Target workgroup ID where the task will be executed
   * @param callable Function pointer to the task to be executed
   * @param args Arguments to be forwarded to the callable
   *
   * @note This function is noexcept and will not throw exceptions
   *
   * This template function binds the given callable with its arguments and submits it
   * as a work item to the specified workgroup. The task will be executed by one of
   * the workers in the target workgroup.
   */
  template <typename... Args>
  void submit(worker_context const& src, workgroup_id group, task_delegate::fnptr callable, Args&&... args) noexcept
  {
    submit_internal(
     src.get_worker(), group,
     detail::work_item::pbind(callable, std::make_tuple<std::decay_t<Args>...>(std::forward<Args>(args)...), group));
  }

  /**
   * @brief Begin scheduler execution, group creation is frozen after this call.
   * @param entry An entry function can be provided that will be executed on all worker threads upon entry.
   */
  void begin_execution(scheduler_worker_entry&& entry = {}, void* user_context = nullptr);
  /**
   * @brief Wait for threads to finish executing and end scheduler execution. Scheduler execution can be restarted
   * using begin_execution. Unlocks scheduler and makes it mutable.
   */
  void end_execution();

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
  void create_group(workgroup_id group, uint32_t thread_offset, uint32_t thread_count, uint32_t priority = 0);
  /**
   * @brief Get the next available group. Group priority controls if a thread is shared between multiple groups, which
   * group is executed first by the thread
   */
  auto create_group(uint32_t thread_offset, uint32_t thread_count, uint32_t priority = 0) -> workgroup_id;
  /**
   * @brief Clear a group, and re-create it
   */
  void clear_group(workgroup_id group);
  /**
   * @brief Get worker count in this group
   */
  [[nodiscard]] auto get_worker_count(workgroup_id g) const noexcept -> uint32_t
  {
    return workgroups_[g.get_index()].thread_count_;
  }

  /**
   * @brief Get worker start index
   */
  [[nodiscard]] auto get_worker_start_idx(workgroup_id g) const noexcept -> uint32_t
  {
    return workgroups_[g.get_index()].start_thread_idx_;
  }

  /**
   * @brief Get worker d
   */
  [[nodiscard]] auto get_logical_divisor(workgroup_id g) const noexcept -> uint32_t
  {
    return workgroups_[g.get_index()].thread_count_ * work_scale;
  }

  [[nodiscard]] auto get_context(worker_context const& wctx, workgroup_id group) const -> worker_context const&
  {
    return memory_block_.workers_[wctx.get_worker().get_index()].get().contexts_[group.get_index()];
  }

  /**
   * @brief If multiple schedulers are active, this function should be called from main thread before using the
   * scheduler
   */
  void take_ownership() noexcept;
  void busy_work(worker_id /*thread*/) noexcept;

private:
  /**
   * @brief Submit a work for execution
   */
  void submit_internal(worker_id src, workgroup_id dst, detail::work_item const& work);

  void assign_priority_order();
  auto compute_group_range(uint32_t worker_index) -> bool;
  void compute_steal_mask(uint32_t worker_index) const;

  void        finish_pending_tasks() noexcept;
  inline void do_work(worker_id /*thread*/, detail::work_item& /*work*/) noexcept;
  void        wake_up(worker_id /*thread*/) noexcept;
  void        run(worker_id /*thread*/);
  void        finalize_worker(worker_id /*thread*/) noexcept;
  auto        get_work(worker_id /*thread*/, detail::work_item& /*work*/) noexcept -> bool;
  auto        try_steal_work(worker_id /*thread*/, detail::work_item& /*work*/) const noexcept -> bool;

  auto work(worker_id /*thread*/) noexcept -> bool;

  struct worker_synchronizer;
  struct tally_publisher;

  uint32_t         worker_count_ = 0;
  std::atomic_bool stop_         = false;

  static constexpr std::size_t cache_line_size = ouly::detail::cache_line_size;

  // Cache-aligned wake data to prevent false sharing
  struct wake_data
  {
    std::atomic_bool      status_{false};
    std::binary_semaphore event_{0};
  };

  using aligned_worker    = ouly::detail::cache_optimized_data<detail::worker>;
  using aligned_wake_data = ouly::detail::cache_optimized_data<wake_data>;

  // Memory layout optimization: Allocate all scheduler data in a single block
  // for better cache locality and reduced allocator overhead
  struct scheduler_memory_block
  {
    // Hot data: accessed frequently during task execution
    std::unique_ptr<aligned_worker[]>      workers_;
    std::unique_ptr<detail::group_range[]> group_ranges_;
    std::unique_ptr<aligned_wake_data[]>   wake_data_;
  } memory_block_;

  // Work groups - frequently accessed during work stealing
  std::vector<detail::workgroup>       workgroups_;
  std::shared_ptr<worker_synchronizer> synchronizer_ = nullptr;

  std::vector<std::thread> threads_;

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

} // namespace ouly::v1
