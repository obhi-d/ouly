#pragma once

#if VML_USE_SSE_AVX

#include <emmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>

#ifdef _MSC_VER
#define acl_cast_i128_ps(v) _mm_castsi128_ps(v)
#define acl_cast_ps_i128(v) _mm_castps_si128(v)
#else
#define acl_cast_i128_ps(v) (__m128)(v)
#define acl_cast_ps_i128(v) (__m128i)(v)
#endif

#endif

#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__clang__) || defined(__GNUC__)
#endif

#include <array>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>

namespace acl
{
#if VML_USE_SSE_AVX
inline auto clear_w_mask() noexcept
{
  return acl_cast_i128_ps(_mm_set_epi32(0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));
}

inline auto xyz0_w1() noexcept
{
  return _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
}

inline auto clear_xyz() noexcept
{
  return acl_cast_i128_ps(_mm_set_epi32(0xFFFFFFFF, 0, 0, 0));
}
#endif
//! Integer representation of a floating-point value.
constexpr int32_t float_to_int(float value)
{
  return *(int32_t*)&value;
}

//! Signed integer representation of a floating-point value.
constexpr uint32_t float_to_uint(float value)
{
  return *(uint32_t*)&value;
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