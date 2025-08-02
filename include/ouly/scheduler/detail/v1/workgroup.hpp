
#pragma once
#include "ouly/allocators/default_allocator.hpp"
#include "ouly/containers/basic_queue.hpp"
#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/spin_lock.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/v1/task_context.hpp"
#include <cstdint>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly::detail::v1
{
static constexpr uint32_t max_local_work_item       = 32; // 1/2 cache lines
static constexpr uint32_t max_work_items_per_worker = 64; // Maximum items per worker to prevent excessive memory usage
static constexpr uint32_t max_steal_workers         = 64; // Maximum workers that can be tracked in steal mask
using work_item                                     = ouly::v1::task_delegate;

struct work_queue_traits
{
  static constexpr uint32_t pool_size_v = 2048;
  using allocator_t                     = ouly::default_allocator<>;
};

using basic_work_queue = ouly::basic_queue<work_item, work_queue_traits>;
using async_work_queue = std::pair<ouly::spin_lock, basic_work_queue>;
using mpmc_work_ring   = ouly::detail::mpmc_ring<work_item, max_work_items_per_worker>;

// Optimized workgroup structure with per-worker queues
struct workgroup
{

  alignas(ouly::detail::cache_line_size) std::atomic<int64_t> tally_ = {0};

  // Per-worker queues within this workgroup - one queue per worker thread
  std::unique_ptr<mpmc_work_ring[]> per_worker_queues_;

  // Hot data cache line: Most frequently accessed during work stealing and submission
  uint32_t thread_count_     = 0; // Read frequently during work stealing
  uint32_t start_thread_idx_ = 0; // Read frequently during queue indexing
  uint32_t end_thread_idx_   = 0; // Read frequently during queue indexing

  // Cold data: Configuration set once during initialization
  uint32_t priority_ = 0;

  auto create_group(uint32_t start, uint32_t count, uint32_t priority) noexcept -> uint32_t
  {
    thread_count_     = count;
    start_thread_idx_ = start;
    end_thread_idx_   = start + count;
    this->priority_   = priority;

    // Allocate per-worker queues for this workgroup
    per_worker_queues_ = std::make_unique<mpmc_work_ring[]>(count);

    return start + count;
  }

  ~workgroup() noexcept = default;

  // Move semantics for workgroup management
  workgroup(workgroup&& other) noexcept
      : per_worker_queues_(std::move(other.per_worker_queues_)), thread_count_(other.thread_count_),
        start_thread_idx_(other.start_thread_idx_), end_thread_idx_(other.end_thread_idx_), priority_(other.priority_)
  {
    other.thread_count_     = 0;
    other.start_thread_idx_ = 0;
    other.end_thread_idx_   = 0;
    other.priority_         = 0;
  }

  auto operator=(workgroup&& other) noexcept -> workgroup&
  {
    if (this != &other)
    {
      // Move all data members
      per_worker_queues_ = std::move(other.per_worker_queues_);
      thread_count_      = other.thread_count_;
      start_thread_idx_  = other.start_thread_idx_;
      end_thread_idx_    = other.end_thread_idx_;
      priority_          = other.priority_;

      // Reset the source object
      other.thread_count_     = 0;
      other.start_thread_idx_ = 0;
      other.end_thread_idx_   = 0;
      other.priority_         = 0;
    }
    return *this;
  }

  workgroup() noexcept                           = default;
  workgroup(const workgroup&)                    = delete;
  auto operator=(const workgroup&) -> workgroup& = delete;

  // Push item to a specific worker's queue within this workgroup
  [[nodiscard]] auto push_item_to_worker(uint32_t worker_offset, work_item const& item) -> bool
  {
    if (worker_offset >= thread_count_)
    {
      return false;
    }

    if (per_worker_queues_[worker_offset].emplace(item))
    {
      tally_.fetch_add(1, std::memory_order_relaxed);
      return true;
    }

    return false;
  }

  // Pop item from a specific worker's queue within this workgroup
  [[nodiscard]] auto pop_item_from_worker(uint32_t worker_offset, work_item& item) const -> bool
  {
    if (worker_offset >= thread_count_)
    {
      return false;
    }

    return per_worker_queues_[worker_offset].pop(item);
  }

  void sink_one_work() noexcept
  {
    tally_.fetch_sub(1, std::memory_order_relaxed);
  }

  [[nodiscard]] auto has_work() const noexcept -> bool
  {
    return tally_.load(std::memory_order_relaxed) > 0;
  }

  [[nodiscard]] auto has_work_strong() const noexcept -> bool
  {
    return tally_.load(std::memory_order_acquire) > 0;
  }
};

} // namespace ouly::detail::v1

#ifdef _MSC_VER
#pragma warning(pop)
#endif
