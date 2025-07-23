/**
 * @file ts_thread_local_allocator.hpp
 * @brief Thread-safe allocator with per-thread arena optimization
 */
#pragma once

#include "ouly/utility/common.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

namespace ouly
{
/**
 * @class ts_thread_local_allocator
 * @brief A thread-safe allocator that uses per-thread arenas for fast allocation
 *
 * This allocator provides extremely fast allocation by giving each thread its own
 * arena for bump-pointer allocation, eliminating contention in the common case.
 * It's designed for frame-based applications where allocations are reset frequently.
 *
 * Key features:
 * - Zero synchronization for allocation in the fast path
 * - Per-thread arenas eliminate contention
 * - Generation-based invalidation for safe reset
 * - Optional stack-style deallocation for the most recent allocation
 * - All allocations are aligned to alignof(std::max_align_t)
 *
 * Thread Safety:
 * - allocate() and deallocate() are thread-safe
 * - reset() must be called from a single thread with no concurrent allocations
 * - release() must be called from a single thread with no concurrent allocations
 *
 * Memory Layout:
 * Each arena consists of a header (arena_t) followed by aligned data storage.
 * All memory is allocated with std::max_align_t alignment.
 *
 * @warning reset() must be called when no worker threads are calling allocate()
 * @warning All allocated memory becomes invalid after reset() or release()
 */
class OULY_API ts_thread_local_allocator
{
public:
  /** @brief Default page size for new arenas (1 MiB) */
  static constexpr uint32_t default_page_size = 1024 * 1024; // 1 MiB

  /** @brief All allocations are aligned to this boundary */
  static constexpr std::size_t alignment = alignof(std::max_align_t);

  // constexpr static size_t min_thread_local_slots = 32; ///< Minimum number of thread-local slots
  /**
   * @brief Default constructor
   * Uses default_page_size for new arenas
   */
  ts_thread_local_allocator() noexcept = default;

  /**
   * @brief Constructor with custom page size
   * @param page_size Size in bytes for new arenas (must be > 0)
   */
  explicit ts_thread_local_allocator(std::size_t page_size) noexcept : default_page_size_{page_size} {}

  /**
   * @brief Move constructor
   * @param other Source allocator (will be reset)
   */
  ts_thread_local_allocator(ts_thread_local_allocator&& other) noexcept
      : default_page_size_{std::exchange(other.default_page_size_, default_page_size)},
        page_list_head_{std::exchange(other.page_list_head_, nullptr)},
        page_list_tail_{std::exchange(other.page_list_tail_, nullptr)},
        available_pages_(std::exchange(other.available_pages_, nullptr)),
        pages_to_free_{std::exchange(other.pages_to_free_, nullptr)}
  {}

  /**
   * @brief Move assignment operator
   * @param other Source allocator (will be reset)
   * @return Reference to this allocator
   */
  auto operator=(ts_thread_local_allocator&& other) noexcept -> ts_thread_local_allocator&
  {
    if (this == &other)
    {
      return *this; // self-assignment, nothing to do
    }
    reset(); // free any existing arenas
    default_page_size_ = std::exchange(other.default_page_size_, default_page_size);
    page_list_head_    = std::exchange(other.page_list_head_, nullptr);
    page_list_tail_    = std::exchange(other.page_list_tail_, nullptr);
    pages_to_free_     = std::exchange(other.pages_to_free_, nullptr);
    available_pages_   = std::exchange(other.available_pages_, nullptr);
    return *this;
  }

  /** @brief Copy constructor is deleted (not copyable) */
  ts_thread_local_allocator(const ts_thread_local_allocator&) = delete;

  /** @brief Copy assignment is deleted (not copyable) */
  auto operator=(const ts_thread_local_allocator&) -> ts_thread_local_allocator& = delete;

  /**
   * @brief Destructor
   * Automatically releases all allocated memory
   */
  ~ts_thread_local_allocator() noexcept
  {
    release(); // free everything that might still be hanging around
  }

