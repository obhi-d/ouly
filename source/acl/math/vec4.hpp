#pragma once

#include "quad.hpp"

namespace acl
{

template <typename scalar_t>
inline vec4_t<scalar_t> mul(vec4_t<scalar_t> const& v, mat4_t<scalar_t> const& m)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {

    auto ret    = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    ret         = _mm_mul_ps(ret, m.r[0]);
    auto v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    v_temp      = _mm_mul_ps(v_temp, m.r[1]);
    ret         = _mm_add_ps(ret, v_temp);
    v_temp      = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
    v_temp      = _mm_mul_ps(v_temp, m.r[2]);
    ret         = _mm_add_ps(ret, v_temp);
    v_temp      = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));
    v_temp      = _mm_mul_ps(v_temp, m.r[3]);
    ret         = _mm_add_ps(ret, v_temp);
    return ret;
  }
  else
  {

    auto z = splat_z(v);
    auto y = splat_y(v);
    auto x = splat_x(v);
    auto w = splat_w(v);

    auto r = mul(w, m.r[3]);
    r      = madd(z, m.r[2], r);
    r      = madd(y, m.r[1], r);
    r      = madd(x, m.r[0], r);

    return r;
  }
}

template <typename scalar_t>
inline auto operator*(vec4_t<scalar_t> const& v, mat4_t<scalar_t> const& a) noexcept
{
  return mul(v, a);
}

} // namespace acl
