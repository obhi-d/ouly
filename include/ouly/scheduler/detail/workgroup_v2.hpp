// SPDX-License-Identifier: MIT
#pragma once
#include "cache_optimized_data.hpp"
#include "ouly/allocators/default_allocator.hpp"
#include "ouly/containers/basic_queue.hpp"
#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/detail/spmc_ring.hpp"
#include "ouly/scheduler/spin_lock.hpp"
#include "ouly/scheduler/task_context_v2.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <span>
#include <utility>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

// Forward declarations
namespace ouly::inline v2
{
class scheduler;
}

namespace ouly::detail::v2
{
static constexpr uint32_t max_workgroup = 32; // Maximum number of workgroups supported
static constexpr uint32_t mpmc_capacity = 256;
// Simplified work item for the new architecture
using work_item = task_delegate;

/**
 * @brief New workgroup architecture with Chase-Lev work-stealing queues
 *
 * Each workgroup contains:
 * - An array of Chase-Lev queues (one per worker in the group)
 * - A mailbox for receiving work from other workgroups
 * - Work availability notification mechanism
 * - Priority and configuration settings
 */
class workgroup
{
public:
  using queue_type      = ::ouly::detail::spmc_ring<work_item>;
  workgroup() noexcept  = default;
  ~workgroup() noexcept = default;

  workgroup(const workgroup&)                    = delete;
  auto operator=(const workgroup&) -> workgroup& = delete;
  workgroup(workgroup&& other) noexcept
      : assigned_workers_(other.assigned_workers_.load(std::memory_order_relaxed)),
        has_work_(other.has_work_.load(std::memory_order_relaxed)), thread_count_(other.thread_count_),
        worker_start_idx_(other.worker_start_idx_), worker_end_idx_(other.worker_end_idx_), priority_(other.priority_),
        owner_(other.owner_), work_queues_(std::move(other.work_queues_)), mailbox_(std::move(other.mailbox_))
  {
    other.thread_count_ = 0;
    other.priority_     = 0;
    other.owner_        = nullptr;
  }

  auto operator=(workgroup&& other) noexcept -> workgroup&
  {
    if (this != &other)
    {
      assigned_workers_.store(other.assigned_workers_.load(std::memory_order_relaxed), std::memory_order_relaxed);
      has_work_.store(other.has_work_.load(std::memory_order_relaxed), std::memory_order_relaxed);
      thread_count_     = other.thread_count_;
      worker_start_idx_ = other.worker_start_idx_;
      worker_end_idx_   = other.worker_end_idx_;
      priority_         = other.priority_;
      owner_            = other.owner_;
      work_queues_      = std::move(other.work_queues_);
      mailbox_          = std::move(other.mailbox_);
    }
    return *this;
  }

  /**
   * @brief Initialize the workgroup with worker threads
   */
  void create_group(uint32_t start, uint32_t thread_count, uint32_t priority) noexcept
  {
    worker_start_idx_ = start;
    worker_end_idx_   = start + thread_count;
    thread_count_     = static_cast<int32_t>(thread_count);
    priority_         = priority;

    // Allocate Chase-Lev queues for each worker in this workgroup
    work_queues_ = std::make_unique<queue_type[]>(thread_count);

    // Reset work availability
    has_work_.store(false, std::memory_order_relaxed);
  }

  /**
   * @brief Submit work to a specific worker's queue within this workgroup
   */
  auto push_work_to_worker(uint32_t worker_offset, work_item const& item) noexcept -> bool
  {
    OULY_ASSERT(std::cmp_less(worker_offset, thread_count_));

    if (work_queues_[worker_offset].push_back(item))
    {
      advertise_work_available();
      return true;
    }
    return false;
  }

  /**
   * @brief Try to pop work from a specific worker's queue
   */
  auto pop_work_from_worker(work_item& out, uint32_t worker_offset) noexcept -> bool
  {
    OULY_ASSERT(std::cmp_less(worker_offset, thread_count_));
    return work_queues_[worker_offset].pop_front(out);
  }

