// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/delegate.hpp"
#include "ouly/utility/user_config.hpp"

namespace ouly::detail::v2
{
class worker;
}

namespace ouly::inline v2
{
class task_context;
class scheduler;

constexpr uint32_t max_task_base_size = 64;

using task_delegate = ouly::basic_delegate<max_task_base_size, void(task_context const&)>;

/**
 * @brief A worker context is a unique identifier that represents where a task can run, it stores the current
 * worker_id, and the workgroup for the current task.
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
   * @note This value might change between cooperative waits, like exectution of parallel_for or busy_wait, so the value
   * should always be fetched from the context object.
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

  struct OULY_API this_context
  {
    static auto get_worker_id() noexcept -> worker_id;
    static auto get() noexcept -> task_context const&;
  };

  void busy_wait(std::binary_semaphore& event) const;

  auto operator<=>(task_context const&) const noexcept = default;

private:
  friend class scheduler;
  friend class detail::v2::worker;

  workgroup_id group_id_;
  uint32_t     offset_       = 0;
  worker_id    index_        = worker_id{0};
  scheduler*   owner_        = nullptr;
  void*        user_context_ = nullptr;
};

static_assert(TaskContext<task_context>, "task_context must satisfy TaskContext concept");

} // namespace ouly::inline v2
