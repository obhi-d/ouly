//
// Created by obhi on 11/17/20.
//
#pragma once
#include <acl/allocators/linear_allocator.hpp>
#include <acl/containers/podvector.hpp>
#include <limits>

namespace acl
{

template <typename Config = acl::config<>>
class linear_stack_allocator : acl::detail::statistics<linear_stack_allocator_tag, Config>
{
public:
  static constexpr uint32_t default_arena_size = 1024 * 1024;

  using tag                  = linear_stack_allocator_tag;
  using statistics           = acl::detail::statistics<linear_stack_allocator_tag, Config>;
  using underlying_allocator = acl::detail::underlying_allocator_t<Config>;
  using size_type            = typename underlying_allocator::size_type;
  using address              = typename underlying_allocator::address;

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
    scoped_rewind(linear_stack_allocator& r) : ref_(&r), marker_(r.get_rewind_point()) {}
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
    for (auto& arena : arenas_)
    {
      underlying_allocator::deallocate(arena.buffer_, arena.arena_size_);
    }
  }

  auto operator=(linear_stack_allocator const&) -> linear_stack_allocator& = delete;

  auto operator=(linear_stack_allocator&& other) noexcept -> linear_stack_allocator&
  {
    assert(k_arena_size_ == other.k_arena_size_);
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

  template <typename Alignment = alignment<>>
  [[nodiscard]] auto allocate(size_type i_size, Alignment i_alignment = {}) -> address
  {

    auto measure = statistics::report_allocate(i_size);
    // assert
    auto fixup = i_alignment - 1;
    // make sure you allocate enough space
    // but keep alignment distance so that next allocations
    // do not suffer from lost alignment
    if (i_alignment)
    {
      i_size += i_alignment;
    }

    address ret_value = null();

    size_type index = current_arena_;
    for (auto end = static_cast<size_type>(arenas_.size()); index < end; ++index)
    {
      if (arenas_[index].left_over_ >= i_size)
      {
        ret_value = allocate_from(index, i_size);
        break;
      }

      current_arena_++;
    }

    if (ret_value == null())
    {
      size_type max_arena_size = std::max<size_type>(i_size, k_arena_size_);
      ret_value                = allocate_from(index = allocate_new_arena(max_arena_size), i_size);
    }

    if (i_alignment)
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto pointer = reinterpret_cast<std::uintptr_t>(ret_value);
      // already aligned
      if ((pointer & fixup) == 0)
      {
        arenas_[index].left_over_ += i_alignment;
        return ret_value;
      }

      auto ret = (pointer + static_cast<std::uintptr_t>(fixup)) & ~static_cast<std::uintptr_t>(fixup);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return reinterpret_cast<address>(ret);
    }

    return ret_value;
  }

  template <typename Alignment = alignment<>>
  [[nodiscard]] auto zero_allocate(size_type i_size, Alignment i_alignment = {}) -> address
  {
    auto z = allocate(i_size, i_alignment);
    std::memset(z, 0, i_size);
    return z;
  }

  template <typename Alignment = alignment<>>
  void deallocate(address i_data, size_type i_size, Alignment i_alignment = {})
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

  auto allocate_from(size_type id, size_type size) -> address
  {
    size_type offset = arenas_[id].arena_size_ - arenas_[id].left_over_;
    arenas_[id].left_over_ -= size;
    return static_cast<std::uint8_t*>(arenas_[id].buffer_) + offset;
  }

  podvector<arena, underlying_allocator> arenas_;
  size_type                              current_arena_ = 0;
  size_type                              k_arena_size_  = default_arena_size;

public:
};

} // namespace acl