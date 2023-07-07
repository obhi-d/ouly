#pragma once

#include "quad.hpp"

namespace acl
{

struct vec4 : public quad
{
  using quad::cref;
  using quad::pref;
  using quad::ref;
  using quad::row_type;
  using quad::scalar_type;
  using quad::type;

  using quad::mul;

  static inline type mul(pref q1, mat4_t const& m);
};

inline vec4::type vec4::mul(pref v, mat4_t const& m)
{
#if VML_USE_SSE_AVX
  quad_t ret, v_temp;
  ret    = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
  ret    = _mm_mul_ps(ret, m.r[0]);
  v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
  v_temp = _mm_mul_ps(v_temp, m.r[1]);
  ret    = _mm_add_ps(ret, v_temp);
  v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
  v_temp = _mm_mul_ps(v_temp, m.r[2]);
  ret    = _mm_add_ps(ret, v_temp);
  v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));
  v_temp = _mm_mul_ps(v_temp, m.r[3]);
  ret    = _mm_add_ps(ret, v_temp);
  return ret;
#else
  quad_t r, x, y, z, w;

  z = splat_z(v);
  y = splat_y(v);
  x = splat_x(v);
  w = splat_w(v);

  r = mul(w, m.r[3]);
  r = madd(z, m.r[2], r);
  r = madd(y, m.r[1], r);
  r = madd(x, m.r[0], r);

  return r;
#endif
}

} // namespace acl