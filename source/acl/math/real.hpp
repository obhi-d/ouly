#pragma once

#include "deduced_types.hpp"

namespace acl
{

static constexpr std::uint32_t element_count = 1;
static constexpr std::uint32_t row_count     = 1;
static constexpr std::uint32_t column_count  = 1;

template <FloatingType floating>
inline static bool equals(floating v1, floating v2) noexcept
{
  return acl::almost_equals_ulps(v1, v2, 4);
}

template <IntegralType floating>
inline static bool equals(floating v1, floating v2) noexcept
{
  return v1 == v2;
}

inline static bool isnan(float v) noexcept
{
  return std::isnan(v);
}

inline static bool isinf(float v) noexcept
{
  return std::isinf(v);
}

inline static bool isnan(double v) noexcept
{
  return std::isnan(v);
}

inline static bool isinf(double v) noexcept
{
  return std::isinf(v);
}

} // namespace acl
