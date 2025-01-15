#pragma once

#include <acl/reflection/detail/bind_helpers.hpp>

namespace acl
{

/**
 * @brief Creates a binding for a member pointer with a specified name.
 *
 * This function template creates a binding between a member pointer and a name at compile-time.
 * It is used for reflection purposes to associate names with class members.
 *
 * @tparam Name A compile-time string literal that will be used as the name of the binding
 * @tparam MPtr A member pointer (either member function pointer or member variable pointer)
 *
 * @requires MPtr must be a member pointer type (verified by acl::detail::IsMemberPtr)
 *
 * @return A compile-time binding object that associates the name with the member pointer
 *
 * @note This function is marked noexcept as it performs only compile-time operations
 */
template <string_literal Name, auto MPtr>
constexpr auto bind() noexcept
  requires(acl::detail::IsMemberPtr<MPtr>)
{
  return acl::detail::decl_member_ptr<Name, typename acl::detail::member_ptr_type<MPtr>::class_t,
                                      typename acl::detail::member_ptr_type<MPtr>::member_t, MPtr>();
}

/**
 * @brief Creates a bindable property with getter and setter for reflection.
 *
 * @tparam Name The name of the property as a string_literal
 * @tparam Getter Member function pointer for getting the property value
 * @tparam Setter Member function pointer for setting the property value
 *
 * @return A compile-time property declaration that can be used for reflection
 *
 * @requires Getter and Setter must be valid member function pointers that form a getter/setter pair
 *
 * @note This function is used to create reflectable properties that can be accessed and modified
 *       through the reflection system using the specified getter and setter methods.
 */
template <string_literal Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
  requires(acl::detail::IsMemberGetterSetter<Getter, Setter>)
{
  return acl::detail::decl_get_set<Name, typename acl::detail::member_getter_type<Getter>::class_t,
                                   typename acl::detail::member_getter_type<Getter>::return_t, Getter, Setter>();
}

/**
 * @brief Creates a binding for a property with free-function getter and setter.
 *
 * @tparam Name String literal representing the property name
 * @tparam Getter Free function pointer acting as property getter
 * @tparam Setter Free function pointer acting as property setter
 *
 * @requires The Getter and Setter must satisfy the IsFreeGetterSetter concept
 *
 * @return A declaration object representing the property binding
 *
 * This function template creates a binding for a property using free functions as
 * getter and setter. The getter and setter must be compatible with the target class
 * and property type.
 */
template <string_literal Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
  requires(acl::detail::IsFreeGetterSetter<Getter, Setter>)
{
  return acl::detail::decl_free_get_set<Name, typename acl::detail::free_getter_type<Getter>::class_t,
                                        typename acl::detail::free_getter_type<Getter>::return_t, Getter, Setter>();
}

/**
 * @brief Creates a binding for a class member using free getter and setter functions with value semantics
 *
 * @tparam Name String literal representing the name of the member
 * @tparam Getter Free function that gets the member value by value
 * @tparam Setter Free function that sets the member value
 *
 * @return constexpr binding declaration
 *
 * @requires The Getter and Setter must satisfy the IsFreeGetterByValSetter concept
 *
 * This function template creates a binding that can be used for reflection purposes,
 * where the member is accessed through free functions rather than member functions.
 * The getter must return by value and the setter must take the value by parameter.
 */
template <string_literal Name, auto Getter, auto Setter>
constexpr auto bind() noexcept
  requires(acl::detail::IsFreeGetterByValSetter<Getter, Setter>)
{
  return acl::detail::decl_free_get_set<Name, typename acl::detail::getter_by_value_type<Getter>::class_t,
                                        typename acl::detail::getter_by_value_type<Getter>::return_t, Getter, Setter>();
}

/**
 * @brief Creates a tuple of bound declarations.
 *
 * This function binds multiple declarations into a tuple for use in reflection.
 * It forwards the arguments while preserving their value categories.
 *
 * @tparam Args Types that satisfy the DeclBase concept
 * @param args Declaration arguments to bind
 * @return constexpr auto A tuple containing the bound declarations
 *
 * @note All arguments must satisfy the DeclBase concept
 * @note This operation is noexcept
 */
template <acl::detail::DeclBase... Args>
constexpr auto bind(Args&&... args) noexcept
{
  return std::make_tuple(std::forward<Args>(args)...);
}

} // namespace acl