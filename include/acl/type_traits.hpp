#pragma once

#include "detail/type_name.hpp"
#include <cstdint>
#include <tuple>
#include <type_traits>

namespace acl
{

template <typename... Option>
struct options : public Option...
{};

template <>
struct options<>
{};

template <typename T>
struct default_options
{};

template <typename Ty = std::void_t<>>
struct allocator_traits
{
  using is_always_equal                        = std::false_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_swap            = std::true_type;
};

///--------------- Basic Options ----------------
namespace opt
{

/// @brief Option to provide member pointer
/// @tparam M
template <auto M>
struct member;

/// @brief Specialization that provided class_type
/// @tparam T class_type for the member
/// @tparam M member_type for the member
/// @tparam MPtr Pointer to member
template <typename T, typename M, M T::*MPtr>
struct member<MPtr>
{
  using class_type  = T;
  using member_type = M;
  using offset      = opt::member<MPtr>;

  inline static member_type& get(class_type& to) noexcept
  {
    return to.*MPtr;
  }

  inline static member_type const& get(class_type const& to) noexcept
  {
    return to.*MPtr;
  }
};

/// @brief Option to control underlying pool size
template <uint32_t PoolSize = 4096>
struct pool_size
{
  static constexpr uint32_t pool_size_v = PoolSize;
};

/// @brief Option to control the pool size of index maps used by container
template <uint32_t PoolSize = 4096>
struct index_pool_size
{
  static constexpr uint32_t index_pool_size_v = PoolSize;
};

/// @brief Self index pool size controls the pool size for back references
template <uint32_t PoolSize = 4096>
struct self_index_pool_size
{
  static constexpr uint32_t self_index_pool_size_v = PoolSize;
};

/// @brief Key index pool size controls the pool size for key indexes in tables
template <uint32_t PoolSize = 4096>
struct keys_index_pool_size
{
  static constexpr uint32_t keys_index_pool_size_v = PoolSize;
};

/// @brief Null value, only applicable to constexpr types
template <auto NullValue>
struct null_value
{
  static constexpr auto null_v = NullValue;
};

/// @brief Indexes will have this as their size type
template <typename T = uint32_t>
struct basic_size_type
{
  using size_type = T;
};

struct assume_pod
{
  static constexpr bool assume_pod_v = true;
};

struct no_fill
{
  static constexpr bool no_fill_v = true;
};

struct trivially_destroyed_on_move
{
  static constexpr bool trivially_destroyed_on_move_v = true;
};

struct use_sparse
{
  static constexpr bool use_sparse_v = true;
};

struct use_sparse_index
{
  static constexpr bool use_sparse_index_v = true;
};

struct self_use_sparse_index
{
  static constexpr bool self_use_sparse_index_v = true;
};

struct keys_use_sparse_index
{
  static constexpr bool keys_use_sparse_index_v = true;
};

struct zero_out_memory
{
  static constexpr bool zero_out_memory_v = true;
};

struct disable_pool_tracking
{
  static constexpr bool disable_pool_tracking_v = true;
};

} // namespace opt

///------------------------------------------------
/// @brief Class type to string_view name of the class
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
// clang-format off
template <typename Traits, typename U>
concept HasNullValue = requires(U t) {
  { Traits::null_v } -> std::convertible_to<U>;
  { Traits::null_v == t } -> std::same_as<bool>;
};

template <typename Traits, typename U>
concept HasNullMethod = requires(U v) {
  { Traits::is_null(v) } noexcept -> std::same_as<bool>;
};

template <typename Traits, typename U>
concept HasNullConstruct = requires(U v) {
  Traits::null_construct(v);
  Traits::null_reset(v);
};

template <typename Traits>
concept HasIndexPoolSize = requires {
  { Traits::index_pool_size_v } -> std::convertible_to<uint32_t>;
};

template <typename Traits>
concept HasPoolSize = requires {
  { Traits::pool_size_v } -> std::convertible_to<uint32_t>;
};

template <typename Traits>
concept HasSelfIndexPoolSize = requires {
  { Traits::self_index_pool_size_v } -> std::convertible_to<uint32_t>;
};

template <typename Traits>
concept HasKeysIndexPoolSize = requires {
  { Traits::keys_index_pool_size_v } -> std::convertible_to<uint32_t>;
};

template <typename Traits>
concept HasBackrefValue = requires { typename Traits::offset; };

template <typename Traits>
concept HasSizeType = requires { typename Traits::size_type; };

template <typename Traits>
concept HasTrivialAttrib = Traits::assume_pod_v;

template <typename Traits>
concept HasNoFillAttrib = Traits::no_fill_v;

template <typename Traits>
concept HasTriviallyDestroyedOnMoveAttrib = Traits::trivially_destroyed_on_move_v;

template <typename Traits>
concept HasUseSparseAttrib = Traits::use_sparse_v;

template <typename Traits>
concept HasUseSparseIndexAttrib = Traits::use_sparse_index_v;

template <typename Traits>
concept HasSelfUseSparseIndexAttrib = Traits::self_use_sparse_index_v;

template <typename Traits>
concept HasKeysUseSparseIndexAttrib = Traits::keys_use_sparse_index_v;

template <typename Traits>
concept HasZeroMemoryAttrib = Traits::zero_out_memory_v;

template <typename Traits>
concept HasDisablePoolTrackingAttrib = Traits::disble_pool_tracking_v;

template <typename V, typename R>
concept OptionalValueLike = requires(V v) {
  {*v } -> std::convertible_to<R>;
  { (bool)v } -> std::convertible_to<bool>;
};
template <typename T>
concept HasAllocatorAttribs = requires { typename T::allocator_type; };

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

template <typename T>
struct pool_size
{
    static constexpr uint32_t value = 4096;
};

template <HasPoolSize T>
struct pool_size<T>
{
    static constexpr uint32_t value = T::pool_size_v;
};

template <typename T>
constexpr uint32_t pool_size_v = detail::pool_size<T>::value;
// clang-format on
} // namespace detail

} // namespace acl
