#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

namespace acl
{

struct nocheck : std::false_type
{};

template <typename T>
struct function_traits;

template <typename R, typename... Args>
struct function_traits<R (*)(Args...)>
{
  static constexpr std::size_t arity = sizeof...(Args);
  using return_type                  = R;
  using args                         = std::tuple<Args...>;
  template <std::size_t Index>
  using arg_type                           = typename std::tuple_element_t<Index, std::tuple<Args...>>;
  constexpr static bool is_free_function   = true;
  constexpr static bool is_member_function = false;
  constexpr static bool is_const_function  = false;
  constexpr static bool is_functor         = false;
};

template <typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...)>
{
  static constexpr std::size_t arity = sizeof...(Args);
  using class_type                   = C;
  using return_type                  = R;
  using args                         = std::tuple<Args...>;
  template <std::size_t Index>
  using arg_type                           = typename std::tuple_element_t<Index, std::tuple<Args...>>;
  constexpr static bool is_free_function   = false;
  constexpr static bool is_member_function = true;
  constexpr static bool is_const_function  = false;
  constexpr static bool is_functor         = false;
};

template <typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...) const>
{
  static constexpr std::size_t arity = sizeof...(Args);
  using return_type                  = R;
  using args                         = std::tuple<Args...>;
  template <std::size_t Index>
  using arg_type                           = typename std::tuple_element_t<Index, std::tuple<Args...>>;
  constexpr static bool is_free_function   = false;
  constexpr static bool is_member_function = true;
  constexpr static bool is_const_function  = true;
  constexpr static bool is_functor         = false;
};

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

// Trait to extract class_type and function_type from a member function pointer
template <auto>
struct member_function
{
  constexpr static bool is_member_function_traits = false;
};

template <typename C, typename Ret, typename... Args, Ret (C::*M)(Args...)>
struct member_function<M>
{
  using class_type         = C;
  using function_type      = Ret (C::*)(Args...);
  using free_function_type = Ret (*)(Args...);
  using return_type        = Ret;
  using args               = std::tuple<Args...>;
  template <std::size_t Index>
  using arg_type = typename std::tuple_element_t<Index, std::tuple<Args...>>;

  static auto invoke(C& instance, Args&&... args)
  {
    return std::invoke(M, instance, std::forward<Args>(args)...);
  }

  constexpr static bool is_member_function_traits = true;
};

template <typename C, typename Ret, typename... Args, Ret (C::*M)(Args...) const>
struct member_function<M>
{
  using class_type         = const C;
  using function_type      = Ret (C::*)(Args...) const;
  using free_function_type = Ret (*)(Args...);
  using return_type        = Ret;
  using args               = std::tuple<Args...>;
  template <std::size_t Index>
  using arg_type = typename std::tuple_element_t<Index, std::tuple<Args...>>;

  static auto invoke(C const& instance, Args&&... args)
  {
    return std::invoke(M, instance, std::forward<Args>(args)...);
  }

  constexpr static bool is_member_function_traits = true;
};
} // namespace acl
