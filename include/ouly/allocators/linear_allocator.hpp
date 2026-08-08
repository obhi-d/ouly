// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/default_allocator.hpp"
#include "ouly/allocators/detail/custom_allocator.hpp"
#include "ouly/utility/common.hpp"
#include <algorithm>
#include <cstring>

namespace ouly
{

/**
 * @brief A linear (arena) allocator that allocates memory in a sequential manner
 *
 * The linear allocator manages a contiguous block of memory and allocates by simply
 * incrementing a pointer. It only supports deallocation of the most recently allocated
 * block (LIFO order).
 *
 * @tparam Config Configuration config for the allocator
 *
 * Features:
 * - Fast allocation (O(1))
 * - Supports aligned allocations
 * - Memory is only freed when the allocator is destroyed
 * - Supports LIFO (Last-In-First-Out) deallocation
 * - Move constructible/assignable but not copy constructible/assignable
 *
 * Example usage:
 * @code
 * linear_allocator alloc(1024); // Creates 1KB arena
 * void* ptr = alloc.allocate(128); // Allocates 128 bytes
 * alloc.deallocate(ptr, 128); // Deallocates if it was the last allocation
 * @endcode
 *
 * @note This allocator is particularly useful for situations where memory is allocated
 * in a specific order and freed all at once, or when memory fragmentation needs to be avoided.
 *
 * @warning Deallocating memory out of order will not reclaim the memory until the allocator
 * is destroyed. Only the most recently allocated block can be effectively deallocated.
 */
template <typename Config = ouly::config<>>
class linear_allocator : ouly::detail::statistics<linear_allocator_tag, Config>
{
public:
  using tag                  = linear_allocator_tag;
  using statistics           = ouly::detail::statistics<linear_allocator_tag, Config>;
  using underlying_allocator = ouly::detail::underlying_allocator_t<Config>;
  using size_type            = underlying_allocator::size_type;
  using address              = underlying_allocator::address;

  template <typename... Args>
  linear_allocator(size_type i_arena_size, [[maybe_unused]] Args... args) noexcept
      : left_over_(i_arena_size), k_arena_size_(i_arena_size)
  {
    statistics::report_new_arena();
    buffer_ = underlying_allocator::allocate(k_arena_size_, {});
  }

  linear_allocator(linear_allocator const&) = delete;

  linear_allocator(linear_allocator&& other) noexcept
      : buffer_(other.buffer_), left_over_(other.left_over_), k_arena_size_(other.k_arena_size_)
  {
    other.buffer_    = nullptr;
    other.left_over_ = 0;
  }

  ~linear_allocator() noexcept
  {
    underlying_allocator::deallocate(buffer_, k_arena_size_);
  }

  auto operator=(linear_allocator const&) -> linear_allocator& = delete;

  auto operator=(linear_allocator&& other) noexcept -> linear_allocator&
  {
    if (this == &other)
    {
      return *this;
    }
    OULY_ASSERT(k_arena_size_ == other.k_arena_size_);
    // the arena we are holding on to would be leaked otherwise
    underlying_allocator::deallocate(buffer_, k_arena_size_);
    buffer_          = other.buffer_;
    left_over_       = other.left_over_;
    other.buffer_    = nullptr;
    other.left_over_ = 0;
    return *this;
  }

  constexpr static auto null() -> address
  {
    return underlying_allocator::null();
  }