  /**
   * @brief Fast-path allocation using bump-pointer on the calling thread's arena
   * @param size Number of bytes to allocate
   * @return Pointer to allocated memory aligned to alignof(std::max_align_t)
   * @note Returns nullptr on allocation failure
   * @note Thread-safe with zero synchronization in the fast path
   */
  auto allocate(std::size_t size) -> void*;

  /**
   * @brief Optional stack-style deallocation for the most recent allocation
   * @param ptr Pointer to memory to deallocate
   * @param size Size of the allocation in bytes
   * @return true if deallocation succeeded, false otherwise
   * @note Only works if ptr was the most recent allocation on this thread's arena
   * @note Safe even with concurrent access from other threads
   * @note Thread-safe - each thread owns its arena
   */
  auto deallocate(void* ptr, std::size_t size) const -> bool;

  /**
   * @brief Reset all arenas for reuse (end-of-frame cleanup)
   * @warning Must be called from a single thread with no concurrent allocate() calls
   * @warning All previously allocated memory becomes invalid after this call
   * @note Advances generation counter to invalidate stale thread-local pages
   * @note Recycles arenas to avoid future allocations
   */
  void reset() noexcept;

  /**
   * @brief Release all memory and reset allocator to initial state
   * @warning Must be called from a single thread with no concurrent allocate() calls
   * @warning All previously allocated memory becomes invalid after this call
   * @note Calls reset() then frees all recycled arenas
   */
  void release() noexcept;

  struct tls_t;

private:
  /**
   * @brief Internal arena structure for memory management
   *
   * Each arena consists of this header followed by aligned data storage.
   * The flexible array member data_ contains the actual allocation space.
   */
  struct arena_t
  {
    std::size_t used_ = 0;       ///< Current bump offset (owned by one thread)
    std::size_t size_ = 0;       ///< Total bytes available in data_[]
    arena_t*    next_ = nullptr; ///< Intrusive list pointer (for free list)

    alignas(std::max_align_t) std::byte data_[1] = {}; ///< Flexible array member for allocations
  };

  /**
   * @brief Create a new arena with specified payload size
   * @param payload_size Size of the data portion in bytes
   * @return Pointer to newly created arena
   */
  static auto create_page(std::size_t payload_size) -> arena_t*;

  /**
   * @brief Try to reuse an arena from the free list
   * @param min_payload Minimum required payload size
   * @return Pointer to reused arena or nullptr if none available
   */
  auto pop_free_list(std::size_t min_payload) -> arena_t*;

  /**
   * @brief Slow path allocation when fast path fails
   * @param size Aligned size to allocate
   * @return Pointer to allocated memory
   */
  auto allocate_slow_path(std::size_t size) -> void*;

  /**
   * @brief Thread-safe removal of TLS slot during thread destruction
   * @param slot TLS slot to remove
   */
  static auto remove_tls_slot(tls_t* slot) noexcept -> void;

  static auto generate_random_id(void* ptr) -> uintptr_t;
  /* ---------- Data members -------------------------------------------- */

  /** @brief Default size for new arenas */
  std::size_t default_page_size_ = default_page_size;

  /** @brief Mutex protecting shared data structures */
  std::mutex page_mutex_;

  /** @brief Head of the arena list (LIFO) */
  arena_t* page_list_head_ = nullptr;

  /** @brief Tail of the arena list (for fast allocation) */
  arena_t* page_list_tail_ = nullptr;

  /** @brief Global pool of recyclable arenas */
  arena_t* available_pages_ = nullptr;

  /** @brief Arenas scheduled for deletion by reset() */
  arena_t* pages_to_free_ = nullptr;

  /** @brief Monotonically-increasing frame ID for generation tracking */
  uintptr_t generation_ = {generate_random_id(this)};
};

} // namespace ouly