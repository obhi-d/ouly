// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/detail/custom_allocator.hpp"
#include "ouly/allocators/detail/memory_stats.hpp"
#include "ouly/allocators/detail/pool_defs.hpp"
#include "ouly/utility/detail/concepts.hpp"

namespace ouly
{

struct object_pool_tag
{};

template <typename T, typename Config = ouly::config<>>
class object_pool
    : ouly::detail::statistics<object_pool_tag, ouly::config<Config, cfg::base_stats<ouly::detail::padding_stats>>>
{
  static_assert(sizeof(T) >= sizeof(void*), "Object type T must be at least void* size for intrusive free list");
  static_assert(alignof(T) >= alignof(void*), "Object type T must have alignment compatible with void*");

public:
  using tag = object_pool_tag;
  using statistics =
   ouly::detail::statistics<object_pool_tag, ouly::config<Config, cfg::base_stats<ouly::detail::padding_stats>>>;
  static constexpr auto atom_size = sizeof(T);
  static constexpr auto pool_size = ouly::detail::pool_size<Config>::value;
  using underlying_allocator      = ouly::detail::underlying_allocator_t<Config>;
  using size_type                 = typename underlying_allocator::size_type;
  using address                   = typename underlying_allocator::address;

  object_pool() noexcept = default;

  object_pool(object_pool const&) = delete;
  object_pool(object_pool&& other) noexcept
      : statistics(std::move(other)), free_list_(other.free_list_), pages_(other.pages_)
  {
    other.free_list_ = nullptr;
    other.pages_     = nullptr;
  }

  auto operator=(object_pool const&) -> object_pool& = delete;
  auto operator=(object_pool&& other) noexcept -> object_pool&
  {
    if (this != &other)
    {
      // Clean up current state
      deallocate_all_pages();

      // Move from other
      static_cast<statistics&>(*this) = std::move(other);
      free_list_                      = other.free_list_;
      pages_                          = other.pages_;

      // Reset other
      other.free_list_ = nullptr;
      other.pages_     = nullptr;
    }
    return *this;
  }

  ~object_pool() noexcept
  {
    deallocate_all_pages();
  }

  constexpr static auto null() -> address
  {
    return underlying_allocator::null();
  }

  /// Allocate a single object from the pool
  [[nodiscard]] auto allocate() -> T*
  {
    [[maybe_unused]] auto measure = statistics::report_allocate(atom_size);

    if (free_list_ == nullptr) [[unlikely]]
    {
      allocate_new_page();
    }

    auto* result = static_cast<T*>(free_list_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    free_list_ = *reinterpret_cast<void**>(free_list_);
    return result;
  }

  /// Deallocate a single object back to the pool
  void deallocate(T* ptr)
  {
    if (ptr == nullptr) [[unlikely]]
    {
      return;
    }

    [[maybe_unused]] auto measure = statistics::report_deallocate(atom_size);

    // Add to free list - store next pointer in the object itself
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    *reinterpret_cast<void**>(ptr) = free_list_;
    free_list_                     = static_cast<void*>(ptr);
  }

  /// Check if the pool is empty (no free objects)
  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return free_list_ == nullptr;
  }

private:
  struct page_header
  {
    page_header* next_;
    size_type    page_size_;
    // Objects follow immediately after this header
  };

  void allocate_new_page()
  {
    constexpr size_type objects_per_page = pool_size;
    constexpr size_type header_size      = sizeof(page_header);
    constexpr size_type object_alignment = alignof(T);
    constexpr size_type object_stride    = (sizeof(T) + object_alignment - 1) & ~(object_alignment - 1);
    constexpr size_type page_data_size   = objects_per_page * object_stride;

    // Calculate total page size with extra space for alignment padding
    constexpr size_type total_page_size = header_size + object_alignment + page_data_size;

    // Allocate page with proper alignment for the entire page
    auto* raw_page = static_cast<std::uint8_t*>(underlying_allocator::allocate(total_page_size));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* header = reinterpret_cast<page_header*>(raw_page);

    // Calculate the aligned start position for objects
    auto* unaligned_start = raw_page + header_size;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto unaligned_addr = reinterpret_cast<std::uintptr_t>(unaligned_start);
    auto aligned_addr   = (unaligned_addr + object_alignment - 1) & ~(object_alignment - 1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* objects_start = reinterpret_cast<std::uint8_t*>(aligned_addr);

    // Initialize header
    header->next_      = pages_;
    header->page_size_ = total_page_size;
    pages_             = header;

    // Initialize free list with all objects in this page
    // Link objects together in reverse order for cache efficiency
    for (size_type i = 0; i < objects_per_page; ++i)
    {
      auto* obj_ptr = objects_start + (i * object_stride);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      *reinterpret_cast<void**>(obj_ptr) = free_list_;
      free_list_                         = obj_ptr;
    }

    statistics::report_new_arena();
  }

  void deallocate_all_pages() noexcept
  {
    page_header* current = pages_;
    while (current)
    {
      page_header* next = current->next_;
      underlying_allocator::deallocate(current, current->page_size_);
      current = next;
    }
    pages_     = nullptr;
    free_list_ = nullptr;
  }

  void*        free_list_ = nullptr; ///< Head of intrusive free list
  page_header* pages_     = nullptr; ///< Linked list of allocated pages
};
} // namespace ouly
