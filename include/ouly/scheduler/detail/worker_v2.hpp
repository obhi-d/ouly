

#pragma once

#include "ouly/allocators/default_allocator.hpp"
#include "ouly/containers/basic_queue.hpp"
#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/detail/workgroup_v2.hpp"
#include "ouly/scheduler/spin_lock.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/worker_context_v2.hpp"
#include <array>
#include <cstdint>

namespace ouly::detail::inline v2
{

class workgroup;
class worker
{
public:
  auto get_context() noexcept -> ouly::v2::worker_context const&
  {
    return current_context_;
  }

  void set_workgroup_info(uint32_t offset, workgroup_id group) noexcept
  {
    current_context_.group_id_ = group;
    current_context_.offset_   = offset;
  }

private:
  workgroup*               assigned_group_ = nullptr; // The workgroup this worker belongs to
  ouly::v2::worker_context current_context_;
};

} // namespace ouly::detail::inline v2
