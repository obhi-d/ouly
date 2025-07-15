// ts_shared_linear_allocator.hpp
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace ouly
{

class ts_shared_linear_allocator
{
public:
  static constexpr uint32_t    default_page_size = 1024 * 1024; // 1 MiB
  static constexpr std::size_t alignment         = alignof(std::max_align_t);

  ts_shared_linear_allocator() noexcept = default;

  explicit ts_shared_linear_allocator(std::size_t page_size) noexcept : default_page_size_{page_size} {}

  ts_shared_linear_allocator(ts_shared_linear_allocator&& other) noexcept
      : default_page_size_{std::exchange(other.default_page_size_, default_page_size)},
        current_page_{other.current_page_.exchange(nullptr)},
        page_list_head_{std::exchange(other.page_list_head_, nullptr)},
        page_list_tail_{std::exchange(other.page_list_tail_, nullptr)},
        available_pages_{std::exchange(other.available_pages_, nullptr)}
  {}

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

  ts_shared_linear_allocator(ts_shared_linear_allocator const&)                    = delete;
  auto operator=(ts_shared_linear_allocator const&) -> ts_shared_linear_allocator& = delete;

  ~ts_shared_linear_allocator() noexcept
  {
    release(); //  free anything still hanging around
  }

  /** Allocate @arena size bytes with alignment guranteed to be alignof(max_align_t).  */
  auto allocate(std::size_t size) noexcept -> void*;

  /**
   * Optional stack-style deallocation for the last allocation.
   * Returns true if `ptr` was the most recent allocation on the current arena
   * and could be rolled back; otherwise does nothing and returns false.
   *
   * Thread-safe but may fail due to race conditions with other threads.
   */
  auto deallocate(void* ptr, std::size_t size) noexcept -> bool;

  /** Reset every arena_t.  Must be called from a single thread with no concurrent allocate(). */
  void reset() noexcept;

  /** Release all arenas and reset the allocator to its initial state. */
  void release() noexcept;

private:
  struct arena_t
  {
    std::atomic_size_t offset_;
    std::size_t        size_;
    arena_t*           next_;

    alignas(std::max_align_t) std::byte data_[1];
  };

  static constexpr auto align_up(std::size_t value) noexcept -> std::size_t
  {
    return (value + alignment - 1U) & ~(alignment - 1U);
  }

  /**
   * Attempt to carve out an aligned block from an existing arena_t.
   * Returns true on success and writes the pointer through a hidden out‑param
   * embedded in the return value via `alloc_ptr`.
   */
  static auto try_allocate_from_page(arena_t* arena, std::size_t size) noexcept -> void*;

  /** Called only when the fast path fails. */
  auto allocate_slow_path(std::size_t size) noexcept -> void*;

  // data ­members
  // pages that occupy more than default_page_size_ are not reused
  std::size_t           default_page_size_ = default_page_size;
  std::atomic<arena_t*> current_page_      = nullptr; // current arena_t, or nullptr if none
  arena_t*              page_list_head_    = nullptr; // LIFO list, owns all pages
  arena_t*              page_list_tail_    = nullptr; // tail of the list, for fast allocation
  arena_t*              pages_to_free_     = nullptr; // pages freed by reset
  arena_t*              available_pages_   = nullptr; // saved arena pages
  std::mutex            page_mutex_;
};
} // namespace ouly