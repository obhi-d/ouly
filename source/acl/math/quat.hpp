
#pragma once
#include "axis_angle.hpp"
#include "quad.hpp"
#include "vec3.hpp"
#include "vec4.hpp"
#include "vml_fcn.hpp"

namespace acl
{

struct quat : public quad
{
  using quad::cref;
  using quad::pref;
  using quad::ref;
  using quad::row_type;
  using quad::scalar_type;
  using quad::type;

  using quad::normalize;

  static inline type    identity();
  static inline type    conjugate(pref);
  static inline type    from_axis_angle(vec3_t const& v, scalar_type ang);
  static inline type    from_axis_angle(axis_angle_t const& v);
  static inline type    from_mat4(mat4_t const& m);
  static inline type    from_mat3(mat3_t const& m);
  static inline type    mul(pref q1, pref q2);
  static inline vec3a_t transform(pref q1, vec3a_t const& q2);
  static inline vec3a_t transform_bounds_extends(pref q1, vec3a_t const& extends);
  static inline type    slerp(pref q1, pref q2, scalar_type t);
  static inline type    inverse(pref q);
};

inline quat::type quat::identity()
{
  return set(0.0f, 0.0f, 0.0f, 1.0f);
}
inline quat::type quat::conjugate(quat::pref q)
{
#if VML_USE_SSE_AVX
  const __m128i k_sign = _mm_set_epi32(0x00000000, 0x80000000, 0x80000000, 0x80000000);
  return _mm_xor_ps(q, acl_cast_i128_ps(k_sign));
#else
  return quad::set(-(q[0]), -(q[1]), -(q[2]), (q[3]));
#endif
}
inline quat::type quat::from_axis_angle(vec3_t const& axis, scalar_type ang)
{
#ifndef NDEBUG
  float len = vec3::length(axis);
  assert(approx_equals(len, 1, acl::k_const_epsilon_med));
#endif
  auto sc = acl::sin_cos(ang * .5f);
  return set(sc.first * axis[0], sc.first * axis[1], sc.first * axis[2], sc.second);
}
inline quat::type quat::from_axis_angle(axis_angle_t const& axis)
{
#if VML_USE_SSE_AVX
#ifndef NDEBUG
  float len = 1; //\todo Find length of axis
  assert(approx_equals(len, 1, acl::k_const_epsilon_med));
#endif
  quad_t N = axis_angle::axis(axis);
  N        = _mm_or_ps(N, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));

