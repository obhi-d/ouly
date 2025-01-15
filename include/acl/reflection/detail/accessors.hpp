#pragma once

#include <iterator>
#include <type_traits>

namespace acl::detail
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

template <auto>
struct member_getter_type;

template <typename T, typename R, R (T::*MF)() const>
struct member_getter_type<MF>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <auto>
struct member_setter_type;

template <typename T, typename R, void (T::*MF)(R)>
struct member_setter_type<MF>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <auto>
struct free_getter_type;
template <auto>
struct getter_by_value_type;

template <typename T, typename R, R (*F)(T const&)>
struct free_getter_type<F>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <typename T, typename R, R (*F)(T)>
struct getter_by_value_type<F>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <auto>
struct free_setter_type;

template <typename T, typename R, void (*F)(T&, R)>
struct free_setter_type<F>
{
  using class_t  = std::decay_t<T>;
  using return_t = R;
  using value_t  = std::decay_t<R>;
};

template <typename Class>
using array_value_type = std::decay_t<decltype(*std::begin(Class()))>;

} // namespace acl::detail