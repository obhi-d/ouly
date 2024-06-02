
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

inline constexpr std::uint16_t float_to_half_i(std::uint32_t i) noexcept
{
  // can use SSE here, but lets
  // do it naive way.
  int s = (i >> 16) & 0x00008000;
  int e = ((i >> 23) & 0x000000ff) - (127 - 15);
  int m = i & 0x007fffff;
  if (e <= 0)
  {
    if (e < -10)
      return 0;
    m = (m | 0x00800000) >> (1 - e);

    return s | (m >> 13);
  }
  else if (e == 0xff - (127 - 15))
  {
    if (m == 0) // Inf
      return s | 0x7c00;
    else // NAN
    {
      m >>= 13;
      return s | 0x7c00 | m | (m == 0);
    }
  }
  else
  {
    if (e > 30) // Overflow
      return s | 0x7c00;
    return s | (e << 10) | (m >> 13);
  }
}

inline constexpr std::uint16_t float_to_half(float f) noexcept
{
  return float_to_half_i(*(std::uint32_t*)&f);
}

inline constexpr std::uint32_t half_to_float_i(std::uint16_t y) noexcept
{
  // can use SSE here, but lets
  // do it naive way.
  int s = (y >> 15) & 0x00000001;
  int e = (y >> 10) & 0x0000001f;
  int m = y & 0x000003ff;

  if (e == 0)
  {
    if (m == 0) // Plus or minus zero
    {
      return s << 31;
    }
    else // Denormalized number -- renormalize it
    {
      while (!(m & 0x00000400))
      {
        m <<= 1;
        e -= 1;
      }

      e += 1;
      m &= ~0x00000400;
    }
  }
  else if (e == 31)
  {
    if (m == 0) // Inf
    {
      return (s << 31) | 0x7f800000;
    }
    else // NaN
    {
      return (s << 31) | 0x7f800000 | (m << 13);
    }
  }

  e = e + (127 - 15);
  m = m << 13;
  return (s << 31) | (e << 23) | m;
}

inline constexpr float half_to_float(std::uint16_t y) noexcept
{
  union
  {
    float         f;
    std::uint32_t i;
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
