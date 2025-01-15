

#pragma once

#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/basic_queue.hpp>
#include <acl/scheduler/spin_lock.hpp>
#include <acl/scheduler/task.hpp>
#include <acl/scheduler/worker_context.hpp>
#include <acl/utility/tagged_ptr.hpp>
#include <cstdint>
#include <limits>
#include <mutex>
#include <semaphore>
#include <tuple>

namespace acl::detail
{

static constexpr uint32_t max_worker_groups   = 32;
static constexpr uint32_t max_local_work_item = 32; // 2 cache lines

using work_item = task_delegate;

struct work_queue_traits
{
  static constexpr uint32_t pool_size_v = 2048;
  using allocator_t                     = acl::default_allocator<>;
};

using work_queue       = acl::basic_queue<work_item, work_queue_traits>;
using async_work_queue = std::pair<acl::spin_lock, work_queue>;

struct workgroup
{
  // Global queues, one per thread group
  std::unique_ptr<acl::detail::async_work_queue[]> work_queues_;
  uint32_t                                         thread_count_     = 0;
  uint32_t                                         start_thread_idx_ = 0;
  uint32_t                                         push_offset_      = 0;
  uint32_t                                         priority_         = 0;

  auto create_group(uint32_t start, uint32_t count, uint32_t priority) noexcept -> uint32_t
  {
    work_queues_      = std::make_unique<acl::detail::async_work_queue[]>(count);
    thread_count_     = count;
    start_thread_idx_ = start;
    this->priority_   = priority;
    return start + count;
  }

  workgroup() noexcept = default;
};

struct wake_event
{
  wake_event() noexcept : semaphore_(0) {}

  void wait() noexcept
  {
    semaphore_.acquire();
  }

  void notify() noexcept
  {
    semaphore_.release();
  }

  std::binary_semaphore semaphore_;
};

struct local_queue
{
  std::atomic_uint32_t                       head_;
  std::atomic_uint32_t                       tail_;
  std::array<work_item, max_local_work_item> queue_;
};

struct group_range
{
  std::array<uint8_t, max_worker_groups> priority_order_{};
  uint32_t                               count_ = 0;
  uint32_t                               mask_  = 0;

  group_range() noexcept
  {
    priority_order_.fill(std::numeric_limits<uint8_t>::max());
  }
};

struct worker
{
  // Context per work group
  std::unique_ptr<worker_context[]> contexts_;
  // Worker specific item
  async_work_queue exlusive_items_;
  // worker id
  worker_id id_;
  // quit event
  std::atomic_bool quitting_ = false;
};

} // namespace acl::detail
