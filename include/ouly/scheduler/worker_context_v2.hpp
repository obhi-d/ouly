// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/utility/user_config.hpp"

namespace ouly::v2
{
class scheduler;
/**
 * @brief A worker represents a specific thread. A worker can belong to any of maximum of 32 worker_groups allowed by
 * the scheduler.
 */
class worker_id
{
public:
  constexpr worker_id() noexcept = default;
  constexpr explicit worker_id(uint32_t id) noexcept : index_(id) {}

  /**
   * @brief Retruns a positive integer != std::numeric_limits<uint32_t>::max() when valid, that represents the index of
   * the current worker thread
   */
  [[nodiscard]] constexpr auto get_index() const noexcept -> uint32_t
  {
    return index_;
  }

  constexpr explicit operator bool() const
  {
    return index_ != std::numeric_limits<uint32_t>::max();
  }

  struct this_worker
  {
    /**
     * @brief Returns the worker id for the current thread
     */
    /**
     * @brief Returns the worker id for the current thread
     */
    static auto get_id() noexcept -> worker_id const&;
  };

  auto operator<=>(worker_id const&) const noexcept = default;

private:
  uint32_t index_ = 0;
};

static constexpr worker_id main_worker_id = worker_id(0);

/**
 * @brief A workgroup is a collection of workers where you can push tasks to be executed. A task have to be assigned
 * to a workgroup for execution. Normally workers may be shared between different workgroups, depending upon how the
 * scheduler was setup. @class workgroup_id is a unique identifier for a given workgroup.
 */
class workgroup_id
{
public:
  constexpr workgroup_id() noexcept = default;
  constexpr explicit workgroup_id(uint32_t id) noexcept : index_(id) {}

  /**
   * @brief Retruns a positive integer != std::numeric_limits<uint32_t>::max() when valid, that represents the index of
   * the current worker thread
   */
  [[nodiscard]] constexpr auto get_index() const noexcept -> uint32_t
  {
    return index_;
  }

  constexpr explicit operator bool() const
  {
    return index_ != std::numeric_limits<uint32_t>::max();
  }

  auto operator<=>(workgroup_id const&) const noexcept = default;

private:
  uint32_t index_ = 0;
};

static constexpr workgroup_id default_workgroup_id = workgroup_id(0);

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
   * @brief Retruns the current worker id
   */
  [[nodiscard]] auto get_worker() const noexcept -> worker_id
  {
    return index_;
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

  [[nodiscard]] auto get_workgroup() const noexcept -> workgroup_id
  {
    return group_id_;
  }

  [[nodiscard]] auto belongs_to(workgroup_id group) const noexcept -> bool
  {
    return (group_mask_ & (1U << group.get_index())) != 0U;
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

  auto operator<=>(worker_context const&) const noexcept = default;

private:
  workgroup_id group_id_;
  worker_id    index_;
  scheduler*   owner_        = nullptr;
  void*        user_context_ = nullptr;
  uint32_t     group_mask_   = 0;
  uint32_t     group_offset_ = 0;
};

using worker_context_opt = ouly::nullable_optional<worker_context>;

/**
 * @brief A worker context descriptor for the scheduler system
 *
 * This class describes a worker thread within the scheduling system,
 * including its relationships to other workers and group associations.
 */
class worker_desc
{
public:
  worker_desc(worker_id id, uint32_t mask) noexcept : index_(id), group_mask_(mask) {}

  /**
   * @brief Get the number of friend workers that can cooperate with this worker
   *
   * @return The count of friend workers
   */
  [[nodiscard]] auto get_friend_worker_count() const noexcept -> uint32_t
  {
    return friend_worker_count_;
  }

  /**
   * @brief Get the starting index of friend workers
   *
   * @return The index of the first friend worker
   */
  [[nodiscard]] auto get_friend_worker_start() const noexcept -> uint32_t
  {
    return friend_worker_start_;
  }

  /**
   * @brief Get the worker's ID
   *
   * @return The worker's ID
   */
  [[nodiscard]] auto id() const noexcept -> worker_id
  {
    return index_;
  }

  /**
   * @brief Retruns the current worker id
   */
  [[nodiscard]] auto get_worker() const noexcept -> worker_id
  {
    return index_;
  }

  /**
   * @brief Check if the worker is a member of a specific group
   *
   * @param group The group ID to check
   * @return True if the worker is a member of the specified group
   */
  [[nodiscard]] auto belongs_to(workgroup_id group) const noexcept -> bool
  {
    return (group_mask_ & (1U << group.get_index())) != 0U;
  }

  /**
   * @brief Get the worker's group mask
   *
   * @return The group mask of the worker
   */
  [[nodiscard]] auto get_group_mask() const noexcept -> uint32_t
  {
    return group_mask_;
  }

  auto operator<=>(worker_desc const&) const noexcept = default;

private:
  uint32_t friend_worker_count_ = 0;
  uint32_t friend_worker_start_ = std::numeric_limits<uint32_t>::max();

  worker_id index_;
  uint32_t  group_mask_ = 0;
};

using scheduler_worker_entry = std::function<void(worker_desc const&)>;
} // namespace ouly::v2