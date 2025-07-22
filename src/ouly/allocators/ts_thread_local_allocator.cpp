
#include "ouly/allocators/ts_thread_local_allocator.hpp"
#include "ouly/allocators/config.hpp"
#include "ouly/utility/common.hpp"

namespace ouly
{
struct ts_thread_local_allocator::tls_t
{
  std::uint32_t                       generation_ = {}; ///< Frame ID when the arena was created
  ts_thread_local_allocator::arena_t* page_       = {}; ///< Pointer to the current arena
};

/* Definition of the thread-local variable */
// NOLINTNEXTLINE
thread_local ts_thread_local_allocator::tls_t local_page = {};
/** Fast-path: bump-pointer allocation on the calling thread’s current arena_t. */
auto ts_thread_local_allocator::allocate(std::size_t size) noexcept -> void*
{
  // Skip alignment if already aligned (common case for POD types)
  size = is_aligned(size) ? size : align_up(size);

  auto page = local_page;
  if (page.page_ != nullptr) [[likely]]
  {
    // Cache the generation locally to avoid repeated atomic loads
    const auto current_generation = generation_.load(std::memory_order_relaxed);
    if (page.generation_ == current_generation) [[likely]]
    {
      auto offset = page.page_->used_;
      if (offset + size <= page.page_->size_) [[likely]]
      {
        void* result      = &page.page_->data_[0] + offset;
        page.page_->used_ = offset + size; // Slightly faster than +=

        if constexpr (ouly::cfg::prefetch_next_allocation)
        {
          // Prefetch the next allocation location if there's room (typical cache line size)
          constexpr std::size_t cache_line_size = 64;
          if (offset + size + cache_line_size <= page.page_->size_) [[likely]]
          {
            prefetch_for_write(&page.page_->data_[0] + offset + size);
          }
        }
        return result; // **zero synchronisation**
      }
    }
  }

  return allocate_slow_path(size);
}
auto ts_thread_local_allocator::deallocate(void* ptr, std::size_t size) noexcept -> bool
{
  size          = align_up(size);
  arena_t* page = local_page.page_;
  if ((page == nullptr) || local_page.generation_ != generation_.load(std::memory_order_relaxed))
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
void ts_thread_local_allocator::reset() noexcept
{
  // 1. Advance the generation counter so any stale thread-local pages
  //    will be considered invalid in the next frame.
  generation_.fetch_add(1, std::memory_order_acq_rel);

  // 2. Reclaim every arena_t in the global free list.
  std::lock_guard<std::mutex> lg{page_mutex_};
  if (page_list_tail_ != nullptr)
  {
    page_list_tail_->next_ = available_pages_;
    available_pages_       = page_list_head_;
  }
  page_list_tail_ = nullptr;
  page_list_head_ = nullptr;

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
    return arena;
  }
  return nullptr;
}
auto ts_thread_local_allocator::allocate_slow_path(std::size_t size) noexcept -> void*
{
  std::size_t payload = std::max(default_page_size_, size);

  if (payload > default_page_size_)
  {
    // If the requested size is larger than the default page size,
    // we allocate a single large page that is not reused.
    auto* arena =
     static_cast<arena_t*>(::operator new(sizeof(arena_t) + payload, std::align_val_t{alignof(std::max_align_t)}));
    arena->size_ = payload;
    arena->used_ = payload;

    std::lock_guard<std::mutex> lg{page_mutex_};

    arena->next_   = pages_to_free_;
    pages_to_free_ = arena; // Add to the free list for reset
    return &arena->data_[0];
  }

  std::lock_guard<std::mutex> lg{page_mutex_};

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
  local_page = {.generation_ = generation_.load(std::memory_order_relaxed), .page_ = page}; // install for this thread

  std::size_t offset = page->used_;
  page->used_ += size;
  return &page->data_[0] + offset;
}
} // namespace ouly