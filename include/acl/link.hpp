#pragma once
#include "type_traits.hpp"
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace acl
{

template <typename Ty, typename SizeType = std::uint32_t>
struct link
{
  using size_type                 = SizeType;
  static constexpr size_type null = std::numeric_limits<size_type>::max();

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

  constexpr inline explicit operator size_type() const
  {
    return offset;
  }

  constexpr inline explicit operator bool() const
  {
    return offset != null;
  }

  constexpr inline auto operator<=>(link const& iSecond) const = default;

  constexpr inline friend auto operator<=>(size_type iFirst, link const& iSecond)
  {
    return iFirst <=> iSecond.offset;
  }

  constexpr inline friend auto operator<=>(link const& iSecond, size_type iFirst)
  {
    return iFirst <=> iSecond.offset;
  }

  size_type offset = null;
};

} // namespace acl