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
	alignas(Align) std::byte data[Len];

	template <typename T>
	T* as() noexcept
	{
		return std::launder(reinterpret_cast<T*>(data));
	}

	template <typename T>
	T const* as() const noexcept
	{
		return std::launder(reinterpret_cast<T const*>(data));
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

template <typename size_type>
constexpr size_type high_bit_mask_v = (static_cast<size_type>(0x80) << ((sizeof(size_type) - 1) * 8));

template <typename size_type>
constexpr size_type log2(size_type val)
{
	return val ? 1 + log2(val >> 1) : -1;
}

template <typename size_type>
inline constexpr size_type hazard_idx(size_type val, std::uint8_t spl)
{
	if constexpr (detail::debug)
	{
		ACL_ASSERT(val < (static_cast<size_type>(1) << ((sizeof(size_type) - 1) * 8)));
		return (static_cast<size_type>(spl) << ((sizeof(size_type) - 1) * 8)) | val;
	}
	else
		return val;
}

template <typename size_type>
inline constexpr std::uint8_t hazard_val(size_type val)
{
	if constexpr (detail::debug)
		return (val >> ((sizeof(size_type) - 1) * 8));
	else
		return val;
}

template <typename size_type>
inline constexpr auto index_val(size_type val)
{
	constexpr size_type one	 = 1;
	constexpr size_type mask = (one << ((sizeof(size_type) - one) * 8)) - 1;
	if constexpr (detail::debug)
		return val & mask;
	else
		return val;
}

template <typename size_type>
inline constexpr size_type revise(size_type val)
{
	if constexpr (detail::debug)
		return hazard_idx(index_val(val), (hazard_val(val) + 1));
	else
		return val;
}

template <typename size_type>
inline constexpr size_type invalidate(size_type val)
{
	return high_bit_mask_v<size_type> | val;
}

template <typename size_type>
inline constexpr size_type validate(size_type val)
{
	return (~high_bit_mask_v<size_type>)&(val);
}

template <typename size_type>
inline constexpr size_type revise_invalidate(size_type val)
{
	if constexpr (detail::debug)
		return (hazard_idx(index_val(val), (hazard_val(val) + 1) | 0x80));
	else
		return invalidate(val);
}

template <typename size_type>
inline constexpr size_type is_valid(size_type val)
{
	return !(high_bit_mask_v<size_type> & val);
}

constexpr uint32_t fnv1a_32(std::string_view view)
{
	constexpr uint32_t prime				= 16777619u;
	constexpr uint32_t offset_basis = 2166136261u;
	uint32_t					 hash					= offset_basis;
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
