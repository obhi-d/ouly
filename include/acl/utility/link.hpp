#pragma once
#include "type_traits.hpp"
#include <acl/detail/utils.hpp>
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
  static constexpr size_type null_v = 0;
  static constexpr size_type mask_v = std::numeric_limits<size_type>::max() >> N;

  constexpr link() noexcept              = default;
  constexpr link(const link& i) noexcept = default;
  constexpr explicit link(size_type i) noexcept : offset(i) {}

  template <typename Uy>
  constexpr explicit link(const link<Uy, SizeType>& i) noexcept
  requires std::convertible_to<Uy*, Ty*>
      : offset(i.offset)
  {}

  constexpr link& operator=(const link& i) noexcept = default;

  template <typename Uy>
  constexpr link& operator=(const link<Uy, SizeType>& i) noexcept
  requires std::convertible_to<Uy*, Ty*>
  {
    offset = i.offset;
    return *this;
  }

  constexpr inline size_type as_index() const noexcept
  {
    if constexpr (detail::debug)
    {
      auto id = detail::index_val(offset);
      return id;
    }
    else
      return offset;
  }

  constexpr inline uint8_t revision() const noexcept
  {
    if constexpr (detail::debug)
    {
      auto id = detail::hazard_val(offset);
      return id;
    }
    else
      return 0;
  }

  constexpr inline link revise() const noexcept
  {
    if constexpr (detail::debug)
    {
      return link(detail::revise(offset));
    }
    else
      return link(offset);
  }

  constexpr inline size_type value() const noexcept
  {
    return offset;
  }

  constexpr inline size_type unmasked() const noexcept
  {
    return offset & mask_v;
  }

  constexpr inline size_type get_mask() const noexcept
  {
    return (offset & (~mask_v));
  }

  constexpr inline size_type has_mask(size_type m) const noexcept
  {
    return (offset & m) != 0;
  }

  constexpr inline void mask(size_type m) noexcept
  {
    offset |= (m & ~mask_v);
  }

  constexpr inline void unmask() noexcept
  {
    offset &= mask_v;
  }

  constexpr inline explicit operator size_type() const noexcept
  {
    return offset;
  }

  constexpr inline explicit operator bool() const noexcept
  {
    return offset != null_v;
  }

  constexpr inline auto operator<=>(link const& second) const noexcept
  {
    return as_index() <=> second.as_index();
  }

  constexpr inline friend auto operator<=>(size_type first, link const& second) noexcept
  {
    return first <=> second.as_index();
  }

  constexpr inline friend auto operator<=>(link const& second, size_type first) noexcept
  {
    return second.as_index() <=> first;
  }

  constexpr inline bool operator==(link const& second) const noexcept
  {
    return as_index() == second.as_index();
  }

  constexpr inline friend bool operator==(size_type first, link const& second) noexcept
  {
    return first == second.as_index();
  }

  constexpr inline friend bool operator==(link const& second, size_type first) noexcept
  {
    return second.as_index() == first;
  }

  constexpr inline bool operator!=(link const& second) const noexcept
  {
    return as_index() != second.as_index();
  }

  constexpr inline friend bool operator!=(size_type first, link const& second) noexcept
  {
    return first != second.as_index();
  }

  constexpr inline friend bool operator!=(link const& second, size_type first) noexcept
  {
    return second.as_index() != first;
  }

  size_type offset = null_v;
};

using vlink = link<std::void_t<>, std::size_t, 8>;
} // namespace acl