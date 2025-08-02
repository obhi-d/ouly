// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/detail/v2/worker.hpp"
#include "ouly/scheduler/detail/v2/workgroup.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/type_traits.hpp"
#include <array>
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <new>
#include <semaphore>
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
 * - task_context API remains unchanged
 * - Workgroup creation and management APIs are identical
 *
 * @note The scheduler must be started with begin_execution() before submitting tasks
 * @note Work group creation is frozen after begin_execution() is called
 * @note Only one scheduler should be active at a time, use take_ownership() if multiple exist
 */
class scheduler
{
  // Type aliases for v2 implementation
  using workgroup_v2 = ouly::detail::v2::workgroup;
  using worker_v2    = ouly::detail::v2::worker;
  using work_item_v2 = ouly::detail::v2::work_item;

public:
  static constexpr uint32_t work_scale = 4;

  OULY_API scheduler() noexcept                      = default;
  OULY_API scheduler(const scheduler&)               = delete;
  auto     operator=(const scheduler&) -> scheduler& = delete;
  ~scheduler() noexcept;

  scheduler(scheduler&& other) noexcept
      : stop_(other.stop_.load()), initializer_(std::move(other.initializer_)), workers_(std::move(other.workers_)),
        workgroups_(std::move(other.workgroups_)), threads_(std::move(other.threads_)),
        entry_fn_(std::move(other.entry_fn_)), worker_count_(other.worker_count_),
        workgroup_count_(other.workgroup_count_)
  {
    other.worker_count_ = 0;
  }

  auto operator=(scheduler&& other) noexcept -> scheduler&
  {
    if (this != &other)
    {
      stop_            = other.stop_.load();
      initializer_     = std::move(other.initializer_);
      workers_         = std::move(other.workers_);
      workgroups_      = std::move(other.workgroups_);
      threads_         = std::move(other.threads_);
      entry_fn_        = std::move(other.entry_fn_);
      worker_count_    = other.worker_count_;
      workgroup_count_ = other.workgroup_count_;
    }
    return *this;
  }

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
  void submit(ouly::v2::task_context const& src, workgroup_id group, C const& task_obj) noexcept
  {
    submit_internal(src, group,
                    ouly::v2::task_delegate::bind(
                     [address = task_obj.address()](ouly::v2::task_context const&)
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
  void submit(ouly::v2::task_context const& current, C const& task_obj) noexcept
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
    requires(std::invocable<Lambda, ouly::v2::task_context const&> &&
             !std::is_same_v<std::decay_t<Lambda>, ouly::v2::task_delegate>)
  void submit(ouly::v2::task_context const& src, workgroup_id group, Lambda&& data) noexcept
  {
    submit_internal(src, group, ouly::v2::task_delegate::bind(std::forward<Lambda>(data)));
  }

  // Callable/lambda submission without explicit group
  template <typename Lambda>
    requires(std::invocable<Lambda, ouly::v2::task_context const&> &&
             !std::is_same_v<std::decay_t<Lambda>, ouly::v2::task_delegate>)
  void submit(ouly::v2::task_context const& current, Lambda&& data) noexcept
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
  void submit(ouly::v2::task_context const& src, workgroup_id group, ouly::v2::task_delegate::function_type ptr,
              PackArgs&&... args) noexcept
  {
    submit_internal(src, group, ouly::v2::task_delegate::bind(ptr, std::forward<PackArgs>(args)...));
  }

  /**
   * @brief Submits a work item to the scheduler
   * @param src The current task context submitting the work
   * @param group The workgroup ID that this task belongs to
   * @param ptr The function pointer to be executed
   * @param args The arguments to be passed to the function pointer as packaged arguments
   */
  template <typename... PackArgs>
  void submit(ouly::v2::task_context const& src, ouly::v2::task_delegate::function_type ptr,
              PackArgs&&... args) noexcept
  {
    submit_internal(src, src.get_workgroup(), ouly::v2::task_delegate::bind(ptr, std::forward<PackArgs>(args)...));
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
  OULY_API void create_group(workgroup_id group, uint32_t start_thread_idx, uint32_t thread_count,
                             uint32_t priority = 0);

  /**
   * @brief Get the next available group
   */
  OULY_API auto create_group(uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority = 0) -> workgroup_id;

  /**
   * @brief Clear a group, and re-create it
   */
  OULY_API void clear_group(workgroup_id group);

  /**
   * @brief Get worker count in this group
   */
  [[nodiscard]] auto get_worker_count(workgroup_id g) const noexcept -> uint32_t;
  /**
   * @brief Get worker start index
   */
  [[nodiscard]] auto get_worker_start_idx(workgroup_id g) const noexcept -> uint32_t;
  /**
   * @brief Get logical divisor for workgroup
   */
  [[nodiscard]] auto get_logical_divisor(workgroup_id g) const noexcept -> uint32_t;

  /**
   * @brief If multiple schedulers are active, this function should be called from main thread before using the
   * scheduler
   */
  OULY_API void take_ownership() noexcept;

  /**
   * @brief Worker busy work loop - called when worker has no immediate work
   */
  OULY_API void busy_work(worker_id /*thread*/) noexcept;

  void busy_work(v2::task_context const& ctx) noexcept
  {
    busy_work(ctx.get_worker());
  }

  OULY_API void wait_for_tasks();

private:
  /**
   * @brief Submit a work for execution - new implementation using mailbox system
   */
  OULY_API void submit_internal(ouly::v2::task_context const& current, workgroup_id dst,
                                detail::v2::work_item const& work);

  /**
   * @brief Run worker thread main loop
   */
  void run_worker(worker_id wid);

  /**
   * @brief Find work for a specific worker
   */
  auto find_work_for_worker(worker_id wid) noexcept -> bool;

  auto enter_context(worker_id wid, workgroup_id needy_wg) noexcept -> bool;

  /**
   * @brief Execute a work item
   */
  void execute_work(worker_id wid, detail::v2::work_item& work) noexcept;

  /**
   * @brief Wake up sleeping workers
   */
  void wake_up_workers(uint32_t count) noexcept;

  /*  */
  void               finish_pending_tasks();
  [[nodiscard]] auto has_work() const -> bool;

  struct worker_initializer;

  static constexpr uint32_t max_workgroup_v2 = ouly::detail::v2::max_workgroup;

  std::atomic_bool                    stop_{false};
  std::atomic_uint32_t                finished_{0};    // Used to ack all finished workers
  std::counting_semaphore<INT_MAX>    wake_tokens_{0}; // Used to wake up workers when work is available
  std::atomic_int32_t                 sleeping_{0};
  std::shared_ptr<worker_initializer> initializer_ = nullptr;

  std::unique_ptr<worker_v2[]> workers_;

  std::unique_ptr<workgroup_v2[]> workgroups_;
  std::vector<std::thread>        threads_;

  // Scheduler state and configuration (cold data)
  scheduler_worker_entry entry_fn_;

  uint32_t worker_count_    = 0;
  uint32_t workgroup_count_ = 0;
};

} // namespace ouly::inline v2
