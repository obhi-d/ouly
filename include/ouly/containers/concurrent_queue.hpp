
// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/object_pool.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/utils.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>
#include <new>
#include <shared_mutex>
#include <span>
#include <type_traits>
#include <utility>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly
{

/**
 * @brief A multi-producer, multi-consumer FIFO queue built from a chain of fixed-size buckets
 *
 * @tparam T The type of elements stored in the queue
 * @tparam Config Configuration for the queue, affecting bucket size and allocation behavior
 *
 * Design (regular mode):
 * - Each bucket holds `pool_size` slots plus two monotonic cursors. Producers claim a slot
 *   with a fetch_add on `enqueue_pos_`; a claim at or beyond `pool_size` means the bucket is
 *   full and is never undone, so a bucket that has filled once can never accept new elements
 *   again. Consumers claim committed slots with a CAS on the independent `dequeue_pos_`.
 *   Slots are never reused within a bucket, which rules out ABA on either cursor.
 * - A producer constructs the element first and then publishes it through a per-slot commit
 *   flag (release store); consumers acquire-load the flag before claiming, so an element can
 *   never be observed mid-construction. If a constructor throws, the slot is poisoned and
 *   silently skipped by consumers; the queue stays functional.
 * - Element operations run under a shared lock; bucket transitions (growing the tail,
 *   retiring an exhausted head bucket, and all bucket-pool traffic) take the exclusive lock.
 *   Because a bucket is only deallocated while the exclusive lock is held, no thread can
 *   still hold a pointer into it, which makes reclamation safe without hazard pointers.
 *   The uncontended shared lock costs two atomic operations, so the common path remains a
 *   handful of atomics per element.
 *
 * Ordering: FIFO per bucket and across buckets. With a single producer the dequeue order is
 * exactly the enqueue order; with concurrent producers, elements are ordered by their slot
 * claims.
 *
 * Fast variant mode (single_threaded_consumer_for_each):
 * When the Config contains ouly::cfg::single_threaded_consumer_for_each:
 * - try_dequeue is disabled (not available)
 * - for_each / for_each_bucket are available for single-threaded traversal
 * - clear is available for single-threaded cleanup
 * - Producers share the regular enqueue path (fetch_add claim + commit flag)
 * - Assumes traversal/clear are called in a single-threaded context with no concurrent
 *   enqueue
 *
 * Usage example:
 * ```cpp
 * // Regular mode
 * concurrent_queue<int> queue;
 * queue.enqueue(42);
 * int value;
 * if (queue.try_dequeue(value)) { }
 *
 * // Fast variant mode
 * using FastConfig = ouly::config<ouly::cfg::single_threaded_consumer_for_each>;
 * concurrent_queue<int, FastConfig> fast_queue;
 * fast_queue.enqueue(42);
 * fast_queue.for_each([](const int& item) { process_item(item); });
 * fast_queue.clear();
 * ```
 *
 * @note try_dequeue may transiently return false while a concurrent enqueue is still
 *       committing the next element; this is inherent to "try" semantics.
 * @note Remaining elements are destroyed when the queue is destroyed.
 */
template <typename T, typename Config = ouly::default_config<T>>
class concurrent_queue
{
public:
  using value_type = T;
  using size_type  = ouly::detail::choose_size_t<uint32_t, Config>;

private:
  static constexpr auto pool_mul  = ouly::detail::log2(ouly::detail::pool_size_v<Config>);
  static constexpr auto pool_size = static_cast<size_type>(1) << pool_mul;

  // Configuration detection
  static constexpr bool is_fast_variant = requires { typename Config::single_threaded_consumer_for_each; };

  // Cache line size for alignment
  static constexpr size_t cache_line_size = 64;

  // Per-slot commit states
  static constexpr uint8_t slot_empty    = 0; // not yet published by a producer
  static constexpr uint8_t slot_ready    = 1; // construction finished, safe to consume
  static constexpr uint8_t slot_poisoned = 2; // constructor threw; skip this slot

  struct alignas(cache_line_size) bucket
  {
    // Producer cursor: monotonic, may overshoot pool_size (overshoot == "bucket full" and
    // is never undone, so a filled bucket can never be written again).
    alignas(cache_line_size) std::atomic<size_type> enqueue_pos_{0};

    // Consumer cursor: only advanced past committed slots, bounded by pool_size.
    alignas(cache_line_size) std::atomic<size_type> dequeue_pos_{0};

    alignas(cache_line_size) std::atomic<bucket*> next_{nullptr};

    // Commit flags, written once per slot by the owning producer (release), read by
    // consumers (acquire). Never reset within a bucket's lifetime.
    std::array<std::atomic<uint8_t>, pool_size> committed_{};

    // The actual storage (separate cache line)
    alignas(cache_line_size) std::array<ouly::detail::aligned_storage<sizeof(T), alignof(T)>, pool_size> data_{};

    bucket()  = default;
    ~bucket() = default;

    // Non-copyable, non-movable to ensure stable addresses
    bucket(const bucket&)                    = delete;
    bucket(bucket&&)                         = delete;
    auto operator=(const bucket&) -> bucket& = delete;
    auto operator=(bucket&&) -> bucket&      = delete;

    /** Number of slots that producers have claimed in this bucket, clamped to capacity. */
    [[nodiscard]] auto claimed_count() const noexcept -> size_type
    {
      return std::min(enqueue_pos_.load(std::memory_order_acquire), pool_size);
    }
  };

  // Separate cache lines for head and tail to minimize false sharing
  alignas(cache_line_size) std::atomic<bucket*> head_{nullptr};
  alignas(cache_line_size) std::atomic<bucket*> tail_{nullptr};
  // Element operations hold this shared; bucket-chain transitions hold it exclusive. The
  // exclusive section is what makes bucket deallocation safe: it cannot overlap any element
  // operation that might still hold a pointer to the bucket.
  alignas(cache_line_size) mutable std::shared_mutex bucket_mutex_;
  alignas(cache_line_size) object_pool<bucket, Config> bucket_pool_;

public:
  /**
   * @brief Construct a new concurrent queue
   */
  concurrent_queue()
  {
    // Initialize with one bucket (constructor context is single-threaded)
    bucket* initial_bucket = allocate_bucket();
    head_.store(initial_bucket, std::memory_order_relaxed);
    tail_.store(initial_bucket, std::memory_order_relaxed);
  }

  /**
   * @brief Destroy the concurrent queue
   * @warning Any remaining elements will be destroyed; destruction must be single-threaded
   */
  ~concurrent_queue()
  {
    bucket* current = head_.load(std::memory_order_relaxed);
    while (current != nullptr)
    {
      destroy_remaining(current);
      bucket* next = current->next_.load(std::memory_order_relaxed);
      current->~bucket();
      bucket_pool_.deallocate(current);
      current = next;
    }
  }

  // Non-copyable, non-movable for simplicity and performance
  concurrent_queue(const concurrent_queue&)                    = delete;
  concurrent_queue(concurrent_queue&&)                         = delete;
  auto operator=(const concurrent_queue&) -> concurrent_queue& = delete;
  auto operator=(concurrent_queue&&) -> concurrent_queue&      = delete;

  /**
   * @brief Enqueue an element by copying
   * @param item The item to enqueue
   */
  void enqueue(const T& item)
    requires std::is_copy_constructible_v<T>
  {
    emplace(item);
  }

  /**
   * @brief Enqueue an element by moving
   * @param item The item to enqueue
   */
  void enqueue(T&& item)
    requires std::is_move_constructible_v<T>
  {
    emplace(std::move(item));
  }

  /**
   * @brief Emplace an element directly in the queue
   * @tparam Args Types of arguments to construct the element
   * @param args Arguments to construct the element
   */
  template <typename... Args>
  void emplace(Args&&... args)
  {
    for (;;)
    {
      {
        std::shared_lock<std::shared_mutex> guard(bucket_mutex_);

        bucket*   tail_bucket = tail_.load(std::memory_order_acquire);
        size_type pos         = tail_bucket->enqueue_pos_.fetch_add(1, std::memory_order_relaxed);

        if (pos < pool_size)
        {
          construct_slot(tail_bucket, pos, std::forward<Args>(args)...);
          return;
        }
        // Bucket full. The overshoot is deliberately not undone: the cursor is monotonic,
        // so no producer can ever claim a slot in this bucket again. Undoing it would let
        // a slow producer claim a slot in a bucket consumers have already retired.
      }
      grow_tail();
    }
  }

  /**
   * @brief Try to dequeue an element (only available in regular mode)
   * @param result Reference to store the dequeued element
   * @return true if an element was dequeued, false if no committed element was available
   */
  [[nodiscard]] auto try_dequeue(T& result) -> bool
    requires(!is_fast_variant)
  {
    for (;;)
    {
      bool head_exhausted = false;
      {
        std::shared_lock<std::shared_mutex> guard(bucket_mutex_);

        bucket*   head_bucket = head_.load(std::memory_order_acquire);
        size_type pos         = head_bucket->dequeue_pos_.load(std::memory_order_relaxed);

        for (;;)
        {
          if (pos >= pool_size)
          {
            // Bucket fully consumed; if a next bucket exists, retire this one and retry.
            head_exhausted = head_bucket->next_.load(std::memory_order_acquire) != nullptr;
            break;
          }

          uint8_t state = head_bucket->committed_[pos].load(std::memory_order_acquire);
          if (state == slot_empty)
          {
            // FIFO frontier not yet published (queue empty, or an enqueue is mid-flight).
            return false;
          }

          if (!head_bucket->dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_acq_rel,
                                                               std::memory_order_relaxed))
          {
            continue; // lost the claim; pos was reloaded
          }

          if (state == slot_ready)
          {
            T* element_ptr = get_element_ptr(head_bucket, pos);
            if constexpr (std::is_move_constructible_v<T>)
            {
              result = std::move(*element_ptr);
            }
            else
            {
              result = *element_ptr;
            }
            std::destroy_at(element_ptr);
            return true;
          }

          // Poisoned slot (constructor threw): we claimed it, skip to the next one.
          pos += 1;
        }
      }

      if (!head_exhausted)
      {
        return false;
      }
      retire_head();
    }
  }

  /**
   * @brief Traverse all elements in the queue with a function (fast variant only)
   * @tparam F Function type that accepts const T& or T&
   * @param func Function to call for each element
   * @note This method assumes single-threaded access and no concurrent enqueue operations
   */
  template <typename F>
  void for_each(F func)
    requires(is_fast_variant)
  {
    bucket* current = head_.load(std::memory_order_acquire);
    while (current != nullptr)
    {
      size_type count = current->claimed_count();
      for (size_type i = 0; i < count; ++i)
      {
        if (current->committed_[i].load(std::memory_order_acquire) == slot_ready)
        {
          func(*get_element_ptr(current, i));
        }
      }
      current = current->next_.load(std::memory_order_acquire);
    }
  }

  /**
   * @brief Traverse all elements in the queue with a function that accepts a span<T> (fast variant only)
   * @tparam F Function type that accepts span<T> or span<const T>
   * @param func Function to call for each bucket's elements
   * @note This method assumes single-threaded access and no concurrent enqueue operations
   * @note If an element constructor threw during enqueue, the corresponding slot in the
   *       span holds an unconstructed element; prefer for_each() for non-nothrow types.
   */
  template <typename F>
  void for_each_bucket(F func)
    requires(is_fast_variant)
  {
    bucket* current = head_.load(std::memory_order_acquire);
    while (current != nullptr)
    {
      func(std::span<T>(get_element_ptr(current, 0), current->claimed_count()));
      current = current->next_.load(std::memory_order_acquire);
    }
  }

  /**
   * @brief Clear all elements from the queue (fast variant only)
   * @note This method assumes single-threaded access and no concurrent enqueue operations
   */
  void clear()
    requires(is_fast_variant)
  {
    std::unique_lock<std::shared_mutex> guard(bucket_mutex_);

    bucket* head_bucket = head_.load(std::memory_order_relaxed);
    bucket* current     = head_bucket;
    while (current != nullptr)
    {
      destroy_remaining(current);
      bucket* next = current->next_.load(std::memory_order_relaxed);
      if (current != head_bucket)
      {
        current->~bucket();
        bucket_pool_.deallocate(current);
      }
      current = next;
    }

    // Recycle the head bucket for reuse.
    size_type claimed = head_bucket->claimed_count();
    for (size_type i = 0; i < claimed; ++i)
    {
      head_bucket->committed_[i].store(slot_empty, std::memory_order_relaxed);
    }
    head_bucket->enqueue_pos_.store(0, std::memory_order_relaxed);
    head_bucket->dequeue_pos_.store(0, std::memory_order_relaxed);
    head_bucket->next_.store(nullptr, std::memory_order_relaxed);
    tail_.store(head_bucket, std::memory_order_release);
  }

  /**
   * @brief Check if the queue appears to be empty
   * @return true if the queue appears empty (may change immediately)
   * @note This is a best-effort check and may return false positives in concurrent scenarios
   */
  [[nodiscard]] auto empty() const noexcept -> bool
  {
    std::shared_lock<std::shared_mutex> guard(bucket_mutex_);

    for (bucket* current = head_.load(std::memory_order_acquire); current != nullptr;
         current         = current->next_.load(std::memory_order_acquire))
    {
      if (current->claimed_count() > current->dequeue_pos_.load(std::memory_order_acquire))
      {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief Get approximate size of the queue
   * @return Approximate number of elements (may be inaccurate in concurrent scenarios)
   * @note This is an expensive operation that traverses the bucket chain
   */
  [[nodiscard]] auto size() const noexcept -> size_type
  {
    std::shared_lock<std::shared_mutex> guard(bucket_mutex_);

    size_type total = 0;
    for (bucket* current = head_.load(std::memory_order_acquire); current != nullptr;
         current         = current->next_.load(std::memory_order_acquire))
    {
      total += current->claimed_count() - current->dequeue_pos_.load(std::memory_order_acquire);
    }
    return total;
  }

private:
  /**
   * @brief Construct an element into a claimed slot and publish it.
   *
   * On exception the slot is poisoned (consumers skip it) and the exception propagates;
   * the queue remains fully functional.
   */
  template <typename... Args>
  void construct_slot(bucket* bucket_ptr, size_type pos, Args&&... args)
  {
    if constexpr (std::is_nothrow_constructible_v<T, Args...>)
    {
      new (get_element_ptr(bucket_ptr, pos)) T(std::forward<Args>(args)...);
    }
    else
    {
      try
      {
        new (get_element_ptr(bucket_ptr, pos)) T(std::forward<Args>(args)...);
      }
      catch (...)
      {
        bucket_ptr->committed_[pos].store(slot_poisoned, std::memory_order_release);
        throw;
      }
    }
    bucket_ptr->committed_[pos].store(slot_ready, std::memory_order_release);
  }

  /**
   * @brief Destroy all committed, not-yet-consumed elements of a bucket.
   * @note Caller must guarantee exclusive access (destructor or clear()).
   */
  void destroy_remaining(bucket* bucket_ptr) noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
      size_type end = bucket_ptr->claimed_count();
      for (size_type i = bucket_ptr->dequeue_pos_.load(std::memory_order_relaxed); i < end; ++i)
      {
        if (bucket_ptr->committed_[i].load(std::memory_order_relaxed) == slot_ready)
        {
          std::destroy_at(get_element_ptr(bucket_ptr, i));
        }
      }
    }
  }

  /**
   * @brief Allocate a new bucket from the pool (caller must hold the exclusive lock,
   * except in the single-threaded constructor)
   */
  auto allocate_bucket() -> bucket*
  {
    bucket* new_bucket = bucket_pool_.allocate();
    // Placement new to ensure proper construction of atomics
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new (new_bucket) bucket();
  }

  /**
   * @brief Advance tail_ to a bucket with free slots, linking a new bucket if needed.
   */
  void grow_tail()
  {
    std::unique_lock<std::shared_mutex> guard(bucket_mutex_);

    bucket* tail_bucket = tail_.load(std::memory_order_relaxed);
    if (tail_bucket->enqueue_pos_.load(std::memory_order_relaxed) < pool_size)
    {
      return; // another producer already advanced to a bucket with space
    }

    bucket* next = tail_bucket->next_.load(std::memory_order_relaxed);
    if (next == nullptr)
    {
      next = allocate_bucket();
      tail_bucket->next_.store(next, std::memory_order_release);
    }
    tail_.store(next, std::memory_order_release);
  }

  /**
   * @brief Retire the head bucket if it is fully consumed and a successor exists.
   *
   * Deallocation happens under the exclusive lock, which cannot overlap any element
   * operation, so no thread can still hold a pointer into the retired bucket.
   */
  void retire_head()
  {
    std::unique_lock<std::shared_mutex> guard(bucket_mutex_);

    bucket* head_bucket = head_.load(std::memory_order_relaxed);
    if (head_bucket->dequeue_pos_.load(std::memory_order_relaxed) < pool_size)
    {
      return; // another consumer already retired it, or items remain
    }

    bucket* next = head_bucket->next_.load(std::memory_order_relaxed);
    if (next == nullptr)
    {
      return;
    }

    head_.store(next, std::memory_order_release);
    head_bucket->~bucket();
    bucket_pool_.deallocate(head_bucket);
  }

  /**
   * @brief Get pointer to element at given position in bucket
   */
  static auto get_element_ptr(bucket* bucket_ptr, size_type pos) noexcept -> T*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return bucket_ptr->data_[pos].template as<T>();
  }
};

} // namespace ouly

#ifdef _MSC_VER
#pragma warning(pop)
#endif
