#pragma once
#include "vec_base.hpp"

namespace acl
{

namespace detail
{
struct ivec3_traits
{
  using scalar_type = int;
  using row_type    = scalar_type;
  using type        = types::vec3_t<scalar_type>;
  using ref         = type&;
  using pref        = type const&;
  using cref        = type const&;

  static constexpr std::uint32_t element_count = 3;
  static constexpr std::uint32_t row_count     = 1;
  static constexpr std::uint32_t column_count  = 3;
};
} // namespace detail
struct ivec3 : public vec_base<detail::ivec3_traits>
{};
} // namespace acl