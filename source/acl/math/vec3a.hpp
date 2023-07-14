#pragma once
#include "quad.hpp"

namespace acl
{

template <typename scalar_t>
inline vec3a_t<scalar_t> make_vec3a(scalar_t x, scalar_t y, scalar_t z) noexcept
{
  return vec3a_t<scalar_t>(x, y, z, 0.0f);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> make_vec3a(scalar_t p) noexcept
{
  return vec3a_t<scalar_t>(p, p, p, 0.0f);
}

template <typename scalar_t, typename stag>
inline vec3a_t<scalar_t> make_vec3a(quad_t<scalar_t, stag> const& p)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return vec3a_t<scalar_t>(_mm_and_ps(p, clear_w_mask()));
  else
    return set_w(p, 0);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> normalize(vec3a_t<scalar_t> const& vec)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {
    if constexpr (has_sse41)
    {
      auto q = _mm_dp_ps(vec.v, vec.v, 0x7F);
      // get the reciprocal
      q = _mm_sqrt_ps(q);
      return vec3a_t<scalar_t>{_mm_div_ps(vec.v, q)};
    }
    else if (has_sse3)
    {
      auto v    = _mm_mul_ps(vec, vec);
      auto shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
      auto sums = _mm_add_ps(v, shuf);
      shuf      = _mm_movehl_ps(shuf, sums); // high half -> low half
      sums      = _mm_add_ss(sums, shuf);
      sums      = _mm_sqrt_ss(sums);
      sums      = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(0, 0, 0, 0));
      return vec3a_t<scalar_t>{_mm_div_ps(vec, sums)};
    }
    else
    {

      // Perform the dot product
      auto q = _mm_mul_ps(vec.v, vec.v);
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
      return vec3a_t<scalar_t>{_mm_div_ps(vec.v, q)};
    }
  }
  else
  {
    float val = std::sqrt(dot(vec, vec));
    assert(val > acl::k_const_epsilon);
    return mul(vec, 1 / val);
  }
}

template <typename scalar_t>
inline scalar_t dot(vec3a_t<scalar_t> const& q1, vec3a_t<scalar_t> const& q2)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return x(vdot(q1, q2));
  else
    return (q1[0] * q2[0]) + (q1[1] * q2[1]) + (q1[2] * q2[2]);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> vdot(vec3a_t<scalar_t> const& vec1, vec3a_t<scalar_t> const& vec2)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {
    if constexpr (has_sse41)
      return _mm_dp_ps(vec1.v, vec2.v, 0x7F);
    else if constexpr (has_sse3)
    {
      auto v    = _mm_mul_ps(vec1.v, vec2.v);
      auto shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
      auto sums = _mm_add_ps(v, shuf);
      shuf      = _mm_movehl_ps(shuf, sums); // high half -> low half
      sums      = _mm_add_ss(sums, shuf);
      return vec3a_t<scalar_t>{sums};
    }
    else
    {
      auto v    = _mm_mul_ps(vec1.v, vec2.v);
      auto shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
      auto sums = _mm_add_ps(v, shuf);                           // sums = [ D+C C+D | B+A A+B ]
      shuf      = _mm_movehl_ps(shuf, sums);                     //  [   C   D | D+C C+D ]  // let the
                                                                 //  compiler avoid a mov by reusing shuf
      sums = _mm_add_ss(sums, shuf);
      return vec3a_t<scalar_t>{sums};
    }
  }
  else
  {
    return set(dot(vec1, vec2), 0, 0);
  }
}

