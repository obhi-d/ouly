#pragma once

#include "deduced_types.hpp"
#include "real.hpp"

namespace acl
{

template <typename scalar_t, typename stag>
inline bool equals(quad_t<scalar_t, stag> const& v1, quad_t<scalar_t, stag> const& v2) noexcept
{
  quad_t<scalar_t, stag> r = sub(v1, v2);
  return (bool)(equals(x(r), 0) && equals(y(r), 0) && equals(z(r), 0) && equals(w(r), 0));
}

template <typename scalar_t, typename stag>
inline bool isnan(quad_t<scalar_t, stag> const& v) noexcept
{
  return hadd(isnanv(v)) != 0;
}

template <typename scalar_t, typename stag>
inline bool isinf(quad_t<scalar_t, stag> const& v) noexcept
{
  return hadd(isinfv(v)) != 0;
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> isnanv(quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_cmpneq_ps(v.v, v.v);
  }
  else
  {
    return set(v[0] != v[0], v[1] != v[1], v[2] != v[2], v[3] != v[3]);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> isinfv(quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    // Mask off the sign bit
    auto vtemp = _mm_and_ps(v.v, _mm_castsi128_ps(_mm_set_epi32(0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff)));
    // Compare to infinity
    vtemp = _mm_cmpeq_ps(vtemp, _mm_castsi128_ps(_mm_set_epi32(0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000)));
    // If any are infinity, the signs are true.
    return vtemp;
  }
  else
  {
    return set(isinf(v[0]), isinf(v[1]), isinf(v[2]), isinf(v[3]));
  }
}

template <typename scalar_t, typename stag>
inline bool isnegative_x(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_cvtsi128_si32(_mm_castps_si128(_mm_cmpgt_ss(_mm_set_ps1(0.0f), q.v))) != 0;
  }
  else
  {
    return q[0] < 0.0f;
  }
}

template <typename scalar_t, typename stag>
inline bool isgreater_x(quad_t<scalar_t, stag> const& q1, quad_t<scalar_t, stag> const& q2) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_cvtsi128_si32(_mm_castps_si128(_mm_cmpgt_ss(q1.v, q2.v))) != 0;
  }
  else
  {
    return q1[0] > q2[0];
  }
}

template <typename scalar_t, typename stag>
inline bool islesser_x(quad_t<scalar_t, stag> const& q1, quad_t<scalar_t, stag> const& q2) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_cvtsi128_si32(_mm_castps_si128(_mm_cmplt_ss(q1.v, q2.v))) != 0;
  }
  else
  {
    return q1[0] < q2[0];
  }
}

template <typename scalar_t>
inline quad_t<scalar_t, default_tag> set(scalar_t v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, default_tag>(_mm_set_ps1(v));
  }
  else
  {
    return {v, v, v, v};
  }
}

template <typename scalar_t>
inline quad_t<scalar_t, default_tag> set(const scalar_t* v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, default_tag>(_mm_load_ps(v));
  }
  else
  {
    return {v[0], v[1], v[2], v[3]};
  }
}

template <typename scalar_t>
inline quad_t<scalar_t, default_tag> set(scalar_t x, scalar_t y, scalar_t z) noexcept
{
  return set(x, y, z, 0);
}

template <typename scalar_t>
inline quad_t<scalar_t, default_tag> set(scalar_t x, scalar_t y, scalar_t z, scalar_t w) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, default_tag>(_mm_set_ps(w, z, y, x));
  }
  else
  {
    return {x, y, z, w};
  }
}

template <typename scalar_t>
inline quad_t<scalar_t, default_tag> set_unaligned(scalar_t const* v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, default_tag>(_mm_loadu_ps(v));
  }
  else
  {
    return {v[0], v[1], v[2], v[3]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> zero() noexcept
  requires(!std::same_as<stag, nonquad_tag>)
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_setzero_ps());
  }
  else
  {
    return {0, 0, 0, 0};
  }
}

template <typename scalar_t, typename stag>
inline scalar_t get(quad_t<scalar_t, stag> const& q, std::uint32_t idx) noexcept
{
  return q[idx];
}

template <typename scalar_t, typename stag>
inline scalar_t x(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_cvtss_f32(q.v);
  }
  else
  {
    return q[0];
  }
}

template <typename scalar_t, typename stag>
inline scalar_t y(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto temp = _mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(1, 1, 1, 1));
    return _mm_cvtss_f32(temp);
  }
  else
  {
    return q[1];
  }
}

