
#include "ouly/allocators/ts_thread_local_allocator.hpp"
#include "ouly/allocators/config.hpp"
#include "ouly/utility/common.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>

namespace ouly
{
struct ts_thread_local_allocator::tls_t
{
  uintptr_t generation_ = std::numeric_limits<uintptr_t>::max(); ///< Frame ID when the arena was created
  arena_t*  page_       = nullptr;                               ///< Pointer to the current arena_t for this thread
};
/* Definition of the thread-local variable */
// NOLINTNEXTLINE
thread_local ts_thread_local_allocator::tls_t local_page = {};

// Generate a 32 bit random id
auto ts_thread_local_allocator::generate_random_id(void* ptr) -> uintptr_t
{
  static std::atomic<uint32_t> next_id{1}; // Start from 1 to avoid zero ID
  static constexpr uintptr_t   mix = 0x9e3779b97f4a7c15ULL;
  // NOLINTBEGIN
  auto a = reinterpret_cast<std::uintptr_t>(ptr);
  auto b = next_id.fetch_add(1, std::memory_order_relaxed);
  auto x = a ^ (b + mix);
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  // NOLINTEND
  return x;
}

/** Fast-path: bump-pointer allocation on the calling thread’s current arena_t. */
auto ts_thread_local_allocator::allocate(std::size_t size) -> void*
{
  // Skip alignment if already aligned (common case for POD types)
  size = is_aligned(size) ? size : align_up(size);
  // Cache the generation locally to avoid repeated atomic loads
  const auto current_generation = generation_;

  auto     local_tls_index = local_page;
  arena_t* page            = nullptr;
  if (local_tls_index.generation_ == current_generation)
  {
    page = local_tls_index.page_;
  }

  if (page != nullptr)
  {
    auto offset = page->used_;
    if (offset + size <= page->size_) [[likely]]
    {
      void* result = &page->data_[0] + offset;
      page->used_  = offset + size; // Slightly faster than +=

      if constexpr (ouly::cfg::prefetch_next_allocation)
      {
        // Prefetch the next allocation location if there's room (typical cache line size)
        constexpr std::size_t cache_line_size = 64;
        if (offset + size + cache_line_size <= page->size_) [[likely]]
        {
          prefetch_for_write(&page->data_[0] + offset + size);
        }
      }
      return result; // **zero synchronisation**
    }
  }

  return allocate_slow_path(size, 1);
}

auto ts_thread_local_allocator::allocate(std::size_t size, std::align_val_t align_val) -> void*
{
  auto const align = static_cast<std::size_t>(align_val);
  OULY_ASSERT(align == 0 || ouly::is_power_of_two(align));

  // Every offset handed out is a multiple of `alignment`, and arena data starts on that boundary,
  // so any request up to it is already satisfied without reserving a single extra byte
  if (align <= alignment)
  {
    return allocate(size);
  }

  size = is_aligned(size) ? size : align_up(size);

  arena_t* page = (local_page.generation_ == generation_) ? local_page.page_ : nullptr;
  if (page != nullptr)
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto const data    = reinterpret_cast<std::uintptr_t>(&page->data_[0]);
    auto const padding = ouly::detail::align_padding(data + page->used_, align);
    if (page->used_ + padding + size <= page->size_) [[likely]]
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      void* result = &page->data_[0] + page->used_ + padding;
      page->used_ += padding + size;
      return result;
    }
  }

  return allocate_slow_path(size, align);
}

auto ts_thread_local_allocator::deallocate(void* ptr, std::size_t size, std::align_val_t /*align_val*/) const -> bool
{
  // Padding inserted ahead of the block stays consumed: the head it was measured from is not
  // recorded, so only the block itself can be rolled back
  return deallocate(ptr, size);
}

auto ts_thread_local_allocator::deallocate(void* ptr, std::size_t size) const -> bool
{
  size          = align_up(size);
  arena_t* page = local_page.page_;
  if ((page == nullptr) || local_page.generation_ != generation_)
  {
    return false;
  }

  auto* byte_ptr = static_cast<std::byte*>(ptr);
  if (byte_ptr + size == &page->data_[0] + page->used_)
  {
    page->used_ -= size;
    return true;
  }
  return false;
}

auto ts_thread_local_allocator::realloc(void* ptr, std::size_t old_size, std::size_t new_size) -> void*
{
  if (ptr == nullptr || old_size == 0)
  {
    return allocate(new_size);
  }

  auto const reserved_old = is_aligned(old_size) ? old_size : align_up(old_size);
  auto const reserved_new = is_aligned(new_size) ? new_size : align_up(new_size);

  arena_t* page = local_page.page_;
  if (page != nullptr && local_page.generation_ == generation_)
  {
    auto* byte_ptr = static_cast<std::byte*>(ptr);
    if (byte_ptr + reserved_old == &page->data_[0] + page->used_)
    {
      std::size_t const base = page->used_ - reserved_old;
      if (base + reserved_new <= page->size_)
      {
        page->used_ = base + reserved_new;
        return ptr;
      }
    }
  }

  void* moved = allocate(new_size);
  if (moved != nullptr) [[likely]]
  {
    std::memcpy(moved, ptr, std::min(old_size, new_size));
  }
  return moved;
}

