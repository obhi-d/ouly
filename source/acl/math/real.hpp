#pragma once

#include "deduced_types.hpp"

namespace acl
{

  static constexpr std::uint32_t element_count = 1;
  static constexpr std::uint32_t row_count     = 1;
  static constexpr std::uint32_t column_count  = 1;

  inline static bool equals(float v1, float v2)
  {
    if (acl::almost_equals_ulps(v1, v2, 4))
      return true;
    return acl::almost_equals_rel_or_abs(v1, v2, acl::k_max_relative_error, acl::k_const_epsilon);
  }

  inline static bool isnan(float v)
  {
    return std::isnan(v);
  }

  inline static bool isinf(float v)
  {
    return std::isinf(v);
  }

  inline static bool equals(double v1, double v2)
  {
    if (acl::almost_equals_ulps(v1, v2, 4))
      return true;
    return acl::almost_equals_rel_or_abs(v1, v2, acl::k_max_relative_error, acl::k_const_epsilon);
  }

  inline static bool isnan(double v)
  {
    return std::isnan(v);
  }

  inline static bool isinf(double v)
  {
    return std::isinf(v);
  }

} // namespace acl
