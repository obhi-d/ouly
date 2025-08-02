

#pragma once

#include "ouly/scheduler/v2/task_context.hpp"
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

  [[nodiscard]] auto get_group_offset() const noexcept -> uint32_t
  {
    return current_context_.get_group_offset();
  }

  [[nodiscard]] auto get_workgroup() const noexcept -> workgroup_id
  {
    return current_context_.get_workgroup();
  }

  [[nodiscard]] auto get_worker_id() const noexcept -> worker_id
  {
    return current_context_.get_worker();
  }

private:
  friend class ouly::v2::scheduler;

  task_context current_context_;
};

} // namespace ouly::detail::v2
