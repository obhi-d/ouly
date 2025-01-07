#pragma once

#include <acl/reflection/deduced_types.hpp>
#include <source_location>

namespace acl::detail
{

/**
 * @brief This function iterates over members registered by bind
 * @param fn A lambda accepting the instance of the object, a type info and the member index. The type info has a value
 * method to access the internal member given the instance of the object
 */
template <typename Class, typename Fn>
void for_each_field(Fn fn, Class& obj) noexcept
{
  using ClassType = std::remove_const_t<Class>;
  static_assert(detail::tuple_size<ClassType> > 0, "Invalid tuple size");
  return [&]<std::size_t... I>(std::index_sequence<I...>, auto tup)
  {
    (fn(obj, std::get<I>(tup), std::integral_constant<std::size_t, I>()), ...);
  }(std::make_index_sequence<detail::tuple_size<ClassType>>(), detail::reflect<ClassType>());
}

/**
 * @brief This function iterates over members registered by bind without the class object specified
 * @param fn A lambda accepting a type info and the member index. The type info has a value method to access the
 * internal member given the instance of the object
 */
template <typename Class, typename Fn>
void for_each_field(Fn fn) noexcept
{
  using ClassType = std::remove_const_t<Class>;
  static_assert(detail::tuple_size<ClassType> > 0, "Invalid tuple size");
  return [&]<std::size_t... I>(std::index_sequence<I...>, auto tup)
  {
    (fn(std::get<I>(tup), std::integral_constant<std::size_t, I>()), ...);
  }(std::make_index_sequence<detail::tuple_size<ClassType>>(), detail::reflect<ClassType>());
}

template <typename Class, std::size_t I>
constexpr auto field_at() noexcept
{
  using ClassType = std::remove_const_t<Class>;
  static_assert(detail::tuple_size<ClassType> > 0, "Invalid tuple size");
  return std::get<I>(detail::reflect<ClassType>());
}

template <typename ClassType>
constexpr auto field_size() noexcept -> uint32_t
{
  return detail::tuple_size<ClassType>;
}

template <auto M>
constexpr auto deduce_field_name() -> std::string_view
{
  auto name = std::string_view{std::source_location::current().function_name()};
  auto pos  = name.find_last_of('&');
  pos       = name.find("::", pos);
  if (pos != std::string_view::npos)
  {
    pos += 2;
    auto next = name.find_first_of(";]>", pos);
    return name.substr(pos, next - pos);
  }
  assert(false && "Failed to deduce field name");
  return {};
}

} // namespace acl::detail