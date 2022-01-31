#pragma once
#include "detail/config.hpp"
#include <cstdint>
#include <type_traits>

namespace acl
{

template <typename Ty = std::void_t<>>
struct traits
{
  using size_type = std::uint32_t;

  static constexpr std::uint32_t pool_size     = 4096;
  static constexpr std::uint32_t idx_pool_size = 4096;
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

} // namespace detail

} // namespace acl