#pragma once
#include "type_traits.hpp"
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace acl
{

template <typename Ty, typename SizeType = std::uint32_t, std::uint32_t N = 1>
struct link
{
  using size_type                   = SizeType;
  static constexpr size_type null_v = std::numeric_limits<size_type>::max();
  static constexpr size_type mask_v = null_v >> N;

  constexpr link()              = default;
  constexpr link(const link& i) = default;
  constexpr explicit link(size_type i) : offset(i) {}

  template <typename Uy>
  constexpr explicit link(const link<Uy, SizeType>& i) requires std::convertible_to<Uy*, Ty*> : offset(i.offset)
  {}

  constexpr link& operator=(const link& i) = default;

  template <typename Uy>
  constexpr link& operator=(const link<Uy, SizeType>& i) requires std::convertible_to<Uy*, Ty*>
  {
    offset = i.offset;
    return *this;
  }

  constexpr inline size_type value() const
  {
    return offset;
  }

  constexpr inline size_type unmasked() const
  {
    return offset & mask_v;
  }

  constexpr inline size_type get_mask() const
  {
    return (offset & (~mask_v));
  }

  constexpr inline size_type has_mask(size_type m) const
  {
    return (offset & m) != 0;
  }

  constexpr inline void mask(size_type m)
  {
    offset |= (m & ~mask_v);
  }

  constexpr inline void unmask()
  {
    offset &= mask_v;
  }

  constexpr inline explicit operator size_type() const
  {
    return offset;
  }

  constexpr inline explicit operator bool() const
  {
    return offset != null_v;
  }

  constexpr inline auto operator<=>(link const& second) const = default;

  constexpr inline friend auto operator<=>(size_type iFirst, link const& second)
  {
    return iFirst <=> second.offset;
  }

  constexpr inline friend auto operator<=>(link const& second, size_type first)
  {
    return first <=> second.offset;
  }

  size_type offset = null_v;
};

using vlink = link<std::void_t<>, std::uint64_t, 8>;
} // namespace acl