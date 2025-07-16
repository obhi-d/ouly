/**
 * @file ts_shared_linear_allocator.hpp
 * @brief Thread-safe linear allocator with shared arena management
 */
#pragma once

#include "ouly/utility/common.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <utility>

namespace ouly
{

/**
 * @class ts_shared_linear_allocator
 * @brief A thread-safe linear allocator with shared arena management
 *
 * This allocator provides thread-safe linear allocation from shared arenas.
 * Multiple threads can allocate from the same arena using atomic operations
 * for the allocation offset. It's designed for high-throughput scenarios
 * where multiple threads need to allocate memory concurrently.
 *
 * Key features:
 * - Lock-free allocation using atomic compare-and-swap
 * - Shared arenas reduce memory fragmentation
 * - Optional stack-style deallocation for the most recent allocation
 * - Arena recycling to minimize system allocations
 * - All allocations are aligned to alignof(std::max_align_t)
 *
 * Thread Safety:
 * - allocate() is fully thread-safe and lock-free
 * - deallocate() is thread-safe but may fail due to races
 * - reset() must be called from a single thread with no concurrent allocations
 * - release() must be called from a single thread with no concurrent allocations
 *
 * Memory Layout:
 * Each arena consists of a header (arena_t) with atomic offset tracking
 * followed by aligned data storage. All memory is allocated with
 * std::max_align_t alignment.
 *
 * @warning reset() and release() must be called from a single thread
 * @warning All allocated memory becomes invalid after reset() or release()
 */
class OULY_API ts_shared_linear_allocator
{
public:
  /** @brief Default page size for new arenas (1 MiB) */
  static constexpr uint32_t default_page_size = 1024 * 1024; // 1 MiB

  /** @brief All allocations are aligned to this boundary */
  static constexpr std::size_t alignment = alignof(std::max_align_t);

  /**
   * @brief Default constructor
   * Uses default_page_size for new arenas
   */
  ts_shared_linear_allocator() noexcept = default;

  /**
   * @brief Constructor with custom page size
   * @param page_size Size in bytes for new arenas (must be > 0)
   */
  explicit ts_shared_linear_allocator(std::size_t page_size) noexcept : default_page_size_{page_size} {}

  /**
   * @brief Move constructor
   * @param other Source allocator (will be reset)
   */
  ts_shared_linear_allocator(ts_shared_linear_allocator&& other) noexcept
      : default_page_size_{std::exchange(other.default_page_size_, default_page_size)},
        current_page_{other.current_page_.exchange(nullptr)},
        page_list_head_{std::exchange(other.page_list_head_, nullptr)},
        page_list_tail_{std::exchange(other.page_list_tail_, nullptr)},
        available_pages_{std::exchange(other.available_pages_, nullptr)}
  {}

  /**
   * @brief Move assignment operator
   * @param other Source allocator (will be reset)
   * @return Reference to this allocator
   */
  auto operator=(ts_shared_linear_allocator&& other) noexcept -> ts_shared_linear_allocator&
  {
    if (this == &other)
    {
      return *this; // self-assignment, nothing to do
    }
    reset(); // free any existing arenas
    default_page_size_ = std::exchange(other.default_page_size_, default_page_size);
    current_page_.store(other.current_page_.exchange(nullptr, std::memory_order_release));
    page_list_head_  = std::exchange(other.page_list_head_, nullptr);
    page_list_tail_  = std::exchange(other.page_list_tail_, nullptr);
    available_pages_ = std::exchange(other.available_pages_, nullptr);
    return *this;
  }

  /** @brief Copy constructor is deleted (not copyable) */
  ts_shared_linear_allocator(ts_shared_linear_allocator const&) = delete;

  /** @brief Copy assignment is deleted (not copyable) */
  auto operator=(ts_shared_linear_allocator const&) -> ts_shared_linear_allocator& = delete;

