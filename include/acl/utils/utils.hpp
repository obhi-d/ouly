#pragma once
#include "config.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <string_view>
#include <tuple>
#include <utility>

#pragma once

namespace acl
{

template <class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct endl_type
{};

static constexpr endl_type endl = {};

template <typename T>
concept DefaultConstructible = std::is_default_constructible_v<T>;

template <typename... Args>
using pack = std::tuple<Args...>;

namespace detail
{
template <typename>
struct is_tuple : std::false_type
{};

template <typename... T>
struct is_tuple<std::tuple<T...>> : std::true_type
{};

template <std::size_t Len, std::size_t Align>
struct aligned_storage
{
  alignas(Align) std::byte data_[Len];

  template <typename T>
  auto as() noexcept -> T*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::launder(reinterpret_cast<T*>(data_));
  }

  template <typename T>
  [[nodiscard]] [[nodiscard]] auto as() const noexcept -> T const*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::launder(reinterpret_cast<T const*>(data_));
  }
};

template <typename... Args>
constexpr auto tuple_element_ptr(std::tuple<Args...>&&) -> std::tuple<std::decay_t<Args>*...>;

template <typename... Args>
constexpr auto tuple_element_cptr(std::tuple<Args...>&&) -> std::tuple<std::decay_t<Args> const*...>;

template <typename... Args>
constexpr auto tuple_element_refs(std::tuple<Args...>&&) -> std::tuple<std::decay_t<Args>&...>;

template <typename... Args>
constexpr auto tuple_element_crefs(std::tuple<Args...>&&) -> std::tuple<std::decay_t<Args> const&...>;

template <typename Tuple>
using tuple_of_ptrs = decltype(tuple_element_ptr(std::declval<Tuple>()));

template <typename Tuple>
using tuple_of_cptrs = decltype(tuple_element_cptr(std::declval<Tuple>()));

template <typename Tuple>
using tuple_of_refs = decltype(tuple_element_refs(std::declval<Tuple>()));

template <typename Tuple>
using tuple_of_crefs = decltype(tuple_element_crefs(std::declval<Tuple>()));

template <typename SizeType>
constexpr SizeType high_bit_mask_v = (static_cast<SizeType>(0x80) << ((sizeof(SizeType) - 1) * 8));

template <typename SizeType>
constexpr auto log2(SizeType val) -> SizeType
{
  return val ? 1 + log2(val >> 1) : -1;
}

template <typename SizeType>
constexpr auto hazard_idx(SizeType val, std::uint8_t spl) -> SizeType
{
  if constexpr (detail::debug)
  {
    constexpr SizeType bit_count = 8;
    assert(val < (static_cast<SizeType>(1) << ((sizeof(SizeType) - 1) * 8)));
    return (static_cast<SizeType>(spl) << ((sizeof(SizeType) - 1) * bit_count)) | val;
  }
  else
  {
    return val;
  }
}

template <typename SizeType>
constexpr auto hazard_val(SizeType val) -> std::uint8_t
{
  if constexpr (detail::debug)
  {
    constexpr SizeType bit_count = 8;
    return (val >> ((sizeof(SizeType) - 1) * bit_count));
  }
  else
  {
    return val;
  }
}

template <typename SizeType>
constexpr auto index_val(SizeType val)
{
  constexpr SizeType bit_count = 8;
  constexpr SizeType one       = 1;
  constexpr SizeType mask      = (one << ((sizeof(SizeType) - one) * bit_count)) - 1;
  if constexpr (detail::debug)
  {
    return val & mask;
  }
  else
  {
    return val;
  }
}

template <typename SizeType>
constexpr auto revise(SizeType val) -> SizeType
{
  if constexpr (detail::debug)
  {
    return hazard_idx(index_val(val), (hazard_val(val) + 1));
  }
  else
  {
    return val;
  }
}

template <typename SizeType>
constexpr auto invalidate(SizeType val) -> SizeType
{
  return high_bit_mask_v<SizeType> | val;
}

template <typename SizeType>
constexpr auto validate(SizeType val) -> SizeType
{
  return (~high_bit_mask_v<SizeType>)&(val);
}

template <typename SizeType>
constexpr auto revise_invalidate(SizeType val) -> SizeType
{
  if constexpr (detail::debug)
  {
    constexpr SizeType high_bit = 0x80;
    return (hazard_idx(index_val(val), (hazard_val(val) + 1) | high_bit));
  }
  else
  {
    return invalidate(val);
  }
}

template <typename SizeType>
constexpr auto is_valid(SizeType val) -> SizeType
{
  return !(high_bit_mask_v<SizeType> & val);
}

constexpr auto fnv1a_32(std::string_view view) -> uint32_t
{
  constexpr uint32_t prime        = 16777619U;
  constexpr uint32_t offset_basis = 2166136261U;
  uint32_t           hash         = offset_basis;
  for (auto v : view)
  {
    hash ^= static_cast<uint32_t>(v);
    hash *= prime;
  }
  return hash;
}

template <typename T>
inline void move(T& dest, T& src)
{
  dest = std::move(src);
}

} // namespace detail
} // namespace acl
