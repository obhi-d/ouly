#pragma once
#include "detail/type_name.hpp"
#include <cstdint>
#include <tuple>
#include <type_traits>

namespace acl
{
template <typename T, typename SizeType = size_t>
constexpr SizeType alignarg = alignof(T) > alignof(std::max_align_t) ? alignof(T) : 0;

template <typename Ty = std::void_t<>>
struct traits
{
  using size_type                                = std::uint32_t;
  static constexpr std::uint32_t pool_size       = 4096;
  static constexpr std::uint32_t index_pool_size = 4096;
  // static constexpr bool          assume_pod  = false;
  // null
  // static constexpr T null_v = {};
  // using offset
  // using offset = acl::offset<&selfref::self>;
};

template <typename Ty = std::void_t<>>
struct allocator_traits
{
  using is_always_equal                        = std::false_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_swap            = std::true_type;
};

template <auto M>
struct offset
{
  static auto& get(auto& to)
  {
    return (to.*M);
  }
  static decltype(auto) get(auto const& to)
  {
    return (to.*M);
  }
};

template <typename T>
constexpr std::string_view type_name()
{
  return detail::type_name<std::remove_cv_t<std::remove_reference_t<T>>>();
}

template <typename T>
constexpr std::uint32_t type_hash()
{
  return detail::type_hash<std::remove_cv_t<std::remove_reference_t<T>>>();
}

struct nocheck : std::false_type
{};

/// Specialize backref
/// template <>
/// struct traits<MyType>
/// {
///   using offset = acl::offset<&MyType::self>;
/// };

template <typename T>
struct function_traits;

template <typename R, typename... Args>
struct function_traits<R (*)(Args...)>
{
  static constexpr size_t arity = sizeof...(Args);
  using result_type             = R;
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
  static constexpr size_t arity = sizeof...(Args);
  using result_type             = R;
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
  static constexpr size_t arity = sizeof...(Args);
  using result_type             = R;
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

namespace detail
{

template <typename Traits, typename U>
concept HasNullValue = requires(U t) {
                         {
                           ((Traits::null_v))
                           } -> std::convertible_to<U>;
                         {
                           Traits::null_v == t
                           } -> std::same_as<bool>;
                       };

template <typename Traits, typename U>
concept HasNullMethod = requires(U v) {
                          {
                            Traits::is_null(v)
                            } noexcept -> std::same_as<bool>;
                        };

template <typename Traits, typename U>
concept HasNullConstruct = requires(U v) {
                             Traits::null_construct(v);
                             Traits::null_reset(v);
                           };

template <typename Traits>
concept HasIndexPoolSize = requires {
                             {
                               ((Traits::index_pool_size))
                               } -> std::convertible_to<uint32_t>;
                           };

template <typename Traits>
concept HasSelfIndexPoolSize = requires {
                                 {
                                   ((Traits::self_index_pool_size))
                                   } -> std::convertible_to<uint32_t>;
                               };

template <typename Traits>
concept HasKeysIndexPoolSize = requires {
                                 {
                                   ((Traits::keys_index_pool_size))
                                   } -> std::convertible_to<uint32_t>;
                               };
template <typename Traits>
concept HasBackrefValue = requires { typename Traits::offset; };

template <typename Traits>
concept HasSizeType = requires { typename Traits::size_type; };

template <typename Traits>
concept HasTrivialAttrib = requires {
                             Traits::assume_pod;
                             {
                               std::bool_constant<Traits::assume_pod>()
                               } -> std::same_as<std::true_type>;
                           };

template <typename Traits>
concept HasNoFillAttrib = requires {
                            Traits::no_fill;
                            {
                              std::bool_constant<Traits::no_fill>()
                              } -> std::same_as<std::true_type>;
                          };

template <typename Traits>
concept HasTriviallyDestroyedOnMoveAttrib = requires {
                                              Traits::no_fill;
                                              {
                                                std::bool_constant<Traits::trivially_destroyed_on_move>()
                                                } -> std::same_as<std::true_type>;
                                            };
template <typename Traits>
concept HasUseSparseAttrib = requires {
                               Traits::use_sparse;
                               {
                                 std::bool_constant<Traits::use_sparse>()
                                 } -> std::same_as<std::true_type>;
                             };

template <typename Traits>
concept HasUseSparseIndexAttrib = requires {
                                    Traits::use_sparse_index;
                                    {
                                      std::bool_constant<Traits::use_sparse_index>()
                                      } -> std::same_as<std::true_type>;
                                  };

template <typename Traits>
concept HasSelfUseSparseIndexAttrib = requires {
                                        Traits::self_use_sparse_index;
                                        {
                                          std::bool_constant<Traits::use_self_sparse_index>()
                                          } -> std::same_as<std::true_type>;
                                      };

template <typename Traits>
concept HasKeysUseSparseIndexAttrib = requires {
                                        Traits::keys_use_sparse_index;
                                        {
                                          std::bool_constant<Traits::use_keys_sparse_index>()
                                          } -> std::same_as<std::true_type>;
                                      };

template <typename Traits>
concept HasZeroMemoryAttrib = requires {
                                Traits::zero_memory;
                                {
                                  std::bool_constant<Traits::zero_memory>()
                                  } -> std::same_as<std::true_type>;
                              };

template <typename V, typename R>
concept OptionalValueLike = requires(V v) {
                              {
                                *v
                                } -> std::convertible_to<R>;
                              {
                                (bool)v
                                } -> std::convertible_to<bool>;
                            };

template <typename S, typename T1, typename... Args>
struct choose_size_ty
{
  using type =
    std::conditional_t<HasSizeType<T1>, typename choose_size_ty<S, T1>::type, typename choose_size_ty<Args...>::type>;
};

template <typename S, typename T1>
struct choose_size_ty<S, T1>
{
  using type = S;
};

template <typename S, HasSizeType T1>
struct choose_size_ty<S, T1>
{
  using type = typename T1::size_type;
};

template <typename S, typename... Args>
using choose_size_t = typename choose_size_ty<S, Args...>::type;

template <typename underlying_allocator_tag>
struct is_static
{
  constexpr inline static bool value = false;
};

template <typename underlying_allocator_tag>
constexpr static bool is_static_v = is_static<underlying_allocator_tag>::value;

template <typename ua_t, class = std::void_t<>>
struct tag
{
  using type = void;
};
template <typename ua_t>
struct tag<ua_t, std::void_t<typename ua_t::tag>>
{
  using type = typename ua_t::tag;
};

template <typename ua_t>
using tag_t = typename tag<ua_t>::type;

template <typename ua_t, class = std::void_t<>>
struct size_type
{
  using type = std::size_t;
};
template <typename ua_t>
struct size_type<ua_t, std::void_t<typename ua_t::size_type>>
{
  using type = typename ua_t::size_type;
};

template <typename ua_t>
using size_t = typename size_type<ua_t>::type;

} // namespace detail

} // namespace acl
