
#pragma once
#include <acl/scheduler/detail/worker.hpp>
#include <acl/utility/config.hpp>
#include <acl/utility/type_traits.hpp>
#include <array>
#include <thread>

namespace acl
{

using scheduler_worker_entry = std::function<void(worker_desc)>;

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
 * acl::scheduler scheduler;
 * scheduler.create_group(acl::workgroup_id(0), 0, 16);  // 16 workers starting at index 0
 * scheduler.create_group(acl::workgroup_id(1), 16, 2);  // 2 workers starting at index 16
 *
 * // Begin execution
 * scheduler.begin_execution();
 *
 * // Submit lambda task
 * acl::async(context, group_id, [](acl::worker_context const& ctx) {
 *   // Task work here
 * });
 *
 * // Submit coroutine task
 * auto task = continue_string();
 * scheduler.submit(acl::main_worker_id, acl::default_workgroup_id, task);
 *
 * // Parallel for loop
 * acl::parallel_for([](int a, acl::worker_context const& ctx) {
 *   // Process element a
 * }, std::span(data), acl::default_workgroup_id);
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
class scheduler
{

public:
  static constexpr uint32_t work_scale = 4;

  ACL_API scheduler() noexcept                   = default;
  ACL_API scheduler(const scheduler&)            = delete;
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
  void submit(worker_id src, workgroup_id group, C const& task_obj) noexcept
  {
    submit(src, group,
           acl::detail::work_item::pbind(
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
   * @requires Lambda must be callable with acl::worker_context const& parameter
   *
   * @note This function is noexcept and will forward the lambda to the internal submit implementation
   */
  template <typename Lambda>
    requires(acl::detail::Callable<Lambda, acl::worker_context const&>)
  void submit(worker_id src, workgroup_id group, Lambda&& data) noexcept
  {
    submit(src, group, acl::detail::work_item::pbind(std::forward<Lambda>(data), group));
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
  void submit(worker_id src, workgroup_id group, Class& ctx) noexcept
  {
    submit(src, group, acl::detail::work_item::pbind<M>(ctx, group));
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
  void submit(worker_id src, workgroup_id group) noexcept
  {
    submit(src, group, acl::detail::work_item::pbind<M>(group));
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
  void submit(worker_id src, workgroup_id group, task_delegate::fnptr callable, Args&&... args) noexcept
  {
    submit(src, group,
           acl::detail::work_item::pbind(callable, std::make_tuple<std::decay_t<Args>...>(std::forward<Args>(args)...),
                                         group));
  }

  /**
   * @brief Submits a coroutine task from one worker to another within a workgroup
   *
   * @tparam C Type satisfying CoroutineTask concept
   * @param src Source worker ID from where the task is submitted
   * @param dst Destination worker ID where the task will be executed
   * @param group Workgroup ID that the task belongs to
   * @param task_obj The coroutine task object to be submitted
   *
   * @details This method creates a work item that resumes the coroutine represented by the task object
   *          when executed. The task is bound to the specified workgroup and transferred from the source
   *          worker to the destination worker.
   *
   * @note This operation is noexcept and is thread-safe
   */
  template <CoroutineTask C>
  void submit(worker_id src, worker_id dst, workgroup_id group, C const& task_obj) noexcept
  {
    submit(src, dst,
           acl::detail::work_item::pbind(
            [address = task_obj.address()](worker_context const&)
            {
              std::coroutine_handle<>::from_address(address).resume();
            },
            group));
  }

  /**
   * @brief Submits a work item from one worker to another within a workgroup
   *
   * @tparam Lambda A callable type that can be invoked with acl::worker_context const&
   * @param src Source worker ID from which the work is submitted
   * @param dst Destination worker ID to which the work is submitted
   * @param group Workgroup ID to which this work item belongs
   * @param data Callable object to be executed on the destination worker
   *
   * @note This function is noexcept and will not throw exceptions
   *
   * @requires Lambda must satisfy the Callable concept with acl::worker_context const&
   */
  template <typename Lambda>
    requires(acl::detail::Callable<Lambda, acl::worker_context const&>)
  void submit(worker_id src, worker_id dst, workgroup_id group, Lambda&& data) noexcept
  {
    submit(src, dst, acl::detail::work_item::pbind(std::forward<Lambda>(data), group));
  }

  /**
   * @brief Submits a member function to be executed by a worker thread
   *
   * @tparam M Member function pointer to be executed
   * @tparam Class Type of the context object
   *
   * @param src Source worker ID initiating the work submission
   * @param dst Destination worker ID that will execute the work
   * @param group Workgroup identifier for the submitted work
   * @param ctx Context object instance on which the member function will be executed
   *
   * @note This function is marked noexcept and will not throw exceptions
   *
   * This is a convenience overload that binds a member function to a context object
   * and creates a work item for execution. It internally uses acl::detail::work_item::pbind
   * to create the bound work item.
   */
  template <auto M, typename Class>
  void submit(worker_id src, worker_id dst, workgroup_id group, Class& ctx) noexcept
  {
    submit(src, dst, acl::detail::work_item::pbind<M>(ctx, group));
  }

  /**
   * @brief Submit a work item between workers with a bound member function.
   *
   * @tparam M Member function pointer to be bound
   * @param src Source worker ID
   * @param dst Destination worker ID
   * @param group Workgroup ID to associate with the work item
   *
   * @note This is a convenience overload that automatically creates a work item
   *       using the provided member function pointer and workgroup.
   */
  template <auto M>
  void submit(worker_id src, worker_id dst, workgroup_id group) noexcept
  {
    submit(src, dst, acl::detail::work_item::pbind<M>(group));
  }

  template <typename... Args>
  /**
   * @brief Submits a task to be executed by a worker.
   *
   * This method submits a callable task from a source worker to be executed by a destination worker,
   * with associated arguments and workgroup identifier.
   *
   * @tparam Args Variadic template parameter for forwarded arguments
   * @param src Source worker ID that submits the task
   * @param dst Destination worker ID that will execute the task
   * @param group Workgroup identifier for the task
   * @param callable Function pointer to be executed
   * @param args Arguments to be forwarded to the callable
   *
   * @note This overload is noexcept and internally binds the arguments to the callable
   *       before submission.
   */
  void submit(worker_id src, worker_id dst, workgroup_id group, task_delegate::fnptr callable, Args&&... args) noexcept
  {
    submit(src, dst,
           acl::detail::work_item::pbind(callable, std::make_tuple<std::decay_t<Args>...>(std::forward<Args>(args)...),
                                         group));
  }

  /**
   * @brief Submit a work for execution in the exclusive worker thread
   */
  ACL_API void submit(worker_id src, worker_id dst, acl::detail::work_item work);

  /**
   * @brief Submit a work for execution
   */
  ACL_API void submit(worker_id src, workgroup_id dst, acl::detail::work_item work);

  /**
   * @brief Begin scheduler execution, group creation is frozen after this call.
   * @param entry An entry function can be provided that will be executed on all worker threads upon entry.
   */
  ACL_API void begin_execution(scheduler_worker_entry&& entry = {}, void* user_context = nullptr);
  /**
   * @brief Wait for threads to finish executing and end scheduler execution. Scheduler execution can be restarted
   * using begin_execution. Unlocks scheduler and makes it mutable.
   */
  ACL_API void end_execution();

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
  ACL_API void create_group(workgroup_id group, uint32_t thread_offset, uint32_t thread_count, uint32_t priority = 0);
  /**
   * @brief Get the next available group. Group priority controls if a thread is shared between multiple groups, which
   * group is executed first by the thread
   */
  ACL_API auto create_group(uint32_t thread_offset, uint32_t thread_count, uint32_t priority = 0) -> workgroup_id;
  /**
   * @brief Clear a group, and re-create it
   */
  ACL_API void clear_group(workgroup_id group);
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

  auto get_context(worker_id worker, workgroup_id group) -> worker_context const&
  {
    return workers_[worker.get_index()].contexts_[group.get_index()];
  }

  /**
   * @brief If multiple schedulers are active, this function should be called from main thread before using the
   * scheduler
   */
  ACL_API void take_ownership() noexcept;
  ACL_API void busy_work(worker_id /*thread*/) noexcept;

private:
  void        finish_pending_tasks() noexcept;
  inline void do_work(worker_id /*thread*/, acl::detail::work_item& /*work*/) noexcept;
  void        wake_up(worker_id /*thread*/) noexcept;
  void        run(worker_id /*thread*/);
  auto        get_work(worker_id /*thread*/) noexcept -> acl::detail::work_item;

  auto work(worker_id /*thread*/) noexcept -> bool;

  scheduler_worker_entry entry_fn_;
  // Work groups
  std::vector<acl::detail::workgroup> workgroups_;
  // Workers present in the scheduler
  std::unique_ptr<acl::detail::worker[]> workers_;
  // Local cache for work items, until they are pushed into global queue
  std::unique_ptr<acl::detail::work_item[]> local_work_;
  // Global work items
  std::unique_ptr<acl::detail::group_range[]> group_ranges_;
  std::unique_ptr<std::atomic_bool[]>         wake_status_;
  std::unique_ptr<acl::detail::wake_event[]>  wake_events_;
  std::vector<std::thread>                    threads_;

  uint32_t         worker_count_         = 0;
  uint32_t         logical_task_divisor_ = default_logical_task_divisior;
  std::atomic_bool stop_                 = false;
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

/**
 * @brief Asynchronously submits a task from one worker to another worker in the scheduler.
 *
 * This function forwards a task with the provided arguments from the current worker context
 * to a destination worker through the scheduler system.
 *
 * @tparam Args Variadic template parameter for forwarded arguments
 * @param current The context of the current worker submitting the task
 * @param dst The target worker ID where the task should be executed
 * @param submit_group The workgroup ID associated with this task submission
 * @param args Arguments to be forwarded to the task
 *
 * @note This is a helper function that internally calls the scheduler's submit method
 */
template <typename... Args>
void async(worker_context const& current, worker_id dst, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().submit(current.get_worker(), dst, submit_group, std::forward<Args>(args)...);
}

/**
 * @brief Asynchronously submits a task to be executed by a worker.
 *
 * This function allows scheduling work to be executed by a specific worker in the scheduler.
 * It forwards the task and its arguments to the scheduler's submit function.
 *
 * @tparam M The message/task type to be executed
 * @tparam Args Variadic template parameter for arguments
 * @param current The context of the current worker
 * @param dst The target worker ID where the task will be executed
 * @param submit_group The workgroup ID under which this task is submitted
 * @param args Arguments to be forwarded to the task
 */
template <auto M, typename... Args>
void async(worker_context const& current, worker_id dst, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().submit<M>(current.get_worker(), dst, submit_group, std::forward<Args>(args)...);
}

} // namespace acl
