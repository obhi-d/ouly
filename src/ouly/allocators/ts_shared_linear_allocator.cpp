
#include "ouly/allocators/ts_shared_linear_allocator.hpp"
#include "ouly/allocators/config.hpp"
#include "ouly/utility/common.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>

namespace ouly
{
auto ts_shared_linear_allocator::allocate(std::size_t size) noexcept -> void*
{
  // Skip alignment if already aligned (common case for POD types)
  size = is_aligned(size) ? size : align_up(size);

  // Fast path – try the current arena_t without taking the global lock.
  arena_t* arena = current_page_.load(std::memory_order_acquire);
  if (arena != nullptr) [[likely]]
  {
    // Try to allocate from the current arena_t.
    // If it fails, we will fall back to the slow path.
    void* ret = try_allocate_from_page(arena, size);
    if (ret != nullptr) [[likely]]
    {
      return ret;
    }
  }

  // Not enough room – slow path: create/steal a fresh arena_t.
  return allocate_slow_path(size, 1);
}

auto ts_shared_linear_allocator::allocate(std::size_t size, std::align_val_t align_val) noexcept -> void*
{
  auto const align = static_cast<std::size_t>(align_val);
  OULY_ASSERT(align == 0 || ouly::is_power_of_two(align));

  // Every offset handed out is a multiple of `alignment`, and arena data starts on that boundary,
  // so any request up to it is already satisfied without reserving a single extra byte
  if (align <= alignment)
  {
    return allocate(size);
  }

  size = reserved_size(size);

  arena_t* arena = current_page_.load(std::memory_order_acquire);
  if (arena != nullptr) [[likely]]
  {
    void* ret = try_allocate_aligned_from_page(arena, size, align);
    if (ret != nullptr) [[likely]]
    {
      return ret;
    }
  }

  return allocate_slow_path(size, align);
}

auto ts_shared_linear_allocator::deallocate(void* ptr, std::size_t size, std::align_val_t /*align_val*/) noexcept
 -> bool
{
  // Padding inserted ahead of the block stays consumed: the head it was measured from is not
  // recorded, so only the block itself can be rolled back
  return deallocate(ptr, size);
}

auto ts_shared_linear_allocator::deallocate(void* ptr, std::size_t size) noexcept -> bool
{
  size = align_up(size);

  arena_t* arena = current_page_.load(std::memory_order_acquire);
  if (arena == nullptr) [[unlikely]]
  {
    return false;
  }

  auto* byte_ptr   = static_cast<std::byte*>(ptr);
  auto* arena_data = &arena->data_[0]; // Cache the data pointer

  // Try to atomically roll back the allocation if it's the most recent one
  for (;;)
  {
    std::size_t current_offset = arena->offset_.load(std::memory_order_relaxed);

    // Check if this is the last allocation (pointer + size == current end)
    if (byte_ptr + size != arena_data + current_offset) [[unlikely]]
    {
      return false; // Not the most recent allocation
    }

    std::size_t new_offset = current_offset - size;

    // Try to atomically roll back the offset
    if (arena->offset_.compare_exchange_weak(current_offset, new_offset, std::memory_order_acq_rel,
                                             std::memory_order_relaxed)) [[likely]]
    {
      return true; // Successfully deallocated
    }
    // CAS failed - another thread modified the offset, retry
  }
}
auto ts_shared_linear_allocator::realloc(void* ptr, std::size_t old_size, std::size_t new_size) noexcept -> void*
{
  if (ptr == nullptr || old_size == 0)
  {
    return allocate(new_size);
  }

  auto const reserved_old = reserved_size(old_size);
  auto const reserved_new = reserved_size(new_size);

  arena_t* arena = current_page_.load(std::memory_order_acquire);
  if (arena != nullptr) [[likely]]
  {
    auto* byte_ptr   = static_cast<std::byte*>(ptr);
    auto* arena_data = &arena->data_[0];

    // Roll the arena head back and forth when this is still the most recent allocation
    for (;;)
    {
      std::size_t current_offset = arena->offset_.load(std::memory_order_relaxed);
      if (byte_ptr + reserved_old != arena_data + current_offset)
      {
        break; // Not the most recent allocation, fall through to allocate and copy
      }

      std::size_t const base       = current_offset - reserved_old;
      std::size_t const new_offset = base + reserved_new;
      if (new_offset > arena->size_)
      {
        break; // arena_t exhausted
      }

      if (arena->offset_.compare_exchange_weak(current_offset, new_offset, std::memory_order_acq_rel,
                                               std::memory_order_relaxed)) [[likely]]
      {
        return ptr;
      }
      // CAS failed - another thread modified the offset, retry
    }
  }

  void* moved = allocate(new_size);
  if (moved != nullptr) [[likely]]
  {
    std::memcpy(moved, ptr, std::min(old_size, new_size));
  }
  return moved;
}

auto ts_shared_linear_allocator::realloc(void* ptr, std::size_t old_size, std::size_t new_size,
                                         std::align_val_t align_val) noexcept -> void*
{
  auto const align = static_cast<std::size_t>(align_val);
  if (align <= alignment)
  {
    return realloc(ptr, old_size, new_size);
  }

  if (ptr == nullptr || old_size == 0)
  {
    return allocate(new_size, align_val);
  }

  auto const reserved_old = reserved_size(old_size);
  auto const reserved_new = reserved_size(new_size);

  arena_t* arena = current_page_.load(std::memory_order_acquire);
  if (arena != nullptr) [[likely]]
  {
    auto* byte_ptr   = static_cast<std::byte*>(ptr);
    auto* arena_data = &arena->data_[0];

    // Growing the block in place keeps its address, and with it the alignment it already has
    for (;;)
    {
      std::size_t current_offset = arena->offset_.load(std::memory_order_relaxed);
      if (byte_ptr + reserved_old != arena_data + current_offset)
      {
        break;
      }

      std::size_t const base       = current_offset - reserved_old;
      std::size_t const new_offset = base + reserved_new;
      if (new_offset > arena->size_)
      {
        break;
      }

      if (arena->offset_.compare_exchange_weak(current_offset, new_offset, std::memory_order_acq_rel,
                                               std::memory_order_relaxed)) [[likely]]
      {
        return ptr;
      }
    }
  }

  void* moved = allocate(new_size, align_val);
  if (moved != nullptr) [[likely]]
  {
    std::memcpy(moved, ptr, std::min(old_size, new_size));
  }
  return moved;
}

void ts_shared_linear_allocator::reset() noexcept
{
  std::unique_lock<std::shared_mutex> lg{page_mutex_};

  // Reset all available pages for reuse
  arena_t* page = available_pages_;
  while (page != nullptr)
  {
    page->offset_.store(0, std::memory_order_relaxed);
    page = page->next_;
  }

  if (page_list_tail_ != nullptr)
  {
    page_list_tail_->next_ = available_pages_;
    available_pages_       = page_list_head_;
  }
  page_list_tail_ = nullptr;
  page_list_head_ = nullptr;

  // Free pages that were marked for deletion
  page = pages_to_free_;
  while (page != nullptr)
  {
    arena_t* next = page->next_;
    ::operator delete(page, std::align_val_t{alignof(arena_t)});
    page = next;
  }
  pages_to_free_ = nullptr;

  current_page_.store(nullptr, std::memory_order_release);
}
void ts_shared_linear_allocator::release() noexcept
{
  reset(); // Reset all arenas and free the global list

  std::unique_lock<std::shared_mutex> lg{page_mutex_};
  arena_t*                            page = available_pages_;
  while (page != nullptr)
  {
    arena_t* next = page->next_;
    ::operator delete(page, std::align_val_t{alignof(arena_t)});
    page = next;
  }
  available_pages_ = nullptr;
}

auto ts_shared_linear_allocator::try_allocate_from_page(arena_t* arena, std::size_t size) noexcept -> void*
{
  // Use a more efficient loop structure
  std::size_t old_off = arena->offset_.load(std::memory_order_relaxed);

  for (;;)
  {
    // Calculate the new offset
    std::size_t new_off = old_off + size;

    if (new_off > arena->size_) [[unlikely]]
    {
      return nullptr; // arena_t exhausted
    }

    // Try to atomically update the offset
    if (arena->offset_.compare_exchange_weak(old_off, new_off, std::memory_order_acq_rel, std::memory_order_relaxed))
     [[likely]]
    {
      void* result = &arena->data_[0] + old_off;

      if constexpr (ouly::cfg::prefetch_next_allocation)
      {
        // Prefetch the next allocation location if there's room
        // This helps with cache locality for subsequent allocations

        // Prefetch the next allocation location if there's room (typical cache line size)
        constexpr std::size_t cache_line_size = 64;
        if (new_off + cache_line_size <= arena->size_) [[likely]]
        {
          prefetch_for_write(&arena->data_[0] + new_off);
        }
      }

      return result; // Successfully allocated
    }
    // CAS failed – somebody else beat us; old_off was updated by compare_exchange_weak
  }
}
auto ts_shared_linear_allocator::try_allocate_aligned_from_page(arena_t* arena, std::size_t size,
                                                                std::size_t align) noexcept -> void*
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto const data = reinterpret_cast<std::uintptr_t>(&arena->data_[0]);

