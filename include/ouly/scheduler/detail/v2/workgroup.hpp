#pragma once

#include "ouly/allocators/default_allocator.hpp"
#include "ouly/containers/basic_queue.hpp"
#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/scheduler/detail/mpmc_ring.hpp"
#include "ouly/scheduler/detail/spmc_ring.hpp"
#include "ouly/scheduler/spin_lock.hpp"
#include "ouly/scheduler/v2/task_context.hpp"
#include <atomic>
#include <bit>
#include <cstdint>
#include <mutex>
#include <numeric>
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
static constexpr uint32_t mpmc_capacity = 1024;
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
  using mailbox = std::unique_ptr<ouly::detail::mpmc_ring<work_item, mpmc_capacity>>;

  static constexpr int32_t  max_fast_context_switch = 64;
  static constexpr uint32_t word_size               = 64;

  using queue_type = ::ouly::detail::spmc_ring<work_item>;

  workgroup() noexcept  = default;
  ~workgroup() noexcept = default;

  workgroup(const workgroup&)                    = delete;
  auto operator=(const workgroup&) -> workgroup& = delete;
  workgroup(workgroup&& other) noexcept
      : has_work_(other.has_work_.load(std::memory_order_relaxed)),
        small_mask_(other.small_mask_.load(std::memory_order_relaxed)), thread_count_(other.thread_count_),
        worker_start_idx_(other.worker_start_idx_), worker_end_idx_(other.worker_end_idx_), priority_(other.priority_),
        owner_(other.owner_), work_queues_(std::move(other.work_queues_)), mailbox_(std::move(other.mailbox_)),
        bitfield_(std::move(other.bitfield_)), bitfield_words_(other.bitfield_words_)
  {
    other.thread_count_ = 0;
    other.priority_     = 0;
    other.owner_        = nullptr;
  }

  auto operator=(workgroup&& other) noexcept -> workgroup&
  {
    if (this != &other)
    {
      has_work_.store(other.has_work_.load(std::memory_order_relaxed), std::memory_order_relaxed);
      small_mask_.store(other.small_mask_.load(std::memory_order_relaxed), std::memory_order_relaxed);
      thread_count_     = other.thread_count_;
      worker_start_idx_ = other.worker_start_idx_;
      worker_end_idx_   = other.worker_end_idx_;
      priority_         = other.priority_;
      owner_            = other.owner_;
      work_queues_      = std::move(other.work_queues_);
      mailbox_          = std::move(other.mailbox_);
      bitfield_         = std::move(other.bitfield_);
      bitfield_words_   = other.bitfield_words_;
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
    thread_count_     = static_cast<int64_t>(thread_count);
    priority_         = priority;

    // Allocate Chase‑Lev queues for each worker
    work_queues_ = std::make_unique<queue_type[]>(thread_count);
    mailbox_     = std::make_unique<ouly::detail::mpmc_ring<work_item, mpmc_capacity>>();

    // Bitmap initialisation

    small_mask_.store((thread_count == max_fast_context_switch) ? ~uint64_t{0} : ((uint64_t{1} << thread_count) - 1),
                      std::memory_order_relaxed);
    if (thread_count_ > max_fast_context_switch)
    {
      uint32_t slow_switch_count = thread_count - max_fast_context_switch;
      bitfield_words_            = (slow_switch_count + word_size - 1) / word_size;
      bitfield_                  = std::make_unique<uint64_t[]>(bitfield_words_);
      for (uint32_t w = 0; w < bitfield_words_; ++w)
      {
        bitfield_[w] = ~uint64_t{0};
      }

      // clear unused high bits in the last word
      uint32_t unused = (bitfield_words_ * word_size) - thread_count;
      if (unused != 0U)
      {
        bitfield_[bitfield_words_ - 1] >>= unused;
      }
    }

    // Reset work availability
    has_work_.store(0, std::memory_order_relaxed);
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
    return work_queues_[worker_offset].pop_back(out);
  }

  /**
   * @brief Try to steal work from any worker's queue in this workgroup
   */
  [[nodiscard]] auto steal_work(work_item& out, uint32_t steal_offset) noexcept -> bool
  {
    OULY_ASSERT(thread_count_ > 0);

    auto attempts = static_cast<uint64_t>(thread_count_);

    for (uint64_t i = 0; i < attempts; ++i)
    {
      uint64_t worker_idx = (steal_offset + i) % attempts;

      if (work_queues_[worker_idx].steal(out))
      {
        return true;
      }
    }

    return false;
  }

  /**
   * @brief Submit work via mailbox (cross-workgroup submission)
   */
  [[nodiscard]] auto submit_to_mailbox(work_item const& item) noexcept -> bool
  {
    if (mailbox_->emplace(item))
    {
      advertise_work_available();
      return true;
    }
    return false;
  }

  /**
   * @brief Try to receive work from mailbox
   */
  [[nodiscard]] auto receive_from_mailbox(work_item& out) noexcept -> bool
  {
    return mailbox_->pop(out);
  }

  /**
   * @brief Check if this workgroup has work available
   */
  [[nodiscard]] auto has_work() const noexcept -> bool
  {
    return has_work_.load(std::memory_order_relaxed) > 0;
  }

  /**
   * @brief Check if this workgroup has work available
   */
  [[nodiscard]] auto has_work_strong() const noexcept -> bool
  {
    return has_work_.load(std::memory_order_acquire) > 0;
  }

  /**
   * @brief Advertise that work is available to the scheduler
   */
  void advertise_work_available() noexcept
  {
    has_work_.fetch_add(1, std::memory_order_relaxed);
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
    std::scoped_lock lock(slot_mutex_);
    small_mask_.store(0, std::memory_order_relaxed);
    bitfield_.reset();
    bitfield_words_ = 0;

    thread_count_ = 0;
    priority_     = 0;
    work_queues_.reset();
    has_work_.store(0, std::memory_order_relaxed);
  }

  void sink_one_work()
  {
    has_work_.fetch_sub(1, std::memory_order_relaxed);
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

  /**
   * @brief Enter the workgroup and claim an available worker slot
   * @return Worker slot index (0 to thread_count_-1) on success, -1 if no slots available
   */
  auto enter() -> int
  {
    uint64_t mask = small_mask_.load(std::memory_order_relaxed);
    while (mask != 0U)
    {
      uint64_t bit      = mask & (~mask + 1); // isolate lowest‑set bit
      uint64_t new_mask = mask & ~bit;        // clear it
      if (small_mask_.compare_exchange_weak(mask, new_mask, std::memory_order_acq_rel, std::memory_order_relaxed))
      {
        return std::countr_zero(bit);
      }
    }
    if (thread_count_ <= max_fast_context_switch)
    {
      return -1; // no free slots
    }

    std::scoped_lock lock(slot_mutex_);
    for (uint32_t w = 0; w < bitfield_words_; ++w)
    {
      uint64_t offset_mask = bitfield_[w];
      if (offset_mask != 0U)
      {
        auto bit = std::countr_zero(offset_mask);
        bitfield_[w] &= ~(uint64_t{1} << static_cast<uint32_t>(bit));
        return static_cast<int>(w * word_size) + bit + max_fast_context_switch;
      }
    }
    return -1;
  }

  /**
   * @brief Exit the workgroup and release the worker slot
   * @param slot_index The slot index that was returned by enter()
   */
  void exit(int slot_index) noexcept
  {
    if (slot_index < 0 || slot_index >= thread_count_)
    {
      return;
    }

    if (slot_index < max_fast_context_switch)
    {
      small_mask_.fetch_or(uint64_t{1} << static_cast<uint32_t>(slot_index), std::memory_order_release);
      return;
    }

    std::scoped_lock lock(slot_mutex_);
    auto             index = static_cast<uint32_t>(slot_index) - max_fast_context_switch;
    uint32_t         w     = index / word_size;
    uint32_t         bit   = index % word_size;
    bitfield_[w] |= (uint64_t{1} << bit);
  }

private:
  friend class ouly::v2::scheduler;

  // --- Slot management (mutex‑protected bitfield) -------------------------
  alignas(cache_line_size) std::atomic_uint64_t has_work_{0};   // Work availability flag
  alignas(cache_line_size) std::atomic_uint64_t small_mask_{0}; // for ≤64 threads

  int64_t  thread_count_     = 0;
  uint32_t worker_start_idx_ = 0;
  uint32_t worker_end_idx_   = 0; // End index for this workgroup
  uint32_t priority_         = 0;

  ouly::v2::scheduler* owner_ = nullptr; // Pointer to the owning scheduler
  // Work queues - one Chase-Lev queue per worker in this workgroup
  std::unique_ptr<queue_type[]> work_queues_;

  // Mailbox for cross-workgroup work submission
  mailbox mailbox_;

  alignas(cache_line_size) std::mutex slot_mutex_;
  alignas(cache_line_size) std::unique_ptr<uint64_t[]> bitfield_; // for >64 threads
  uint32_t bitfield_words_{0};
};

} // namespace ouly::detail::v2

#ifdef _MSC_VER
#pragma warning(pop)
#endif
