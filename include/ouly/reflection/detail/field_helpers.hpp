// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/reflection/detail/aggregate.hpp"
#include "ouly/reflection/detail/deduced_types.hpp"
#include "ouly/reflection/detail/derived_concepts.hpp"

#include <source_location>
#include <type_traits>
#include <utility>

namespace ouly::detail
{

/**
 * @brief This function iterates over members registered by bind
 * @param fn A lambda accepting the instance of the object, a type info and the member index. The type info has a value
 * method to access the internal member given the instance of the object
 */
template <ExplicitlyReflected Class, typename Fn>
void for_each_field(Fn fn, Class& obj)
{
  using ClassType = std::remove_const_t<Class>;
  static_assert(field_count<ClassType> > 0, "Invalid tuple size");
  return [&]<std::size_t... I>(std::index_sequence<I...>, auto tup)
  {
    (fn(obj, std::get<I>(tup), std::integral_constant<std::size_t, I>()), ...);
  }(std::make_index_sequence<field_count<ClassType>>(), reflect<ClassType>());
}

/**
 * @brief This function iterates over members registered by bind without the class object specified
 * @param fn A lambda accepting a type info and the member index. The type info has a value method to access the
 * internal member given the instance of the object
 */
template <ExplicitlyReflected Class, typename Fn>
void for_each_field(Fn fn)
{
  using ClassType = std::remove_const_t<Class>;
  static_assert(field_count<ClassType> > 0, "Invalid tuple size");
  return [&]<std::size_t... I>(std::index_sequence<I...>, auto tup)
  {
    (fn(std::get<I>(tup), std::integral_constant<std::size_t, I>()), ...);
  }(std::make_index_sequence<field_count<ClassType>>(), reflect<ClassType>());
}

template <ExplicitlyReflected Class, std::size_t I>
constexpr auto field_at() noexcept
{
  using ClassType = std::remove_const_t<Class>;
  static_assert(field_count<ClassType> > 0, "Invalid tuple size");
  return std::get<I>(reflect<ClassType>());
}

template <typename T>
struct field_ref
{
  // NOLINTNEXTLINE
  T& member_;
};

template <class T>
field_ref(T&) -> field_ref<T>;

template <typename T, auto A>
[[nodiscard]] consteval auto function_name_member() noexcept -> std::string_view
{
  return std::source_location::current().function_name();
}

template <typename T>
[[nodiscard]] consteval auto function_name_type() noexcept -> std::string_view
{
  return std::source_location::current().function_name();
}

template <typename T, auto const A>
consteval auto deduce_field_name() -> decltype(auto)
{
#if __cpp_lib_source_location >= 201907L
#if defined(_MSC_VER)
  constexpr auto name = std::string_view{std::source_location::current().function_name()};
#elif defined(__clang__)
  constexpr auto name = function_name_member<T, &A.member_>();
#else
  constexpr auto name = function_name_member<T, &A.member_>();
#endif
#elif defined(_MSC_VER)
  constexpr auto name = std::string_view{__FUNCSIG__};
#else
  constexpr auto name = std::string_view{__PRETTY_FUNCTION__};
#endif
#if defined(__clang__)
  constexpr auto beg_mem     = name.substr(name.find("A ="));
  constexpr auto end_mem     = beg_mem.substr(0, beg_mem.find_first_of(']'));
  constexpr auto member_name = end_mem.substr(end_mem.find_last_of('.') + 1);
#elif defined(_MSC_VER)
  constexpr auto beg_mem     = name.substr(name.rfind("->") + 2);
  constexpr auto member_name = beg_mem.substr(0, beg_mem.find_first_of(">}("));
#elif defined(__GNUC__)
  constexpr auto beg_mem     = name.substr(name.find("A ="));
  constexpr auto end_mem     = beg_mem.substr(0, beg_mem.find_first_of(';') - 1);
  constexpr auto member_name = end_mem.substr(end_mem.find_last_of(':') + 1);
#endif
  constexpr std::size_t length = member_name.size();
  return string_literal<length + 1>{member_name.data()};
}

template <Aggregate T>
consteval auto get_field_names() noexcept -> decltype(auto)
{
  auto constexpr names = aggregate_lookup<T>(
   [](auto&&... args) constexpr -> decltype(auto)
   {
     return std::make_tuple(field_ref{args}...);
   });

  using tup_t = std::remove_cvref_t<decltype(names)>;

  return [&]<std::size_t... I>(std::index_sequence<I...>) constexpr -> decltype(auto)
  {
    return std::make_tuple(deduce_field_name<T, std::get<I>(names)>()...);
  }(std::make_index_sequence<std::tuple_size_v<tup_t>>());
}

template <Aggregate T>
consteval auto get_field_ptr_types() noexcept -> decltype(auto)
{
  return aggregate_lookup<T>(
   [](auto... args) constexpr -> decltype(auto)
   {
     using type = std::tuple<std::add_pointer_t<std::remove_cvref_t<decltype(args)>>...>;
     return type();
   });
}

template <Aggregate T>
using field_ptr_types = std::remove_cvref_t<decltype(get_field_ptr_types<T>())>;

template <Aggregate T>
consteval auto get_field_cptr_types() noexcept -> decltype(auto)
{
  return aggregate_lookup<T>(
   [](auto... args) constexpr -> decltype(auto)
   {
     using type = std::tuple<std::add_pointer_t<std::remove_cvref_t<decltype(args)> const>...>;
     return type();
   });
}

template <Aggregate T>
using field_cptr_types = std::remove_cvref_t<decltype(get_field_cptr_types<T>())>;

template <Aggregate T>
consteval auto get_field_types() noexcept -> decltype(auto)
{
  return aggregate_lookup<T>(
   []<typename... Args>(Args...) constexpr -> decltype(auto)
   {
     using type = std::tuple<std::remove_cvref_t<Args>...>;
     return type();
   });
}

template <Aggregate T>
using field_types = std::remove_cvref_t<decltype(get_field_types<T>())>;

template <Aggregate T>
constexpr auto get_field_refs(T& ref) noexcept -> decltype(auto)
{
  return aggregate_lookup(
   []<typename... Args>(Args&&... args) constexpr -> decltype(auto)
   {
     return std::make_tuple(std::forward<Args>(args)...);
   },
   ref);
}

template <auto I, Aggregate T>
constexpr auto get_field_ref(T& ref) noexcept -> decltype(auto)
{
  return aggregate_lookup(
   [](auto&&... args) constexpr -> decltype(auto)
   {
     return [&]<auto... Is>(std::index_sequence<Is...>) constexpr -> decltype(auto)
     {
       // NOLINTNEXTLINE
       return [](decltype(reinterpret_cast<void const*>(Is))..., auto* nth, auto*...) -> decltype(auto)
       {
         return *nth;
       }(&args...);
     }(std::make_index_sequence<I>{});
   },
   ref);
}

template <auto I, Aggregate T>
using field_type = std::remove_cvref_t<decltype(get_field_ref<I>(std::declval<T&>()))>;

template <typename Transform>
auto transform_field_name(std::string_view name) -> std::string
{
  return std::string{Transform::transform(name)};
}

template <Aggregate T, typename Transform>
auto get_cached_field_names() -> auto const&
{
  auto constexpr names = aggregate_lookup<T>(
   [](auto&&... args) constexpr -> decltype(auto)
   {
     return std::make_tuple(field_ref{args}...);
   });

  using tup_t = std::remove_cvref_t<decltype(names)>;

  static auto field_names = [&]<std::size_t... I>(std::index_sequence<I...>) constexpr -> decltype(auto)
  {
    return std::make_tuple(transform_field_name<Transform>(deduce_field_name<T, std::get<I>(names)>())...);
  }(std::make_index_sequence<std::tuple_size_v<tup_t>>());

  return field_names;
}

} // namespace ouly::detail
