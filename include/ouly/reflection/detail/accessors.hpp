// SPDX-License-Identifier: MIT
#pragma once

#include <iterator>
#include <type_traits>

namespace ouly::detail
{

// Types
template <auto>
struct member_ptr_type;

template <typename T, typename M, M T::* P>
struct member_ptr_type<P>
{
  using class_t  = std::decay_t<T>;
  using member_t = std::decay_t<M>;
};

// The accessor traits match on the type of the function pointer rather than on its value: noexcept is
// part of a function's type, and specializing on the value lets a noexcept pointer also match the
// plain form (by function pointer conversion), which some compilers then reject as ambiguous.
template <typename T, typename R>
struct accessor_traits
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <typename F>
struct member_getter_fn
{};

template <typename T, typename R>
struct member_getter_fn<R (T::*)() const> : accessor_traits<T, R>
{};

template <typename T, typename R>
struct member_getter_fn<R (T::*)() const noexcept> : accessor_traits<T, R>
{};

template <auto MF>
struct member_getter_type : member_getter_fn<decltype(MF)>
{};

template <typename F>
struct member_setter_fn
{};

template <typename T, typename R>
struct member_setter_fn<void (T::*)(R)> : accessor_traits<T, R>
{};

template <typename T, typename R>
struct member_setter_fn<void (T::*)(R) noexcept> : accessor_traits<T, R>
{};

template <auto MF>
struct member_setter_type : member_setter_fn<decltype(MF)>
{};

template <typename F>
struct free_getter_fn
{};

template <typename T, typename R>
struct free_getter_fn<R (*)(T const&)> : accessor_traits<T, R>
{};

template <typename T, typename R>
struct free_getter_fn<R (*)(T const&) noexcept> : accessor_traits<T, R>
{};

template <auto F>
struct free_getter_type : free_getter_fn<decltype(F)>
{};

template <typename F>
struct getter_by_value_fn
{};

// Only getters that take the object by value: a T const& parameter is the free_getter form, and
// matching it here too made bind<Name, &get, &set> ambiguous for free const& getters.
template <typename T, typename R>
  requires(!std::is_reference_v<T>)
struct getter_by_value_fn<R (*)(T)> : accessor_traits<T, R>
{};

template <typename T, typename R>
  requires(!std::is_reference_v<T>)
struct getter_by_value_fn<R (*)(T) noexcept> : accessor_traits<T, R>
{};

template <auto F>
struct getter_by_value_type : getter_by_value_fn<decltype(F)>
{};

template <typename F>
struct free_setter_fn
{};

template <typename T, typename R>
struct free_setter_fn<void (*)(T&, R)> : accessor_traits<T, R>
{};

template <typename T, typename R>
struct free_setter_fn<void (*)(T&, R) noexcept> : accessor_traits<T, R>
{};

template <auto F>
struct free_setter_type : free_setter_fn<decltype(F)>
{};

template <typename Class>
using array_value_type = std::decay_t<decltype(*std::begin(Class()))>;

} // namespace ouly::detail