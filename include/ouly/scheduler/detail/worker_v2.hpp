

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
  auto get_context() noexcept -> worker_context&
  {
    return current_context_;
  }

private:
  workgroup*     assigned_group_ = nullptr; // The workgroup this worker belongs to
  worker_context current_context_;
};

} // namespace ouly::detail::inline v2
