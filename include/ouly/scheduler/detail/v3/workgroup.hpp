// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/detail/spmc_ring.hpp"
#include "ouly/scheduler/v3/task_context.hpp"
#include "ouly/utility/user_config.hpp"
#include <atomic>
#include <cstdint>
#include <memory>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly::detail::v3
{
static constexpr uint32_t max_workgroup    = 32;   // Maximum number of workgroups supported
static constexpr uint32_t mailbox_capacity = 1024; // Capacity of the cross-thread mailbox

using work_item = ouly::v3::task_delegate;

/**
 * @brief Description of a workgroup, captured before begin_execution() freezes the layout.
 */
struct workgroup_desc
{
  uint32_t start_        = 0;
  uint32_t thread_count_ = 0;
  uint32_t priority_     = 0;
};

/**
 * @brief v3 workgroup: fixed worker membership, per-member Chase-Lev deque + MPMC mailbox.
 *
 * Key difference from v2: the `queued_` counter tracks items *currently sitting in queues*.
 * It is incremented when an item is pushed and decremented when an item is successfully
 * dequeued (before execution). This makes `has_queued()` an accurate signal that idle
 * workers can use to decide to park, instead of spinning while unrelated tasks execute.
 */
class workgroup
{
public:
  using queue_type   = ouly::detail::spmc_ring<work_item>;
  using mailbox_type = ouly::detail::mpmc_ring<work_item, mailbox_capacity>;

  workgroup() noexcept  = default;
  ~workgroup() noexcept = default;

  workgroup(workgroup const&)                    = delete;
  auto operator=(workgroup const&) -> workgroup& = delete;
  workgroup(workgroup&&)                         = delete;
  auto operator=(workgroup&&) -> workgroup&      = delete;

  void create_group(uint32_t start, uint32_t thread_count, uint32_t priority) noexcept
  {
    start_        = start;
    thread_count_ = thread_count;
    priority_     = priority;
    queues_       = std::make_unique<queue_type[]>(thread_count);
    mailbox_      = std::make_unique<mailbox_type>();
    queued_.store(0, std::memory_order_relaxed);
  }

  void clear() noexcept
  {
    // Drop queued items; in-flight accounting is owned by the scheduler.
    if (queues_)
    {
      for (uint32_t i = 0; i < thread_count_; ++i)
      {
        ouly::detail::vector_access(queues_, i).clear();
      }
    }
    if (mailbox_)
    {
      mailbox_->clear();
    }
    queued_.store(0, std::memory_order_relaxed);
  }

  [[nodiscard]] auto contains(uint32_t worker_index) const noexcept -> bool
  {
    return worker_index >= start_ && worker_index < start_ + thread_count_;
  }

  [[nodiscard]] auto get_offset(uint32_t worker_index) const noexcept -> uint32_t
  {
    return worker_index - start_;
  }

  /**
   * @brief Push to the calling member worker's own deque. Single producer per deque:
   * only the worker owning `offset` may call this.
   */
  [[nodiscard]] auto push_local(uint32_t offset, work_item const& item) noexcept -> bool
  {
    OULY_ASSERT(offset < thread_count_);
    if (ouly::detail::vector_access(queues_, offset).push_back(item))
    {
      // seq_cst so the producer's later wake-epoch read/modify observes this in a total
      // order with a parking worker's recheck of `queued_` (lost-wakeup prevention).
      queued_.fetch_add(1, std::memory_order_seq_cst);
      return true;
    }
    return false;
  }

  /**
   * @brief Push from any thread (cross-group or external submission).
   */
  [[nodiscard]] auto push_mailbox(work_item const& item) noexcept -> bool
  {
    if (mailbox_->emplace(item))
    {
      queued_.fetch_add(1, std::memory_order_seq_cst);
      return true;
    }
    return false;
  }

  /**
   * @brief Take one item as the member worker at `offset`: own deque first, then the
   * mailbox, then steal from sibling deques starting at a randomized position.
   */
  [[nodiscard]] auto take(work_item& out, uint32_t offset, uint32_t steal_seed) noexcept -> bool
  {
    if (ouly::detail::vector_access(queues_, offset).pop_back(out))
    {
      sink_one();
      return true;
    }
    if (mailbox_->pop(out))
    {
      sink_one();
      return true;
    }
    for (uint32_t i = 0; i < thread_count_; ++i)
    {
      uint32_t victim = (steal_seed + i) % thread_count_;
      if (victim == offset)
      {
        continue;
      }
      if (ouly::detail::vector_access(queues_, victim).steal(out))
      {
        sink_one();
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Take one item using only multi-consumer-safe operations (mailbox pop and
   * steals). Safe to call from any thread, unlike take().
   */
  [[nodiscard]] auto take_any(work_item& out) noexcept -> bool
  {
    if (mailbox_->pop(out))
    {
      sink_one();
      return true;
    }
    for (uint32_t i = 0; i < thread_count_; ++i)
    {
      if (ouly::detail::vector_access(queues_, i).steal(out))
      {
        sink_one();
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] auto has_queued() const noexcept -> bool
  {
    return queued_.load(std::memory_order_acquire) > 0;
  }

  [[nodiscard]] auto get_start_thread_idx() const noexcept -> uint32_t
  {
    return start_;
  }

  [[nodiscard]] auto get_thread_count() const noexcept -> uint32_t
  {
    return thread_count_;
  }

  [[nodiscard]] auto get_priority() const noexcept -> uint32_t
  {
    return priority_;
  }

private:
  void sink_one() noexcept
  {
    queued_.fetch_sub(1, std::memory_order_acq_rel);
  }

  alignas(cache_line_size) std::atomic<int64_t> queued_{0};

  std::unique_ptr<queue_type[]> queues_;
  std::unique_ptr<mailbox_type> mailbox_;

  uint32_t start_        = 0;
  uint32_t thread_count_ = 0;
  uint32_t priority_     = 0;
};

} // namespace ouly::detail::v3

#ifdef _MSC_VER
#pragma warning(pop)
#endif
