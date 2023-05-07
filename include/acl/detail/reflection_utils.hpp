//
// Created by obhi on 9/18/20.
//

#pragma once
#include "../reflection.hpp"

namespace acl::detail
{
template <typename T>
requires(detail::NativeStringLike<T> || detail::CastableToStringView<T>)
static inline std::string_view as_string(T const& val)
{
  return std::string_view(val);
}

template <typename T>
requires(!detail::NativeStringLike<T> && !detail::CastableToStringView<T> && !detail::TransformToString<T> &&
         detail::ConvertibleToString<T>) static inline std::string as_string(T const& val)
{
  return acl::to_string(val);
}

template <typename T>
requires(!detail::NativeStringLike<T> && !detail::CastableToStringView<T> &&
         (detail::TransformToString<T> || detail::TransformToStringView<T>)) static inline auto as_string(T const& val)
{
  return acl::to_string(val);
}

template <typename C, typename... Args>
requires(detail::HasValueType<C> && detail::HasEmplace<C, container_value_type<C>> &&
         !detail::HasEmplaceBack<C, container_value_type<C>>) static inline void emplace(C& c, Args&&... args)
{
  c.emplace(std::forward<Args>(args)...);
}

template <typename C, typename... Args>
requires(detail::HasValueType<C> && !detail::HasEmplace<C, container_value_type<C>> &&
         detail::HasEmplaceBack<C, container_value_type<C>>) static inline void emplace(C& c, Args&&... args)
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
static constexpr inline auto capacity(C const& c)
{
  return c.capacity();
}

template <typename C>
static constexpr inline uint32_t capacity(C const& c)
{
  return 0;
}

template <detail::HasReserve C, typename SizeType = std::size_t>
static inline void reserve(C& c, SizeType sz)
{
  c.reserve(sz);
}

template <typename C, typename SizeType = std::size_t>
static inline void reserve(C&, SizeType)
{}

template <detail::HasResize C, typename SizeType = std::size_t>
static inline void resize(C& c, SizeType sz)
{
  c.resize(sz);
}
template <typename C, typename SizeType = std::size_t>
static inline void resize(C&, SizeType)
{}
template <detail::HasSize C>
static inline auto size(C const& c)
{
  return c.size();
}

template <typename C>
static inline uint32_t size(C const& c)
{
  return 0;
}

} // namespace acl::detail
