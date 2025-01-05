//
// Created by obhi on 9/18/20.
//

#pragma once
#include <acl/utils/reflection.hpp>
#include <type_traits>

namespace acl::detail
{
template <typename T>
	requires(detail::NativeStringLike<T> || detail::CastableToStringView<T>)
static inline auto as_string(T const& val) -> std::string_view
{
	return std::string_view(val);
}

template <typename T>
	requires(!detail::NativeStringLike<T> && !detail::CastableToStringView<T> && !detail::TransformToString<T> &&
					 detail::ConvertibleToString<T>)
static inline auto as_string(T const& val) -> std::string
{
	return acl::to_string(val);
}

template <typename T>
	requires(!detail::NativeStringLike<T> && !detail::CastableToStringView<T> &&
					 (detail::TransformToString<T> || detail::TransformToStringView<T>))
static inline auto as_string(T const& val)
{
	return acl::to_string(val);
}

template <typename C, typename... Args>
	requires(detail::HasValueType<C> && detail::HasEmplace<C, container_value_type<C>> &&
					 !detail::HasEmplaceBack<C, container_value_type<C>>)
static inline void emplace(C& c, Args&&... args)
{
	c.emplace(std::forward<Args>(args)...);
}

template <typename C, typename... Args>
	requires(detail::HasValueType<C> && !detail::HasEmplace<C, container_value_type<C>> &&
					 detail::HasEmplaceBack<C, container_value_type<C>>)
static inline void emplace(C& c, Args&&... args)
{
	c.emplace_back(std::forward<Args>(args)...);
}

template <typename C, typename... Args>
	requires(detail::HasValueType<C> && !detail::HasEmplace<C, container_value_type<C>> &&
					 !detail::HasEmplaceBack<C, container_value_type<C>> && detail::HasPushBack<C, container_value_type<C>>)
static inline void emplace(C& c, Args&&... args)
{
	c.push_back(std::forward<Args>(args)...);
}

template <detail::HasCapacity C>
static constexpr auto capacity(C const& c)
{
	return c.capacity();
}

template <typename C>
static constexpr auto capacity(C const& c) -> uint32_t
{
	return 0;
}

template <detail::HasReserve C, typename SizeType = std::size_t>
static inline void reserve(C& c, SizeType sz)
{
	c.reserve(sz);
}

template <typename C, typename SizeType = std::size_t>
static inline void reserve(C& /*unused*/, SizeType /*unused*/)
{}

template <detail::HasResize C, typename SizeType = std::size_t>
static inline void resize(C& c, SizeType sz)
{
	c.resize(sz);
}
template <typename C, typename SizeType = std::size_t>
static inline void resize(C& /*unused*/, SizeType /*unused*/)
{}
template <detail::HasSize C>
static inline auto size(C const& c)
{
	return c.size();
}

template <typename C>
static inline auto size(C const& c) -> uint32_t
{
	return 0;
}

template <typename K, typename V, typename Opt>
struct map_value_type
{
	using key_type	 = K;
	using value_type = V;

	using map_key_field_name	 = detail::key_field_name_t<Opt>;
	using map_value_field_name = detail::value_field_name_t<Opt>;

	K key_;
	V value_;

	map_value_type() noexcept = default;
	map_value_type(K k, V v) noexcept : key_(std::move(k)), value_(std::move(v)) {}
	~map_value_type() noexcept = default;

	map_value_type(map_value_type const&) noexcept										= default;
	map_value_type(map_value_type&&) noexcept													= default;
	auto operator=(map_value_type const&) noexcept -> map_value_type& = default;
	auto operator=(map_value_type&&) noexcept -> map_value_type&			= default;

	static auto constexpr reflect() noexcept
	{
		return acl::bind(acl::bind<map_key_field_name::value, &map_value_type::key_>(),
										 acl::bind<map_value_field_name::value, &map_value_type::value_>());
	}
};

template <typename V, typename Opt>
struct string_map_value_type
{
	using key_type						 = std::string_view;
	using value_type					 = V;
	using map_value_field_name = detail::value_field_name_t<Opt>;

	using is_string_map_value_type = std::true_type;
	std::string_view key_;
	V								 value_;

	string_map_value_type() noexcept = default;
	string_map_value_type(std::string_view k, V v) noexcept : key_(k), value_(std::move(v)) {}
	~string_map_value_type() noexcept = default;

	string_map_value_type(string_map_value_type const&) noexcept										= default;
	string_map_value_type(string_map_value_type&&) noexcept													= default;
	auto operator=(string_map_value_type const&) noexcept -> string_map_value_type& = default;
	auto operator=(string_map_value_type&&) noexcept -> string_map_value_type&			= default;

	static auto constexpr reflect() noexcept
	{
		return acl::bind(acl::bind<map_value_field_name::value, &string_map_value_type::value_>());
	}
};

} // namespace acl::detail
