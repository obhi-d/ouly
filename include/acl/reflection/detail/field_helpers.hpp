#pragma once

#include <acl/reflection/detail/aggregate.hpp>
#include <acl/reflection/detail/deduced_types.hpp>
#include <acl/reflection/detail/derived_concepts.hpp>

#include <source_location>
#include <type_traits>
#include <utility>

namespace acl::detail
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
[[nodiscard]] constexpr auto function_name() noexcept -> std::string_view
{
  return std::source_location::current().function_name();
}

template <typename T>
[[nodiscard]] constexpr auto function_name() noexcept -> std::string_view
{
  return std::source_location::current().function_name();
}

template <typename T, auto A>
constexpr auto deduce_field_name() -> decltype(auto)
{
  constexpr auto name = function_name<T, &A.member_>();
#if defined(__clang__)
  constexpr auto        beg_mem     = name.substr(name.find_last_of('.') + 1);
  constexpr auto        member_name = beg_mem.substr(0, beg_mem.find_first_of(']'));
  constexpr std::size_t length      = member_name.size();

  return string_literal<length + 1>{member_name.data()};
#elif defined(_MSC_VER)
  constexpr auto        beg_mem     = name.substr(name.find("->") + 2);
  constexpr auto        member_name = beg_mem.substr(0, beg_mem.find_first_of('>'));
  constexpr std::size_t length      = member_name.size();

  return string_literal<length + 1>{member_name.data()};
#elif defined(__GNUC__)
  constexpr auto        beg_mem     = name.substr(name.find("A ="));
  constexpr auto        end_mem     = beg_mem.substr(0, beg_mem.find_first_of(';') - 1);
  constexpr auto        member_name = end_mem.substr(end_mem.find_last_of(':') + 1);
  constexpr std::size_t length      = member_name.size();

  return string_literal<length + 1>{member_name.data()};
#else
  constexpr std::size_t length = name.size();
  return string_literal<length + 1>{name.data()};
#endif
}

template <Aggregate T>
constexpr auto get_field_names() noexcept -> decltype(auto)
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
       return [](decltype((void*)Is)..., auto* nth, auto*...) -> decltype(auto)
       {
         return *nth;
       }(&args...);
     }(std::make_index_sequence<I>{});
   },
   ref);
}

template <auto I, Aggregate T>
using field_type = std::remove_cvref_t<decltype(get_field_ref<I>(std::declval<T&>()))>;

} // namespace acl::detail