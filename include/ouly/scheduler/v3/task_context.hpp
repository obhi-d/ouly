// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/delegate.hpp"
#include "ouly/utility/user_config.hpp"

namespace ouly::detail::v3
{
class worker;
}

namespace ouly::v3
{
class task_context;
class scheduler;

constexpr uint32_t max_task_base_size = 64;

using task_delegate = ouly::basic_delegate<max_task_base_size, void(task_context const&)>;

/**
 * @brief A task context identifies where a task is running: the current worker_id and the
 * workgroup of the currently executing task.
 *
 * In the v3 scheduler a worker's membership in workgroups is fixed at begin_execution() time,
 * so the (workgroup, offset) pair stored here only changes while a task from another workgroup
 * the worker belongs to is being executed, and is restored afterwards.
 */
class task_context
{
public:
  task_context() noexcept = default;
  task_context(scheduler& s, void* user_context, uint32_t offset, worker_id id) noexcept
      : offset_(offset), index_(id), owner_(&s), user_context_(user_context)
  {}

  void init(scheduler& s, void* user_context, uint32_t offset, worker_id id) noexcept
  {
    offset_       = offset;
    index_        = id;
    owner_        = &s;
    user_context_ = user_context;
  }

  /**
   * @brief get the current worker's index relative to the group's thread start offset
   * @note This value can change between cooperative waits (parallel_for, cooperative_wait),
   * always fetch it from the context object.
   */
  [[nodiscard]] auto get_group_offset() const noexcept -> uint32_t
  {
    return offset_;
  }

  /**
   * @brief Returns the current workgroup id
   */
  [[nodiscard]] auto get_workgroup() const noexcept -> workgroup_id
  {
    return group_id_;
  }

  [[nodiscard]] auto get_scheduler() const -> scheduler&
  {
    OULY_ASSERT(owner_);
    return *owner_;
  }

  /**
   * @brief Returns the current worker id
   */
  [[nodiscard]] auto get_worker() const noexcept -> worker_id
  {
    return index_;
  }

  template <typename T>
  [[nodiscard]] auto get_user_context() const noexcept -> T*
  {
    return static_cast<T*>(user_context_);
  }

  struct this_context
  {
    OULY_API static auto get_worker_id() noexcept -> worker_id;
    OULY_API static auto get() noexcept -> task_context const&;
  };

  /**
   * @brief Wait for an event while helping the scheduler execute queued work.
   *
   * Unlike the v1/v2 implementations this does not spin indefinitely: when no work is
   * available the calling thread blocks on the event with short timeouts, so idle CPUs
   * are released back to the OS.
   */
  OULY_API void cooperative_wait(std::binary_semaphore& event) const;

  auto operator<=>(task_context const&) const noexcept = default;

private:
  friend class scheduler;
  friend class detail::v3::worker;

  workgroup_id group_id_;
  uint32_t     offset_       = 0;
  worker_id    index_        = worker_id{0};
  scheduler*   owner_        = nullptr;
  void*        user_context_ = nullptr;
};

static_assert(TaskContext<task_context>, "task_context must satisfy TaskContext concept");

} // namespace ouly::v3
