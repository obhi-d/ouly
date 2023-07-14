
#pragma once
#include "axis_angle.hpp"
#include "quad.hpp"
#include "vec3.hpp"
#include "vec4.hpp"
#include "vml_fcn.hpp"

namespace acl
{

template <typename scalar_t>
inline quat_t<scalar_t> make_identity_quaternion() noexcept
{
  return quat_t<scalar_t>(0.0f, 0.0f, 0.0f, 1.0f);
}

template <typename scalar_t>
inline quat_t<scalar_t> conjugate(quat_t<scalar_t> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {
    const __m128i k_sign = _mm_set_epi32(0x00000000, 0x80000000, 0x80000000, 0x80000000);
    return quat_t<scalar_t>(_mm_xor_ps(q, _mm_castsi128_ps(k_sign)));
  }
  else
  {
    return quat_t<scalar_t>(-(q[0]), -(q[1]), -(q[2]), (q[3]));
  }
}

template <typename scalar_t>
inline quat_t<scalar_t> make_quaternion(vec3_t<scalar_t> const& axis, scalar_t ang) noexcept
{
#ifndef NDEBUG
  float len = length(axis);
  assert(approx_equals(len, 1, acl::k_const_epsilon_med));
#endif
  auto sc = acl::sin_cos(ang * .5f);
  return quat_t<scalar_t>(sc.first * axis[0], sc.first * axis[1], sc.first * axis[2], sc.second);
}

template <typename scalar_t>
inline quat_t<scalar_t> from_axis_angle(axis_angle_t<scalar_t> const& ax) noexcept
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {
#ifndef NDEBUG
    float len = 1; //\todo Find length of axis
    assert(approx_equals(len, 1, acl::k_const_epsilon_med));
#endif
    auto N = axis(ax);
    N      = _mm_or_ps(N.v, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));

    auto sc  = acl::sin_cos(w(ax) * .5f);
    auto vsc = _mm_set_ps(sc.first, sc.first, sc.first, sc.second);
    return quat_t<scalar_t>(_mm_mul_ps(N, vsc));
  }
  else
  {
    auto sc = acl::sin_cos(w(ax) * .5f);
    return quat_t<scalar_t>(sc.first * ax[0], sc.first * ax[1], sc.first * ax[2], sc.second);
  }
}

template <typename scalar_t>
inline quat_t<scalar_t> make_quaternion(mat4_t<scalar_t> const& m) noexcept
{
  return from_mat3(*reinterpret_cast<const mat3_t<scalar_t>*>(&m));
}

template <typename scalar_t>
inline quat_t<scalar_t> make_quaternion(mat3_t<scalar_t> const& m) noexcept
{
  
  auto trace = m.e[0][0] + m.e[1][1] + m.e[2][2] + 1.0f;
  if (trace > 0.0f)
  {
    return set((m.e[1][2] - m.e[2][1]) / (2.0f * std::sqrt(trace)), (m.e[2][0] - m.e[0][2]) / (2.0f * std::sqrt(trace)),
               (m.e[0][1] - m.e[1][0]) / (2.0f * std::sqrt(trace)), std::sqrt(trace) / 2.0f);
  }
  int maxi    = 0;
  auto maxdiag = m.e[0][0];

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

  
  switch (maxi)
  {
  case 0: {
    auto s    = 2.0f * std::sqrt(1.0f + m.e[0][0] - m.e[1][1] - m.e[2][2]);
    auto invS = 1 / s;
    return set(0.25f * s, (m.e[0][1] + m.e[1][0]) * invS, (m.e[0][2] + m.e[2][0]) * invS,
               (m.e[1][2] - m.e[2][1]) * invS);
  }
  case 1: {
    auto s    = 2.0f * std::sqrt(1.0f + m.e[1][1] - m.e[0][0] - m.e[2][2]);
    auto invS = 1 / s;
    return set((m.e[0][1] + m.e[1][0]) * invS, 0.25f * s, (m.e[1][2] + m.e[2][1]) * invS,
               (m.e[2][0] - m.e[0][2]) * invS);
  }
  case 2:
  default: {
    auto s    = 2.0f * std::sqrt(1.0f + m.e[2][2] - m.e[0][0] - m.e[1][1]);
    auto invS = 1 / s;
    return set((m.e[0][2] + m.e[2][0]) * invS, (m.e[1][2] + m.e[2][1]) * invS, 0.25f * s,
               (m.e[0][1] - m.e[1][0]) * invS);
  }
  }
}