  std::size_t old_off = arena->offset_.load(std::memory_order_relaxed);

  for (;;)
  {
    // Pad only by what this arena's head actually needs, which is nothing once it happens to sit
    // on the requested boundary
    auto const  padding = ouly::detail::align_padding(data + old_off, align);
    std::size_t new_off = old_off + padding + size;

    if (new_off > arena->size_) [[unlikely]]
    {
      return nullptr; // arena_t exhausted
    }

    if (arena->offset_.compare_exchange_weak(old_off, new_off, std::memory_order_acq_rel, std::memory_order_relaxed))
     [[likely]]
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      return &arena->data_[0] + old_off + padding;
    }
    // CAS failed – somebody else beat us; old_off was updated by compare_exchange_weak
  }
}

auto ts_shared_linear_allocator::allocate_slow_path(std::size_t size, std::size_t align) noexcept -> void*
{
  std::unique_lock<std::shared_mutex> lg{page_mutex_};

  auto const try_page = [size, align](arena_t* page) noexcept -> void*
  {
    return align > alignment ? try_allocate_aligned_from_page(page, size, align) : try_allocate_from_page(page, size);
  };

  // Re‑check: another thread could already have installed a fresh arena_t
  // while we were waiting for the lock.
  arena_t* arena = current_page_.load(std::memory_order_acquire);
  if (arena != nullptr) [[likely]]
  {
    void* ret = try_page(arena);
    if (ret != nullptr) [[likely]]
    {
      return ret; // Fast path succeeded
    }
  }

  // A fresh arena is only padded when the request is over-aligned with respect to the arena data
  // boundary, so reserve for that case alone
  std::size_t const reserved = size + (align > alignment ? align - 1 : 0);

  // Need a brand‑new arena_t, large enough for this request.
  std::size_t page_size = std::max(default_page_size_, reserved);

  if (page_size <= default_page_size_)
  {
    if (available_pages_ != nullptr && available_pages_->size_ >= reserved)
    {
      // Reuse an available page only when it is large enough for this request
      arena            = available_pages_;
      available_pages_ = arena->next_;
      arena->offset_.store(0, std::memory_order_relaxed); // Reset the offset
    }
    else
    {
      arena = static_cast<arena_t*>(::operator new(sizeof(arena_t) + page_size, std::align_val_t{alignof(arena_t)}));
      arena->size_ = page_size;
      arena->offset_.store(0, std::memory_order_relaxed);
    }

    arena->next_    = page_list_head_;
    page_list_head_ = arena;
    if (page_list_tail_ == nullptr) [[unlikely]]
    {
      page_list_tail_ = arena; // First page, set tail
    }
    current_page_.store(arena, std::memory_order_release);
  }
  else
  {
    // This is a single large page, not reused
    arena = static_cast<arena_t*>(::operator new(sizeof(arena_t) + page_size, std::align_val_t{alignof(arena_t)}));
    arena->size_ = page_size;
    arena->offset_.store(0, std::memory_order_relaxed);
    arena->next_   = pages_to_free_;
    pages_to_free_ = arena; // Add to the free list for reset
  }

  return try_page(arena);
}
} // namespace ouly