template <typename scalar_t>
inline vec3a_t<scalar_t> cross(vec3a_t<scalar_t> const& vec1, vec3a_t<scalar_t> const& vec2)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {
    // y1,z1,x1,w1
    auto vTemp1 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(3, 0, 2, 1));
    // z2,x2,y2,w2
    auto vTemp2 = _mm_shuffle_ps(vec2, vec2, _MM_SHUFFLE(3, 1, 0, 2));
    // Perform the left
    // operation
    auto vResult = _mm_mul_ps(vTemp1, vTemp2);
    // z1,x1,y1,w1
    vTemp1 = _mm_shuffle_ps(vTemp1, vTemp1, _MM_SHUFFLE(3, 0, 2, 1));
    // y2,z2,x2,w2
    vTemp2 = _mm_shuffle_ps(vTemp2, vTemp2, _MM_SHUFFLE(3, 1, 0, 2));
    // Perform the right
    // operation
    vTemp1 = _mm_mul_ps(vTemp1, vTemp2);
    // Subract the right from
    // left, and return answer
    vResult = _mm_sub_ps(vResult, vTemp1);
    // set w to zero
    return vec3a_t<scalar_t>{_mm_and_ps(vResult, clear_w_mask())};
  }
  else
    return set(vec1[1] * vec2[2] - vec1[2] * vec2[1], vec1[2] * vec2[0] - vec1[0] * vec2[2],
               vec1[0] * vec2[1] - vec1[1] * vec2[0]);
}

template <typename scalar_t>
inline bool greater_all(vec3a_t<scalar_t> const& q1, vec3a_t<scalar_t> const& q2)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return ((_mm_movemask_ps(_mm_cmpgt_ps(q1, q2)) & 0x7) == 0x7);
  else
    return (q1[0] > q2[0] && q1[1] > q2[1] && q1[2] > q2[2]) != 0;
}

template <typename scalar_t>
inline bool greater_any(vec3a_t<scalar_t> const& q1, vec3a_t<scalar_t> const& q2)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return ((_mm_movemask_ps(_mm_cmpgt_ps(q1, q2))) & 0x7) != 0;
  else
    return (q1[0] > q2[0] || q1[1] > q2[1] || q1[2] > q2[2]) != 0;
}

template <typename scalar_t>
inline bool lesser_all(vec3a_t<scalar_t> const& a, vec3a_t<scalar_t> const& b)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return ((_mm_movemask_ps(_mm_cmplt_ps(a, b)) & 0x7) == 0x7);
  else
    return a[0] < b[0] && a[1] < b[1] && a[2] < b[2];
}

template <typename scalar_t>
inline bool lesser_any(vec3a_t<scalar_t> const& a, vec3a_t<scalar_t> const& b)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return (_mm_movemask_ps(_mm_cmplt_ps(a, b)) & 0x7) != 0;
  else
    return a[0] < b[0] || a[1] < b[1] || a[2] < b[2];
}

template <typename scalar_t>
inline vec3a_t<scalar_t> mul(vec3a_t<scalar_t> const& v, mat4_t<scalar_t> const& m)
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
  {

    auto ret    = _mm_shuffle_ps(v.v, v.v, _MM_SHUFFLE(0, 0, 0, 0));
    ret         = _mm_mul_ps(ret, m.r[0]);
    auto v_temp = _mm_shuffle_ps(v.v, v.v, _MM_SHUFFLE(1, 1, 1, 1));
    v_temp      = _mm_mul_ps(v_temp, m.r[1]);
    ret         = _mm_add_ps(ret, v_temp);
    v_temp      = _mm_shuffle_ps(v.v, v.v, _MM_SHUFFLE(2, 2, 2, 2));
    v_temp      = _mm_mul_ps(v_temp, m.r[2]);
    ret         = _mm_add_ps(ret, v_temp);
    ret         = _mm_add_ps(ret, m.r[3]);
    return vec3a_t<scalar_t>(_mm_and_ps(ret, clear_w_mask()));
  }
  else
  {
    quad_t<scalar_t> r, x, y, z;

    z = splat_z(v);
    y = splat_y(v);
    x = splat_x(v);

    r = madd(z, m.r[2], m.r[3]);
    r = madd(y, m.r[1], r);
    r = madd(x, m.r[0], r);

    return make_vec3a(r);
  }
}
} // namespace acl