  /**
   * @brief Try to steal work from any worker's queue in this workgroup
   */
  [[nodiscard]] auto steal_work(work_item& out, uint32_t avoid_worker_offset = UINT32_MAX) noexcept -> bool
  {
    OULY_ASSERT(thread_count_ > 0);

    // Try to steal from a random worker's queue to reduce contention
    thread_local uint32_t steal_index = 0;
    auto                  attempts    = static_cast<uint32_t>(thread_count_);

    for (uint32_t i = 0; i < attempts; ++i)
    {
      uint32_t worker_idx = (steal_index + i) % attempts;
      if (worker_idx == avoid_worker_offset)
      {
        continue; // Skip the worker we want to avoid
      }

      if (work_queues_[worker_idx].steal(out))
      {
        steal_index = (worker_idx + 1) % attempts; // Update for next time
        return true;
      }
    }

    return false;
  }

  /**
   * @brief Submit work via mailbox (cross-workgroup submission)
   */
  void submit_to_mailbox(work_item const& item) noexcept
  {
    std::lock_guard lock(mailbox_mutex_);
    mailbox_.push_back(item);
    advertise_work_available();
  }

  /**
   * @brief Try to receive work from mailbox
   */
  auto receive_from_mailbox(work_item& out) noexcept -> bool
  {
    std::lock_guard lock(mailbox_mutex_);
    return mailbox_.pop_front(out);
  }

  /**
   * @brief Check if this workgroup has work available
   */
  [[nodiscard]] auto has_work() const noexcept -> bool
  {
    return has_work_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Advertise that work is available to the scheduler
   */
  void advertise_work_available() noexcept
  {
    has_work_.store(true, std::memory_order_release);
  }

  /**
   * @brief Clear work available flag
   */
  void clear_work_available() noexcept
  {
    has_work_.store(false, std::memory_order_relaxed);
  }

  /**
   * @brief Initialize the workgroup with worker threads and scheduler reference
   */
  void initialize(uint32_t start, uint32_t thread_count, uint32_t priority, void* owner) noexcept
  {
    owner_ = static_cast<ouly::v2::scheduler*>(owner);
    create_group(start, thread_count, priority);
  }

  /**
   * @brief Clear the workgroup
   */
  void clear() noexcept
  {
    thread_count_ = 0;
    priority_     = 0;
    work_queues_.reset();
    assigned_workers_.store(0, std::memory_order_relaxed);
    has_work_.store(false, std::memory_order_relaxed);
  }

  /**
   * @brief Get the start worker index
   */
  [[nodiscard]] auto get_start_thread_idx() const noexcept -> uint32_t
  {
    return worker_start_idx_;
  }

  /**
   * @brief Get the end worker index
   */
  [[nodiscard]] auto get_end_thread_idx() const noexcept -> uint32_t
  {
    return worker_start_idx_ + static_cast<uint32_t>(thread_count_);
  }

  // Accessors
  [[nodiscard]] auto get_thread_count() const noexcept -> uint32_t
  {
    return static_cast<uint32_t>(thread_count_);
  }

  [[nodiscard]] auto get_priority() const noexcept -> uint32_t
  {
    return priority_;
  }

  auto enter() -> int
  {
    int32_t prev = assigned_workers_.fetch_add(1, std::memory_order_relaxed);
    if (prev >= thread_count_)
    {
      assigned_workers_.fetch_sub(1, std::memory_order_relaxed);
      return -1;
    }
    // success
    std::atomic_thread_fence(std::memory_order_acquire);
    return prev;
  }

  void exit() noexcept
  {
    std::atomic_thread_fence(std::memory_order_release);
    assigned_workers_.fetch_sub(1, std::memory_order_relaxed);
  }

private:
  friend class ouly::v2::scheduler;

  // Work availability flag - advertises to scheduler
  alignas(cache_line_size) std::atomic_int32_t assigned_workers_{0};
  alignas(cache_line_size) std::atomic_bool has_work_{false}; // Flag to indicate work availability  // Configuration
  alignas(cache_line_size) int32_t thread_count_ = 0;
  uint32_t worker_start_idx_                     = 0;
  uint32_t worker_end_idx_                       = 0; // End index for this workgroup
  uint32_t priority_                             = 0;

  ouly::v2::scheduler* owner_ = nullptr; // Pointer to the owning scheduler
  // Work queues - one Chase-Lev queue per worker in this workgroup
  std::unique_ptr<queue_type[]> work_queues_;

  // Mailbox for cross-workgroup work submission
  std::mutex                   mailbox_mutex_;
  ouly::basic_queue<work_item> mailbox_;
};

} // namespace ouly::detail::v2

#ifdef _MSC_VER
#pragma warning(pop)
#endif