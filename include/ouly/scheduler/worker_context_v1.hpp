// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/user_config.hpp"
#include <functional>
#include <semaphore>

namespace ouly::v1
{
class scheduler;

/**
 * @brief A worker context is a unique identifier that represents where a task can run, it stores the current
 * worker_id, and the workgroup for the current task.
 */
class worker_context
{
public:
  worker_context() noexcept = default;
  worker_context(scheduler& s, void* user_context, worker_id id, workgroup_id group, uint32_t mask,
                 uint32_t offset) noexcept
      : group_id_(group), index_(id), owner_(&s), user_context_(user_context), group_mask_(mask), group_offset_(offset)
  {}

  /**
   * @brief get the current worker's index relative to the group's thread start offset
   */
  [[nodiscard]] auto get_group_offset() const noexcept -> uint32_t
  {
    return group_offset_;
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
  auto get_user_context() const noexcept -> T*
  {
    return static_cast<T*>(user_context_);
  }

  /**
   * @brief returns the context on the current thread for a given worker group
   */
  static auto get(workgroup_id group) noexcept -> worker_context const&;

  void busy_wait(std::binary_semaphore& event);

  auto operator<=>(worker_context const&) const noexcept = default;

private:
  [[nodiscard]] auto get_workgroup() const noexcept -> workgroup_id
  {
    return group_id_;
  }

  [[nodiscard]] auto belongs_to(workgroup_id group) const noexcept -> bool
  {
    return (group_mask_ & (1U << group.get_index())) != 0U;
  }

  friend class scheduler;

  workgroup_id group_id_;
  worker_id    index_;
  scheduler*   owner_        = nullptr;
  void*        user_context_ = nullptr;

  uint32_t group_mask_   = 0;
  uint32_t group_offset_ = 0;
};

using scheduler_worker_entry = std::function<void(worker_id const&)>;
} // namespace ouly::v1