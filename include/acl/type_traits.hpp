#pragma once
#include "detail/config.hpp"
#include "detail/type_name.hpp"
#include <cstdint>
#include <type_traits>

namespace acl
{

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
  return std::string_view(detail::type_name<std::remove_cv_t<std::remove_reference_t<T>>>().name(),
                          detail::type_name<std::remove_cv_t<std::remove_reference_t<T>>>().size());
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

namespace detail
{

template <typename T>
struct function_traits;

template <typename R, typename... Args>
struct function_traits<R (*)(Args...)>
{
  static constexpr size_t arity = sizeof...(Args);
};

template <typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...)>
{
  static constexpr size_t arity = sizeof...(Args);
};

template <typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...) const>
{
  static constexpr size_t arity = sizeof...(Args);
};

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};


template <typename Traits, typename U>
concept has_null_value = requires(U t) {
                           {
                             ((Traits::null_v))
                             } -> std::convertible_to<U>;
                           {
                             Traits::null_v == t
                             } -> std::same_as<bool>;
                         };

template <typename Traits, typename U>
concept has_null_method = requires(U v) {
                            {
                              Traits::is_null(v)
                              } noexcept -> std::same_as<bool>;
                          };

template <typename Traits, typename U>
concept has_null_construct = requires(U v) {
                               Traits::null_construct(v);
                               Traits::null_reset(v);
                             };

template <typename Traits>
concept has_index_pool_size = requires {
                                {
                                  ((Traits::index_pool_size))
                                  } -> std::convertible_to<uint32_t>;
                              };

template <typename Traits>
concept has_self_index_pool_size = requires {
                                     {
                                       ((Traits::self_index_pool_size))
                                       } -> std::convertible_to<uint32_t>;
                                   };

template <typename Traits>
concept has_keys_index_pool_size = requires {
                                     {
                                       ((Traits::keys_index_pool_size))
                                       } -> std::convertible_to<uint32_t>;
                                   };
template <typename Traits>
concept has_backref_v = requires { typename Traits::offset; };

template <typename Traits>
concept has_size_type_v = requires { typename Traits::size_type; };

template <typename Traits>
concept has_has_pod_attrib = requires {
                               Traits::assume_pod;
                               {
                                 std::bool_constant<Traits::assume_pod>()
                                 } -> std::same_as<std::true_type>;
                             };

template <typename Traits>
concept has_no_fill_attrib = requires {
                               Traits::no_fill;
                               {
                                 std::bool_constant<Traits::no_fill>()
                                 } -> std::same_as<std::true_type>;
                             };

template <typename Traits>
concept has_trivially_destroyed_on_move_attrib = requires {
                               Traits::no_fill;
                               {
                                 std::bool_constant<Traits::trivially_destroyed_on_move>()
                                 } -> std::same_as<std::true_type>;
                             };
template <typename Traits>
concept has_use_sparse_attrib = requires {
                                  Traits::use_sparse;
                                  {
                                    std::bool_constant<Traits::use_sparse>()
                                    } -> std::same_as<std::true_type>;
                                };

template <typename Traits>
concept has_use_sparse_index_attrib = requires {
                                        Traits::use_sparse_index;
                                        {
                                          std::bool_constant<Traits::use_sparse_index>()
                                          } -> std::same_as<std::true_type>;
                                      };

template <typename Traits>
concept has_self_use_sparse_index_attrib = requires {
                                             Traits::self_use_sparse_index;
                                             {
                                               std::bool_constant<Traits::use_self_sparse_index>()
                                               } -> std::same_as<std::true_type>;
                                           };

template <typename Traits>
concept has_keys_use_sparse_index_attrib = requires {
                                             Traits::keys_use_sparse_index;
                                             {
                                               std::bool_constant<Traits::use_keys_sparse_index>()
                                               } -> std::same_as<std::true_type>;
                                           };

template <typename Traits>
concept has_zero_memory_attrib = requires {
                                   Traits::zero_memory;
                                   {
                                     std::bool_constant<Traits::zero_memory>()
                                     } -> std::same_as<std::true_type>;
                                 };


template <typename S, typename T1, typename... Args>
struct choose_size_ty
{
  using type = std::conditional_t<has_size_type_v<T1>, typename choose_size_ty<S, T1>::type, typename choose_size_ty<Args...>::type>;
};

template <typename S, typename T1>
struct choose_size_ty<S, T1>
{
  using type = S;
};

template <typename S, has_size_type_v T1>
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