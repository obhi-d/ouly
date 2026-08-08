// SPDX-License-Identifier: MIT
//
// Created by obhi on 11/17/20.
//
#pragma once
#include "ouly/allocators/linear_allocator.hpp"
#include <limits>

namespace ouly
{

template <typename Config = ouly::config<>>
class linear_stack_allocator : ouly::detail::statistics<linear_stack_allocator_tag, Config>
{
public:
  static constexpr uint32_t default_arena_size = 1024 * 1024;

  using tag                  = linear_stack_allocator_tag;
  using statistics           = ouly::detail::statistics<linear_stack_allocator_tag, Config>;
  using underlying_allocator = ouly::detail::underlying_allocator_t<Config>;
  using size_type            = underlying_allocator::size_type;
  using address              = underlying_allocator::address;

  struct rewind_point
  {
    size_type arena_;
    size_type left_over_;
  };

  struct scoped_rewind
  {
    scoped_rewind(scoped_rewind const&)                    = delete;
    auto operator=(scoped_rewind const&) -> scoped_rewind& = delete;
    scoped_rewind(scoped_rewind&& mv) noexcept : marker_(mv.marker_), ref_(mv.ref_)
    {
      mv.marker_.arena_ = std::numeric_limits<size_type>::max();
    }
    auto operator=(scoped_rewind&& mv) noexcept -> scoped_rewind&
    {
      marker_           = mv.marker_;
      ref_              = mv.ref_;
      mv.marker_.arena_ = std::numeric_limits<size_type>::max();
      return *this;
    }
    scoped_rewind(linear_stack_allocator& r) : marker_(r.get_rewind_point()), ref_(&r) {}
    ~scoped_rewind()
    {
      if (marker_.arena_ != std::numeric_limits<size_type>::max())
      {
        ref_->rewind(marker_);
      }
    }

    rewind_point            marker_;
    linear_stack_allocator* ref_;
  };

  linear_stack_allocator() noexcept = default;
  explicit linear_stack_allocator(size_type i_arena_size) noexcept : k_arena_size_(i_arena_size) {}

  linear_stack_allocator(linear_stack_allocator const&) = delete;

  linear_stack_allocator(linear_stack_allocator&& other) noexcept
      : arenas_(std::move(other.arenas_)), current_arena_(other.current_arena_), k_arena_size_(other.k_arena_size_)
  {
    other.current_arena_ = 0;
  }

  ~linear_stack_allocator() noexcept
  {
    for (auto& arena_entry : arenas_)
    {
      underlying_allocator::deallocate(arena_entry.buffer_, arena_entry.arena_size_);
    }
  }

  auto operator=(linear_stack_allocator const&) -> linear_stack_allocator& = delete;

  auto operator=(linear_stack_allocator&& other) noexcept -> linear_stack_allocator&
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

  [[nodiscard]] auto get_auto_rewind_point() -> scoped_rewind
  {
    return scoped_rewind(*this);
  }

  [[nodiscard]] auto get_rewind_point() const -> rewind_point
  {
    rewind_point m;
    m.arena_ = current_arena_;
    if (current_arena_ < arenas_.size())
    {
      m.left_over_ = arenas_[current_arena_].left_over_;
    }
    else
    {
      m.left_over_ = std::numeric_limits<size_type>::max();
    }
    return m;
  }

  /**
   * @brief Allocate `i_size` bytes aligned to `i_alignment`
   *
   * Arenas are probed with the padding their own head actually needs, so a head that already
   * satisfies the requested alignment consumes exactly `i_size` bytes.
   */
  template <typename Alignment = alignment<>>
  [[nodiscard]] auto allocate(size_type i_size, Alignment i_alignment = {}) -> address
  {

    [[maybe_unused]] auto measure = statistics::report_allocate(i_size);

    auto const align = ouly::detail::alignment_of(i_alignment);

    for (auto end = static_cast<size_type>(arenas_.size()); current_arena_ < end; ++current_arena_)
    {
      if (auto ret_value = allocate_from(arenas_[current_arena_], i_size, align); ret_value != null())
      {
        return ret_value;
      }
    }

    // A fresh arena is only padded when the request is over-aligned with respect to the underlying
    // allocator, so reserve for that case alone
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
   * is allocated and the contents are copied over; the old block is reclaimed by the next rewind.
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
    std::memcpy(moved, i_data, std::min(i_old_size, i_new_size));
    return moved;
  }

  template <typename Alignment = alignment<>>
  void deallocate(address /*i_data*/, size_type /*i_size*/, Alignment /*i_alignment*/ = {})
  {
    // does not support deallocate, only rewinds are supported
  }

  void smart_rewind()
  {
    // delete remaining arenas_
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

  void rewind(rewind_point marker)
  {
    current_arena_ = marker.arena_;
    if (current_arena_ < arenas_.size())
    {
      arenas_[current_arena_].left_over_ = std::min(marker.left_over_, arenas_[current_arena_].arena_size_);
    }
    auto end = static_cast<size_type>(arenas_.size());
    for (size_type i = marker.arena_ + 1; i < end; ++i)
    {
      arenas_[i].reset();
    }
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

  /** @brief Padding a fresh arena may need, which is none unless the request is over-aligned */
  static constexpr auto worst_case_padding(std::size_t align) -> size_type
  {
    constexpr auto guaranteed = ouly::detail::guaranteed_alignment_v<underlying_allocator>;
    return align > guaranteed ? static_cast<size_type>(align - 1) : size_type{0};
  }

  /** @brief Bump the head of `item`, padding it only by what the alignment actually requires */
  static auto allocate_from(arena& item, size_type size, std::size_t align) -> address
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto const head    = reinterpret_cast<std::uintptr_t>(item.buffer_) + (item.arena_size_ - item.left_over_);
    auto const padding = static_cast<size_type>(ouly::detail::align_padding(head, align));
    if (item.left_over_ < size || (item.left_over_ - size) < padding)
    {
      return null();
    }

    item.left_over_ -= (padding + size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    return reinterpret_cast<address>(head + padding);
  }

  std::vector<arena> arenas_;
  size_type          current_arena_ = 0;
  size_type          k_arena_size_  = default_arena_size;

public:
};

} // namespace ouly