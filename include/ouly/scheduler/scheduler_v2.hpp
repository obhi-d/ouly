// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/detail/worker_v2.hpp"
#include "ouly/scheduler/detail/workgroup_v2.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/type_traits.hpp"
#include <array>
#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <cstdint>
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

  OULY_API scheduler() noexcept                  = default;
  OULY_API scheduler(const scheduler&)           = delete;
  scheduler(scheduler&&)                         = delete;
  auto operator=(const scheduler&) -> scheduler& = delete;
  auto operator=(scheduler&&) -> scheduler&      = delete;
  ~scheduler() noexcept;

  /**
   * @brief Submits a coroutine-based task to be executed by the scheduler
   * @param current The current task context submitting the task
   * @param group The workgroup ID that this task belongs to
   * @param task_obj The task object containing the coroutine to be resumed
   *
   * This overload wraps the coroutine resume function in a delegate and binds the
   * workgroup ID as compressed data. This allows execute_work() to recover the
   * correct workgroup when the task runs.
   */
  template <CoroutineTask C>
  void submit(task_context const& current, workgroup_id group, C const& task_obj) noexcept
  {
    auto work_fn = [address = task_obj.address()](task_context const&)
    {
      std::coroutine_handle<>::from_address(address).resume();
    };
    submit_internal(current, group, detail::v2::work_item::bind(std::move(work_fn)));
  }

  /**
   * @brief Submits a work item to be executed by the scheduler.
   * @tparam Lambda Type of the callable work item
   * @param current ID of the worker submitting the work item
   * @param group ID of the workgroup this item belongs to
   * @param data Callable object to be executed
   */
  template <typename Lambda>
    requires(::ouly::detail::Callable<Lambda, task_context const&>)
  void submit(task_context const& current, workgroup_id group, Lambda&& data) noexcept
  {
    submit_internal(current, group, detail::v2::work_item::bind(std::forward<Lambda>(data)));
  }

  /**
   * @brief Submits a member function to be executed as a work item in the scheduler
   * @tparam M Member function pointer to be executed
   * @tparam Class Type of the context object
   * @param current Context of the submitting worker
   * @param group Workgroup ID for the work item
   * @param ctx Reference to the context object
   */
  template <auto M, typename Class>
  void submit(task_context const& current, workgroup_id group, Class& ctx) noexcept
  {
    submit_internal(current, group, detail::v2::work_item::bind<M>(ctx));
  }

  /**
   * @brief Submits a work item to the scheduler bound to a member function.
   * @tparam M Member function pointer to be bound to the work item
   * @param current Source worker context submitting the work
   * @param group Workgroup ID for the submitted work
   */
  template <auto M>
  void submit(task_context const& current, workgroup_id group) noexcept
  {
    submit_internal(current, group, detail::v2::work_item::bind<M>());
  }

  /**
   * @brief Submits a task to be executed by the scheduler.
   * @tparam Args Variadic template parameter pack for callable arguments
   * @param current Source worker context submitting the task
   * @param group Target workgroup ID where the task will be executed
   * @param callable Function pointer to the task to be executed
   * @param args Arguments to be forwarded to the callable
   */
  template <typename... Args>
  void submit(task_context const& current, workgroup_id group, void (*callable)(task_context const&, Args...),
              Args&&... args) noexcept
  {
    submit_internal(
     current, group,
     detail::v2::work_item::bind(callable, std::make_tuple<std::decay_t<Args>...>(std::forward<Args>(args)...)));
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
  void submit(task_context const& current, C const& task_obj) noexcept
  {
    submit(current, current.get_workgroup(), task_obj);
  }

  // Callable/lambda submission without explicit group
  template <typename Lambda>
    requires(::ouly::detail::Callable<Lambda, task_context const&>)
  void submit(task_context const& current, Lambda&& data) noexcept
  {
    submit(current, current.get_workgroup(), std::forward<Lambda>(data));
  }

  // Member function submission without explicit group
  template <auto M, typename Class>
  void submit(task_context const& current, Class& ctx) noexcept
  {
    submit<M>(current, current.get_workgroup(), ctx);
  }

  // Member function without object (static) submission without explicit group
  template <auto M>
  void submit(task_context const& current) noexcept
  {
    submit<M>(current, current.get_workgroup());
  }

  // Free function pointer submission without explicit group
  template <typename... Args>
  void submit(task_context const& current, void (*callable)(task_context const&, Args...), Args&&... args) noexcept
  {
    submit(current, current.get_workgroup(), callable, std::forward<Args>(args)...);
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

private:
  /**
   * @brief Submit a work for execution - new implementation using mailbox system
   */
  OULY_API void submit_internal(task_context const& current, workgroup_id dst, detail::v2::work_item const& work);

  /**
   * @brief Run worker thread main loop
   */
  void run_worker(worker_id wid);

  /**
   * @brief Find work for a specific worker
   */
  auto find_work_for_worker(worker_id wid) noexcept -> bool;

  auto enter_context(worker_id wid, detail::v2::workgroup* needy_wg) noexcept -> bool;

  /**
   * @brief Execute a work item
   */
  void execute_work(worker_id wid, detail::v2::work_item const& work) noexcept;

  /*  */
  void finish_pending_tasks();
  void finalize_worker(worker_id wid);

  struct worker_synchronizer;
  struct tally_publisher;

  static constexpr uint32_t max_workgroup_v2 = ouly::detail::v2::max_workgroup;

  using workgroup_list = ouly::detail::mpmc_ring<ouly::detail::v2::workgroup*, ouly::detail::v2::mpmc_capacity>;
  workgroup_list needy_workgroups_;

  std::condition_variable work_available_cv_;
  std::mutex              work_available_mutex_;

  std::atomic_bool                     stop_{false};
  std::atomic_int32_t                  wake_tokens_{0}; // Used to wake up workers when work is available
  std::atomic_int32_t                  sleeping_{0};
  std::shared_ptr<worker_synchronizer> synchronizer_ = nullptr;

  std::unique_ptr<worker_v2[]> workers_;

  std::array<workgroup_v2, max_workgroup_v2> workgroups_;

  std::vector<std::thread> threads_;

  // Scheduler state and configuration (cold data)
  scheduler_worker_entry entry_fn_;

  uint32_t worker_count_    = 0;
  uint32_t workgroup_count_ = 0;
};

} // namespace ouly::inline v2
