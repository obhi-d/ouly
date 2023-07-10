#pragma once
#include "ivec2.hpp"
#include "rect.hpp"

namespace acl
{
namespace detail
{
struct irect_traits
{

  using type        = types::rect_t<int>;
  using ref         = type&;
  using pref        = type const&;
  using cref        = type const&;
  using row_type    = types::vec2_t<int>;
  using row_tag     = ivec2;
  using scalar_type = int;

  static constexpr std::uint32_t element_count = 8;
  static constexpr std::uint32_t row_count     = 2;
  static constexpr std::uint32_t column_count  = 2;
};
} // namespace detail
struct irect : public detail::rect_base<detail::irect_traits>
{};
} // namespace acl
