#pragma once
#include "vec3a.hpp"

namespace acl
{

struct sphere : public quad
{
  using quad::set;
  inline static sphere_t set(vec3a::pref _, float radius);
  inline static quad_t   vradius(quad::pref _);
  inline static float    radius(quad::pref _);
  inline static vec3a_t  center(quad::pref _);
  inline static float    max_radius(vec3a_t const&);
  inline static sphere_t scale_radius(sphere::pref, float scale);
};
inline sphere_t sphere::set(vec3a::pref _, float radius)
{
  return quad::set_w(_, radius);
}
inline float sphere::radius(quad::pref _)
{
  return quad::w(_);
}
inline quad_t sphere::vradius(quad::pref _)
{
#if VML_USE_SSE_AVX
  return _mm_shuffle_ps(_, _, _MM_SHUFFLE(0, 0, 0, 3));
#else
  return {quad::w(_), 0, 0, 0};
#endif
}
inline vec3a_t sphere::center(quad::pref _)
{
  return vec3a::from_vec4(_);
}
inline float sphere::max_radius(vec3a_t const& _)
{
  return vec3a::length(_);
}
inline sphere_t sphere::scale_radius(sphere::pref p, float scale)
{
  return quad::mul(p, quat::set(1.0f, 1.0f, 1.0f, scale));
}
} // namespace acl