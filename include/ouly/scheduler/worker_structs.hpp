// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/utility/user_config.hpp"
#include <compare>
#include <cstdint>
#include <functional>
#include <limits>
#include <semaphore>

namespace ouly
{
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
  uint32_t index_ = std::numeric_limits<uint32_t>::max();
};

static constexpr workgroup_id default_workgroup_id = workgroup_id(0);

using scheduler_worker_entry = std::function<void(worker_id const&)>;

template <typename T>
concept TaskContext = requires(const T& ctx) {
  { ctx.get_worker() } -> std::convertible_to<worker_id>;
  { ctx.get_group_offset() } -> std::convertible_to<uint32_t>;
  { ctx.template get_user_context<void>() } -> std::convertible_to<void*>;
  { T::this_context::get() } -> std::convertible_to<const T&>;
  { ctx.busy_wait(std::declval<std::binary_semaphore&>()) } -> std::same_as<void>;
};
} // namespace ouly
