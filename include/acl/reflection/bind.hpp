#pragma once

#include <acl/reflection/detail/bind_helpers.hpp>

namespace acl
{

template <string_literal Name, auto MPtr>
constexpr auto bind() noexcept
  requires(detail::IsMemberPtr<MPtr>)
{
  return detail::decl_member_ptr<Name, typename detail::member_ptr_type<MPtr>::class_t,
                                 typename detail::member_ptr_type<MPtr>::member_t, MPtr>();
}

template <string_literal Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
  requires(detail::IsMemberGetterSetter<Getter, Setter>)
{
  return detail::decl_get_set<Name, typename detail::member_getter_type<Getter>::class_t,
                              typename detail::member_getter_type<Getter>::return_t, Getter, Setter>();
}

template <string_literal Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
  requires(detail::IsFreeGetterSetter<Getter, Setter>)
{
  return detail::decl_free_get_set<Name, typename detail::free_getter_type<Getter>::class_t,
                                   typename detail::free_getter_type<Getter>::return_t, Getter, Setter>();
}

template <string_literal Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
  requires(detail::IsFreeGetterByValSetter<Getter, Setter>)
{
  return detail::decl_free_get_set<Name, typename detail::getter_by_value_type<Getter>::class_t,
                                   typename detail::getter_by_value_type<Getter>::return_t, Getter, Setter>();
}

template <detail::DeclBase... Args>
constexpr auto bind(Args&&... args) noexcept
{
  return std::make_tuple(std::forward<Args>(args)...);
}

} // namespace acl