  auto   sc          = acl::sin_cos(quad::w(axis) * .5f);
  quad_t vSineCosine = quad::set(sc.first, sc.first, sc.first, sc.second);
  return _mm_mul_ps(N, vSineCosine);
#else
  auto sc = acl::sin_cos(quad::w(axis) * .5f);
  return set(sc.first * axis[0], sc.first * axis[1], sc.first * axis[2], sc.second);
#endif
}
inline quat::type quat::from_mat4(mat4_t const& m)
{
  return from_mat3(*reinterpret_cast<const mat3_t*>(&m));
}
inline quat::type quat::from_mat3(mat3_t const& m)
{
  int   maxi;
  float maxdiag, trace;
  trace = m.e[0][0] + m.e[1][1] + m.e[2][2] + 1.0f;
  if (trace > 0.0f)
  {
    return set((m.e[1][2] - m.e[2][1]) / (2.0f * sqrt(trace)), (m.e[2][0] - m.e[0][2]) / (2.0f * sqrt(trace)),
               (m.e[0][1] - m.e[1][0]) / (2.0f * sqrt(trace)), sqrt(trace) / 2.0f);
  }
  maxi    = 0;
  maxdiag = m.e[0][0];

  if (m.e[1][1] > maxdiag)
  {
    maxdiag = m.e[1][1];
    maxi    = 1;
  }

  if (m.e[2][2] > maxdiag)
  {
    maxdiag = m.e[2][2];
    maxi    = 2;
  }

  float s, invS;
  switch (maxi)
  {
  case 0:
    s    = 2.0f * sqrt(1.0f + m.e[0][0] - m.e[1][1] - m.e[2][2]);
    invS = 1 / s;
    return set(0.25f * s, (m.e[0][1] + m.e[1][0]) * invS, (m.e[0][2] + m.e[2][0]) * invS,
               (m.e[1][2] - m.e[2][1]) * invS);

  case 1:
    s    = 2.0f * sqrt(1.0f + m.e[1][1] - m.e[0][0] - m.e[2][2]);
    invS = 1 / s;
    return set((m.e[0][1] + m.e[1][0]) * invS, 0.25f * s, (m.e[1][2] + m.e[2][1]) * invS,
               (m.e[2][0] - m.e[0][2]) * invS);
  case 2:
  default:
    s    = 2.0f * sqrt(1.0f + m.e[2][2] - m.e[0][0] - m.e[1][1]);
    invS = 1 / s;
    return set((m.e[0][2] + m.e[2][0]) * invS, (m.e[1][2] + m.e[2][1]) * invS, 0.25f * s,
               (m.e[0][1] - m.e[1][0]) * invS);
  }
}
inline quat::type quat::mul(pref q1, pref q2)
{
#if VML_USE_SSE_AVX
#if VML_USE_SSE_LEVEL >= 3
#define _mm_pshufd(r, i) acl_cast_i128_ps(_mm_shuffle_epi32(acl_cast_ps_i128(r), i))
  // @link
  // http://momchil-velikov.blogspot.com/2013/10/fast-sse-quternion-multiplication.html
  // Copy to SSE registers and use as few as possible for x86
  __m128 t0 = _mm_pshufd(q1, _MM_SHUFFLE(3, 3, 3, 3)); /* 1, 0.5 */
  __m128 t1 = _mm_pshufd(q2, _MM_SHUFFLE(2, 3, 0, 1)); /* 1, 0.5 */

  __m128 t3 = _mm_pshufd(q1, _MM_SHUFFLE(0, 0, 0, 0)); /* 1, 0.5 */
  __m128 t4 = _mm_pshufd(q2, _MM_SHUFFLE(1, 0, 3, 2)); /* 1, 0.5 */

  __m128 t5 = _mm_pshufd(q1, _MM_SHUFFLE(1, 1, 1, 1)); /* 1, 0.5 */
  __m128 t6 = _mm_pshufd(q2, _MM_SHUFFLE(2, 0, 3, 1)); /* 1, 0.5 */

  /* [d,d,d,d]*[z,w,x,y] = [dz,dw,dx,dy] */
  __m128 m0 = _mm_mul_ps(t0, t1); /* 5/4, 1 */

  /* [a,a,a,a]*[y,x,w,z] = [ay,ax,aw,az]*/
  __m128 m1 = _mm_mul_ps(t3, t4); /* 5/4, 1 */

  /* [b,b,b,b]*[z,x,w,y] = [bz,bx,bw,by]*/
  __m128 m2 = _mm_mul_ps(t5, t6); /* 5/4, 1 */

  /* [c,c,c,c]*[w,z,x,y] = [cw,cz,cx,cy] */
  __m128 t7 = _mm_pshufd(q1, _MM_SHUFFLE(2, 2, 2, 2)); /* 1, 0.5 */
  __m128 t8 = _mm_pshufd(q2, _MM_SHUFFLE(3, 2, 0, 1)); /* 1, 0.5 */

  __m128 m3 = _mm_mul_ps(t7, t8); /* 5/4, 1 */

  /* 1 */
  /* [dz,dw,dx,dy]+-[ay,ax,aw,az] = [dz+ay,dw-ax,dx+aw,dy-az] */
  __m128 e = _mm_addsub_ps(m0, m1); /* 3, 1 */

  /* 2 */
  /* [dx+aw,dz+ay,dy-az,dw-ax] */
  e = _mm_pshufd(e, _MM_SHUFFLE(1, 3, 0, 2)); /* 1, 0.5 */

  /* [dx+aw,dz+ay,dy-az,dw-ax]+-[bz,bx,bw,by] =
   * [dx+aw+bz,dz+ay-bx,dy-az+bw,dw-ax-by]*/
  e = _mm_addsub_ps(e, m2); /* 3, 1 */

  /* 2 */
  /* [dz+ay-bx,dw-ax-by,dy-az+bw,dx+aw+bz] */
  e = _mm_pshufd(e, _MM_SHUFFLE(2, 0, 1, 3)); /* 1, 0.5 */

  /* [dz+ay-bx,dw-ax-by,dy-az+bw,dx+aw+bz]+-[cw,cz,cx,cy]
     = [dz+ay-bx+cw,dw-ax-by-cz,dy-az+bw+cx,dx+aw+bz-cy] */
  e = _mm_addsub_ps(e, m3); /* 3, 1 */

  /* 2 */
  /* [dw-ax-by-cz,dz+ay-bx+cw,dy-az+bw+cx,dx+aw+bz-cy] */
  e = _mm_pshufd(e, _MM_SHUFFLE(2, 3, 1, 0)); /* 1, 0.5 */
  return e;
#else
#define _mm_pshufd(r, i) acl_cast_i128_ps(_mm_shuffle_epi32(acl_cast_ps_i128(r), i))

  // Copy to SSE registers and use as few as possible for x86
  __m128 result;
  {
    result = acl::quad::mul(acl::quad::splat_w(q2), q1);
  }
  {
    const __m128i k_sign = _mm_set_epi32(0x80000000, 0x80000000, 0x00000000, 0x00000000);
    __m128        t      = _mm_pshufd(q1, _MM_SHUFFLE(0, 1, 2, 3));
    t                    = acl::quad::mul(acl::quad::splat_x(q2), t);
    t                    = _mm_xor_ps(t, acl_cast_i128_ps(k_sign));
    result               = acl::quad::add(t, result);
  }
  {
    const __m128i k_sign = _mm_set_epi32(0x80000000, 0x00000000, 0x00000000, 0x80000000);
    __m128        t      = _mm_pshufd(q1, _MM_SHUFFLE(1, 0, 3, 2));
    t                    = acl::quad::mul(acl::quad::splat_y(q2), t);
    t                    = _mm_xor_ps(t, acl_cast_i128_ps(k_sign));
    result               = acl::quad::add(t, result);
  }
  {
    const __m128i k_sign = _mm_set_epi32(0x80000000, 0x00000000, 0x80000000, 0x00000000);
    __m128        t      = _mm_pshufd(q1, _MM_SHUFFLE(2, 3, 0, 1));
    t                    = acl::quad::mul(acl::quad::splat_z(q2), t);
    t                    = _mm_xor_ps(t, acl_cast_i128_ps(k_sign));
    result               = acl::quad::add(t, result);
  }
  return result;
#endif
#else
  return set((q2[3] * q1[0]) + (q2[0] * q1[3]) - (q2[1] * q1[2]) + (q2[2] * q1[1]),
             (q2[3] * q1[1]) + (q2[0] * q1[2]) + (q2[1] * q1[3]) - (q2[2] * q1[0]),
             (q2[3] * q1[2]) - (q2[0] * q1[1]) + (q2[1] * q1[0]) + (q2[2] * q1[3]),
             (q2[3] * q1[3]) - (q2[0] * q1[0]) - (q2[1] * q1[1]) - (q2[2] * q1[2]));
#endif
}
inline vec3a_t quat::transform(pref q, vec3a_t const& v)
{
// quad_t u = vec3a::from_vec4(q);
#ifdef VML_QUAT_MUL_BY_CONJUGATE
  quad_t       qb  = conjugate(q);
  const quad_t two = set(2.0f);
  return vec3a::from_vec4(quad::add(quad::add(quad::mul(splat_x(quad::mul_x(two, quad::vdot(q, v))), q),
                                              quad::mul(quad::splat_w(quad::mul(q, two)), vec3a::cross(q, v))),
                                    quad::mul(quad::splat_x(quad::vdot(qb, qb)), q)));
#else
  vec3a_t uv  = vec3a::cross(q, v);
  vec3a_t uuv = vec3a::cross(q, uv);
  return vec3a::add(vec3a::add(v, quad::mul(uv, quad::splat_w(quad::mul(quad::set(2.0f), q)))), vec3a::add(uuv, uuv));
#endif
}
inline vec3a_t quat::transform_bounds_extends(pref rot, vec3a_t const& v)
{
#if VML_USE_SSE_AVX
  const __m128 fff0 = acl_cast_i128_ps(_mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));

  quad_t r0, r1, r2;
  quad_t q0, q1;
  quad_t v0, v1, v2;

  q0 = _mm_add_ps(rot, rot);
  q1 = _mm_mul_ps(rot, q0);

  v0 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 0, 0, 1));
  v1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 1, 2, 2));
  r0 = _mm_sub_ps(_mm_set_ps(0.0f, 1.0f, 1.0f, 1.0f), v0);
  r0 = _mm_sub_ps(r0, v1);

  v0 = _mm_shuffle_ps(rot, rot, _MM_SHUFFLE(3, 1, 0, 0));
  v1 = _mm_shuffle_ps(q0, q0, _MM_SHUFFLE(3, 2, 1, 2));
  v0 = _mm_mul_ps(v0, v1);

  v1 = _mm_shuffle_ps(rot, rot, _MM_SHUFFLE(3, 3, 3, 3));
  v2 = _mm_shuffle_ps(q0, q0, _MM_SHUFFLE(3, 0, 2, 1));
  v1 = _mm_mul_ps(v1, v2);

  r1 = _mm_add_ps(v0, v1);
  r2 = _mm_sub_ps(v0, v1);

  v0 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(1, 0, 2, 1));
  v0 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(1, 3, 2, 0));
  v1 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(2, 2, 0, 0));
  v1 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(2, 0, 2, 0));

  q1 = _mm_shuffle_ps(r0, v0, _MM_SHUFFLE(1, 0, 3, 0));
  q1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 3, 2, 0));

  __m128 t0 = quad::abs(_mm_mul_ps(quad::splat_x(v), q1));
  q1        = _mm_shuffle_ps(r0, v0, _MM_SHUFFLE(3, 2, 3, 1));
  q1        = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 3, 0, 2));
  __m128 t1 = quad::abs(_mm_mul_ps(quad::splat_y(v), q1));
  q1        = _mm_shuffle_ps(v1, r0, _MM_SHUFFLE(3, 2, 1, 0));
  __m128 t2 = quad::abs(_mm_mul_ps(quad::splat_z(v), q1));
  return vec3a::from_vec4(quad::add(t0, quad::add(t1, t2)));
