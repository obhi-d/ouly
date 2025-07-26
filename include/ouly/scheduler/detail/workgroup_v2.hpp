// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/allocators/default_allocator.hpp"
#include "ouly/containers/basic_queue.hpp"
#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/chase_lev_queue.hpp"
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/spin_lock.hpp"
#include "ouly/scheduler/task.hpp"
#include "ouly/scheduler/worker_context_v2.hpp"
#include <atomic>
#include <cstdint>
#include <span>

namespace ouly::detail::inline v2
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
  using queue_type      = chase_lev_queue<work_item>;
  workgroup() noexcept  = default;
  ~workgroup() noexcept = default;

  workgroup(const workgroup&)                    = delete;
  auto operator=(const workgroup&) -> workgroup& = delete;
  workgroup(workgroup&&)                         = delete;
  auto operator=(workgroup&&) -> workgroup&      = delete;

  /**
   * @brief Initialize the workgroup with worker threads
   */
  void create_group(uint32_t thread_count, uint32_t priority) noexcept
  {
    thread_count_ = thread_count;
    priority_     = priority;

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
    OULY_ASSERT(worker_offset < thread_count_);

    work_queues_[worker_offset].push_front(item);
    advertise_work_available();
    return true;
  }

  /**
   * @brief Try to pop work from a specific worker's queue
   */
  auto pop_work_from_worker(work_item& out, uint32_t worker_offset) noexcept -> bool
  {
    OULY_ASSERT(worker_offset < thread_count_);
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
    uint32_t              attempts    = thread_count_;

    for (uint32_t i = 0; i < attempts; ++i)
    {
      uint32_t worker_idx = (steal_index + i) % thread_count_;
      if (worker_idx == avoid_worker_offset)
      {
        continue; // Skip the worker we want to avoid
      }

      if (work_queues_[worker_idx].pop_back(out))
      {
        steal_index = (worker_idx + 1) % thread_count_; // Update for next time
        return true;
      }
    }

    return false;
  }

  /**
   * @brief Submit work via mailbox (cross-workgroup submission)
   */
  auto submit_to_mailbox(work_item const& item) noexcept -> bool
  {
    bool success = mailbox_.emplace(item);
    if (success)
    {
      advertise_work_available();
    }
    return success;
  }

  /**
   * @brief Try to receive work from mailbox
   */
  auto receive_from_mailbox(work_item& out) noexcept -> bool
  {
    return mailbox_.pop(out);
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

  // Accessors
  [[nodiscard]] auto get_thread_count() const noexcept -> uint32_t
  {
    return thread_count_;
  }
  [[nodiscard]] auto get_priority() const noexcept -> uint32_t
  {
    return priority_;
  }

private:
  scheduler* owner_ = nullptr; // Pointer to the owning scheduler
  // Work queues - one Chase-Lev queue per worker in this workgroup
  std::unique_ptr<queue_type[]> work_queues_;

  // Mailbox for cross-workgroup work submission
  mpmc_ring<work_item, mpmc_capacity> mailbox_;

  // Work availability flag - advertises to scheduler
  std::atomic<bool> has_work_{false};

  // Configuration
  uint32_t thread_count_ = 0;
  uint32_t priority_     = 0;
};

} // namespace ouly::detail::inline v2
