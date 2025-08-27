
// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/object_pool.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/utils.hpp"
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <thread>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly
{

/**
 * @brief A high-performance lock-free concurrent queue with fallback mutex for bucket allocation
 *
 * @tparam T The type of elements stored in the queue
 * @tparam Config Configuration for the queue, affecting bucket size and allocation behavior
 *
 * concurrent_queue implements a multi-producer, multi-consumer (MPMC) queue optimized for
 * high-throughput scenarios. It uses a lock-free fast path for enqueue/dequeue operations
 * and only acquires a mutex when allocating new buckets.
 *
 * Fast variant mode (single_threaded_consumer_for_each):
 * When the Config contains ouly::cfg::single_threaded_consumer_for_each:
 * - try_dequeue is disabled (not available)
 * - for_each method is available for single-threaded traversal
 * - clear method is available for single-threaded cleanup
 * - Optimized enqueue using fetch_add instead of compare_exchange
 * - Assumes traversal/clear are called in single-threaded context with no concurrent enqueue
 *
 * Key optimizations:
 * - Lock-free fast path for enqueue/dequeue operations
 * - Cache-aligned bucket structure to minimize false sharing
 * - Exponential backoff for contention handling
 * - Memory ordering optimizations for maximum performance
 * - Bucket reuse through object pool for memory efficiency
 *
 * Design:
 * - Uses linked list of fixed-size buckets
 * - Each bucket contains an array of elements and atomic tail counter
 * - Head and tail pointers are atomic for lock-free traversal
 * - Mutex only protects bucket allocation, not element operations
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
 * @note This queue is optimized for scenarios with multiple producers and consumers
 * @note Elements must be movable or copyable
 * @warning Destructors are called on dequeue, not on queue destruction for remaining elements
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
  static constexpr int    max_backoff     = 256;

  struct alignas(cache_line_size) bucket
  {
    // Hot data: frequently accessed atomics
    alignas(cache_line_size) std::atomic<size_type> tail_{0};
    alignas(cache_line_size) std::atomic<bucket*> next_{nullptr};

    // Cold data: the actual storage (separate cache line)
    alignas(cache_line_size) std::array<ouly::detail::aligned_storage<sizeof(T), alignof(T)>, pool_size> data_{};

    bucket()  = default;
    ~bucket() = default;

    // Non-copyable, non-movable to ensure stable addresses
    bucket(const bucket&)                    = delete;
    bucket(bucket&&)                         = delete;
    auto operator=(const bucket&) -> bucket& = delete;
    auto operator=(bucket&&) -> bucket&      = delete;
  };

  // Separate cache lines for head and tail to minimize false sharing
  alignas(cache_line_size) std::atomic<bucket*> head_{nullptr};
  alignas(cache_line_size) std::atomic<bucket*> tail_{nullptr};
  alignas(cache_line_size) mutable std::shared_mutex bucket_mutex_;
  alignas(cache_line_size) object_pool<bucket, Config> bucket_pool_;

public:
  /**
   * @brief Construct a new concurrent queue
   */
  concurrent_queue()
  {
    // Initialize with one bucket (constructor context is single-threaded)
    bucket* initial_bucket = bucket_pool_.allocate();
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    new (initial_bucket) bucket();
    head_.store(initial_bucket, std::memory_order_relaxed);
    tail_.store(initial_bucket, std::memory_order_relaxed);
  }

  /**
   * @brief Destroy the concurrent queue
   * @warning Any remaining elements will be destroyed
   */
  ~concurrent_queue()
  {
    if constexpr (is_fast_variant)
    {
      // Fast variant: use optimized clear for single-threaded cleanup
      clear();
    }
    else
    {
      // Regular mode: drain remaining elements using try_dequeue
      T dummy;
      while (try_dequeue(dummy))
      {
      }
    }

    // Clean up remaining buckets
    bucket* current = head_.load(std::memory_order_relaxed);
    while (current != nullptr)
    {
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
    requires(is_fast_variant)
  {
    // Fast variant: optimized for single-threaded consumer with for_each
    // Use fetch_add for faster enqueue since we don't need to handle concurrent dequeue
    for (int backoff = 1;; backoff = std::min(backoff * 2, max_backoff))
    {
      bucket* tail_bucket = tail_.load(std::memory_order_acquire);

      // Try to claim a slot using fetch_add (faster than compare_exchange)
      size_type claimed_pos = tail_bucket->tail_.fetch_add(1, std::memory_order_acq_rel);

      if (claimed_pos < pool_size)
      {
        // Successfully claimed slot, construct element
        new (get_element_ptr(tail_bucket, claimed_pos)) T(std::forward<Args>(args)...);
        return;
      }

      // Bucket is full, need to undo the increment and get new bucket
      tail_bucket->tail_.fetch_sub(1, std::memory_order_acq_rel);

      // Slow path: need new bucket
      if (ensure_next_bucket(tail_bucket))
      {
        // Successfully added new bucket, retry enqueue
        continue;
      }

      // Exponential backoff before retry
      for (int i = 0; i < backoff; ++i)
      {
        std::this_thread::yield();
      }
    }
  }

  /**
   * @brief Emplace an element directly in the queue
   * @tparam Args Types of arguments to construct the element
   * @param args Arguments to construct the element
   */
  template <typename... Args>
  void emplace(Args&&... args)
    requires(!is_fast_variant)
  {

    // Regular mode: use compare_exchange for thread-safe enqueue/dequeue
    for (int backoff = 1;; backoff = std::min(backoff * 2, max_backoff))
    {
      bucket*   tail_bucket = tail_.load(std::memory_order_acquire);
      size_type tail_pos    = tail_bucket->tail_.load(std::memory_order_relaxed);

      // Fast path: try to claim a slot in current tail bucket
      if (tail_pos < pool_size)
      {
        if (tail_bucket->tail_.compare_exchange_weak(tail_pos, tail_pos + 1, std::memory_order_acq_rel,
                                                     std::memory_order_relaxed))
        {

          // Successfully claimed slot, construct element
          new (get_element_ptr(tail_bucket, tail_pos)) T(std::forward<Args>(args)...);
          return;
        }
        // CAS failed, retry immediately
        continue;
      }

      // Slow path: need new bucket
      if (ensure_next_bucket(tail_bucket))
      {
        // Successfully added new bucket, retry enqueue
        continue;
      }

      // Exponential backoff before retry
      for (int i = 0; i < backoff; ++i)
      {
        std::this_thread::yield();
      }
    }
  }

  /**
   * @brief Try to dequeue an element (only available in regular mode)
   * @param result Reference to store the dequeued element
   * @return true if an element was dequeued, false if queue was empty
   */
  [[nodiscard]] auto try_dequeue(T& result) -> bool
    requires(!is_fast_variant)
  {
    for (int backoff = 1;; backoff = std::min(backoff * 2, max_backoff))
    {
      bucket* head_bucket = head_.load(std::memory_order_acquire);

      // Try to decrement tail to claim an element
      size_type tail_pos = head_bucket->tail_.load(std::memory_order_acquire);

      while (tail_pos > 0)
      {
        if (head_bucket->tail_.compare_exchange_weak(tail_pos, tail_pos - 1, std::memory_order_acq_rel,
                                                     std::memory_order_relaxed))
        {

          // Successfully claimed element at position tail_pos - 1
          T* element_ptr = get_element_ptr(head_bucket, tail_pos - 1);

          // Move/copy the element
          if constexpr (std::is_move_constructible_v<T>)
          {
            result = std::move(*element_ptr);
          }
          else
          {
            result = *element_ptr;
          }

          // Destroy the element
          element_ptr->~T();
          return true;
        }
        // CAS failed, tail_pos is updated, try again if > 0
      }

      // This bucket is empty (tail_pos == 0), try to advance head
      bucket* next_bucket = head_bucket->next_.load(std::memory_order_acquire);
      if (next_bucket == nullptr)
      {
        // Truly empty queue
        return false;
      }

      // Try to advance head pointer
      if (head_.compare_exchange_weak(head_bucket, next_bucket, std::memory_order_acq_rel, std::memory_order_relaxed))
      {
        // Successfully advanced, call destructor and deallocate old bucket
        head_bucket->~bucket();
        bucket_pool_.deallocate(head_bucket);
      } // Exponential backoff before retry
      for (int i = 0; i < backoff; ++i)
      {
        std::this_thread::yield();
      }
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
    bucket* current = head_.load(std::memory_order_relaxed);

    while (current != nullptr)
    {
      // Get the count of elements in this bucket (relaxed since single-threaded)
      size_type count = current->tail_.load(std::memory_order_relaxed);

      // Process all elements in this bucket
      for (size_type i = 0; i < count; ++i)
      {
        T* element_ptr = get_element_ptr(current, i);
        func(*element_ptr);
      }

      // Move to next bucket
      current = current->next_.load(std::memory_order_relaxed);
    }
  }

  /**
   * @brief Traverse all elements in the queue with a function that accepts a span<T> (fast variant only)
   * @tparam F Function type that accepts span<T> or span<const T>
   * @param func Function to call for each element
   * @note This method assumes single-threaded access and no concurrent enqueue operations
   */
  template <typename F>
  void for_each_bucket(F func)
    requires(is_fast_variant)
  {
    bucket* current = head_.load(std::memory_order_relaxed);

    while (current != nullptr)
    {
      std::span<T> elements = get_elements_span(current);
      func(elements);

      // Move to next bucket
      current = current->next_.load(std::memory_order_relaxed);
    }
  }

  /**
   * @brief Clear all elements from the queue (fast variant only)
   * @note This method assumes single-threaded access and no concurrent enqueue operations
   */
  void clear()
    requires(is_fast_variant)
  {
    bucket* current = head_.load(std::memory_order_relaxed);

    while (current != nullptr)
    {
      // Get the count of elements in this bucket
      size_type count = current->tail_.load(std::memory_order_relaxed);

      // Destroy all elements in this bucket
      for (size_type i = 0; i < count; ++i)
      {
        T* element_ptr = get_element_ptr(current, i);
        element_ptr->~T();
      }

      // Reset the tail count
      current->tail_.store(0, std::memory_order_relaxed);

      // Move to next bucket
      bucket* next = current->next_.load(std::memory_order_relaxed);

      // If this is not the head bucket, deallocate it
      if (current != head_.load(std::memory_order_relaxed))
      {
        current->~bucket();
        bucket_pool_.deallocate(current);
      }

      current = next;
    }

    // Reset head and tail to point to the original head bucket
    bucket* head_bucket = head_.load(std::memory_order_relaxed);
    if (head_bucket != nullptr)
    {
      // Clear the next pointer of the head bucket
      head_bucket->next_.store(nullptr, std::memory_order_relaxed);
      tail_.store(head_bucket, std::memory_order_relaxed);
    }
  }

  /**
   * @brief Check if the queue appears to be empty
   * @return true if the queue appears empty (may change immediately)
   * @note This is a best-effort check and may return false positives in concurrent scenarios
   */
  [[nodiscard]] auto empty() const noexcept -> bool
  {
    bucket*   head_bucket = head_.load(std::memory_order_acquire);
    size_type tail_pos    = head_bucket->tail_.load(std::memory_order_acquire);

    if (tail_pos > 0)
    {
      return false;
    }

    // Check if there's a next bucket with elements
    bucket* next_bucket = head_bucket->next_.load(std::memory_order_acquire);
    return next_bucket == nullptr || next_bucket->tail_.load(std::memory_order_acquire) == 0;
  }

  /**
   * @brief Get approximate size of the queue
   * @return Approximate number of elements (may be inaccurate in concurrent scenarios)
   * @note This is an expensive operation that traverses the bucket chain
   */
  [[nodiscard]] auto size() const noexcept -> size_type
  {
    size_type total   = 0;
    bucket*   current = head_.load(std::memory_order_acquire);

    while (current != nullptr)
    {
      total += current->tail_.load(std::memory_order_acquire);
      current = current->next_.load(std::memory_order_acquire);
    }

    return total;
  }

private:
  /**
   * @brief Allocate a new bucket from the pool (must be called under mutex protection)
   */
  auto allocate_bucket() -> bucket*
  {
    bucket* new_bucket = bucket_pool_.allocate();
    // Placement new to ensure proper construction of atomics
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new (new_bucket) bucket();
  }

  /**
   * @brief Ensure that the given bucket has a next bucket
   * @param current_bucket The bucket that needs a next bucket
   * @return true if next bucket exists or was created, false on failure
   */
  auto ensure_next_bucket(bucket* current_bucket) -> bool
  {
    std::unique_lock<std::shared_mutex> lock(bucket_mutex_);

    // Check if next bucket already exists (double-checked locking)
    bucket* next = current_bucket->next_.load(std::memory_order_acquire);
    if (next != nullptr)
    {
      // Someone else already created the next bucket, advance tail pointer
      tail_.compare_exchange_weak(current_bucket, next, std::memory_order_acq_rel, std::memory_order_relaxed);
      return true;
    }

    // Allocate and construct new bucket (already protected by mutex)
    bucket* new_bucket = bucket_pool_.allocate();
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    new (new_bucket) bucket();

    // Link the new bucket atomically
    current_bucket->next_.store(new_bucket, std::memory_order_release);

    // Advance tail pointer to the new bucket
    tail_.compare_exchange_weak(current_bucket, new_bucket, std::memory_order_acq_rel, std::memory_order_relaxed);

    return true;
  }

  /**
   * @brief Get pointer to element at given position in bucket
   */
  static auto get_element_ptr(bucket* bucket_ptr, size_type pos) noexcept -> T*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return bucket_ptr->data_[pos].template as<T>();
  }

  /**
   * @brief Get a span of elements in the bucket
   */
  static auto get_elements_span(bucket* bucket_ptr) noexcept -> std::span<T>
  {
    size_type count = bucket_ptr->tail_.load(std::memory_order_relaxed);
    return std::span<T>(get_element_ptr(bucket_ptr, 0), count);
  }
};

} // namespace ouly

#ifdef _MSC_VER
#pragma warning(pop)
#endif
