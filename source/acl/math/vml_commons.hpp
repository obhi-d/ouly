#pragma once

#ifdef ACL_USE_SSE3
#ifndef ACL_USE_SSE2
# define ACL_USE_SSE2
#endif
#endif


#ifdef ACL_USE_SSE41
#ifndef ACL_USE_SSE3
# define ACL_USE_SSE3
#endif
#ifndef ACL_USE_SSE2
# define ACL_USE_SSE2
#endif
#endif

#ifdef ACL_USE_AVX
#ifndef ACL_USE_SSE41
# define ACL_USE_SSE41
#endif
#ifndef ACL_USE_SSE3
# define ACL_USE_SSE3
#endif
#ifndef ACL_USE_SSE2
# define ACL_USE_SSE2
#endif
#endif

#include <immintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__clang__) || defined(__GNUC__)
#endif

#include <array>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <type_traits>

namespace acl
{
struct noinit{};
static constexpr noinit noinit_v{};

//! Integer representation of a floating-point value.
inline int32_t float_to_int(float value)
{
  return *(int32_t*)&value;
}

//! Signed integer representation of a floating-point value.
inline uint32_t float_to_uint(float value)
{
  return *(uint32_t*)&value;
}

//! Integer representation of a floating-point value.
inline int64_t float_to_int(double value)
{
  return *(int64_t*)&value;
}

//! Signed integer representation of a floating-point value.
inline uint64_t float_to_uint(double value)
{
  return *(uint64_t*)&value;
}

constexpr float pixel_align(float v)
{
  return (float)(int32_t)(v + (v > 0 ? 0.5f : -0.5f));
}

constexpr float degrees_to_radians(float a)
{
  return a * 0.0174532925f;
}

constexpr float radians_to_degrees(float a)
{
  return a * 57.295779513f;
}

constexpr bool approx_equals(float v1, float v2, float roundoff)
{
  return ((v2 - roundoff) <= v1 && v1 <= (v2 + roundoff));
}

} // namespace acl
