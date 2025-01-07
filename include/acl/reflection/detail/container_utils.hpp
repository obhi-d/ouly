
#pragma once

#include <acl/reflection/detail/deduced_types.hpp>

namespace acl::detail
{
template <typename T>
  requires(NativeStringLike<T> || CastableToStringView<T>)
static inline auto as_string(T const& val) -> std::string_view
{
  return std::string_view(val);
}

template <typename T>
  requires(!NativeStringLike<T> && !CastableToStringView<T> && !TransformToString<T> && ConvertibleToString<T>)
static inline auto as_string(T const& val) -> std::string
{
  return acl::to_string(val);
}

template <typename T>
  requires(!NativeStringLike<T> && !CastableToStringView<T> && (TransformToString<T> || TransformToStringView<T>))
static inline auto as_string(T const& val)
{
  return acl::to_string(val);
}

template <typename C, typename... Args>
  requires(HasValueType<C> && HasEmplace<C, container_value_type<C>> && !HasEmplaceBack<C, container_value_type<C>>)
static inline void emplace(C& c, Args&&... args)
{
  c.emplace(std::forward<Args>(args)...);
}

template <typename C, typename... Args>
  requires(HasValueType<C> && !HasEmplace<C, container_value_type<C>> && HasEmplaceBack<C, container_value_type<C>>)
static inline void emplace(C& c, Args&&... args)
{
  c.emplace_back(std::forward<Args>(args)...);
}

template <typename C, typename... Args>
  requires(HasValueType<C> && !HasEmplace<C, container_value_type<C>> && !HasEmplaceBack<C, container_value_type<C>> &&
           HasPushBack<C, container_value_type<C>>)
static inline void emplace(C& c, Args&&... args)
{
  c.push_back(std::forward<Args>(args)...);
}

template <HasCapacity C>
static constexpr auto capacity(C const& c)
{
  return c.capacity();
}

template <typename C>
static constexpr auto capacity(C const& c) -> uint32_t
{
  return 0;
}

template <HasReserve C, typename SizeType = std::size_t>
static inline void reserve(C& c, SizeType sz)
{
  c.reserve(sz);
}

template <typename C, typename SizeType = std::size_t>
static inline void reserve(C& /*unused*/, SizeType /*unused*/)
{}

template <HasResize C, typename SizeType = std::size_t>
static inline void resize(C& c, SizeType sz)
{
  c.resize(sz);
}
template <typename C, typename SizeType = std::size_t>
static inline void resize(C& /*unused*/, SizeType /*unused*/)
{}
template <HasSize C>
static inline auto size(C const& c)
{
  return c.size();
}

template <typename C>
static inline auto size(C const& c) -> uint32_t
{
  return 0;
}
} // namespace acl::detail