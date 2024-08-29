
#pragma once

#include "vml_commons.hpp"
#include <bit>
#include <cassert>
#include <cmath>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace acl
{

// acl function overrides
template <typename T>
inline T recip_sqrt(T val) noexcept
{
  return static_cast<T>(1.f) / std::sqrt(val);
}

template <typename scalar_t>
inline std::pair<scalar_t, scalar_t> sin_cos(scalar_t i_val) noexcept
{
  return {std::sin(i_val), std::cos(i_val)};
}

//-------------------------------------------------------
// Other utilities
inline std::uint32_t bit_pos(std::uint32_t v) noexcept
{
#ifdef _MSC_VER
  unsigned long index;
  _BitScanForward(&index, v);
  return index;
#elif defined(__clang__) || defined(__GNUC__)
  return __builtin_ffs(v) - 1;
#else
#error Undefined
#endif
}

inline uint32_t bit_count(std::uint32_t i) noexcept
{
  return (uint32_t)std::popcount(i);
}

inline constexpr std::uint32_t prev_pow2(std::uint32_t v) noexcept
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return (++v) >> 1;
}

inline constexpr std::uint32_t next_pow2(std::uint32_t v) noexcept
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return ++v;
}

inline constexpr bool is_pow2(std::uint32_t val) noexcept
{
  return (val & (val - 1)) == 0;
}
//-------------------------------------------------------
// inlined functions
// wrap angle between -pi & +pi
template <typename IntType>
inline constexpr IntType round_up(IntType number, IntType multiple) noexcept
{
  IntType remainder = number % multiple;
  if (remainder == 0)
    return number;
  return number + multiple - remainder;
}

template <typename scalar_t>
inline constexpr scalar_t wrap_pi(scalar_t theta) noexcept
{
  theta += static_cast<scalar_t>(acl::k_pi_d);
  theta -= std::floor(theta * static_cast<scalar_t>(acl::k_1_by_2pi_d)) * static_cast<scalar_t>(acl::k_2pi_d);
  theta -= static_cast<scalar_t>(acl::k_pi_d);
  return theta;
}

// float to fixed point conversion, float must be
// between 0-1
inline constexpr std::uint32_t float_to_fixed(float f, std::uint32_t n) noexcept
{
  // value * maxvalue
  return static_cast<std::uint32_t>(f * ((1 << (n)) - 1));
}
// fixed to float, float returned is between
// zero and one
inline constexpr float fixed_to_float(std::uint32_t f, std::uint32_t n) noexcept
{
  // value / maxvalue
  return static_cast<float>(f) / (float)((1 << n) - 1);
}
// fixed to fixed, fixed returned is between
// zero and one

inline constexpr std::uint32_t fixed_to_fixed(std::uint32_t f, std::uint32_t f_base, std::uint32_t req_base) noexcept
{
  // ((max(reqb)/max(fb)) * f)
  // the trick is if reqb < fb we can straightforwardly
  // divide by pow(2,reqb-fb), so
  if (req_base < f_base)
    return (f >> (req_base - f_base));
  return f * ((1 << req_base) - 1 / ((1 << f_base) - 1));
}

//-- half-float conversions.
//-- to speed up we can use tables
// (really small tables with mantissa, exponent, offset)
//-- this might be required elsewhere, for now just do it
// algorithmetically

inline constexpr uint16_t float_to_half_i(uint32_t tmpu) noexcept
{
  // 1 : 8 : 23
  uint16_t sign = static_cast<uint16_t>((tmpu & 0x80000000) >> 31);
  uint16_t exponent = static_cast<uint16_t>((tmpu & 0x7F800000) >> 23);
  uint32_t significand = tmpu & 0x7FFFFF;

         //     NCNN_LOGE("%d %d %d", sign, exponent, significand);

         // 1 : 5 : 10
  uint16_t fp16;
  if (exponent == 0)
  {
    // zero or denormal, always underflow
    fp16 = (sign << 15) | (0x00 << 10) | 0x00;
  }
  else if (exponent == 0xFF)
  {
    // infinity or NaN
    fp16 = (sign << 15) | (0x1F << 10) | (significand ? 0x200 : 0x00);
  }
  else
  {
    // normalized
    short newexp = exponent + (-127 + 15);
    if (newexp >= 31)
    {
      // overflow, return infinity
      fp16 = (sign << 15) | (0x1F << 10) | 0x00;
    }
    else if (newexp <= 0)
    {
      // Some normal fp32 cannot be expressed as normal fp16
      fp16 = (sign << 15) | (0x00 << 10) | 0x00;
    }
    else
    {
      // normal fp16
      fp16 = (sign << 15) | static_cast<uint16_t>(newexp << 10) | static_cast<uint16_t>(significand >> 13);
    }
  }

  return fp16;
}

inline constexpr uint16_t float_to_half(float f) noexcept
{
  union
  {
    float    f;
    uint32_t i;
  } o;
  o.f = f;
  return float_to_half_i(o.i);
}

inline constexpr uint32_t half_to_float_i(uint16_t value) noexcept
{
  uint16_t sign = (value & 0x8000) >> 15;
  uint16_t exponent = (value & 0x7c00) >> 10;
  uint16_t significand = value & 0x03FF;
  uint32_t tmpu;
  if (exponent == 0)
  {
    if (significand == 0)
    {
      // zero
      tmpu = (sign << 31);
    }
    else
    {
      // denormal
      exponent = 0;
      // find non-zero bit
      while ((significand & 0x200) == 0)
      {
        significand <<= 1;
        exponent++;
      }
      significand <<= 1;
      significand &= 0x3FF;
      tmpu = (sign << 31) | ((-exponent + (-15 + 127)) << 23) | (significand << 13);
    }
  }
  else if (exponent == 0x1F)
  {
    // infinity or NaN
    tmpu = (sign << 31) | (0xFF << 23) | (significand << 13);
  }
  else
  {
    // normalized
    tmpu = (sign << 31) | ((exponent + (-15 + 127)) << 23) | (significand << 13);
  }

  return tmpu;
}

