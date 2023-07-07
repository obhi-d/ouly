#pragma once

#include "deduced_types.hpp"

namespace acl
{

struct real
{

  using type        = float;
  using ref         = type&;
  using pref        = type;
  using cref        = type;
  using scalar_type = float;
  using row_type    = float;

  static constexpr std::uint32_t element_count = 1;
  static constexpr std::uint32_t row_count     = 1;
  static constexpr std::uint32_t column_count  = 1;

  inline static bool equals(pref v1, pref v2)
  {
    if (acl::almost_equals_ulps(v1, v2, 4))
      return true;
    return acl::almost_equals_rel_or_abs(v1, v2, acl::k_max_relative_error, acl::k_const_epsilon);
  }

  inline static bool isnan(pref v)
  {
    return std::isnan(v);
  }
  inline static bool isinf(pref v)
  {
    return std::isinf(v);
  }
};

} // namespace acl