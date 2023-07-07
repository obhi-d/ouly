#pragma once

#include "vec3a.hpp"

namespace acl
{

struct plane : public quad
{
  using quad::cref;
  using quad::pref;
  using quad::ref;
  using quad::row_type;
  using quad::scalar_type;
  using quad::type;

  using quad::set;
  static inline type        set(vec3a::pref normal, float d);
  static inline type        normalize(pref p);
  static inline scalar_type dot(pref p, vec3a::pref v);
  static inline vec3a_t     vdot(pref p, vec3a::pref v);
  static inline scalar_type dot_with_normal(pref p, vec3a::pref v);
  static inline vec3a_t     abs_normal(pref p);
  static inline vec3a_t     get_normal(pref p);
};

inline plane::type acl::plane::set(vec3a::pref i_normal, float d)
{
  return quad::set_w(i_normal, d);
}

inline plane::type acl::plane::normalize(pref i_plane)
{
#if VML_USE_SSE_AVX
#if VML_USE_SSE_LEVEL >= 4
  type q = _mm_dp_ps(i_plane, i_plane, 0x7F);
  // get the reciprocal
  q = _mm_sqrt_ps(q);
  return _mm_div_ps(i_plane, q);
#elif VML_USE_SSE_LEVEL >= 3
  __m128 v    = _mm_and_ps(_mm_mul_ps(i_plane, i_plane), clear_w_mask());
  __m128 shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
  __m128 sums = _mm_add_ps(v, shuf);
  shuf        = _mm_movehl_ps(shuf, sums); // high half -> low half
  sums        = _mm_add_ss(sums, shuf);
  sums        = _mm_sqrt_ss(sums);
  sums        = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(0, 0, 0, 0));
  return _mm_div_ps(i_plane, sums);
#else
  // Perform the dot product
  type q = _mm_mul_ps(i_plane, i_plane);
  // x=dot[1], y=dot[2]
  type temp = _mm_shuffle_ps(q, q, _MM_SHUFFLE(2, 1, 2, 1));
  // Result[0] = x+y
  q = _mm_add_ss(q, temp);
  // x=dot[2]
  temp = _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1, 1, 1, 1));
  // Result[0] = (x+y)+z
  q = _mm_add_ss(q, temp);
  // Splat x
  q = _mm_sqrt_ss(q);
  q = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 0, 0, 0));
  return _mm_div_ps(i_plane, q);
#endif
#else
  float val = acl::sqrt(i_plane[0] * i_plane[0] + i_plane[1] * i_plane[1] + i_plane[2] * i_plane[2]);
  assert(val > acl::k_const_epsilon);
  return mul(i_plane, 1 / val);
#endif
}

inline vec3a_t plane::vdot(pref p, vec3a::pref v)
{
#if VML_USE_SSE_AVX
  return quad::vdot(p, _mm_or_ps(v, xyz0_w1()));
#else
  return set(dot(p, v), 0, 0, 0);
#endif
}

inline float plane::dot(pref p, pref v)
{
#if VML_USE_SSE_AVX
  return quad::x(vdot(p, v));
#else
  return p[0] * v[0] + p[1] * v[1] + p[2] * v[2] + p[3];
#endif
}

inline plane::scalar_type plane::dot_with_normal(pref p, pref vec3a)
{
  return vec3a::dot(p, vec3a);
}

inline vec3a_t plane::abs_normal(pref p)
{
#if VML_USE_SSE_AVX
  return vec3a::abs(_mm_and_ps(p, clear_w_mask()));
#else
  return vec3a::abs(vec3a::set(p[0], p[1], p[2], 0));
#endif
}

inline vec3a_t plane::get_normal(pref p)
{
#if VML_USE_SSE_AVX
  return _mm_and_ps(p, clear_w_mask());
#else
  return vec3a::set(p[0], p[1], p[2], 0);
#endif
}

} // namespace acl