  /**
   * @brief Destructor
   * Automatically releases all allocated memory
   */
  ~ts_shared_linear_allocator() noexcept
  {
    release(); //  free anything still hanging around
  }

  /**
   * @brief Thread-safe allocation from shared arenas
   * @param size Number of bytes to allocate
   * @return Pointer to allocated memory aligned to alignof(std::max_align_t)
   * @note Returns nullptr on allocation failure
   * @note Lock-free using atomic compare-and-swap operations
   */
  auto allocate(std::size_t size) noexcept -> void*;

  /**
   * @brief Optional stack-style deallocation for the most recent allocation
   * @param ptr Pointer to memory to deallocate
   * @param size Size of the allocation in bytes
   * @return true if deallocation succeeded, false otherwise
   * @note Only works if ptr was the most recent allocation on the current arena
   * @note May fail due to race conditions with other threads
   * @note Thread-safe but not guaranteed to succeed
   */
  auto deallocate(void* ptr, std::size_t size) noexcept -> bool;

  /**
   * @brief Reset all arenas for reuse
   * @warning Must be called from a single thread with no concurrent allocate() calls
   * @warning All previously allocated memory becomes invalid after this call
   * @note Resets all arena offsets to 0 and recycles arenas for reuse
   */
  void reset() noexcept;

  /**
   * @brief Release all memory and reset allocator to initial state
   * @warning Must be called from a single thread with no concurrent allocate() calls
   * @warning All previously allocated memory becomes invalid after this call
   * @note Calls reset() then frees all recycled arenas
   */
  void release() noexcept;

private:
  /**
   * @brief Internal arena structure for thread-safe memory management
   *
   * Each arena consists of this header followed by aligned data storage.
   * The atomic offset_ enables lock-free allocation by multiple threads.
   */
  struct arena_t
  {
    std::atomic_size_t offset_; ///< Current allocation offset (thread-safe)
    std::size_t        size_;   ///< Total bytes available in data_[]
    arena_t*           next_;   ///< Intrusive list pointer

    alignas(std::max_align_t) std::byte data_[1]; ///< Flexible array member for allocations
  };

  /**
   * @brief Align a size value up to the allocator's alignment boundary
   * @param value Size to align
   * @return Aligned size (multiple of alignment)
   */
  static constexpr auto align_up(std::size_t value) noexcept -> std::size_t
  {
    return (value + alignment - 1U) & ~(alignment - 1U);
  }

  /**
   * @brief Attempt to allocate from an existing arena using atomic operations
   * @param arena Arena to allocate from
   * @param size Aligned size to allocate
   * @return Pointer to allocated memory or nullptr if arena is full
   * @note Uses compare-and-swap for thread-safe allocation
   */
  static auto try_allocate_from_page(arena_t* arena, std::size_t size) noexcept -> void*;

  /**
   * @brief Slow path allocation when fast path fails
   * @param size Aligned size to allocate
   * @return Pointer to allocated memory
   * @note Creates new arenas or reuses available ones
   */
  auto allocate_slow_path(std::size_t size) noexcept -> void*;

  /* ---------- Data members -------------------------------------------- */

  /** @brief Default size for new arenas (pages larger than this aren't reused) */
  std::size_t default_page_size_ = default_page_size;

  /** @brief Current arena for allocation (atomic for thread safety) */
  std::atomic<arena_t*> current_page_ = nullptr;

  /** @brief Head of the arena list (LIFO) */
  arena_t* page_list_head_ = nullptr;

  /** @brief Tail of the arena list (for fast allocation) */
  arena_t* page_list_tail_ = nullptr;

  /** @brief Arenas scheduled for deletion by reset() */
  arena_t* pages_to_free_ = nullptr;

  /** @brief Pool of available arenas for reuse */
  arena_t* available_pages_ = nullptr;

  /** @brief Mutex protecting shared data structures */
  std::mutex page_mutex_;
};
} // namespace ouly