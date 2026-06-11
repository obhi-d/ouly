// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/detail/v3/workgroup.hpp"
#include "ouly/scheduler/v3/task_context.hpp"
#include <array>
#include <cstdint>

namespace ouly::detail::v3
{

/**
 * @brief v3 worker: fixed workgroup membership, priority-sorted.
 *
 * The context's (group, offset) pair is set for the duration of each executed task and
 * restored afterwards, so nested helping (cooperative waits) never corrupts the context.
 */
class worker
{
public:
  [[nodiscard]] auto get_context() const noexcept -> ouly::v3::task_context const&
  {
    return context_;
  }

  [[nodiscard]] auto get_worker_id() const noexcept -> worker_id
  {
    return context_.get_worker();
  }

private:
  friend class ouly::v3::scheduler;

  ouly::v3::task_context context_;

  // Indices of workgroups this worker belongs to, sorted by descending priority.
  std::array<uint8_t, max_workgroup> group_order_{};
  uint32_t                           group_count_ = 0;
};

} // namespace ouly::detail::v3
