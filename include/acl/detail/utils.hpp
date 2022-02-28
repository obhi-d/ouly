#pragma once
#include "config.hpp"
#include <cassert>
#include <cstdint>
#include <limits>
#include <tuple>

#pragma once

namespace acl
{

namespace detail
{
template <std::size_t Len, std::size_t Align>
struct aligned_storage
{
  alignas(Align) std::uint8_t data[Len];
};

template <typename... Args>
using tuple_of = std::tuple<Args...>;

template <typename... Args>
constexpr auto tuple_element_pointer(std::tuple<Args...>&&)
{
  return std::tuple<Args*, ...>{};
}

template <typename size_type>
constexpr size_type invalidated_mask_v = (static_cast<size_type>(0x80) << ((sizeof(size_type) - 1) * 8));

template <typename size_type>
constexpr size_type log2(size_type val)
{
  return val ? 1 + log2(val >> 1) : -1;
}

template <typename size_type>
inline size_type hazard_idx(size_type val, std::uint8_t spl)
{
  if constexpr (detail::debug)
  {
    assert(val < (static_cast<size_type>(1) << ((sizeof(size_type) - 1) * 8)));
    return (static_cast<size_type>(spl) << ((sizeof(size_type) - 1) * 8)) | val;
  }
  else
    return val;
}

template <typename size_type>
inline std::uint8_t hazard_val(size_type val)
{
  if constexpr (detail::debug)
    return (val >> ((sizeof(size_type) - 1) * 8));
  else
    return val;
}

template <typename size_type>
inline auto index_val(size_type val)
{
  if constexpr (detail::debug)
    return val & ((1 << ((sizeof(size_type) - 1) * 8)) - 1);
  else
    return val;
}

template <typename size_type>
inline size_type revise(size_type val)
{
  if constexpr (detail::debug)
    return hazard_idx(index_val(val), (hazard_val(val) + 1) & 0x7f);
  else
    return val;
}

template <typename size_type>
inline size_type invalidate(size_type val)
{
  return invalidated_mask_v<size_type> | val;
}

template <typename size_type>
inline size_type validate(size_type val)
{
  return (~invalidated_mask_v<size_type>)&(val);
}

template <typename size_type>
inline size_type revise_invalidate(size_type val)
{
  if constexpr (detail::debug)
    return hazard_idx(index_val(val), (hazard_val(val) + 1) | 0x80);
  else
    return invalidate(val);
}

template <typename size_type>
inline size_type is_valid(size_type val)
{
  return !(invalidated_mask_v<size_type> & val);
}

} // namespace detail

} // namespace acl