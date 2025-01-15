#pragma once

#include <acl/allocators/linear_allocator.hpp>

namespace acl
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
template <typename Config = acl::config<>>
class linear_arena_allocator : acl::detail::statistics<linear_arena_allocator_tag, Config>
{
public:
  static constexpr uint32_t default_arena_size = 4 * 1024 * 1024;

  using tag                  = linear_arena_allocator_tag;
  using statistics           = acl::detail::statistics<linear_arena_allocator_tag, Config>;
  using underlying_allocator = acl::detail::underlying_allocator_t<Config>;
  using size_type            = typename underlying_allocator::size_type;
  using address              = typename underlying_allocator::address;

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
    for (auto& arena : arenas_)
    {
      underlying_allocator::deallocate(arena.buffer_, arena.arena_size_);
    }
  }

  auto operator=(linear_arena_allocator const&) -> linear_arena_allocator& = delete;

  auto operator=(linear_arena_allocator&& other) noexcept -> linear_arena_allocator&
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

      if (arenas_[index].left_over_ < k_minimum_size && index != current_arena_)
      {
        std::swap(arenas_[index], arenas_[current_arena_++]);
      }
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
    auto measure = statistics::report_deallocate(i_size);

    for (size_type id = static_cast<size_type>(arenas_.size()) - 1; id >= current_arena_; --id)
    {
      if (in_range(arenas_[id], i_data))
      {
        // merge back?

        size_type new_left_over = arenas_[id].left_over_ + i_size;
        size_type offset        = (arenas_[id].arena_size_ - new_left_over);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        if (reinterpret_cast<std::uint8_t*>(arenas_[id].buffer_) + offset == reinterpret_cast<std::uint8_t*>(i_data))
        {
          arenas_[id].left_over_ = new_left_over;
        }
        else if (i_alignment)
        {
          i_size += i_alignment;

          new_left_over = arenas_[id].left_over_ + i_size;
          offset        = (arenas_[id].arena_size_ - new_left_over);

          // This memory fixed up by alignment and is within range of alignment
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          if ((reinterpret_cast<std::uintptr_t>(i_data) -
               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
               (reinterpret_cast<std::uintptr_t>(arenas_[id].buffer_) + offset)) < i_alignment)
          {
            arenas_[id].left_over_ = new_left_over;
          }
        }

        break;
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

  acl::vector<arena> arenas_;
  size_type          current_arena_ = 0;
  size_type          k_arena_size_  = default_arena_size;

public:
};

} // namespace acl