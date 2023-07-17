#pragma once

#ifdef ACL_USE_SSE3
#ifndef ACL_USE_SSE2
#define ACL_USE_SSE2
#endif
#endif

#ifdef ACL_USE_SSE41
#ifndef ACL_USE_SSE3
#define ACL_USE_SSE3
#endif
#ifndef ACL_USE_SSE2
#define ACL_USE_SSE2
#endif
#endif

#ifdef ACL_USE_AVX
#ifndef ACL_USE_SSE41
#define ACL_USE_SSE41
#endif
#ifndef ACL_USE_SSE3
#define ACL_USE_SSE3
#endif
#ifndef ACL_USE_SSE2
#define ACL_USE_SSE2
#endif
#endif

#include <immintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__clang__) || defined(__GNUC__)
#endif

#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <type_traits>

namespace acl
{
static constexpr float k_round_off                 = 0.000001f;
static constexpr float k_pi                        = 3.14159265358900f;
static constexpr float k_2pi                       = 2 * k_pi;
static constexpr float k_pi_by_2                   = k_pi / 2;
static constexpr float k_1_by_pi                   = 1.0f / k_pi;
static constexpr float k_1_by_2pi                  = 1.0f / k_2pi;
static constexpr float k_degrees_to_radians_factor = k_pi / 180.0f;
static constexpr float k_radians_to_degrees_factor = 180.0f / k_pi;

static constexpr double k_pi_d                        = 3.14159265358979311599796346854;
static constexpr double k_2pi_d                       = 2 * k_pi;
static constexpr double k_pi_by_2_d                   = k_pi / 2;
static constexpr double k_1_by_pi_d                   = 1.0 / k_pi;
static constexpr double k_1_by_2pi_d                  = 1.0 / k_2pi_d;
static constexpr double k_degrees_to_radians_factor_d = k_pi_d / 180.0;
static constexpr double k_radians_to_degrees_factor_d = 180.0 / k_pi_d;

static constexpr float k_scalar_max = 3.402823466e+38f;
static constexpr float k_scalar_big = 999999999.0f;

static constexpr float k_const_epsilon      = 1.192092896e-06f;
static constexpr float k_const_epsilon_med  = 0.0009765625f;
static constexpr float k_const_epsilon_big  = 0.0625f;
static constexpr float k_max_relative_error = 0.005f;

static constexpr double k_const_epsilon_d      = 1.192092896e-06;
static constexpr double k_max_relative_error_d = 0.005;

static constexpr float k_default_gamma = 2.2f;

struct noinit
{};
static constexpr noinit noinit_v{};

//! Integer representation of a floating-point value.
inline int32_t float_to_int(float value) noexcept
{
  return *(int32_t*)&value;
}

//! Signed integer representation of a floating-point value.
inline uint32_t float_to_uint(float value) noexcept
{
  return *(uint32_t*)&value;
}

//! Integer representation of a floating-point value.
inline int64_t float_to_int(double value) noexcept
{
  return *(int64_t*)&value;
}

//! Signed integer representation of a floating-point value.
inline uint64_t float_to_uint(double value) noexcept
{
  return *(uint64_t*)&value;
}

inline float uint_to_float(std::uint32_t f) noexcept
{
  // value * maxvalue
  return *reinterpret_cast<float*>(&f);
}

inline double uint_to_float(uint64_t f) noexcept
{
  // value * maxvalue
  return *reinterpret_cast<double*>(&f);
}

constexpr float pixel_align(float v) noexcept
{
  return (float)(int32_t)(v + (v > 0 ? 0.5f : -0.5f));
}

template <typename scalar_t>
constexpr bool approx_equals(scalar_t v1, scalar_t v2, scalar_t roundoff) noexcept
{
  return ((v2 - roundoff) <= v1 && v1 <= (v2 + roundoff));
}

} // namespace acl
