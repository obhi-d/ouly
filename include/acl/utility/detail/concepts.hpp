#pragma once

#include <acl/utility/config.hpp>
#include <acl/utility/transforms.hpp>
#include <acl/utility/type_traits.hpp>
#include <concepts>
#include <cstdint>

namespace acl::detail
{

template <typename F, typename... Args>
concept Callable = requires(F f, Args&&... args) {
  { f(std::forward<Args>(args)...) };
};

// Concept to check if a type is a free function pointer
template <typename F, typename... Args>
concept Function = std::is_function_v<std::remove_pointer_t<F>>;

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
concept HasSelfIndexValue = requires { typename Traits::self_index; };

template <typename Traits>
concept HasDirectMapping = Traits::use_direct_mapping_v;
;

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

template <typename Traits>
concept HasLinkType = requires { typename Traits::link_type; };

template <typename V>
concept OptionalValueLike = requires(V v) {
  typename V::value_type;
  { v.has_value() } -> std::convertible_to<bool>;
  { v.value() } -> std::convertible_to<typename V::value_type>;
  { v.value_or(std::declval<typename V::value_type>()) } -> std::convertible_to<typename V::value_type>;
};
template <typename T>
concept HasAllocatorAttribs = requires { typename T::allocator_t; };

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

template <typename T, bool>
struct link_value;

template <typename T>
struct link_value<T, true>
{
  using type = typename T::link_type;
};

template <typename T>
struct link_value<T, false>
{
  using type = void;
};

template <typename T>
using link_value_t = typename link_value<T, HasLinkType<T>>::type;

template <typename UaT, class = std::void_t<>>
struct tag
{
  using type = void;
};
template <typename UaT>
struct tag<UaT, std::void_t<typename UaT::tag>>
{
  using type = typename UaT::tag;
};

template <typename UaT>
using tag_t = typename tag<UaT>::type;

template <typename UaT, class = std::void_t<>>
struct size_type
{
  using type = std::size_t;
};
template <typename UaT>
struct size_type<UaT, std::void_t<typename UaT::size_type>>
{
  using type = typename UaT::size_type;
};

template <typename T>
struct pool_size
{
  static constexpr uint32_t value = cfg::default_pool_size;
};

template <HasPoolSize T>
struct pool_size<T>
{
  static constexpr uint32_t value = T::pool_size_v;
};

template <typename T>
constexpr uint32_t pool_size_v = acl::detail::pool_size<T>::value;

template <typename T>
struct transform_type
{
  using type = acl::pass_through_transform;
};

template <typename T>
  requires requires { typename T::is_string_transform; }
struct transform_type<T>
{
  using type = T;
};

template <typename T>
using transform_t = typename transform_type<T>::type;

} // namespace acl::detail