inline constexpr float half_to_float(uint16_t y) noexcept
{
  union
  {
    float    f;
    uint32_t i;
  } o;
  o.i = half_to_float_i(y);
  return o.f;
}

template <typename scalar_t>
inline constexpr scalar_t to_radians(scalar_t value) noexcept
{
  return static_cast<scalar_t>(k_degrees_to_radians_factor_d) * (value);
}

template <typename scalar_t>
inline constexpr scalar_t to_degrees(scalar_t value) noexcept
{
  return static_cast<scalar_t>(k_radians_to_degrees_factor_d) * (value);
}

/* Famous fast reciprocal std::sqrt */
inline constexpr float fast_recip_sqrt(float x) noexcept
{
  std::int32_t i;
  float        y, r;
  y = x * 0.5f;
  i = *(std::int32_t*)(&x);
  i = 0x5f3759df - (i >> 1);
  r = *(float*)(&i);
  r = r * (1.5f - r * r * y);
  return r;
}

/* sin of angle in the range of [0, pi/2]*/
inline constexpr float sin_of_ang_between_0_to_half_pi(float a) noexcept
{
  float s, t;
  s = a * a;
  t = -2.39e-08f;
  t *= s;
  t += 2.7526e-06f;
  t *= s;
  t += -1.98409e-04f;
  t *= s;
  t += 8.3333315e-03f;
  t *= s;
  t += -1.666666664e-01f;
  t *= s;
  t += 1.0f;
  t *= a;
  return t;
}

/* Arc tan when x and y are positives */
inline constexpr float arc_tan_positive_xy(float y, float x) noexcept
{
  float a, d, s, t;
  if (y > x)
  {
    a = -x / y;
    d = acl::k_pi / 2;
  }
  else
  {
    a = y / x;
    d = 0.0f;
  }
  s = a * a;
  t = 0.0028662257f;
  t *= s;
  t += -0.0161657367f;
  t *= s;
  t += 0.0429096138f;
  t *= s;
  t += -0.0752896400f;
  t *= s;
  t += 0.1065626393f;
  t *= s;
  t += -0.1420889944f;
  t *= s;
  t += 0.1999355085f;
  t *= s;
  t += -0.3333314528f;
  t *= s;
  t += 1.0f;
  t *= a;
  t += d;
  return t;
}

inline bool almost_equals_ulps(float i_a, float i_b, int max_ulps) noexcept
{
  assert(sizeof(float) == sizeof(int));
  if (i_a == i_b)
    return true;
  int a_int = *(int*)&i_a;
  // Make a_int lexicographically ordered as a twos-complement int
  if (a_int < 0)
    a_int = 0x80000000 - a_int;
  // Make b_int lexicographically ordered as a twos-complement int
  int b_int = *(int*)&i_b;
  if (b_int < 0)
    b_int = 0x80000000 - b_int;

  int int_diff = std::abs(a_int - b_int);
  if (int_diff <= max_ulps)
    return true;
  return false;
}

inline bool almost_equals_rel_or_abs(float i_a, float i_b, float max_diff, float max_rel_diff) noexcept
{
  float diff = std::abs(i_a - i_b);
  if (diff < max_diff)
    return true;
  i_a           = std::abs(i_a);
  i_b           = std::abs(i_b);
  float largest = i_b > i_a ? i_b : i_a;
  if (diff <= largest * max_rel_diff)
    return true;
  return false;
}

inline bool almost_equals_ulps(double i_a, double i_b, int max_ulps) noexcept
{
  if (i_a == i_b)
    return true;
  int64_t a_int = *(int64_t*)&i_a;
  // Make a_int lexicographically ordered as a twos-complement int
  if (a_int < 0)
    a_int = 0x8000000000000000 - a_int;
  // Make b_int lexicographically ordered as a twos-complement int
  int64_t b_int = *(int64_t*)&i_b;
  if (b_int < 0)
    b_int = 0x8000000000000000 - b_int;

  int64_t int_diff = std::abs(a_int - b_int);
  if (int_diff <= max_ulps)
    return true;
  return false;
}

inline bool almost_equals_rel_or_abs(double i_a, double i_b, double max_diff, double max_rel_diff) noexcept
{
  double diff = std::abs(i_a - i_b);
  if (diff < max_diff)
    return true;
  i_a            = std::abs(i_a);
  i_b            = std::abs(i_b);
  double largest = i_b > i_a ? i_b : i_a;
  if (diff <= largest * max_rel_diff)
    return true;
  return false;
}
template <typename type>
inline void clamp(type& clampwhat, type lowvalue, type hivalue) noexcept
{
  clampwhat = std::max(lowvalue, std::min(clampwhat, hivalue));
}

// Bit Utils
inline uint32_t log2_next_positive(uint32_t x)
{
#ifdef _MSC_VER
  return (32 - __lzcnt(x - 1));
#elif defined(__GNUC__) || defined(__clang__)
  return (32 - __builtin_clz(x - 1));
#else
  return bit_pos(next_pow2(x));
#endif
}

inline uint32_t log2_next(uint32_t x)
{
#ifdef _MSC_VER
  return x == 1 ? 0 : (32 - __lzcnt(x - 1));
#elif defined(__GNUC__) || defined(__clang__)
  return x == 1 ? 0 : (32 - __builtin_clz(x - 1));
#else
  return bit_pos(next_pow2(x));
#endif
}

} // namespace acl