#else
  float xx = rot[0] * rot[0];
  float yy = rot[1] * rot[1];
  float zz = rot[2] * rot[2];
  float xy = rot[0] * rot[1];
  float xz = rot[0] * rot[2];
  float yz = rot[1] * rot[2];
  float wx = rot[3] * rot[0];
  float wy = rot[3] * rot[1];
  float wz = rot[3] * rot[2];

  quad_t t0 = quad::abs(quad_t{v[0] * (1 - 2 * (yy + zz)), v[0] * (2 * (xy + wz)), v[0] * (2 * (xz - wy))});

  quad_t t1 = quad::abs(quad_t{v[1] * (2 * (xy - wz)), v[1] * (1 - 2 * (xx + zz)), v[1] * (2 * (yz + wx))});

  quad_t t2 = quad::abs(quad_t{v[2] * (2 * (xz + wy)), v[2] * (2 * (yz - wx)), v[2] * (1 - 2 * (xx + yy))});

  return vec3a::from_vec4(quad::add(t0, quad::add(t1, t2)));
#endif
}
inline quat::type quat::slerp(pref from, pref to, scalar_type t)
{
  float cosom, abs_cosom, sinom, omega, scale0, scale1;
  cosom     = vec4::dot(from, to);
  abs_cosom = acl::abs(cosom);
  if ((1.0f - abs_cosom) > acl::k_const_epsilon)
  {
    omega  = acl::arc_cos(abs_cosom);
    sinom  = 1.0f / acl::sin(omega);
    scale0 = acl::sin((1.0f - t) * omega) * sinom;
    scale1 = acl::sin(t * omega) * sinom;
  }
  else
  {
    scale0 = 1.0f - t;
    scale1 = t;
  }

  scale1 = (cosom >= 0.0f) ? scale1 : -scale1;
  return quad::add(quad::mul(from, scale0), quad::mul(to, scale1));
}
inline quat::type quat::inverse(pref q)
{
  return conjugate(q);
}

} // namespace acl