template <typename scalar_t>
inline quat_t<scalar_t> mul(quat_t<scalar_t> const& q1, quat_t<scalar_t> const& q2) noexcept
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {
    if constexpr (has_sse3)
    {
#define _mm_pshufd(r, i) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r), i))
      // @link
      // http://momchil-velikov.blogspot.com/2013/10/fast-sse-quternion-multiplication.html
      // Copy to SSE registers and use as few as possible for x86
      __m128 t0 = _mm_pshufd(q1.v, _MM_SHUFFLE(3, 3, 3, 3)); /* 1, 0.5 */
      __m128 t1 = _mm_pshufd(q2.v, _MM_SHUFFLE(2, 3, 0, 1)); /* 1, 0.5 */

      __m128 t3 = _mm_pshufd(q1.v, _MM_SHUFFLE(0, 0, 0, 0)); /* 1, 0.5 */
      __m128 t4 = _mm_pshufd(q2.v, _MM_SHUFFLE(1, 0, 3, 2)); /* 1, 0.5 */

      __m128 t5 = _mm_pshufd(q1.v, _MM_SHUFFLE(1, 1, 1, 1)); /* 1, 0.5 */
      __m128 t6 = _mm_pshufd(q2.v, _MM_SHUFFLE(2, 0, 3, 1)); /* 1, 0.5 */

      /* [d,d,d,d]*[z,w,x,y] = [dz,dw,dx,dy] */
      __m128 m0 = _mm_mul_ps(t0, t1); /* 5/4, 1 */

      /* [a,a,a,a]*[y,x,w,z] = [ay,ax,aw,az]*/
      __m128 m1 = _mm_mul_ps(t3, t4); /* 5/4, 1 */

      /* [b,b,b,b]*[z,x,w,y] = [bz,bx,bw,by]*/
      __m128 m2 = _mm_mul_ps(t5, t6); /* 5/4, 1 */

      /* [c,c,c,c]*[w,z,x,y] = [cw,cz,cx,cy] */
      __m128 t7 = _mm_pshufd(q1.v, _MM_SHUFFLE(2, 2, 2, 2)); /* 1, 0.5 */
      __m128 t8 = _mm_pshufd(q2.v, _MM_SHUFFLE(3, 2, 0, 1)); /* 1, 0.5 */

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
    }
    else
    {

      // Copy to SSE registers and use as few as possible for x86
      __m128 result;
      {
        result = mul(splat_w(q2), q1).v;
      }
      {
        const __m128i k_sign = _mm_set_epi32(0x80000000, 0x80000000, 0x00000000, 0x00000000);
        __m128        t      = _mm_pshufd(q1, _MM_SHUFFLE(0, 1, 2, 3));
        t                    = mul(splat_x(q2), t);
        t                    = _mm_xor_ps(t, _mm_castsi128_ps(k_sign));
        result               = _mm_add_ps(t, result);
      }
      {
        const __m128i k_sign = _mm_set_epi32(0x80000000, 0x00000000, 0x00000000, 0x80000000);
        __m128        t      = _mm_pshufd(q1, _MM_SHUFFLE(1, 0, 3, 2));
        t                    = mul(splat_y(q2), t);
        t                    = _mm_xor_ps(t, _mm_castsi128_ps(k_sign));
        result               = _mm_add_ps(t, result);
      }
      {
        const __m128i k_sign = _mm_set_epi32(0x80000000, 0x00000000, 0x80000000, 0x00000000);
        __m128        t      = _mm_pshufd(q1, _MM_SHUFFLE(2, 3, 0, 1));
        t                    = mul(splat_z(q2), t);
        t                    = _mm_xor_ps(t, _mm_castsi128_ps(k_sign));
        result               = _mm_add_ps(t, result);
      }
      return result;
    }
  }

  else
    return set((q2[3] * q1[0]) + (q2[0] * q1[3]) - (q2[1] * q1[2]) + (q2[2] * q1[1]),
               (q2[3] * q1[1]) + (q2[0] * q1[2]) + (q2[1] * q1[3]) - (q2[2] * q1[0]),
               (q2[3] * q1[2]) - (q2[0] * q1[1]) + (q2[1] * q1[0]) + (q2[2] * q1[3]),
               (q2[3] * q1[3]) - (q2[0] * q1[0]) - (q2[1] * q1[1]) - (q2[2] * q1[2]));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> rotate(vec3a_t<scalar_t> const& v, quat_t<scalar_t> const& q) noexcept
{
  // quad_t u = vec3a::from_vec4(q);
  vec3a_t<scalar_t> uv  = cross(*(vec3a_t<scalar_t> const*)&q, v);
  vec3a_t<scalar_t> uuv = cross(*(vec3a_t<scalar_t> const*)&q, uv);
  return add(add(v, mul(uv, splat_w(mul(quat_t<scalar_t>(2.0f), q)))), add(uuv, uuv));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> rotate_bounds_extends(vec3a_t<scalar_t> const& v, quat_t<scalar_t> const& rot)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {
    const __m128 fff0 = _mm_castsi128_ps(_mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));

    auto q0 = _mm_add_ps(rot.v, rot.v);
    auto q1 = _mm_mul_ps(rot.v, q0);

    auto v0 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 0, 0, 1));
    auto v1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 1, 2, 2));
    auto r0 = _mm_sub_ps(_mm_set_ps(0.0f, 1.0f, 1.0f, 1.0f), v0);
    r0      = _mm_sub_ps(r0, v1);

    v0 = _mm_shuffle_ps(rot.v, rot.v, _MM_SHUFFLE(3, 1, 0, 0));
    v1 = _mm_shuffle_ps(q0, q0, _MM_SHUFFLE(3, 2, 1, 2));
    v0 = _mm_mul_ps(v0, v1);

    v1      = _mm_shuffle_ps(rot.v, rot.v, _MM_SHUFFLE(3, 3, 3, 3));
    auto v2 = _mm_shuffle_ps(q0, q0, _MM_SHUFFLE(3, 0, 2, 1));
    v1      = _mm_mul_ps(v1, v2);

    auto r1 = _mm_add_ps(v0, v1);
    auto r2 = _mm_sub_ps(v0, v1);

    v0 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(1, 0, 2, 1));
    v0 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(1, 3, 2, 0));
    v1 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(2, 2, 0, 0));
    v1 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(2, 0, 2, 0));

    q1 = _mm_shuffle_ps(r0, v0, _MM_SHUFFLE(1, 0, 3, 0));
    q1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 3, 2, 0));

    auto t0 = abs(_mm_mul_ps(splat_x(v), q1));
    q1      = _mm_shuffle_ps(r0, v0, _MM_SHUFFLE(3, 2, 3, 1));
    q1      = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 3, 0, 2));
    auto t1 = abs(_mm_mul_ps(splat_y(v), q1));
    q1      = _mm_shuffle_ps(v1, r0, _MM_SHUFFLE(3, 2, 1, 0));
    auto t2 = abs(_mm_mul_ps(splat_z(v), q1));
    return make_vec3a(add(t0, add(t1, t2)));
  }
  else
  {
    float xx = rot[0] * rot[0];
    float yy = rot[1] * rot[1];
    float zz = rot[2] * rot[2];
    float xy = rot[0] * rot[1];
    float xz = rot[0] * rot[2];
    float yz = rot[1] * rot[2];
    float wx = rot[3] * rot[0];
    float wy = rot[3] * rot[1];
    float wz = rot[3] * rot[2];

    auto t0 = abs(quad_t<scalar_t>{v[0] * (1 - 2 * (yy + zz)), v[0] * (2 * (xy + wz)), v[0] * (2 * (xz - wy))});

    auto t1 = abs(quad_t<scalar_t>{v[1] * (2 * (xy - wz)), v[1] * (1 - 2 * (xx + zz)), v[1] * (2 * (yz + wx))});

    auto t2 = abs(quad_t<scalar_t>{v[2] * (2 * (xz + wy)), v[2] * (2 * (yz - wx)), v[2] * (1 - 2 * (xx + yy))});

    return make_vec3a(add(t0, add(t1, t2)));
  }
}

