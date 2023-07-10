#pragma once
#include "quad.hpp"
#include "vec3a.hpp"

namespace acl
{
struct axis_angle : public quad
{
  using quad::set;
  static inline axis_angle_t set(vec3a::pref axis, float angle)
  {
    return quad::set_w(vec3a::normalize(axis), angle);
  }
  static inline axis_angle_t set_assume_normalized(vec3a::pref axis, float angle)
  {
    return quad::set_w(axis, angle);
  }
  static inline vec3a_t axis(quad::pref q)
  {
    return vec3a::from_vec4(q);
  }
  static inline float angle(quad::pref q)
  {
    return quad::w(q);
  }
  static inline quad_t vangle(quad::pref _)
  {
#if VML_USE_SSE_AVX
    return _mm_shuffle_ps(_, _, _MM_SHUFFLE(0, 0, 0, 3));
#else
    return {quad::w(_), 0, 0, 0};
#endif
  }
};
} // namespace acl
