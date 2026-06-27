// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/co_task.hpp"
#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/v3/worker.hpp"
#include "ouly/scheduler/detail/v3/workgroup.hpp"
#include "ouly/scheduler/v3/task_context.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/type_traits.hpp"
#include <array>
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <thread>
#include <vector>

namespace ouly::v3
{

/**
 * @brief A work-stealing task scheduler designed for game engines.
 *
 * Why v3 exists: the v1/v2 schedulers keep worker threads hot — v1 workers never park at
 * all, and v2 workers spin whenever *any* task is executing anywhere, because work
 * availability is only cleared after a task finishes. In a game engine the scheduler runs
 * alongside render, audio, IO and driver threads, and burning every core on idle spinning
 * destroys frame pacing, laptop battery and thermal headroom.
 *
 * Design:
 * - Fixed workgroup membership: a worker belongs to the workgroups whose thread range
 *   covers it, decided at begin_execution(). No slot claiming, no migration, no locks on
 *   the hot path.
 * - Per (workgroup, member) Chase-Lev deque for local push/pop, per-workgroup MPMC
 *   mailbox for cross-group and external submissions.
 * - Accurate queue accounting: a workgroup advertises only items actually sitting in its
 *   queues. Idle workers therefore park even while long tasks execute elsewhere.
 * - Bounded spinning: a worker that finds no work spins a short, configurable window
 *   (set_idle_spin_count), then parks on a futex (C++20 atomic wait) eventcount. Lost
 *   wakeups are prevented by an epoch + sleeper-count protocol; there are no semaphore
 *   tokens to leak.
 * - Wake chaining: a worker that dequeues an item and observes more queued work wakes one
 *   more sleeper, so bursts (parallel_for) fan out without broadcast storms.
 * - wait_for_tasks() helps execute work, then blocks on a futex until all submitted tasks
 *   (queued and in-flight) complete — the main thread does not spin between frames.
 *
 * The public API mirrors v1/v2: submit() overloads, task_context, workgroup creation,
 * busy_work(), wait_for_tasks(), begin/end_execution().
 *
 * @note The scheduler must be started with begin_execution() before submitting tasks.
 * @note Workgroup creation is frozen after begin_execution() is called.
 * @note Tasks of a workgroup only execute on workers that are members of that workgroup.
 */
class scheduler
{
public:
  static constexpr uint32_t work_scale = 4;

  using delegate_type = ouly::v3::task_delegate;
  using context_type  = ouly::v3::task_context;

  OULY_API scheduler() noexcept                  = default;
  scheduler(scheduler const&)                    = delete;
  auto operator=(scheduler const&) -> scheduler& = delete;
  OULY_API ~scheduler() noexcept;

  /**
   * @brief Move is only valid before begin_execution() (no worker threads running).
   */
  scheduler(scheduler&& other) noexcept
      : workers_(std::move(other.workers_)), workgroups_(std::move(other.workgroups_)),
        threads_(std::move(other.threads_)), workgroup_descs_(other.workgroup_descs_),
        entry_fn_(std::move(other.entry_fn_)), worker_count_(other.worker_count_),
        workgroup_count_(other.workgroup_count_), idle_spin_limit_(other.idle_spin_limit_),
        stop_(other.stop_.load(std::memory_order_relaxed))
  {
    OULY_ASSERT(other.threads_.empty());
    other.worker_count_    = 0;
    other.workgroup_count_ = 0;
  }

  auto operator=(scheduler&& other) noexcept -> scheduler&
  {
    if (this != &other)
    {
      OULY_ASSERT(other.threads_.empty());
      stop_.store(other.stop_.load(std::memory_order_relaxed), std::memory_order_relaxed);
      workers_               = std::move(other.workers_);
      workgroups_            = std::move(other.workgroups_);
      threads_               = std::move(other.threads_);
      workgroup_descs_       = other.workgroup_descs_;
      entry_fn_              = std::move(other.entry_fn_);
      worker_count_          = other.worker_count_;
      workgroup_count_       = other.workgroup_count_;
      idle_spin_limit_       = other.idle_spin_limit_;
      other.worker_count_    = 0;
      other.workgroup_count_ = 0;
    }
    return *this;
  }

  /**
   * @brief Submits a coroutine-based task to be executed by the scheduler
   */
  template <CoroutineTask C>
  void submit(task_context const& src, workgroup_id group, C&& task_obj) noexcept
  {
    if constexpr (std::is_rvalue_reference_v<C&&>)
    {
      submit_internal(src, group, delegate_type::bind(ouly::detail::co_lambda_executor<C>(std::forward<C>(task_obj))));
    }
    else
    {
      submit_internal(src, group,
                      delegate_type::bind(
                       [address = task_obj.address()](task_context const&) -> void
                       {
                         std::coroutine_handle<>::from_address(address).resume();
                       }));
    }
  }

  // Coroutine task submission without explicit group
  template <CoroutineTask C>
  void submit(task_context const& current, C&& task_obj) noexcept
  {
    submit(current, current.get_workgroup(), std::forward<C>(task_obj));
  }