template <typename scalar_t>
inline quat_t<scalar_t> slerp(quat_t<scalar_t> const& from, quat_t<scalar_t> const& to, scalar_t t)
{
  scalar_t sinom, omega, scale0, scale1;
  auto     cosom     = dot(from, to);
  auto     abs_cosom = std::abs(cosom);
  if ((1.0f - abs_cosom) > acl::k_const_epsilon)
  {
    omega  = std::acos(abs_cosom);
    sinom  = 1.0f / std::sin(omega);
    scale0 = std::sin((1.0f - t) * omega) * sinom;
    scale1 = std::sin(t * omega) * sinom;
  }
  else
  {
    scale0 = 1.0f - t;
    scale1 = t;
  }

  scale1 = (cosom >= 0.0f) ? scale1 : -scale1;
  return add(mul(from, scale0), mul(to, scale1));
}

template <typename scalar_t>
inline quat_t<scalar_t> inverse(quat_t<scalar_t> const& q)
{
  return conjugate(q);
}

template <typename scalar_t>
inline quat_t<scalar_t> operator*(quat_t<scalar_t> const& q1, quat_t<scalar_t> const& q2) noexcept
{
  return mul(q1, q2);
}

template <typename scalar_t>
inline quat_t<scalar_t> operator*(vec3a_t<scalar_t> const& q1, quat_t<scalar_t> const& q2) noexcept
{
  return mul(q1, q2);
}

} // namespace acl
