#pragma once
#include "config.hpp"
#include <cassert>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>

#pragma once

namespace acl
{

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
  alignas(Align) std::uint8_t data[Len];
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

template <typename size_type>
constexpr size_type invalidated_mask_v = (static_cast<size_type>(0x80) << ((sizeof(size_type) - 1) * 8));

/*
*
* TODO Fix natvis
*
template <typename T1, typename... Args>
struct tuple_array_visualizer_base : tuple_array_visualizer_base<Args...>
{
  using base = tuple_array_visualizer_base<Args...>;
  T1 data;
};

template <typename T1>
struct tuple_array_visualizer_base<T1>
{
  T1 data;
};

template <typename Traits>
concept has_base = requires
{
  typename Traits::base;
};

template <typename T, typename D, bool HasBase>
struct tuple_array_visualizer_base_ref;

template <typename T, typename D>
struct tuple_array_visualizer_base_ref<T, D, true>
{
  using base = tuple_array_visualizer_base_ref<T, typename D::base, has_base<typename D::base>>;
  using type = D;
  T data;
};

template <typename T, typename D>
struct tuple_array_visualizer_base_ref<T, D, false>
{
  using type = D;
  T data;
};

template <typename... Args>
constexpr auto tuple_array_visualizer_ctd(std::tuple<Args...>&&) -> tuple_array_visualizer_base<std::decay_t<Args>*...>;

template <typename Tuple>
using tuple_array_viz_alias = decltype(tuple_array_visualizer_ctd(std::declval<Tuple>()));

template <typename Type, typename Tuple>
using tuple_array_visualizer =
  tuple_array_visualizer_base_ref<Type, tuple_array_viz_alias<Tuple>, has_base<tuple_array_viz_alias<Tuple>>>;

template <typename T>
static auto do_not_optimize(T&& v)
{
  return v;
}
*/

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
  constexpr size_type one  = 1;
  constexpr size_type mask = (one << ((sizeof(size_type) - one) * 8)) - 1;
  if constexpr (detail::debug)
    return val & mask;
  else
    return val;
}

template <typename size_type>
inline size_type revise(size_type val)
{
  constexpr size_type one  = 1;
  constexpr size_type mask = (one << (((sizeof(size_type) - one) * 8) + 1));
  
  if constexpr (detail::debug)
    return val + mask;
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