  /**
   * @brief Allocate `i_size` bytes aligned to `i_alignment`
   *
   * Only the padding the arena head actually needs is consumed: when the head already satisfies the
   * requested alignment - which is always the case while the arena base is at least as aligned as
   * the request and every previous allocation kept the head aligned - the allocation costs exactly
   * `i_size` bytes.
   *
   * @return The aligned address, or `null()` when the arena cannot fit the request
   */
  template <typename Alignment = alignment<>>
  [[nodiscard]] auto allocate(size_type i_size, Alignment i_alignment = {}) -> address
  {
    [[maybe_unused]] auto measure = statistics::report_allocate(i_size);

    auto const align = ouly::detail::alignment_of(i_alignment);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto const head    = reinterpret_cast<std::uintptr_t>(buffer_) + (k_arena_size_ - left_over_);
    auto const padding = static_cast<size_type>(ouly::detail::align_padding(head, align));

    OULY_ASSERT(left_over_ >= i_size && (left_over_ - i_size) >= padding);
    if (left_over_ < i_size || (left_over_ - i_size) < padding)
    {
      return null();
    }

    left_over_ -= (padding + i_size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    return reinterpret_cast<address>(head + padding);
  }

  template <typename Alignment = alignment<>>
  [[nodiscard]] auto zero_allocate(size_type i_size, Alignment i_alignment = {}) -> address
  {
    auto z = allocate(i_size, i_alignment);
    if (z != null())
    {
      std::memset(z, 0, i_size);
    }
    return z;
  }

  /**
   * @brief Resize an existing allocation, growing it in place whenever possible
   *
   * When @p i_data is the most recent allocation of the arena the block is grown or shrunk by moving
   * the arena head, and the same address is returned. Otherwise a fresh block is allocated and the
   * contents are copied over.
   *
   * @param i_data      Address returned by a previous `allocate` call
   * @param i_old_size  Size the block was allocated with
   * @param i_new_size  Requested new size
   * @param i_alignment Alignment the block was allocated with
   * @return Address of the resized block, which may differ from @p i_data
   */
  template <typename Alignment = alignment<>>
  [[nodiscard]] auto realloc(address i_data, size_type i_old_size, size_type i_new_size, Alignment i_alignment = {})
   -> address
  {
    if (i_data == nullptr || i_old_size == 0)
    {
      return allocate(i_new_size, i_alignment);
    }

    if (resize_in_place(i_data, i_old_size, i_new_size))
    {
      return i_data;
    }

    // No deallocate: the only block this allocator can reclaim is the one at the head, and that case
    // was already handled above
    auto moved = allocate(i_new_size, i_alignment);
    if (moved != null())
    {
      std::memcpy(moved, i_data, std::min(i_old_size, i_new_size));
    }
    return moved;
  }

  /**
   * @brief Give a block back to the arena
   *
   * Only the block sitting at the head of the arena is reclaimed; anything else is released when the
   * allocator is destroyed. Padding inserted ahead of an aligned block is not reclaimed, since the
   * position of the previous head is not recorded.
   */
  template <typename Alignment = alignment<>>
  void deallocate(address i_data, size_type i_size, [[maybe_unused]] Alignment i_alignment = {})
  {
    [[maybe_unused]] auto measure = statistics::report_deallocate(i_size);

    // merge back?
    size_type new_left_over = left_over_ + i_size;
    size_type offset        = (k_arena_size_ - new_left_over);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (static_cast<std::uint8_t*>(buffer_) + offset == static_cast<std::uint8_t*>(i_data))
    {
      left_over_ = new_left_over;
    }
  }

  auto get_free_size() const -> size_type
  {
    return left_over_;
  }

  auto operator<=>(linear_allocator const&) const = default;

private:
  /** @brief Move the arena head when @p i_data is the block sitting at the top of the arena */
  auto resize_in_place(address i_data, size_type i_old_size, size_type i_new_size) -> bool
  {
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* head = static_cast<std::uint8_t*>(buffer_) + (k_arena_size_ - left_over_);
    auto* end  = static_cast<std::uint8_t*>(i_data) + i_old_size;
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (end != head)
    {
      return false;
    }

    if (i_new_size <= i_old_size)
    {
      [[maybe_unused]] auto measure = statistics::report_deallocate(i_old_size - i_new_size);
      left_over_ += i_old_size - i_new_size;
      return true;
    }

    auto const extra = i_new_size - i_old_size;
    if (extra > left_over_)
    {
      return false;
    }

    [[maybe_unused]] auto measure = statistics::report_allocate(extra);
    left_over_ -= extra;
    return true;
  }

  address   buffer_;
  size_type left_over_;
  size_type k_arena_size_;
};

} // namespace ouly