
#pragma once

#include <cstddef>
#include <utility>

namespace acl
{

// Helper struct to extract the I-th type from a parameter pack
template <std::size_t I, typename T>
struct tuple_leaf
{
  T value_;
};

// Base case for the tuple definition (empty tuple)
template <typename... Args>
struct tuple_impl;

template <std::size_t I, typename... T>
struct tuple_element_impl;

// Helper struct to get the I-th type
template <std::size_t I, typename First, typename... Rest>
struct tuple_element_impl<I, First, Rest...>
{
  using type = typename tuple_element_impl<I - 1, Rest...>::type;
};

// Base case for tuple_element when I == 0
template <typename First, typename... Rest>
struct tuple_element_impl<0, First, Rest...>
{
  using type = First;
};

template <std::size_t... Is, typename... Args>
struct tuple_impl<std::index_sequence<Is...>, Args...> : tuple_leaf<Is, Args>...
{
  // Aggregate initialization will automatically initialize tuple_leaf members
};

// Main tuple class
template <typename... Args>
struct tuple : tuple_impl<std::index_sequence_for<Args...>, Args...>
{
  // Type alias for extracting the I-th type
  template <std::size_t I>
  using type = typename tuple_element_impl<I, Args...>::type;

  // Accessor function to get the I-th element
  template <std::size_t I>
  auto get() noexcept -> auto&
  {
    return static_cast<tuple_leaf<I, typename tuple::type<I>>&>(*this).value;
  }

  // Const accessor function
  template <std::size_t I>
  auto get() const noexcept -> const auto&
  {
    return static_cast<tuple_leaf<I, typename tuple::type<I>> const&>(*this).value;
  }
};

template <std::size_t I, typename T>
using tuple_element_t = typename T::template type<I>;

} // namespace acl

namespace std
{

template <typename... T>
struct tuple_size<acl::tuple<T...>> : std::integral_constant<size_t, sizeof...(T)>
{};

template <size_t I, class... T>
struct tuple_element<I, acl::tuple<T...>>
{
  using type = acl::tuple<T...>::template type<I>;
};

} // namespace std
