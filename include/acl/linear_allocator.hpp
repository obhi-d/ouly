#pragma once

#include "default_allocator.hpp"
#include "detail/common.hpp"

namespace acl
{

struct linear_allocator_tag
{};

template <typename underlying_allocator = acl::default_allocator<>, bool k_compute_stats = false>
class linear_allocator : detail::statistics<linear_allocator_tag, k_compute_stats, underlying_allocator>
{
public:
  using tag        = linear_allocator_tag;
  using statistics = detail::statistics<linear_allocator_tag, k_compute_stats, underlying_allocator>;
  using size_type  = typename underlying_allocator::size_type;
  using address    = typename underlying_allocator::address;

  template <typename... Args>
  linear_allocator(size_type i_arena_size, Args&&... i_args)
      : k_arena_size(i_arena_size), left_over(i_arena_size), statistics(std::forward<Args>(i_args)...)
  {
    statistics::report_new_arena();
    buffer = underlying_allocator::allocate(k_arena_size, 0);
  }

  ~linear_allocator()
  {
    underlying_allocator::deallocate(buffer, k_arena_size);
  }

  inline constexpr static address null()
  {
    return underlying_allocator::null();
  }

  address allocate(size_type i_size, size_type i_alignment = 0)
  {
    auto measure = statistics::report_allocate(i_size);
    // assert
    auto const fixup = i_alignment - 1;
    // make sure you allocate enough space
    // but keep alignment distance so that next allocations
    // do not suffer from lost alignment
    if (i_alignment)
      i_size += i_alignment;

    assert(left_over >= i_size);
    size_type offset = k_arena_size - left_over;
    left_over -= i_size;
    if (i_alignment)
    {
      auto pointer = reinterpret_cast<std::uintptr_t>(buffer) + static_cast<std::uintptr_t>(offset);
      if ((pointer & fixup) == 0)
      {
        left_over += i_alignment;
        return reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(buffer) + offset);
      }
      else
      {
        auto ret = (pointer + static_cast<std::uintptr_t>(fixup)) & ~static_cast<std::uintptr_t>(fixup);
        return reinterpret_cast<address>(ret);
      }
    }
    else
      return reinterpret_cast<address>(reinterpret_cast<std::uint8_t*>(buffer) + offset);
  }

  address zero_allocate(size_type i_size, size_type i_alignment = 0)
  {
    auto z = allocate(i_size, i_alignment);
    std::memset(z, 0, i_size);
    return z;
  }

  void deallocate(address i_data, size_type i_size, size_type i_alignment = 0)
  {
    auto measure = statistics::report_deallocate(i_size);

    // merge back?
    size_type new_left_over = left_over + i_size;
    size_type offset        = (k_arena_size - new_left_over);
    if (reinterpret_cast<std::uint8_t*>(buffer) + offset == reinterpret_cast<std::uint8_t*>(i_data))
      left_over = new_left_over;
    else if (i_alignment)
    {
      i_size += i_alignment;

      new_left_over = left_over + i_size;
      offset        = (k_arena_size - new_left_over);

      // This memory fixed up by alignment and is within range of alignment
      if ((reinterpret_cast<std::uintptr_t>(i_data) - (reinterpret_cast<std::uintptr_t>(buffer) + offset)) <
          i_alignment)
        left_over = new_left_over;
    }
  }

  size_type get_free_size() const
  {
    return left_over;
  }

  inline auto operator<=>(linear_allocator const&) const = default;

private:
  address         buffer;
  size_type       left_over;
  const size_type k_arena_size;
};

} // namespace acl