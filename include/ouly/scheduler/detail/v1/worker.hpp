// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/default_allocator.hpp"
#include "ouly/containers/basic_queue.hpp"
#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/detail/v1/workgroup.hpp"
#include "ouly/scheduler/spin_lock.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/v1/task_context.hpp"
#include <array>
#include <cstdint>

namespace ouly::detail::v1
{
static constexpr uint32_t max_worker_groups = 32;

constexpr auto init_priority_order() noexcept -> std::array<uint8_t, max_worker_groups>
{
  std::array<uint8_t, max_worker_groups> order{};
  order.fill(std::numeric_limits<uint8_t>::max());
  return order;
}

struct group_range
{
  std::array<uint8_t, max_worker_groups> priority_order_ = init_priority_order();
  uint32_t                               count_          = 0;
  uint32_t                               mask_           = 0;

  // Store the range of threads this worker can steal from
  // This represents threads that belong to at least one shared workgroup
  uint32_t steal_range_start_ = 0;
  uint32_t steal_range_end_   = 0;

  // Bitset of threads this worker can steal from (for precise control)
  // Bit i is set if this worker can steal from worker i
  uint64_t steal_mask_ = 0;
};

// Cache-aligned worker structure optimized for memory access patterns
struct worker
{
  // Context per work group - accessed during work execution
  // Pointer is stable, actual contexts allocated separately for better locality
  std::unique_ptr<ouly::v1::task_context[]> contexts_;

  worker_id id_                  = worker_id{0};
  worker_id min_steal_friend_id_ = worker_id{0};
  worker_id max_steal_friend_id_ = worker_id{0};

  ouly::v1::task_context* current_context_ = nullptr;

  // No local queues needed - work is organized per workgroup per worker
  // This eliminates the complexity of multiple queue types and work validation
};

} // namespace ouly::detail::v1
