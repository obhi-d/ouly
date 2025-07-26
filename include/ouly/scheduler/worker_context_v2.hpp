// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/delegate.hpp"
#include "ouly/utility/user_config.hpp"

namespace ouly::inline v2
{
class worker_context;
class scheduler;
constexpr uint32_t max_task_data_size = 20;
constexpr uint32_t max_task_base_size = 24;

using task_delegate = ouly::basic_delegate<max_task_base_size, void(worker_context const&)>;

/**
 * @brief A worker context is a unique identifier that represents where a task can run, it stores the current
 * worker_id, and the workgroup for the current task.
 */
class worker_context
{
public:
  worker_context() noexcept = default;
  worker_context(scheduler& s, void* user_context, uint32_t offset, worker_id id) noexcept
      : offset_(offset), index_(id), owner_(&s), user_context_(user_context)
  {}

  /**
   * @brief get the current worker's index relative to the group's thread start offset
   */
  [[nodiscard]] auto get_group_offset() const noexcept -> uint32_t
  {
    return offset_;
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
  friend class scheduler;
  friend class worker;

  workgroup_id group_id_;
  uint32_t     offset_       = 0;
  worker_id    index_        = worker_id{0};
  scheduler*   owner_        = nullptr;
  void*        user_context_ = nullptr;
};

} // namespace ouly::inline v2