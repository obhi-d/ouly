// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/default_allocator.hpp"
#include "ouly/containers/basic_queue.hpp"
#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/spin_lock.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/worker_context.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <semaphore>
#include <type_traits>
#include <utility>

namespace ouly::detail
{

static constexpr uint32_t max_worker_groups   = 32;
static constexpr uint32_t max_local_work_item = 32; // 1/2 cache lines

using work_item = task_delegate;

struct work_queue_traits
{
  static constexpr uint32_t pool_size_v = 2048;
  using allocator_t                     = ouly::default_allocator<>;
};

using work_queue       = ouly::basic_queue<work_item, work_queue_traits>;
using async_work_queue = std::pair<ouly::spin_lock, work_queue>;

// Optimized workgroup structure with separated hot and cold data
struct workgroup
{
  // Atomic push offset gets its own cache line to prevent false sharing
  std::atomic<uint32_t> push_offset_{0U};

  // Hot data cache line 1: Most frequently accessed during work stealing and submission
  uint32_t thread_count_     = 0; // Read frequently during work stealing
  uint32_t start_thread_idx_ = 0; // Read frequently during queue indexing

  // Work queues pointer - frequently dereferenced but the pointer itself is stable
  ouly::detail::async_work_queue* work_queues_ = nullptr;

  // Cold data: Configuration set once during initialization
  uint32_t priority_ = 0;

  auto create_group(uint32_t start, uint32_t count, uint32_t priority) noexcept -> uint32_t
  {
    // Use aligned allocation for work queues to ensure proper cache alignment
    work_queues_ = static_cast<ouly::detail::async_work_queue*>(::operator new[](
     count * sizeof(ouly::detail::async_work_queue), std::align_val_t{detail::cache_line_size}, std::nothrow));

    if (work_queues_ != nullptr)
    {
      // Initialize the work queues using placement new
      for (uint32_t i = 0; i < count; ++i)
      {
        new (&work_queues_[i]) ouly::detail::async_work_queue{};
      }
    }

    thread_count_     = count;
    start_thread_idx_ = start;
    this->priority_   = priority;
    push_offset_.store(0, std::memory_order_relaxed);
    return start + count;
  }

  ~workgroup() noexcept
  {
    if (work_queues_ != nullptr)
    {
      // Explicitly destroy each work queue
      for (uint32_t i = 0; i < thread_count_; ++i)
      {
        work_queues_[i].~async_work_queue();
      }
      ::operator delete[](work_queues_, std::align_val_t{detail::cache_line_size});
      work_queues_ = nullptr; // Prevent dangling pointer
    }
  }

  // Move semantics for workgroup management
  workgroup(workgroup&& other) noexcept
      : push_offset_(other.push_offset_.load()), thread_count_(other.thread_count_),
        start_thread_idx_(other.start_thread_idx_),
        work_queues_(other.work_queues_), priority_(other.priority_)
  {
    other.work_queues_  = nullptr;
    other.thread_count_ = 0;
  }

  auto operator=(workgroup&& other) noexcept -> workgroup&
  {
    if (this != &other)
    {
      if (work_queues_ != nullptr)
      {
        for (uint32_t i = 0; i < thread_count_; ++i)
        {
          work_queues_[i].~async_work_queue();
        }
        ::operator delete[](work_queues_, std::align_val_t{detail::cache_line_size});
      }

      thread_count_     = other.thread_count_;
      start_thread_idx_ = other.start_thread_idx_;
      push_offset_.store(other.push_offset_.load());
      work_queues_ = other.work_queues_;
      priority_    = other.priority_;

      other.work_queues_  = nullptr;
      other.thread_count_ = 0;
    }
    return *this;
  }

  workgroup() noexcept                           = default;
  workgroup(const workgroup&)                    = delete;
  auto operator=(const workgroup&) -> workgroup& = delete;
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


// Cache-aligned worker structure optimized for memory access patterns
struct worker
{
  // Exclusive work items queue - frequently accessed during task submission/execution
  // Aligned to prevent false sharing with worker ID
  async_work_queue exclusive_items_;

  // Context per work group - accessed during work execution
  // Pointer is stable, actual contexts allocated separately for better locality
  std::unique_ptr<worker_context[]> contexts_;

  worker_id id_ = worker_id{0};
};

} // namespace ouly::detail
