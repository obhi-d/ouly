// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/delegate.hpp"
#include "ouly/utility/user_config.hpp"
#include <functional>
#include <semaphore>

namespace ouly::v1
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
  task_context(scheduler& s, void* user_context, worker_id id, workgroup_id group, uint32_t mask,
               uint32_t offset) noexcept
      : group_id_(group), index_(id), owner_(&s), user_context_(user_context), group_mask_(mask), group_offset_(offset)
  {}

  void init(scheduler& s, void* user_context, worker_id id, workgroup_id group, uint32_t mask, uint32_t offset) noexcept
  {
    group_id_     = group;
    index_        = id;
    owner_        = &s;
    user_context_ = user_context;
    group_mask_   = mask;
    group_offset_ = offset;
  }

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
  [[nodiscard]] auto get_user_context() const noexcept -> T*
  {
    return static_cast<T*>(user_context_);
  }

  struct OULY_API this_context
  {
    static auto get_worker_id() noexcept -> worker_id;
    static auto get() noexcept -> task_context const&;
  };

  [[nodiscard]] auto get_workgroup() const noexcept -> workgroup_id
  {
    return group_id_;
  }

  void busy_wait(std::binary_semaphore& event) const;

  auto operator<=>(task_context const&) const noexcept = default;

private:
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

static_assert(TaskContext<task_context>, "task_context must satisfy TaskContext concept");
} // namespace ouly::v1