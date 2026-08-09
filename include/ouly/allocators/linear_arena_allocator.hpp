// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/linear_allocator.hpp"

namespace ouly
{

/**
 * @brief A linear arena allocator that manages memory in contiguous blocks (arenas)
 *
 * This allocator maintains a list of memory arenas and allocates memory linearly within them.
 * When an arena is full, it creates a new one. Memory can only be deallocated in reverse order
 * of allocation within each arena. The allocator supports aligned allocations.
 *
 * @tparam Config Configuration config for the allocator including statistics tracking and
 *                 underlying allocator selection
 *
 * Key features:
 * - Linear allocation within fixed-size arenas
 * - Support for aligned allocations
 * - Memory can be deallocated in LIFO order within arenas
 * - Ability to rewind memory state
 * - Optional statistics tracking
 * - Configurable arena size (default 4MB)
 * - Move constructible but not copy constructible
 *
 * Memory management:
 * - Allocations are made linearly within the current arena
 * - When an arena is full, a new one is created
 * - Minimum allocation size is 64 bytes
 * - Supports zero-initialization of allocated memory
 * - Smart rewind functionality to clean up unused arenas
 *
 * Usage notes:
 * - Best suited for temporary allocations with defined lifetime
 * - Efficient for sequential allocations
 * - Deallocation only works effectively for LIFO order
 * - Memory fragmentation is minimized due to linear allocation pattern
 */
template <typename Config = ouly::config<>>
class linear_arena_allocator : ouly::detail::statistics<linear_arena_allocator_tag, Config>
{
public:
  static constexpr uint32_t default_arena_size = 4 * 1024 * 1024;

  using tag                  = linear_arena_allocator_tag;
  using statistics           = ouly::detail::statistics<linear_arena_allocator_tag, Config>;
  using underlying_allocator = ouly::detail::underlying_allocator_t<Config>;
  using size_type            = underlying_allocator::size_type;
  using address              = underlying_allocator::address;

  static constexpr size_type k_minimum_size = 64;

  linear_arena_allocator() noexcept = default;
  explicit linear_arena_allocator(size_type i_arena_size) : k_arena_size_(i_arena_size) {}

  linear_arena_allocator(linear_arena_allocator const&) = delete;

  linear_arena_allocator(linear_arena_allocator&& other) noexcept
      : arenas_(std::move(other.arenas_)), current_arena_(other.current_arena_), k_arena_size_(other.k_arena_size_)
  {
    other.current_arena_ = 0;
  }

  ~linear_arena_allocator() noexcept
  {
    for (auto& arena_item : arenas_)
    {
      underlying_allocator::deallocate(arena_item.buffer_, arena_item.arena_size_);
    }
  }

  auto operator=(linear_arena_allocator const&) -> linear_arena_allocator& = delete;

  auto operator=(linear_arena_allocator&& other) noexcept -> linear_arena_allocator&
  {
    OULY_ASSERT(k_arena_size_ == other.k_arena_size_);
    arenas_              = std::move(other.arenas_);
    current_arena_       = other.current_arena_;
    other.current_arena_ = 0;
    return *this;
  }

  constexpr static auto null() -> address
  {
    return underlying_allocator::null();
  }

  /**
   * @brief Allocate `i_size` bytes aligned to `i_alignment`
   *
   * Each aligned allocation reserves a small rollback header by default, using its natural
   * alignment padding whenever that gap is large enough. Configure `cfg::disable_rollback` to use a
   * metadata-free bump path when individual deallocation is not needed.
   */
  template <typename Alignment = alignment<>>
  [[nodiscard]] auto allocate(size_type i_size, Alignment i_alignment = {}) -> address
  {

    [[maybe_unused]] auto measure = statistics::report_allocate(i_size);

    auto const align = ouly::detail::alignment_of(i_alignment);

    size_type index = current_arena_;
    for (auto end = static_cast<size_type>(arenas_.size()); index < end; ++index)
    {
      if (auto ret_value = allocate_from(arenas_[index], i_size, align); ret_value != null())
      {
        return ret_value;
      }

      if (arenas_[index].left_over_ < k_minimum_size && index != current_arena_)
      {
        std::swap(arenas_[index], arenas_[current_arena_++]);
      }
    }

    // A fresh arena also needs room for rollback metadata when it is enabled.
    size_type max_arena_size = std::max<size_type>(i_size + worst_case_padding(align), k_arena_size_);
    return allocate_from(arenas_[allocate_new_arena(max_arena_size)], i_size, align);
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
   * When @p i_data is the most recent allocation of the arena it was taken from, the block is grown
   * or shrunk by moving that arena's head and the same address is returned. Otherwise a fresh block
   * is allocated, the contents are copied over and the old block is released.
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

    auto moved = allocate(i_new_size, i_alignment);
    if (moved == null())
    {
      return moved;
    }
    std::memcpy(moved, i_data, std::min(i_old_size, i_new_size));
    deallocate(i_data, i_old_size, i_alignment);
    return moved;
  }

