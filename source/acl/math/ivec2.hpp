#pragma once
#include "vec_base.hpp"

namespace acl
{

namespace detail
{
struct ivec2_traits
{

  using scalar_type = int;
  using row_type    = scalar_type;
  using type        = types::vec2_t<scalar_type>;
  using ref         = type&;
  using pref        = type const&;
  using cref        = type const&;

  static constexpr std::uint32_t element_count = 2;
  static constexpr std::uint32_t row_count     = 1;
  static constexpr std::uint32_t column_count  = 2;
};
} // namespace detail
struct ivec2 : public vec_base<detail::ivec2_traits>
{};
} // namespace acl