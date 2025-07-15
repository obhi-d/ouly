// ts_thread_local_allocator.hpp
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace ouly
{
class ts_thread_local_allocator
{
public:
  static constexpr uint32_t    default_page_size = 1024 * 1024; // 1 MiB
  static constexpr std::size_t alignment         = alignof(std::max_align_t);

  ts_thread_local_allocator() noexcept = default;
  explicit ts_thread_local_allocator(std::size_t page_size) noexcept : default_page_size_{page_size} {}

  ts_thread_local_allocator(ts_thread_local_allocator&& other) noexcept
      : default_page_size_{std::exchange(other.default_page_size_, default_page_size)},
        available_pages_{std::exchange(other.available_pages_, nullptr)},
        generation_(other.generation_.exchange(0, std::memory_order_relaxed) + 1)
  {}

  auto operator=(ts_thread_local_allocator&& other) noexcept -> ts_thread_local_allocator&
  {
    if (this == &other)
    {
      return *this; // self-assignment, nothing to do
    }
    reset(); // free any existing arenas
    default_page_size_ = std::exchange(other.default_page_size_, default_page_size);
    available_pages_   = std::exchange(other.available_pages_, nullptr);
    generation_.store(other.generation_.exchange(0, std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    return *this;
  }

  ts_thread_local_allocator(const ts_thread_local_allocator&)                    = delete;
  auto operator=(const ts_thread_local_allocator&) -> ts_thread_local_allocator& = delete;

  ~ts_thread_local_allocator() noexcept
  {
    reset(); // free everything that might still be hanging around
  }

  /** Fast-path: bump-pointer allocation on the calling thread’s current arena_t. */
  auto allocate(std::size_t size) noexcept -> void*;

  /**
   * Optional stack-style deallocation.
   * Returns true if `ptr` was at the top of the calling thread’s current arena_t
   * and could be popped; otherwise does nothing and returns false.
   *
   * Safe even if other threads race, because each thread owns its arena_t.
   */
  auto deallocate(void* ptr, std::size_t size) noexcept -> bool;

  /** End-of-frame: must run when no worker threads are calling allocate(). */
  void reset() noexcept;

  void release() noexcept;

private:
  /* ---------- Page layout: header + buffer in ONE allocation ---------- */
  struct arena_t
  {
    std::size_t   used_;       // bump offset (owned by ONE thread)
    std::size_t   size_;       // total bytes in data_[]
    std::uint32_t generation_; // frame id when the arena_t was created
    arena_t*      next_;       // intrusive list (free list)

    alignas(std::max_align_t) std::byte data_[1]; // flexible array member
  };

  /* ---------- helpers -------------------------------------------------- */
  static constexpr auto align_up(std::size_t value) noexcept -> std::size_t
  {
    return (value + alignment - 1U) & ~(alignment - 1U);
  }

  auto create_page(std::size_t payload_size) -> arena_t*;

  auto pop_free_list(std::size_t min_payload) -> arena_t*;

  auto allocate_slow_path(std::size_t size) noexcept -> void*;

  /* ---------- data members -------------------------------------------- */
  std::size_t           default_page_size_ = default_page_size; // default arena_t size
  std::mutex            page_mutex_;
  arena_t*              page_list_head_  = nullptr;
  arena_t*              page_list_tail_  = nullptr; // tail of the list, for fast allocation
  arena_t*              available_pages_ = nullptr; // global pool of recyclable pages
  arena_t*              pages_to_free_   = nullptr; // pages freed by reset
  std::atomic<uint32_t> generation_      = {0};     // monotonically-increasing frame id

  // ***Each thread gets its own “current page” pointer***
  static thread_local arena_t* local_page;
};

} // namespace ouly