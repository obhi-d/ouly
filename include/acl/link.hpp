#pragma once
#include <cstdint>
#include <limits>
#include <type_traits>
#include <concepts>
#include <compare>

namespace acl
{

template <typename Ty, typename SizeType = std::uint32_t>
struct link
{
  static constexpr SizeType null = std::numeric_limits<SizeType>::max();

  constexpr link()              = default;
  constexpr link(const link& i) = default;
  constexpr explicit link(SizeType i) : offset(i) {}

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

  constexpr inline SizeType value() const
  {
    return offset;
  }

  constexpr inline explicit operator SizeType() const
  {
    return offset;
  }

  constexpr inline explicit operator bool() const
  {
    return offset != null;
  }

  constexpr inline auto operator<=>(link const& iSecond) const = default;

  constexpr inline friend auto operator<=>(SizeType iFirst, link const& iSecond)
  {
    return iFirst <=> iSecond.offset;
  }

  constexpr inline friend auto operator<=>(link const& iSecond, SizeType iFirst)
  {
    return iFirst <=> iSecond.offset;
  }

  SizeType offset = null;
};

} // namespace acl