template <typename scalar_t, typename stag>
inline scalar_t z(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto temp = _mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(2, 2, 2, 2));
    return _mm_cvtss_f32(temp);
  }
  else
  {
    return q[2];
  }
}

template <typename scalar_t, typename stag>
inline scalar_t w(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto temp = _mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(3, 3, 3, 3));
    return _mm_cvtss_f32(temp);
  }
  else
  {
    return q[3];
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> min(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_min_ps(a.v, b.v));
  }
  else
  {
    quad_t<scalar_t, stag> r;
    for (std::uint32_t i = 0; i < a.xyzw.size(); ++i)
      r[i] = std::min(a[i], b[i]);
    return r;
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> max(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_max_ps(a.v, b.v));
  }
  else
  {
    quad_t<scalar_t, stag> r;
    for (std::uint32_t i = 0; i < a.xyzw.size(); ++i)
      r[i] = std::max(a[i], b[i]);
    return r;
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_x(scalar_t val) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_set_ss(val));
  }
  else
  {
    return {val, 0, 0, 0};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_x(quad_t<scalar_t, stag> const& q, scalar_t val) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto v = _mm_set_ss(val);
    return quad_t<scalar_t, stag>(_mm_move_ss(q.v, v));
  }
  else
  {
    return {val, q[1], q[2], q[3]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_y(quad_t<scalar_t, stag> const& q, scalar_t val) noexcept
{
  return {q[0], val, q[2], q[3]};
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_z(quad_t<scalar_t, stag> const& q, scalar_t val) noexcept
{
  return {q[0], q[1], val, q[3]};
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_w(quad_t<scalar_t, stag> const& q, scalar_t val) noexcept
{
  return {q[0], q[1], q[2], val};
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_x(quad_t<scalar_t, stag> const& q, quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_move_ss(q.v, v));
  }
  else
  {
    return {v[0], q[1], q[2], q[3]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_y(quad_t<scalar_t, stag> const& q, quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto res = _mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(3, 2, 0, 1));
    // Replace the x component
    res = _mm_move_ss(res, v.v);
    // Swap y and x again
    return quad_t<scalar_t, stag>(_mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 2, 0, 1)));
  }
  else
  {
    return {q[0], v[0], q[2], q[3]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_z(quad_t<scalar_t, stag> const& q, quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto res = _mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(3, 0, 1, 2));
    // Replace the x component
    res = _mm_move_ss(res, v.v);
    // Swap y and x again
    return quad_t<scalar_t, stag>(_mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 0, 1, 2)));
  }
  else
  {
    return {q[0], q[1], v[0], q[3]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_w(quad_t<scalar_t, stag> const& q, quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto res = _mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(0, 2, 1, 3));
    // Replace the x component
    res = _mm_move_ss(res, v.v);
    // Swap y and x again
    return quad_t<scalar_t, stag>(_mm_shuffle_ps(res, res, _MM_SHUFFLE(0, 2, 1, 3)));
  }
  else
  {
    return {q[0], q[1], q[2], v[0]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> half_x(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_mul_ss(_mm_set_ss(0.5f), q.v));
  }
  else
  {
    return {q[0] * 0.5f, 0.0f, 0.0f, 0.0f};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> splat_x(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(0, 0, 0, 0)));
  }
  else
  {
    return {q[0], q[0], q[0], q[0]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> splat_y(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(1, 1, 1, 1)));
  }
  else
  {
    return {q[1], q[1], q[1], q[1]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> splat_z(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(2, 2, 2, 2)));
  }
  else
  {
    return {q[2], q[2], q[2], q[2]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> splat_w(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return quad_t<scalar_t, stag>(_mm_shuffle_ps(q.v, q.v, _MM_SHUFFLE(3, 3, 3, 3)));
  }
  else
  {
    return {q[3], q[3], q[3], q[3]};
  }
}

template <typename scalar_t, typename stag>
inline bool greater_any(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return (_mm_movemask_ps(_mm_cmpgt_ps(a.v, b.v))) != 0;
  }
  else
  {
    return a[0] > b[0] || a[1] > b[1] || a[2] > b[2] || a[3] > b[3];
  }
}

template <typename scalar_t, typename stag>
inline bool greater_all(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return (_mm_movemask_ps(_mm_cmpgt_ps(a.v, b.v)) == 0xF);
  }
  else
  {
    return a[0] > b[0] && a[1] > b[1] && a[2] > b[2] && a[3] > b[3];
  }
}

template <typename scalar_t, typename stag>
inline bool lesser_any(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return (_mm_movemask_ps(_mm_cmplt_ps(a.v, b.v))) != 0;
  }
  else
  {
    return a[0] < b[0] || a[1] < b[1] || a[2] < b[2] || a[3] < b[3];
  }
}

template <typename scalar_t, typename stag>
inline bool lesser_all(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return (_mm_movemask_ps(_mm_cmplt_ps(a.v, b.v)) == 0xF);
  }
  else
  {
    return a[0] < b[0] && a[1] < b[1] && a[2] < b[2] && a[3] < b[3];
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> abs(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {

    return _mm_and_ps(q.v, _mm_castsi128_ps(_mm_set_epi32(0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff)));
  }
  else
  {
    return set(std::abs(q.v[0]), std::abs(q.v[1]), std::abs(q.v[2]), std::abs(q.v[3]));
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> negate(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    const __m128i k_sign = _mm_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000);
    return _mm_xor_ps(q.v, _mm_castsi128_ps(k_sign));
  }
  else
  {
    return set(-(q.v[0]), -(q.v[1]), -(q.v[2]), -(q.v[3]));
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> negate_w(quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    const __m128i k_sign = _mm_set_epi32(0x80000000, 0x00000000, 0x00000000, 0x00000000);
    return _mm_xor_ps(q.v, _mm_castsi128_ps(k_sign));
  }
  else
  {
    return set((q.v[0]), (q.v[1]), (q.v[2]), -(q.v[3]));
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> add(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_add_ps(a.v, b.v);
  }
  else
  {
    return set(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2], a.v[3] + b.v[3]);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> sub(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_sub_ps(a.v, b.v);
  }
  else
  {
    return set(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2], a.v[3] - b.v[3]);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> mul(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_mul_ps(a.v, b.v);
  }
  else
  {
    return set(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2], a.v[3] * b.v[3]);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> add_x(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_add_ss(a.v, b.v);
  }
  else
  {
    return set(a.v[0] + b.v[0], 0, 0, 0);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> sub_x(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_sub_ss(a.v, b.v);
  }
  else
  {
    return set(a.v[0] - b.v[0], 0, 0, 0);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> mul_x(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_mul_ss(a.v, b.v);
  }
  else
  {
    return set(a.v[0] * b.v[0], 0, 0, 0);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> sqrt_x(quad_t<scalar_t, stag> const& a) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_sqrt_ss(a.v);
  }
  else
  {
    return set(std::sqrt(a.v[0]), 0, 0, 0);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> recip_sqrt_x(quad_t<scalar_t, stag> const& a) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_rsqrt_ss(a.v);
  }
  else
  {
    return set(acl::recip_sqrt(a.v[0]), 0, 0, 0);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> div(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_div_ps(a.v, b.v);
  }
  else
  {
    return set(a.v[0] / b.v[0], a.v[1] / b.v[1], a.v[2] / b.v[2], a.v[3] / b.v[3]);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> mul(quad_t<scalar_t, stag> const& q, scalar_t val) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    __m128 res = _mm_set_ps1(val);
    return _mm_mul_ps(q.v, res);
  }
  else
  {
    return set(q.v[0] * val, q.v[1] * val, q.v[2] * val, q.v[3] * val);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> mul(scalar_t val, quad_t<scalar_t, stag> const& q) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    __m128 res = _mm_set_ps1(val);
    return _mm_mul_ps(q.v, res);
  }
  else
  {
    return set(q.v[0] * val, q.v[1] * val, q.v[2] * val, q.v[3] * val);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> madd(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& v,
                                   quad_t<scalar_t, stag> const& c) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    quad_t<scalar_t, stag> t = _mm_mul_ps(a.v, v);
    return _mm_add_ps(t, c.v);
  }
  else
  {
    return add(mul(a.v, v), c.v);
  }
}

template <typename scalar_t, typename stag>
inline scalar_t hadd(quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    if constexpr (has_sse3)
    {
      __m128 shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
      __m128 sums = _mm_add_ps(v, shuf);
      shuf        = _mm_movehl_ps(shuf, sums); // high half -> low half
      sums        = _mm_add_ss(sums, shuf);
      return _mm_cvtss_f32(sums);
    }
    else
    {
      __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
      __m128 sums = _mm_add_ps(v, shuf);                           // sums = [ D+C C+D | B+A A+B ]
      shuf        = _mm_movehl_ps(shuf, sums);                     //  [   C   D | D+C C+D ]  // let the
                                                                   //  compiler avoid a.v mov by reusing shuf
      sums = _mm_add_ss(sums, shuf);
      return _mm_cvtss_f32(sums);
    }
  }
  else
  {
    return v[0] + v[1] + v[2] + v[3];
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> vhadd(quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    if constexpr (has_sse3)
    {
      __m128 shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
      __m128 sums = _mm_add_ps(v, shuf);
      shuf        = _mm_movehl_ps(shuf, sums); // high half -> low half
      sums        = _mm_add_ss(sums, shuf);
      return quad_t<scalar_t, stag>{sums};
    }
    else
    {
      __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
      __m128 sums = _mm_add_ps(v, shuf);                           // sums = [ D+C C+D | B+A A+B ]
      shuf        = _mm_movehl_ps(shuf, sums);                     //  [   C   D | D+C C+D ]  // let the
                                                                   //  compiler avoid a.v mov by reusing shuf
      sums = _mm_add_ss(sums, shuf);
      return quad_t<scalar_t, stag>{sums};
    }
  }
  else
  {
    return set(v[0] + v[1] + v[2] + v[3], 0.0f, 0.0f, 0.0f);
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> recip_sqrt(quad_t<scalar_t, stag> const& qpf) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    const __m128 approx = _mm_rsqrt_ps(qpf);
    const __m128 muls   = _mm_mul_ps(_mm_mul_ps(qpf, approx), approx);
    return _mm_mul_ps(_mm_mul_ps(_mm_set_ps1(0.5f), approx), _mm_sub_ps(_mm_set_ps1(3.0f), muls));
  }
  else
  {
    return set(acl::recip_sqrt(qpf[0]), acl::recip_sqrt(qpf[1]), acl::recip_sqrt(qpf[2]), acl::recip_sqrt(qpf[3]));
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> select(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b,
                                     quad_t<scalar_t, stag> const& c) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    quad_t<scalar_t, stag> vtemp1 = _mm_andnot_ps(c.v, a.v);
    quad_t<scalar_t, stag> vtemp2 = _mm_and_ps(b.v, c.v);
    return _mm_or_ps(vtemp1, vtemp2);
  }
  else
  {
    quad_t<scalar_t, stag> ret;
    auto                   iret = reinterpret_cast<std::uint32_t*>(&ret);
    auto                   iv1  = reinterpret_cast<std::uint32_t const*>(&a.v);
    auto                   iv2  = reinterpret_cast<std::uint32_t const*>(&b.v);
    auto                   ic   = reinterpret_cast<std::uint32_t const*>(&c.v);
    for (int i = 0; i < 4; ++i)
      iret[i] = (~ic[i] & iv1[i]) | (ic[i] & iv2[i]);
    return ret;
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> vdot(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    if constexpr (has_sse41)
      return quad_t<scalar_t, stag>{_mm_dp_ps(a.v, b.v, 0xFF)};
    else if constexpr (has_sse3)
    {
      auto v    = _mm_mul_ps(a.v, b.v);
      auto shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
      auto sums = _mm_add_ps(v, shuf);
      shuf      = _mm_movehl_ps(shuf, sums); // high half -> low half
      sums      = _mm_add_ss(sums, shuf);
      return quad_t<scalar_t, stag>{sums};
    }
    else
    {
      auto v    = _mm_mul_ps(a.v, b.v);
      auto shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
      auto sums = _mm_add_ps(v, shuf);                           // sums = [ D+C C+D | B+A A+B ]
      shuf      = _mm_movehl_ps(shuf, sums);                     //  [   C   D | D+C C+D ]  // let the
                                                                 //  compiler avoid a.v mov by reusing shuf
      sums = _mm_add_ss(sums, shuf);
      return quad_t<scalar_t, stag>{sums};
    }
  }
  else
  {
    return set(dot(a.v, b.v), 0, 0, 0);
  }
}

template <typename scalar_t, typename stag>
inline scalar_t dot(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return x(vdot(a.v, b.v));
  }
  else
  {
    return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2] + a.v[3] * b.v[3];
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> normalize(quad_t<scalar_t, stag> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    if constexpr (has_sse41)
    {
      auto q = _mm_dp_ps(v.v, v.v, 0xFF);
      // get the reciprocal
      q = _mm_sqrt_ps(q);
      return quad_t<scalar_t, stag>{_mm_div_ps(v.v, q)};
    }
    else
    {
      auto q = vdot(v, v).v;
      q      = _mm_sqrt_ss(q);
      q      = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 0, 0, 0));
      return quad_t<scalar_t, stag>(_mm_div_ps(v.v, q));
    }
  }
  else
  {
    scalar_t res = scalar_t(1.0f) / std::sqrt(dot(v, v));
    return set(v[0] * res, v[1] * res, v[2] * res, v[3] * res);
  }
}
inline auto clear_w_mask() noexcept
{
  return _mm_castsi128_ps(_mm_set_epi32(0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));
}

inline auto xyz0_w1() noexcept
{
  return _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
}

inline auto clear_xyz() noexcept
{
  return _mm_castsi128_ps(_mm_set_epi32(0xFFFFFFFF, 0, 0, 0));
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> lerp(quad_t<scalar_t, stag> const& src, quad_t<scalar_t, stag> const& dst,
                                   scalar_t t) noexcept
{
  return madd(set(t), sub(dst, src), src);
}

template <typename scalar_t, typename stag>
inline scalar_t length(quad_t<scalar_t, stag> const& vec) noexcept
{
  return std::sqrt(dot(vec, vec));
}

template <typename scalar_t, typename stag>
inline scalar_t sqlength(quad_t<scalar_t, stag> const& vec) noexcept
{
  return dot(vec, vec);
}

template <typename scalar_t, typename stag>
inline scalar_t distance(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  return length(sub(a.v, b.v));
}

template <typename scalar_t, typename stag>
inline scalar_t sqdistance(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  return sqlength(sub(a.v, b.v));
}
template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> half(quad_t<scalar_t, stag> const& a) noexcept
{
  return mul(a.v, 0.5f);
}
template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_000w(quad_t<scalar_t, stag> const& a, std::uint8_t select) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    switch (select)
    {
    case 0:
      return quad_t<scalar_t, stag>(_mm_and_ps(_mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(0, 2, 1, 0)), clear_xyz()));
    case 1:
      return quad_t<scalar_t, stag>(_mm_and_ps(_mm_movelh_ps(a.v, a.v), clear_xyz()));
    case 2:
      return quad_t<scalar_t, stag>(_mm_and_ps(_mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(2, 2, 1, 0)), clear_xyz()));
    case 3:
      return quad_t<scalar_t, stag>(_mm_and_ps(a.v, clear_xyz()));
    }
    assert(0 && "Not allowed!");
    return quad_t<scalar_t, stag>();
  }
  else
  {
    return {0, 0, 0, a.v[select]};
  }
}
template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> set_111w(quad_t<scalar_t, stag> const& a, std::uint8_t select) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return _mm_or_ps(_mm_set_ps(0.0f, 1.0f, 1.0f, 1.0f), set_000w(a.v, select));
  }
  else
  {
    return {1.0, 1.0, 1.0, a.v[select]};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> exp(quad_t<scalar_t, stag> const& a) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return exp_ps(a.v);
  }
  else
  {
    return {std::exp(a.x), std::exp(a.y), std::exp(a.z), std::exp(a.w)};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> log(quad_t<scalar_t, stag> const& a) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return log_ps(a.v);
  }
  else
  {
    return {std::log(a.x), std::log(a.y), std::log(a.z), std::log(a.w)};
  }
}

