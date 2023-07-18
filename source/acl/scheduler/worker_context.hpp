
#pragma once

#include <acl/utils/nullable_optional.hpp>
#include <cstdint>

namespace acl
{
class scheduler;
/// @brief A worker represents a specific thread. A worker can belong to any of maximum of 32 worker_groups allowed by
/// the scheduler.
class worker_id
{
public:
  constexpr worker_id() noexcept = default;
  constexpr explicit worker_id(uint32_t id) noexcept : index(id) {}

  /// @brief Retruns a positive integer != std::numeric_limits<uint32_t>::max() when valid, that represents the index of
  /// the current worker thread
  inline constexpr uint32_t get_index() const noexcept
  {
    return index;
  }

  inline constexpr explicit operator bool() const
  {
    return index != std::numeric_limits<uint32_t>::max();
  }

private:
  uint32_t index = 0;
};

static constexpr worker_id main_worker_id = worker_id(0);

/// @brief A work group is a collection of workers where you can push tasks to be executed. A task have to be assigned
/// to a work group for execution. Normally workers may be shared between different work groups, depending upon how the
/// scheduler was setup. @class work_group_id is a unique identifier for a given work group.
class work_group_id
{
public:
  constexpr work_group_id() noexcept = default;
  constexpr explicit work_group_id(uint32_t id) noexcept : index(id) {}

  /// @brief Retruns a positive integer != std::numeric_limits<uint32_t>::max() when valid, that represents the index of
  /// the current worker thread
  inline constexpr uint32_t get_index() const noexcept
  {
    return index;
  }

  inline constexpr explicit operator bool() const
  {
    return index != std::numeric_limits<uint32_t>::max();
  }

private:
  uint32_t index = 0;
};

static constexpr work_group_id default_work_group_id = work_group_id(0);

/// @brief A worker context is a unique identifier that represents where a task can run, it stores the current
/// worker_id, and the work_group for the current task.
class worker_context
{
public:
  worker_context(scheduler& s, worker_id id, work_group_id group, uint32_t mask, uint32_t offset) noexcept
      : owner(s), index(id), group_id(group), group_mask(mask), group_offset(offset)
  {}

  /// @brief Retruns the current worker id
  worker_id get_worker() const noexcept
  {
    return index;
  }

  /// @brief get the current worker's index relative to the group's thread start offset
  inline uint32_t get_group_offset() const noexcept
  {
    return group_offset;
  }

  scheduler& get_scheduler() const
  {
    return owner;
  }

  work_group_id get_work_group() const noexcept
  {
    return group_id;
  }

  bool belongs_to(work_group_id group) const noexcept
  {
    return group_mask & (1u << group.get_index());
  }

  /// @brief returns the context on the current thread for a given worker group
  static worker_context const& get_context(work_group_id group);

private:
  scheduler&    owner;
  worker_id     index;
  work_group_id group_id;
  uint32_t      group_mask   = 0;
  uint32_t      group_offset = 0;
};

using worker_context_opt = acl::nullable_optional<worker_context>;

} // namespace acl