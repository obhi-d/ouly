// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/detail/worker_v2.hpp"
#include "ouly/scheduler/detail/workgroup_v2.hpp"
#include "ouly/scheduler/task.hpp"
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

// Type aliases for v2 implementation
using workgroup_v2                  = ouly::detail::v2::workgroup;
using worker_v2                     = ouly::detail::v2::worker;
using work_item_v2                  = ouly::detail::v2::work_item;
constexpr uint32_t max_workgroup_v2 = ouly::detail::v2::max_workgroup;

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
  void submit(task_context const& current, workgroup_id group, C const& task_obj) noexcept
  {
    auto work_fn = [address = task_obj.address()](task_context const&)
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
    requires(::ouly::detail::Callable<Lambda, ouly::task_context const&>)
  void submit(task_context const& current, workgroup_id group, Lambda&& data) noexcept
  {
    submit_internal(current, group, detail::v2::work_item::pbind(std::forward<Lambda>(data), group));
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
  void submit(task_context const& current, workgroup_id group, Class& ctx) noexcept
  {
    submit_internal(current, group, detail::v2::work_item::pbind<M>(ctx, group));
  }

  /**
   * @brief Submits a work item to the scheduler bound to a member function.
   * @tparam M Member function pointer to be bound to the work item
   * @param src Source worker ID submitting the work
   * @param group Workgroup ID for the submitted work
   */
  template <auto M>
  void submit(task_context const& current, workgroup_id group) noexcept
  {
    submit_internal(current, group, detail::v2::work_item::pbind<M>(group));
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
  void submit(task_context const& current, workgroup_id group, void (*callable)(task_context const&, Args...),
              Args&&... args) noexcept
  {
    submit_internal(current, group,
                    detail::v2::work_item::pbind(
                     callable, std::make_tuple<std::decay_t<Args>...>(std::forward<Args>(args)...), group));
  }

  /**
   * @brief Begin scheduler execution, group creation is frozen after this call.
   * @param entry An entry function can be provided that will be executed on all worker threads upon entry.
   * @param worker_count Number of worker threads to create (0 = hardware_concurrency)
   * @param user_context User context pointer passed to worker threads
   */
  OULY_API void begin_execution(scheduler_worker_entry&& entry = {}, uint32_t worker_count = {},
                                void* user_context = nullptr);

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
  void run_worker(worker_id worker_id);

  /**
   * @brief Find work for a specific worker
   */
  auto find_work_for_worker(worker_id worker_id) noexcept -> bool;

  /**
   * @brief Execute a work item
   */
  void execute_work(worker_id worker_id, detail::v2::work_item const& work) noexcept;

  /**
   * @brief Update worker assignments when workgroups change
   */
  void update_worker_assignments();

  struct worker_synchronizer;
  struct tally_publisher;

  using workgroup_list = ouly::detail::mpmc_ring<workgroup_v2*, max_workgroup_v2>;
  workgroup_list needy_workgroups_;

  std::condition_variable work_available_cv_;
  std::mutex              work_available_mutex_;

  std::atomic_bool                     stop_{false};
  std::atomic_int32_t                  wake_tokens_{0}; // Used to wake up workers when work is available
  std::atomic_int32_t                  sleeping_{0};
  std::shared_ptr<worker_synchronizer> synchronizer_ = nullptr;

  std::unique_ptr<detail::v2::worker[]>    workers_;
  std::unique_ptr<detail::v2::workgroup[]> workgroups_;

  std::vector<std::thread> threads_;

  // Scheduler state and configuration (cold data)
  scheduler_worker_entry entry_fn_;

  uint32_t worker_count_ = 0;
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
void async(task_context const& current, workgroup_id submit_group, Args&&... args)
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
void async(task_context const& current, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().submit<M>(current.get_worker(), submit_group, std::forward<Args>(args)...);
}

} // namespace ouly::inline v2
