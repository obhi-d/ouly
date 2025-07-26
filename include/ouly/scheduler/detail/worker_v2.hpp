

#pragma once

#include "ouly/scheduler/task_context_v2.hpp"
#include <cstdint>

namespace ouly::detail::v2
{

class workgroup; // Forward declaration

class worker
{
public:
  [[nodiscard]] auto get_context() const noexcept -> task_context const&
  {
    return current_context_;
  }

  void set_workgroup_info(uint32_t offset, workgroup_id group) noexcept
  {
    current_context_.group_id_ = group;
    current_context_.offset_   = offset;
  }

private:
  friend class ouly::v2::scheduler;

  int64_t      tally_           = 0;
  workgroup*   assigned_group_  = nullptr; // The workgroup this worker belongs to
  uint32_t     assigned_offset_ = 0;       // Offset within the workgroup
  task_context current_context_;
};

} // namespace ouly::detail::v2
