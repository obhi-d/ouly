
#pragma once

#include <acl/utils/nullable_optional.hpp>
#include <cassert>
#include <compare>
#include <cstdint>
#include <limits>

namespace acl
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
  constexpr explicit worker_id(uint32_t id) noexcept : index(id) {}

  /**
   * @brief Retruns a positive integer != std::numeric_limits<uint32_t>::max() when valid, that represents the index of
   * the current worker thread
   */
  inline constexpr uint32_t get_index() const noexcept
  {
    return index;
  }

  inline constexpr explicit operator bool() const
  {
    return index != std::numeric_limits<uint32_t>::max();
  }

  /**
   * @brief Returns the worker id for the current thread
   */
  static worker_id const& get() noexcept;

  inline auto operator<=>(worker_id const&) const noexcept = default;

private:
  uint32_t index = 0;
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
  constexpr explicit workgroup_id(uint32_t id) noexcept : index(id) {}

  /**
   * @brief Retruns a positive integer != std::numeric_limits<uint32_t>::max() when valid, that represents the index of
   * the current worker thread
   */
  inline constexpr uint32_t get_index() const noexcept
  {
    return index;
  }

  inline constexpr explicit operator bool() const
  {
    return index != std::numeric_limits<uint32_t>::max();
  }

  inline auto operator<=>(workgroup_id const&) const noexcept = default;

private:
  uint32_t index = 0;
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
  worker_context(scheduler& s, worker_id id, workgroup_id group, uint32_t mask, uint32_t offset) noexcept
      : owner(&s), index(id), group_id(group), group_mask(mask), group_offset(offset)
  {}

  /**
   * @brief Retruns the current worker id
   */
  worker_id get_worker() const noexcept
  {
    return index;
  }

  /**
   * @brief get the current worker's index relative to the group's thread start offset
   */
  inline uint32_t get_group_offset() const noexcept
  {
    return group_offset;
  }

  scheduler& get_scheduler() const
  {
    assert(owner);
    return *owner;
  }

  workgroup_id get_workgroup() const noexcept
  {
    return group_id;
  }

  bool belongs_to(workgroup_id group) const noexcept
  {
    return group_mask & (1u << group.get_index());
  }

  /**
   * @brief returns the context on the current thread for a given worker group
   */
  static worker_context const& get(workgroup_id group) noexcept;

  inline auto operator<=>(worker_context const&) const noexcept = default;

private:
  scheduler*   owner = nullptr;
  worker_id    index;
  workgroup_id group_id;
  uint32_t     group_mask   = 0;
  uint32_t     group_offset = 0;
};

using worker_context_opt = acl::nullable_optional<worker_context>;

class worker_desc
{
public:
  worker_desc(worker_id id, uint32_t mask) noexcept : index(id), group_mask(mask) {}
  /**
   * @brief Retruns the current worker id
   */
  worker_id get_worker() const noexcept
  {
    return index;
  }

  bool belongs_to(workgroup_id group) const noexcept
  {
    return group_mask & (1u << group.get_index());
  }

  inline auto operator<=>(worker_desc const&) const noexcept = default;

private:
  uint32_t friend_worker_count = 0;
  uint32_t friend_worker_start = std::numeric_limits<uint32_t>::max();

  worker_id index;
  uint32_t  group_mask = 0;
};

} // namespace acl