auto ts_thread_local_allocator::realloc(void* ptr, std::size_t old_size, std::size_t new_size,
                                        std::align_val_t align_val) -> void*
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

  auto const reserved_old = is_aligned(old_size) ? old_size : align_up(old_size);
  auto const reserved_new = is_aligned(new_size) ? new_size : align_up(new_size);

  arena_t* page = local_page.page_;
  if (page != nullptr && local_page.generation_ == generation_)
  {
    auto* byte_ptr = static_cast<std::byte*>(ptr);
    // Growing the block in place keeps its address, and with it the alignment it already has
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (byte_ptr + reserved_old == &page->data_[0] + page->used_)
    {
      std::size_t const base = page->used_ - reserved_old;
      if (base + reserved_new <= page->size_)
      {
        page->used_ = base + reserved_new;
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

void ts_thread_local_allocator::reset() noexcept
{
  // 1. Advance the generation counter so any stale thread-local pages
  //    will be considered invalid in the next frame.
  generation_++;

  // 2. Reclaim every arena_t in the global free list.
  std::unique_lock<std::shared_mutex> lg{page_mutex_};
  if (page_list_tail_ != nullptr)
  {
    page_list_tail_->next_ = available_pages_;
    available_pages_       = page_list_head_;
  }
  page_list_tail_ = nullptr;
  page_list_head_ = nullptr;

  // 3. Free large allocations that were scheduled for deletion
  auto* page = pages_to_free_;
  while (page != nullptr)
  {
    arena_t* next = page->next_;
    ::operator delete(page, std::align_val_t{alignof(std::max_align_t)});
    page = next;
  }
  pages_to_free_ = nullptr;
}

void ts_thread_local_allocator::release() noexcept
{
  reset();
  arena_t* page = available_pages_;
  while (page != nullptr)
  {
    arena_t* next = page->next_;
    ::operator delete(page, std::align_val_t{alignof(std::max_align_t)});
    page = next;
  }
  available_pages_ = nullptr;
}
auto ts_thread_local_allocator::create_page(std::size_t payload_size) -> arena_t*
{
  std::size_t total = sizeof(arena_t) + payload_size;
  void*       raw   = ::operator new(total, std::align_val_t{alignof(std::max_align_t)});

  auto* page  = static_cast<arena_t*>(raw);
  page->used_ = 0;
  page->size_ = payload_size;
  page->next_ = nullptr;
  return page;
}
auto ts_thread_local_allocator::pop_free_list(std::size_t min_payload) -> arena_t*
{
  if (available_pages_ != nullptr && available_pages_->size_ >= min_payload)
  {
    auto* arena      = available_pages_;
    available_pages_ = arena->next_;
    // A recycled page still carries the bump offset it had when it was handed back; without this
    // the memory reset() was supposed to reclaim would stay unavailable
    arena->used_ = 0;
    return arena;
  }
  return nullptr;
}
auto ts_thread_local_allocator::allocate_slow_path(std::size_t size, std::size_t align) -> void*
{
  // A fresh page is only padded when the request is over-aligned with respect to the page data
  // boundary, so reserve for that case alone
  std::size_t const reserved = size + (align > alignment ? align - 1 : 0);
  std::size_t       payload  = std::max(default_page_size_, reserved);

  if (payload > default_page_size_)
  {
    // If the requested size is larger than the default page size,
    // we allocate a single large page that is not reused.
    auto* arena =
     static_cast<arena_t*>(::operator new(sizeof(arena_t) + payload, std::align_val_t{alignof(std::max_align_t)}));
    arena->size_ = payload;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto const padding = ouly::detail::align_padding(reinterpret_cast<std::uintptr_t>(&arena->data_[0]), align);
    arena->used_       = padding + size; // Only mark what this request needs, not the entire payload

    std::unique_lock<std::shared_mutex> lg{page_mutex_};

    arena->next_   = pages_to_free_;
    pages_to_free_ = arena; // Add to the free list for reset
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &arena->data_[0] + padding;
  }

  std::unique_lock<std::shared_mutex> lg{page_mutex_};

  // 1) Try to steal a recycled arena_t from the global free list
  arena_t* page = pop_free_list(payload);

  // 2) Otherwise allocate a brand new arena_t
  if (page == nullptr)
  {
    page = create_page(payload);
  }

  // 3) Install the page as the current thread’s arena_t
  if (page_list_tail_ == nullptr)
  {
    page_list_tail_ = page; // first page in the list
  }

  page->next_     = page_list_head_; // insert at the head of the list
  page_list_head_ = page;            // head of the list, for fast allocation

  if (local_page.generation_ != generation_)
  {
    local_page.generation_ = generation_;
  }

  local_page.page_ = page;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto const  data    = reinterpret_cast<std::uintptr_t>(&page->data_[0]);
  auto const  padding = ouly::detail::align_padding(data + page->used_, align);
  std::size_t offset  = page->used_ + padding;
  page->used_         = offset + size;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return &page->data_[0] + offset;
}

auto ts_thread_local_allocator::remove_tls_slot(tls_t* slot) noexcept -> void
{
  // Thread-safe removal from the linked list
  // Since we can't easily remove from a singly-linked list atomically,
  // we just clear the slot's data and let reset() handle cleanup
  slot->page_       = nullptr;
  slot->generation_ = std::numeric_limits<uintptr_t>::max();
}
} // namespace ouly