  /**
   * @brief Give a block back to the arena it came from
   *
   * Only the block sitting at the head of its arena is reclaimed. Its rollback header restores the
   * exact previous bump offset, including alignment padding. With `cfg::disable_rollback` this is a
   * no-op.
   */
  template <typename Alignment = alignment<>>
  void deallocate([[maybe_unused]] address i_data, [[maybe_unused]] size_type i_size,
                  [[maybe_unused]] Alignment i_alignment = {})
  {
    if constexpr (ouly::detail::linear_rollback_enabled_v<Config>)
    {
      [[maybe_unused]] auto measure = statistics::report_deallocate(i_size);

      // iterate downwards without underflowing the unsigned index when current_arena_ is 0
      for (auto id = static_cast<size_type>(arenas_.size()); id > current_arena_;)
      {
        --id;
        if (in_range(arenas_[id], i_data))
        {
          auto& item = arenas_[id];
          // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          auto* head = static_cast<std::uint8_t*>(item.buffer_) + (item.arena_size_ - item.left_over_);
          auto* end  = static_cast<std::uint8_t*>(i_data) + i_size;
          // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          if (end == head)
          {
            auto const align = ouly::detail::alignment_of(i_alignment);
            if (align > 1)
            {
              auto const bump = ouly::detail::load_linear_bump<size_type>(i_data);
              OULY_ASSERT(bump <= item.arena_size_);
              item.left_over_ = item.arena_size_ - bump;
            }
            else
            {
              item.left_over_ += i_size;
            }
          }

          break;
        }
      }
    }
  }

  void smart_rewind()
  {
    // delete remaining arenas
    for (size_type index = current_arena_ + 1, end = static_cast<size_type>(arenas_.size()); index < end; ++index)
    {
      underlying_allocator::deallocate(arenas_[index].buffer_, arenas_[index].arena_size_);
    }
    arenas_.resize(current_arena_ + 1);
    current_arena_ = 0;
    for (auto& ar : arenas_)
    {
      ar.reset();
    }
  }

  void rewind()
  {
    current_arena_ = 0;
    for (auto& ar : arenas_)
    {
      ar.reset();
    }
  }

  [[nodiscard]] auto get_arena_count() const -> std::uint32_t
  {
    return static_cast<std::uint32_t>(arenas_.size());
  }

private:
  struct arena
  {
    address   buffer_;
    size_type left_over_;
    size_type arena_size_;
    arena() = default;
    arena(address i_buffer, size_type i_left_over, size_type i_arena_size)
        : buffer_(i_buffer), left_over_(i_left_over), arena_size_(i_arena_size)
    {}

    void reset()
    {
      left_over_ = arena_size_;
    }
  };

  /** @brief Move the head of the owning arena when @p i_data is the block sitting at its top */
  auto resize_in_place(address i_data, size_type i_old_size, size_type i_new_size) -> bool
  {
    for (auto id = static_cast<size_type>(arenas_.size()); id > current_arena_;)
    {
      --id;
      if (!in_range(arenas_[id], i_data))
      {
        continue;
      }

      auto& item = arenas_[id];
      // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      auto* head = static_cast<std::uint8_t*>(item.buffer_) + (item.arena_size_ - item.left_over_);
      auto* end  = static_cast<std::uint8_t*>(i_data) + i_old_size;
      // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      if (end != head)
      {
        return false;
      }

      if (i_new_size <= i_old_size)
      {
        [[maybe_unused]] auto measure = statistics::report_deallocate(i_old_size - i_new_size);
        item.left_over_ += i_old_size - i_new_size;
        return true;
      }

      auto const extra = i_new_size - i_old_size;
      if (extra > item.left_over_)
      {
        return false;
      }

      [[maybe_unused]] auto measure = statistics::report_allocate(extra);
      item.left_over_ -= extra;
      return true;
    }
    return false;
  }

  auto in_range(const arena& i_arena, address i_data) -> bool
  {
    return (i_arena.buffer_ <= i_data &&
            i_data < (static_cast<std::uint8_t*>(i_arena.buffer_) + i_arena.arena_size_)) != 0;
  }

  auto allocate_new_arena(size_type size) -> size_type
  {
    statistics::report_new_arena();

    auto index = static_cast<size_type>(arenas_.size());
    arenas_.emplace_back(underlying_allocator::allocate(size), size, size);
    return index;
  }

  /** @brief Worst-case rollback metadata and alignment padding needed in a fresh arena */
  static constexpr auto worst_case_padding(std::size_t align) -> size_type
  {
    if constexpr (ouly::detail::linear_rollback_enabled_v<Config>)
    {
      return align > 1 ? static_cast<size_type>(sizeof(size_type) + align - 1) : size_type{0};
    }
    else
    {
      constexpr auto guaranteed = ouly::detail::guaranteed_alignment_v<underlying_allocator>;
      return align > guaranteed ? static_cast<size_type>(align - 1) : size_type{0};
    }
  }

  /** @brief Bump the head of `item`, reserving rollback metadata when configured */
  static auto allocate_from(arena& item, size_type size, std::size_t align) -> address
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto const bump    = item.arena_size_ - item.left_over_;
    auto const head    = reinterpret_cast<std::uintptr_t>(item.buffer_) + bump;
    auto const padding = ouly::detail::linear_allocation_padding<Config, size_type>(head, align);
    if (item.left_over_ < size || (item.left_over_ - size) < padding)
    {
      return null();
    }

    item.left_over_ -= (padding + size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto result = reinterpret_cast<address>(head + padding);
    if constexpr (ouly::detail::linear_rollback_enabled_v<Config>)
    {
      if (align > 1)
      {
        ouly::detail::store_linear_bump(result, bump);
      }
    }
    return result;
  }

  ouly::vector<arena> arenas_;
  size_type           current_arena_ = 0;
  size_type           k_arena_size_  = default_arena_size;

public:
};

} // namespace ouly