template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> pow(quad_t<scalar_t, stag> const& a, scalar_t exp) noexcept
{
  return {std::pow(a.x, exp), std::pow(a.y, exp), std::pow(a.z, exp), std::pow(a.w, exp)};
}

/// @brief Power function for positive numbers only
template <typename scalar_t, typename stag>
inline quad_t<scalar_t, stag> ppow(quad_t<scalar_t, stag> const& a, scalar_t exp) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    return exp_ps(_mm_mul_ps(_mm_set1_ps(exp), log_ps(a.v)));
  }
  else
  {
    return {std::pow(a.x, exp), std::pow(a.y, exp), std::pow(a.z, exp), std::pow(a.w, exp)};
  }
}

template <typename scalar_t, typename stag>
inline auto operator+(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  return add(a.v, b.v);
}

template <typename scalar_t, typename stag>
inline auto operator-(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  return sub(a.v, b.v);
}

template <typename scalar_t, typename stag>
inline auto operator/(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  return div(a.v, b.v);
}

template <typename scalar_t, typename stag>
inline auto operator*(quad_t<scalar_t, stag> const& a, quad_t<scalar_t, stag> const& b) noexcept
{
  return mul(a.v, b.v);
}

template <typename scalar_t, typename stag>
inline auto operator*(quad_t<scalar_t, stag> const& a, scalar_t b) noexcept
{
  return mul(a.v, b);
}

template <typename scalar_t, typename stag>
inline auto operator*(scalar_t a, quad_t<scalar_t, stag> const& b) noexcept
{
  return mul(a, b.v);
}

} // namespace acl
