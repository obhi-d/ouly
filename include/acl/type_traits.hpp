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
  using size_type                              = std::uint32_t;
  static constexpr std::uint32_t pool_size     = 4096;
  static constexpr std::uint32_t idx_pool_size = 4096;
  static constexpr bool          assume_pod_v  = false;
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
  static void set(auto& to, auto link)
  {
    reinterpret_cast<std::remove_cvref_t<decltype(link)>&>(to.*M) = link;
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

template <typename Traits>
concept has_backref_v = requires
{
  typename Traits::offset;
};

template <typename Traits>
concept has_size_type_v = requires
{
  typename Traits::size_type;
};

template <typename T1, typename... Args>
struct choose_size_ty
{
  using type = std::conditional_t<has_size_type_v<T1>, typename T1::size_type, typename choose_size_ty<Args...>::type>;
};

template <typename T1>
struct choose_size_ty<T1>
{
  using type = std::conditional_t<has_size_type_v<T1>, typename T1::size_type, std::size_t>;
};

template <typename... Args>
using choose_size_t = typename choose_size_ty<Args...>::type;

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