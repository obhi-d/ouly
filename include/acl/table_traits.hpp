#pragma once
#include "detail/config.hpp"
#include <cstdint>
#include <type_traits>

namespace acl
{

template <typename Ty>
constexpr std::uint32_t pool_size_v = 4096;

template <typename Ty>
constexpr std::uint32_t idx_pool_size_v = 4096;

template <typename Ty>
using size_type = std::uint32_t;

template <auto M>
struct offset
{
  static void set(auto& to, auto link)
  {
    reinterpret_cast<std::remove_cv<decltype(link)>&>(to.*M) = link;
  }
  static auto get(auto const& to)
  {
    return (to.*M);
  }
};

template <typename Ty>
struct backref;

/// Specialize backref
/// template <>
/// struct backref<MyType>
/// {
///   using offset = acl::offset<&MyType::some>;
/// };

namespace detail
{
template <typename Ty>
concept has_backref_v = requires
{
  typename backref<Ty>::offset;
};

} // namespace detail

} // namespace acl