#pragma once

#include "vec3a.hpp"

namespace acl
{

template <typename scalar_t>
inline plane_t<scalar_t> make_plane(vec3a_t<scalar_t> const& i_normal, float d)
{
  return set_w(i_normal, d);
}

template <typename scalar_t>
inline plane_t<scalar_t> normalize(plane_t<scalar_t> const& i_plane)
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    if constexpr (has_sse41)
    {
      auto q = _mm_dp_ps(i_plane, i_plane, 0x7F);
      // get the reciprocal
      q = _mm_sqrt_ps(q);
      return _mm_div_ps(i_plane, q);
    }
    else if constexpr (has_sse3)
    {
      __m128 v    = _mm_and_ps(_mm_mul_ps(i_plane, i_plane), clear_w_mask());
      __m128 shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
      __m128 sums = _mm_add_ps(v, shuf);
      shuf        = _mm_movehl_ps(shuf, sums); // high half -> low half
      sums        = _mm_add_ss(sums, shuf);
      sums        = _mm_sqrt_ss(sums);
      sums        = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(0, 0, 0, 0));
      return _mm_div_ps(i_plane, sums);
    }
    else
    {
      // Perform the dot product
      auto q = _mm_mul_ps(i_plane, i_plane);
      // x=dot[1], y=dot[2]
      auto temp = _mm_shuffle_ps(q, q, _MM_SHUFFLE(2, 1, 2, 1));
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
    }
  }
  else
  {
    float val = std::sqrt(i_plane[0] * i_plane[0] + i_plane[1] * i_plane[1] + i_plane[2] * i_plane[2]);
    assert(val > acl::k_const_epsilon);
    return mul(i_plane, 1 / val);
  }
}

template <typename scalar_t>
inline vec3a_t<scalar_t> vdot(plane_t<scalar_t> const& p, vec3a_t<scalar_t> const& v)
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
    return vdot(p, _mm_or_ps(v, xyz0_w1()));
  else
    return set(dot(p, v), 0, 0, 0);
}

template <typename scalar_t>
inline float dot(plane_t<scalar_t> const& p, vec3a_t<scalar_t> const& v)
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
    return x(vdot(p, v));
  else
    return p[0] * v[0] + p[1] * v[1] + p[2] * v[2] + p[3];
}

template <typename scalar_t>
inline scalar_t dot_with_normal(plane_t<scalar_t> const& p, vec3a_t<scalar_t> const& v)
{
  return dot(make_vec3a(p), v);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> abs_normal(plane_t<scalar_t> const& p)
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
    return abs(_mm_and_ps(p, clear_w_mask()));
  else
    return abs(vec3a_t<scalar_t>(p[0], p[1], p[2], 0));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> get_normal(plane_t<scalar_t> const& p)
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
    return _mm_and_ps(p, clear_w_mask());
  else
    return vec3a_t<scalar_t>(p[0], p[1], p[2], 0);
}

} // namespace acl