  /**
   * @brief Submits a callable work item to be executed by the scheduler.
   */
  template <typename Lambda>
    requires(std::invocable<Lambda, task_context const&> && !std::is_same_v<std::decay_t<Lambda>, delegate_type>)
  void submit(task_context const& src, workgroup_id group, Lambda&& data) noexcept
  {
    submit_internal(src, group, delegate_type::bind(std::forward<Lambda>(data)));
  }

  // Callable/lambda submission without explicit group
  template <typename Lambda>
    requires(std::invocable<Lambda, task_context const&> && !std::is_same_v<std::decay_t<Lambda>, delegate_type>)
  void submit(task_context const& current, Lambda&& data) noexcept
  {
    submit(current, current.get_workgroup(), std::forward<Lambda>(data));
  }

  /**
   * @brief Submits a function pointer with packaged arguments.
   */
  template <typename... PackArgs>
  void submit(task_context const& src, workgroup_id group, delegate_type::function_type ptr,
              PackArgs&&... args) noexcept
  {
    submit_internal(src, group, delegate_type::bind(ptr, std::forward<PackArgs>(args)...));
  }

  template <typename... PackArgs>
  void submit(task_context const& src, delegate_type::function_type ptr, PackArgs&&... args) noexcept
  {
    submit_internal(src, src.get_workgroup(), delegate_type::bind(ptr, std::forward<PackArgs>(args)...));
  }

  /**
   * @brief Begin scheduler execution, group creation is frozen after this call.
   * @param entry An entry function executed on all worker threads upon entry.
   * @param user_context User context pointer passed to worker contexts.
   */
  OULY_API void begin_execution(scheduler_worker_entry&& entry = {}, void* user_context = nullptr);

  /**
   * @brief Wait for all tasks to finish and stop all worker threads.
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
   * @brief Ensure a work-group by id
   */
  OULY_API void create_group(workgroup_id group, uint32_t start_thread_idx, uint32_t thread_count,
                             uint32_t priority = 0);

  /**
   * @brief Create the next available group
   */
  OULY_API auto create_group(uint32_t start_thread_idx, uint32_t thread_count, uint32_t priority = 0) -> workgroup_id;

  /**
   * @brief Drop all queued (not yet executing) work of a group.
   */
  OULY_API void clear_group(workgroup_id group);

  /**
   * @brief Get worker count in this group
   */
  [[nodiscard]] OULY_API auto get_worker_count(workgroup_id g) const noexcept -> uint32_t;

  /**
   * @brief Get worker start index
   */
  [[nodiscard]] OULY_API auto get_worker_start_idx(workgroup_id g) const noexcept -> uint32_t;

  /**
   * @brief Get logical divisor for workgroup
   */
  [[nodiscard]] OULY_API auto get_logical_divisor(workgroup_id g) const noexcept -> uint32_t;

  /**
   * @brief If multiple schedulers are active, call from the owning (main) thread before use.
   */
  OULY_API void take_ownership() noexcept;

  /**
   * @brief Try to execute a small amount of queued work on the calling worker.
   */
  OULY_API void busy_work(worker_id thread) noexcept;

  void busy_work(task_context const& ctx) noexcept
  {
    busy_work(ctx.get_worker());
  }

  /**
   * @brief Help execute work, then block (futex) until every submitted task has finished.
   */
  OULY_API void wait_for_tasks();

  /**
   * @brief Number of failed acquire attempts before an idle worker parks on the futex.
   *
   * Lower values release CPUs faster, higher values reduce wake latency under
   * intermittent load. Must be called before begin_execution().
   */
  void set_idle_spin_count(uint32_t spins) noexcept
  {
    idle_spin_limit_ = spins;
  }

private:
  friend class task_context;

  OULY_API void submit_internal(task_context const& current, workgroup_id dst, detail::v3::work_item const& work);

  void run_worker(worker_id wid);

  auto try_execute_one(worker_id wid) noexcept -> bool;
  void execute_work(detail::v3::worker& wkr, uint32_t group_index, detail::v3::work_item& work) noexcept;
  void notify_workers(uint32_t count) noexcept;
  void finish_task() noexcept;

  [[nodiscard]] auto has_queued_work(detail::v3::worker const& wkr) const noexcept -> bool;

  static constexpr uint32_t default_idle_spin_limit = OULY_V3_SCHEDULER_SPIN_COUNT;

  // Eventcount for parking idle workers (futex via C++20 atomic wait).
  ouly::detail::cache_aligned_atomic<uint32_t> wake_epoch_{uint32_t{0}};
  ouly::detail::cache_aligned_atomic<uint32_t> sleepers_{uint32_t{0}};

  // Tasks submitted but not yet finished executing (queued + in-flight).
  ouly::detail::cache_aligned_atomic<uint32_t> pending_{uint32_t{0}};

  std::unique_ptr<detail::v3::worker[]>    workers_;
  std::unique_ptr<detail::v3::workgroup[]> workgroups_;
  std::vector<std::thread>                 threads_;

  std::array<detail::v3::workgroup_desc, detail::v3::max_workgroup> workgroup_descs_{};

  scheduler_worker_entry entry_fn_;

  uint32_t worker_count_    = 0;
  uint32_t workgroup_count_ = 0;
  uint32_t idle_spin_limit_ = default_idle_spin_limit;

  std::atomic<uint32_t> pending_waiters_{0};
  std::atomic_bool      stop_{false};
};

} // namespace ouly::v3
