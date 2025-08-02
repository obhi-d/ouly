// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/detail/v1/worker.hpp"
#include "ouly/scheduler/v1/task_context.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/common.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/type_traits.hpp"
#include <array>
#include <atomic>
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
 * ouly::async(context, group_id, [](ouly::task_context const& ctx) {
 *   // Task work here
 * });
 *
 * // Submit coroutine task
 * auto task = continue_string();
 * scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, task);
 *
 * // Parallel for loop
 * ouly::parallel_for([](int a, ouly::task_context const& ctx) {
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
  auto operator=(const scheduler&) -> scheduler& = delete;

  scheduler(scheduler&& other) noexcept
      : worker_count_(other.worker_count_), stop_(other.stop_.load()), workers_(std::move(other.workers_)),
        group_ranges_(std::move(other.group_ranges_)), wake_data_(std::move(other.wake_data_)),
        workgroups_(std::move(other.workgroups_)), threads_(std::move(other.threads_)),
        entry_fn_(std::move(other.entry_fn_))
  {
    other.worker_count_ = 0;
  }

  auto operator=(scheduler&& other) noexcept -> scheduler&
  {
    if (this != &other)
    {
      workgroups_         = std::move(other.workgroups_);
      workers_            = std::move(other.workers_);
      worker_count_       = other.worker_count_;
      stop_               = other.stop_.load();
      entry_fn_           = std::move(other.entry_fn_);
      group_ranges_       = std::move(other.group_ranges_);
      wake_data_          = std::move(other.wake_data_);
      other.worker_count_ = 0;
    }
    return *this;
  }

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
   * @see task_context
   */
  template <CoroutineTask C>
  void submit(ouly::v1::task_context const& src, workgroup_id group, C const& task_obj) noexcept
  {
    submit_internal(src.get_worker(), group,
                    ouly::v1::task_delegate::bind(
                     [address = task_obj.address()](ouly::v1::task_context const&)
                     {
                       std::coroutine_handle<>::from_address(address).resume();
                     }));
  }

  /**
   * @brief Overloads that deduce the workgroup from the current task context.
   *
   * These overloads allow callers to omit the workgroup ID when the task should run
   * in the same workgroup as the submitting context. They forward the work to the
   * existing submit() overloads that take an explicit workgroup.
   */

  // Coroutine task submission without explicit group
  template <CoroutineTask C>
  void submit(ouly::v1::task_context const& current, C const& task_obj) noexcept
  {
    submit(current, current.get_workgroup(), task_obj);
  }

  /**
   * @brief Submits a work item to be executed by the scheduler.
   *
   * @tparam Lambda Type of the callable work item
   * @param src ID of the worker submitting the work item
   * @param group ID of the workgroup this item belongs to
   * @param data Callable object to be executed
   *
   * @requires Lambda must be callable with ouly::task_context const& parameter
   *
   * @note This function is noexcept and will forward the lambda to the internal submit implementation
   */
  template <typename Lambda>
    requires(std::invocable<Lambda, ouly::v1::task_context const&> &&
             !std::is_same_v<std::decay_t<Lambda>, ouly::v1::task_delegate>)
  void submit(ouly::v1::task_context const& src, workgroup_id group, Lambda&& data) noexcept
  {
    submit_internal(src.get_worker(), group, ouly::v1::task_delegate::bind(std::forward<Lambda>(data)));
  }

  // Callable/lambda submission without explicit group
  template <typename Lambda>
    requires(std::invocable<Lambda, ouly::v1::task_context const&> &&
             !std::is_same_v<std::decay_t<Lambda>, ouly::v1::task_delegate>)
  void submit(ouly::v1::task_context const& current, Lambda&& data) noexcept
  {
    submit(current, current.get_workgroup(), std::forward<Lambda>(data));
  }

  /**
   * @brief Submits a work item to the scheduler
   * @param src The current task context submitting the work
   * @param group The workgroup ID that this task belongs to
   * @param ptr The function pointer to be executed
   * @param args The arguments to be passed to the function pointer as packaged arguments
   */
  template <typename... PackArgs>
  void submit(ouly::v1::task_context const& src, workgroup_id group, ouly::v1::task_delegate::function_type ptr,
              PackArgs&&... args) noexcept
  {
    submit_internal(src.get_worker(), group, ouly::v1::task_delegate::bind(ptr, std::forward<PackArgs>(args)...));
  }

  /**
   * @brief Submits a work item to the scheduler
   * @param src The current task context submitting the work
   * @param group The workgroup ID that this task belongs to
   * @param ptr The function pointer to be executed
   * @param args The arguments to be passed to the function pointer as packaged arguments
   */
  template <typename... PackArgs>
  void submit(ouly::v1::task_context const& src, ouly::v1::task_delegate::function_type ptr,
              PackArgs&&... args) noexcept
  {
    submit_internal(src.get_worker(), src.get_workgroup(),
                    ouly::v1::task_delegate::bind(ptr, std::forward<PackArgs>(args)...));
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

  [[nodiscard]] auto get_context(task_context const& wctx, workgroup_id group) const -> task_context const&
  {
    return workers_[wctx.get_worker().get_index()].get().contexts_[group.get_index()];
  }

  /**
   * @brief If multiple schedulers are active, this function should be called from main thread before using the
   * scheduler
   */
  void take_ownership() noexcept;
  void busy_work(worker_id /*thread*/) noexcept;
  void busy_work(v1::task_context const& ctx) noexcept
  {
    busy_work(ctx.get_worker());
  }

  OULY_API void wait_for_tasks();

private:
  /**
   * @brief Submit a work for execution
   */
  void submit_internal(worker_id src, workgroup_id dst, ouly::v1::task_delegate const& work);

  void assign_priority_order();
  auto compute_group_range(uint32_t worker_index) -> bool;

  void        finish_pending_tasks();
  inline void do_work(workgroup_id id, worker_id /*thread*/, ouly::v1::task_delegate& /*work*/) noexcept;
  void        wake_up(worker_id /*thread*/) noexcept;
  void        run_worker(worker_id /*thread*/);
  auto        get_work(worker_id thread, ouly::v1::task_delegate& work) noexcept -> workgroup_id;

  auto work(worker_id /*thread*/) noexcept -> bool;

  [[nodiscard]] auto has_work() const -> bool;

  uint32_t             worker_count_ = 0;
  std::atomic_bool     stop_         = false;
  std::atomic_uint32_t finished_     = 0; // Used to ack all finished workers

  static constexpr std::size_t cache_line_size = ouly::detail::cache_line_size;

  // Cache-aligned wake data to prevent false sharing
  struct wake_data
  {
    std::atomic_bool      status_{false};
    std::binary_semaphore event_{0};
  };

  using aligned_worker    = ouly::detail::cache_optimized_data<ouly::detail::v1::worker>;
  using aligned_wake_data = ouly::detail::cache_optimized_data<wake_data>;

  // Memory layout optimization: Allocate all scheduler data in a single block
  // for better cache locality and reduced allocator overhead
  // Hot data: accessed frequently during task execution
  std::unique_ptr<aligned_worker[]>                workers_;
  std::unique_ptr<ouly::detail::v1::group_range[]> group_ranges_;
  std::unique_ptr<aligned_wake_data[]>             wake_data_;

  // Work groups - frequently accessed during work stealing
  std::vector<ouly::detail::v1::workgroup> workgroups_;

  std::vector<std::thread> threads_;

  // Scheduler state and configuration (cold data)
  scheduler_worker_entry entry_fn_;
};

} // namespace ouly::v1
