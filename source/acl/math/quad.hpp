#pragma once

#include "deduced_types.hpp"
#include "real.hpp"

namespace acl
{
struct quad
{
  using type        = types::quad_t<float>;
  using ref         = type&;
  using pref        = type const&;
  using cref        = type const&;
  using scalar_type = float;
  using row_type    = float;

  enum : unsigned int
  {
    element_count = 4
  };

  static inline bool equals(quad::pref v1, quad::pref v2);
  static inline bool isnan(pref v);
  static inline bool isinf(pref v);
  static inline type isnanv(pref v);
  static inline type isinfv(pref v);
  static inline bool isnegative_x(pref v);
  static inline bool isgreater_x(pref v1, pref v2);
  static inline bool islesser_x(pref v1, pref v2);
  static inline type set(scalar_type v);
  static inline type set(scalar_type x, scalar_type y, scalar_type z);
  static inline type set(scalar_type x, scalar_type y, scalar_type z, scalar_type w);
  static inline type set(scalar_type const* v);
  static inline type set_unaligned(scalar_type const* v);
  static inline type set_x(scalar_type x);
  static inline type set_x(pref v, scalar_type x);
  static inline type set_y(pref v, scalar_type y);
  static inline type set_z(pref v, scalar_type z);
  static inline type set_w(pref v, scalar_type w);
  //! Set the 0th element of v from 0th element of q
  static inline type set_x(pref v, pref q);
  //! Set the 1st element of v from 0th element of q
  static inline type set_y(pref v, pref q);
  //! Set the 2nd element of v from 0th element of q
  static inline type set_z(pref v, pref q);
  //! Set the 3rd element of v from 0th element of q
  static inline type set_w(pref v, pref q);
  //! Return the 0th element of v
  static inline scalar_type x(pref v);
  //! Return the 1st element of v
  static inline scalar_type y(pref v);
  //! Return the 2nd element of v
  static inline scalar_type z(pref v);
  //! Return the 3rd element of v
  static inline scalar_type w(pref v);
  //! Return min of all elements
  static inline type min(pref a, pref b);
  //! Return max of all elements
  static inline type        max(pref a, pref b);
  static inline type        zero();
  static inline type        splat_x(pref v);
  static inline type        splat_y(pref v);
  static inline type        splat_z(pref v);
  static inline type        splat_w(pref v);
  static inline type        select(pref v1, pref v2, pref control);
  static inline scalar_type get(pref v, std::uint32_t i);
  static inline type        abs(pref v);
  static inline type        negate(pref v);
  static inline type        negate_w(pref v);
  static inline type        add(pref a, pref b);
  static inline type        sub(pref a, pref b);
  static inline type        mul(pref a, pref b);
  //! Add only the [0]th index
  static inline type add_x(pref a, pref b);
  //! Sub only the [0]th index
  static inline type sub_x(pref a, pref b);
  //! Multiply only the [0]th index
  static inline type mul_x(pref a, pref b);
  //! Sqrt  only the [0]th index
  static inline type sqrt_x(pref a);
  //! Reciprocal sqrt  only the [0]th index
  static inline type recip_sqrt_x(pref a);
  //! Half of 0th element
  static inline type half_x(pref a);
  //! Multiply the scalar to all elements of a
  static inline type mul(pref a, scalar_type b);
  //! Multiply the scalar to all elements of a
  static inline type        mul(scalar_type b, pref a);
  static inline type        half(pref a);
  static inline type        div(pref a, pref b);
  static inline type        madd(pref v, pref m, pref a);
  static inline scalar_type hadd(pref q1);
  static inline type        vhadd(pref q1);
  static inline bool        greater_all(pref q1, pref q2);
  static inline bool        greater_any(pref q1, pref q2);
  static inline bool        lesser_all(pref q1, pref q2);
  static inline bool        lesser_any(pref q1, pref q2);
  static inline type        vdot(pref q1, pref q2);
  static inline scalar_type dot(pref q1, pref q2);
  static inline scalar_type sqlength(pref c1);
  static inline scalar_type length(pref c1);
  static inline scalar_type distance(pref vec1, pref vec2);
  static inline scalar_type sqdistance(pref vec1, pref vec2);
  static inline type        normalize(pref v);
  static inline type        lerp(pref src, pref dest, scalar_type t);
  static inline type        recip_sqrt(pref qpf);
  //! set the vector as 0, 0, 0, w -> where w = a[select]
  static inline type set_000w(pref a, std::uint8_t select);
  //! set the vector as 1, 1, 1, w -> where w = a[select]
  static inline type set_111w(pref a, std::uint8_t select);
};

inline bool quad::equals(quad::pref v1, quad::pref v2)
{
  type r = quad::sub(v1, v2);
  return (bool)(real::equals(quad::x(r), 0) && real::equals(quad::y(r), 0) && real::equals(quad::z(r), 0) &&
                real::equals(quad::w(r), 0));
}

inline bool quad::isnan(quad::pref v)
{
  return quad::hadd(quad::isnanv(v)) != 0;
}

inline bool quad::isinf(quad::pref v)
{
  return quad::hadd(quad::isinfv(v)) != 0;
}

inline quad::type quad::isnanv(quad::pref v)
{
#if VML_USE_SSE_AVX
  return _mm_cmpneq_ps(v, v);
#else
  return quad::set(v[0] != v[0], v[1] != v[1], v[2] != v[2], v[3] != v[3]);
#endif
}

inline quad::type quad::isinfv(quad::pref v)
{
#if VML_USE_SSE_AVX
  // Mask off the sign bit
  type vtemp = _mm_and_ps(v, acl_cast_i128_ps(_mm_set_epi32(0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff)));
  // Compare to infinity
  vtemp = _mm_cmpeq_ps(vtemp, acl_cast_i128_ps(_mm_set_epi32(0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000)));
  // If any are infinity, the signs are true.
  return vtemp;

#else
  return quad::set(real::isinf(v[0]), real::isinf(v[1]), real::isinf(v[2]), real::isinf(v[3]));
#endif
}

inline bool quad::isnegative_x(pref q)
{
#if VML_USE_SSE_AVX
  return _mm_cvtsi128_si32(acl_cast_ps_i128(_mm_cmpgt_ss(_mm_set_ps1(0.0f), q))) != 0;
#else
  return q[0] < 0.0f;
#endif
}

inline bool quad::isgreater_x(pref q1, pref q2)
{
#if VML_USE_SSE_AVX
  return _mm_cvtsi128_si32(acl_cast_ps_i128(_mm_cmpgt_ss(q1, q2))) != 0;
#else
  return q1[0] > q2[0];
#endif
}

inline bool quad::islesser_x(pref q1, pref q2)
{
#if VML_USE_SSE_AVX
  return _mm_cvtsi128_si32(acl_cast_ps_i128(_mm_cmplt_ss(q1, q2))) != 0;
#else
  return q1[0] < q2[0];
#endif
}

inline quad::type quad::set(scalar_type v)
{
#if VML_USE_SSE_AVX
  return _mm_set_ps1(v);
#else
  return {v, v, v, v};
#endif
}

inline quad::type quad::set(const scalar_type* v)
{
#if VML_USE_SSE_AVX
  return _mm_load_ps(v);
#else
  return {v[0], v[1], v[2], v[3]};
#endif
}

inline quad::type quad::set(scalar_type x, scalar_type y, scalar_type z)
{
  return set(x, y, z, 0);
}

inline quad::type quad::set(scalar_type x, scalar_type y, scalar_type z, scalar_type w)
{
#if VML_USE_SSE_AVX
  return _mm_set_ps(w, z, y, x);
#else
  return {x, y, z, w};
#endif
}

inline quad::type quad::set_unaligned(scalar_type const* v)
{
#if VML_USE_SSE_AVX
  return _mm_loadu_ps(v);
#else
  return {v[0], v[1], v[2], v[3]};
#endif
}

inline quad::type quad::zero()
{
#if VML_USE_SSE_AVX
  return _mm_setzero_ps();
#else
  return {0, 0, 0, 0};
#endif
}

inline quad::scalar_type quad::get(quad::pref q, std::uint32_t idx)
{
#if VML_USE_SSE_AVX
#if defined(_MSC_VER)
  return q.m128_f32[idx];
#else
  return reinterpret_cast<scalar_type const*>(&q)[idx];
#endif
#else
  return q[idx];
#endif
}

inline quad::scalar_type quad::x(quad::pref q)
{
#if VML_USE_SSE_AVX
  return _mm_cvtss_f32(q);
#else
  return q[0];
#endif
}

inline quad::scalar_type quad::y(quad::pref q)
{
#if VML_USE_SSE_AVX
  type temp = _mm_shuffle_ps(q, q, _MM_SHUFFLE(1, 1, 1, 1));
  return _mm_cvtss_f32(temp);
#else
  return q[1];
#endif
}

inline quad::scalar_type quad::z(quad::pref q)
{
#if VML_USE_SSE_AVX
  type temp = _mm_shuffle_ps(q, q, _MM_SHUFFLE(2, 2, 2, 2));
  return _mm_cvtss_f32(temp);
#else
  return q[2];
#endif
}

inline quad::scalar_type quad::w(quad::pref q)
{
#if VML_USE_SSE_AVX
  type temp = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3));
  return _mm_cvtss_f32(temp);
#else
  return q[3];
#endif
}

inline quad::type quad::min(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_min_ps(a, b);
#else
  type r;
  for (std::uint32_t i = 0; i < element_count; ++i)
    r[i] = std::min(a[i], b[i]);
  return r;
#endif
}

inline quad::type quad::max(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_max_ps(a, b);
#else
  type r;
  for (std::uint32_t i = 0; i < element_count; ++i)
    r[i] = std::max(a[i], b[i]);
  return r;
#endif
}

inline quad::type quad::set_x(scalar_type val)
{
#if VML_USE_SSE_AVX
  return _mm_set_ss(val);
#else
  return {val, 0, 0, 0};
#endif
}

inline quad::type quad::set_x(quad::pref q, scalar_type val)
{
#if VML_USE_SSE_AVX
  type v = _mm_set_ss(val);
  return _mm_move_ss(q, v);
#else
  return {val, q[1], q[2], q[3]};
#endif
}

inline quad::type quad::set_y(quad::pref q, scalar_type val)
{
#if VML_USE_SSE_AVX
#ifdef _MSC_VER
  type res;
  res.m128_f32[0] = q.m128_f32[0];
  res.m128_f32[1] = val;
  res.m128_f32[2] = q.m128_f32[2];
  res.m128_f32[3] = q.m128_f32[3];
  return res;
#else
  type res = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 2, 0, 1));
  type v   = _mm_set_ss(val);
  // Replace the x component
  res = _mm_move_ss(res, v);
  // Swap y and x again
  return _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 2, 0, 1));
#endif
#else
  return {q[0], val, q[2], q[3]};
#endif
}

inline quad::type quad::set_z(quad::pref q, scalar_type val)
{
#if VML_USE_SSE_AVX
#ifdef _MSC_VER
  type res;
  res.m128_f32[0] = q.m128_f32[0];
  res.m128_f32[1] = q.m128_f32[1];
  res.m128_f32[2] = val;
  res.m128_f32[3] = q.m128_f32[3];
  return res;
#else
  type res = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 1, 2));
  type v   = _mm_set_ss(val);
  // Replace the x component
  res = _mm_move_ss(res, v);
  // Swap y and x again
  return _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 0, 1, 2));
#endif
#else
  return {q[0], q[1], val, q[3]};
#endif
}

inline quad::type quad::set_w(quad::pref q, scalar_type val)
{
#if VML_USE_SSE_AVX
#ifdef _MSC_VER
  type res;
  res.m128_f32[0] = q.m128_f32[0];
  res.m128_f32[1] = q.m128_f32[1];
  res.m128_f32[2] = q.m128_f32[2];
  res.m128_f32[3] = val;
  return res;
#else
  type res = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 2, 1, 3));
  type v   = _mm_set_ss(val);
  // Replace the x component
  res = _mm_move_ss(res, v);
  // Swap y and x again
  return _mm_shuffle_ps(res, res, _MM_SHUFFLE(0, 2, 1, 3));
#endif
#else
  return {q[0], q[1], q[2], val};
#endif
}

inline quad::type quad::set_x(quad::pref q, quad::pref v)
{
#if VML_USE_SSE_AVX
  return _mm_move_ss(q, v);
#else
  return {v[0], q[1], q[2], q[3]};
#endif
}

inline quad::type quad::set_y(quad::pref q, quad::pref v)
{
#if VML_USE_SSE_AVX
  type res = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 2, 0, 1));
  // Replace the x component
  res = _mm_move_ss(res, v);
  // Swap y and x again
  return _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 2, 0, 1));
#else
  return {q[0], v[0], q[2], q[3]};
#endif
}

inline quad::type quad::set_z(quad::pref q, quad::pref v)
{
#if VML_USE_SSE_AVX
  type res = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 1, 2));
  // Replace the x component
  res = _mm_move_ss(res, v);
  // Swap y and x again
  return _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 0, 1, 2));
#else
  return {q[0], q[1], v[0], q[3]};
#endif
}

inline quad::type quad::set_w(quad::pref q, quad::pref v)
{
#if VML_USE_SSE_AVX
  type res = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 2, 1, 3));
  // Replace the x component
  res = _mm_move_ss(res, v);
  // Swap y and x again
  return _mm_shuffle_ps(res, res, _MM_SHUFFLE(0, 2, 1, 3));
#else
  return {q[0], q[1], q[2], v[0]};
#endif
}

inline quad::type quad::half_x(quad::pref q)
{
#if VML_USE_SSE_AVX
  return _mm_mul_ss(_mm_set_ss(0.5f), q);
#else
  return {q[0] * 0.5f, 0.0f, 0.0f, 0.0f};
#endif
}

inline quad::type quad::splat_x(quad::pref q)
{
#if VML_USE_SSE_AVX
  return _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 0, 0, 0));
#else
  return {q[0], q[0], q[0], q[0]};
#endif
}

inline quad::type quad::splat_y(quad::pref q)
{
#if VML_USE_SSE_AVX
  return _mm_shuffle_ps(q, q, _MM_SHUFFLE(1, 1, 1, 1));
#else
  return {q[1], q[1], q[1], q[1]};
#endif
}

inline quad::type quad::splat_z(quad::pref q)
{
#if VML_USE_SSE_AVX
  return _mm_shuffle_ps(q, q, _MM_SHUFFLE(2, 2, 2, 2));
#else
  return {q[2], q[2], q[2], q[2]};
#endif
}

inline quad::type quad::splat_w(quad::pref q)
{
#if VML_USE_SSE_AVX
  return _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3));
#else
  return {q[3], q[3], q[3], q[3]};
#endif
}

inline bool quad::greater_any(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return (_mm_movemask_ps(_mm_cmpgt_ps(a, b))) != 0;
#else
  return a[0] > b[0] || a[1] > b[1] || a[2] > b[2] || a[3] > b[3];
#endif
}

inline bool quad::greater_all(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return (_mm_movemask_ps(_mm_cmpgt_ps(a, b)) == 0xF);
#else
  return a[0] > b[0] && a[1] > b[1] && a[2] > b[2] && a[3] > b[3];
#endif
}

inline bool quad::lesser_any(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return (_mm_movemask_ps(_mm_cmplt_ps(a, b))) != 0;
#else
  return a[0] < b[0] || a[1] < b[1] || a[2] < b[2] || a[3] < b[3];
#endif
}

inline bool quad::lesser_all(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return (_mm_movemask_ps(_mm_cmplt_ps(a, b)) == 0xF);
#else
  return a[0] < b[0] && a[1] < b[1] && a[2] < b[2] && a[3] < b[3];
#endif
}

inline quad::type quad::abs(quad::pref q)
{
#if VML_USE_SSE_AVX

  return _mm_and_ps(q, acl_cast_i128_ps(_mm_set_epi32(0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff)));
#else
  return quad::set(acl::abs(q[0]), acl::abs(q[1]), acl::abs(q[2]), acl::abs(q[3]));
#endif
}

inline quad::type quad::negate(quad::pref q)
{
#if VML_USE_SSE_AVX
  const __m128i k_sign = _mm_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000);
  return _mm_xor_ps(q, acl_cast_i128_ps(k_sign));
#else
  return quad::set(-(q[0]), -(q[1]), -(q[2]), -(q[3]));
#endif
}

inline quad::type quad::negate_w(quad::pref q)
{
#if VML_USE_SSE_AVX
  const __m128i k_sign = _mm_set_epi32(0x80000000, 0x00000000, 0x00000000, 0x00000000);
  return _mm_xor_ps(q, acl_cast_i128_ps(k_sign));
#else
  return quad::set((q[0]), (q[1]), (q[2]), -(q[3]));
#endif
}

inline quad::type quad::add(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_add_ps(a, b);
#else
  return quad::set(a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]);
#endif
}

inline quad::type quad::sub(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_sub_ps(a, b);
#else
  return quad::set(a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]);

#endif
}

inline quad::type quad::mul(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_mul_ps(a, b);
#else
  return quad::set(a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]);
#endif
}

inline quad::type quad::add_x(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_add_ss(a, b);
#else
  return quad::set(a[0] + b[0], 0, 0, 0);
#endif
}

inline quad::type quad::sub_x(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_sub_ss(a, b);
#else
  return quad::set(a[0] - b[0], 0, 0, 0);

#endif
}

inline quad::type quad::mul_x(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_mul_ss(a, b);
#else
  return quad::set(a[0] * b[0], 0, 0, 0);
#endif
}

inline quad::type quad::sqrt_x(quad::pref a)
{
#if VML_USE_SSE_AVX
  return _mm_sqrt_ss(a);
#else
  return quad::set(acl::sqrt(a[0]), 0, 0, 0);
#endif
}

inline quad::type quad::recip_sqrt_x(quad::pref a)
{
#if VML_USE_SSE_AVX
  return _mm_rsqrt_ss(a);
#else
  return quad::set(acl::recip_sqrt(a[0]), 0, 0, 0);
#endif
}

inline quad::type quad::div(quad::pref a, quad::pref b)
{
#if VML_USE_SSE_AVX
  return _mm_div_ps(a, b);
#else
  return quad::set(a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3]);
#endif
}

inline quad::type quad::mul(quad::pref q, scalar_type val)
{
#if VML_USE_SSE_AVX
  __m128 res = _mm_set_ps1(val);
  return _mm_mul_ps(q, res);
#else
  return quad::set(q[0] * val, q[1] * val, q[2] * val, q[3] * val);
#endif
}

inline quad::type quad::mul(scalar_type val, quad::pref q)
{
#if VML_USE_SSE_AVX
  __m128 res = _mm_set_ps1(val);
  return _mm_mul_ps(q, res);
#else
  return quad::set(q[0] * val, q[1] * val, q[2] * val, q[3] * val);
#endif
}

inline quad::type quad::madd(quad::pref a, quad::pref v, quad::pref c)
{
#if VML_USE_SSE_AVX
  type t = _mm_mul_ps(a, v);
  return _mm_add_ps(t, c);
#else
  return quad::add(quad::mul(a, v), c);
#endif
}

inline quad::scalar_type quad::hadd(quad::pref v)
{
#if VML_USE_SSE_AVX
#if VML_USE_SSE_LEVEL >= 3
  __m128 shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
  __m128 sums = _mm_add_ps(v, shuf);
  shuf        = _mm_movehl_ps(shuf, sums); // high half -> low half
  sums        = _mm_add_ss(sums, shuf);
  return x(sums);
#else
  __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
  __m128 sums = _mm_add_ps(v, shuf);                           // sums = [ D+C C+D | B+A A+B ]
  shuf        = _mm_movehl_ps(shuf, sums);                     //  [   C   D | D+C C+D ]  // let the
                                                               //  compiler avoid a mov by reusing shuf
  sums = _mm_add_ss(sums, shuf);
  return x(sums);
#endif
#else
  return v[0] + v[1] + v[2] + v[3];
#endif
}

inline quad::type quad::vhadd(quad::pref v)
{
#if VML_USE_SSE_AVX
#if VML_USE_SSE_LEVEL >= 3
  __m128 shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
  __m128 sums = _mm_add_ps(v, shuf);
  shuf        = _mm_movehl_ps(shuf, sums); // high half -> low half
  sums        = _mm_add_ss(sums, shuf);
  return sums;
#else
  __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
  __m128 sums = _mm_add_ps(v, shuf);                           // sums = [ D+C C+D | B+A A+B ]
  shuf        = _mm_movehl_ps(shuf, sums);                     //  [   C   D | D+C C+D ]  // let the
                                                               //  compiler avoid a mov by reusing shuf
  sums = _mm_add_ss(sums, shuf);
  return sums;
#endif
#else
  return set(v[0] + v[1] + v[2] + v[3], 0.0f, 0.0f, 0.0f);
#endif
}

inline quad::type quad::recip_sqrt(quad::pref qpf)
{
#if VML_USE_SSE_AVX
#if VML_PREFER_SPEED_OVER_ACCURACY
  return _mm_rsqrt_ps(qpf);
#else
  const __m128 approx = _mm_rsqrt_ps(qpf);
  const __m128 muls   = _mm_mul_ps(_mm_mul_ps(qpf, approx), approx);
  return _mm_mul_ps(_mm_mul_ps(_mm_set_ps1(0.5f), approx), _mm_sub_ps(_mm_set_ps1(3.0f), muls));
#endif
#else
  return quad::set(acl::recip_sqrt(qpf[0]), acl::recip_sqrt(qpf[1]), acl::recip_sqrt(qpf[2]), acl::recip_sqrt(qpf[3]));
#endif
}

inline quad::type quad::select(quad::pref v1, quad::pref v2, quad::pref control)
{
#if VML_USE_SSE_AVX
  type vtemp1 = _mm_andnot_ps(control, v1);
  type vtemp2 = _mm_and_ps(v2, control);
  return _mm_or_ps(vtemp1, vtemp2);
#else
  type ret;
  auto iret = reinterpret_cast<std::uint32_t*>(&ret);
  auto iv1  = reinterpret_cast<std::uint32_t const*>(&v1);
  auto iv2  = reinterpret_cast<std::uint32_t const*>(&v2);
  auto ic   = reinterpret_cast<std::uint32_t const*>(&control);
  for (int i = 0; i < 4; ++i)
    iret[i] = (~ic[i] & iv1[i]) | (ic[i] & iv2[i]);
  return ret;
#endif
}

inline quad::type quad::vdot(quad::pref vec1, quad::pref vec2)
{
#if VML_USE_SSE_AVX
#if VML_USE_SSE_LEVEL >= 4
  return _mm_dp_ps(vec1, vec2, 0xFF);
#elif VML_USE_SSE_LEVEL >= 3
  __m128 v    = _mm_mul_ps(vec1, vec2);
  __m128 shuf = _mm_movehdup_ps(v); // broadcast elements 3,1 to 2,0
  __m128 sums = _mm_add_ps(v, shuf);
  shuf        = _mm_movehl_ps(shuf, sums); // high half -> low half
  sums        = _mm_add_ss(sums, shuf);
  return sums;
#else
  __m128 v    = _mm_mul_ps(vec1, vec2);
  __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
  __m128 sums = _mm_add_ps(v, shuf);                           // sums = [ D+C C+D | B+A A+B ]
  shuf        = _mm_movehl_ps(shuf, sums);                     //  [   C   D | D+C C+D ]  // let the
                                                               //  compiler avoid a mov by reusing shuf
  sums = _mm_add_ss(sums, shuf);
  return sums;
#endif
#else
  return set(dot(vec1, vec2), 0, 0, 0);
#endif
}

inline quad::scalar_type quad::dot(quad::pref q1, quad::pref q2)
{
#if VML_USE_SSE_AVX
  return quad::x(quad::vdot(q1, q2));
#else
  return q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];
#endif
}

inline quad::type quad::normalize(quad::pref vec)
{
#if VML_USE_SSE_AVX
#if VML_USE_SSE_LEVEL >= 4
  type q = _mm_dp_ps(vec, vec, 0xFF);
  // get the reciprocal
  q = _mm_sqrt_ps(q);
  return _mm_div_ps(vec, q);
#else
  type q = vdot(vec, vec);
  q      = _mm_sqrt_ss(q);
  q      = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 0, 0, 0));
  return _mm_div_ps(vec, q);
#endif
#else
  float res = acl::sqrt(quad::dot(vec, vec));
  return quad::set(vec[0] / res, vec[1] / res, vec[2] / res, vec[3] / res);
#endif
}

inline quad::type quad::lerp(quad::pref src, quad::pref dst, quad::scalar_type t)
{
  return quad::madd(quad::set(t), quad::sub(dst, src), src);
}

inline quad::scalar_type quad::length(quad::pref vec)
{
  return acl::sqrt(quad::dot(vec, vec));
}

inline quad::scalar_type quad::sqlength(quad::pref vec)
{
  return quad::dot(vec, vec);
}

inline quad::scalar_type quad::distance(quad::pref vec1, quad::pref vec2)
{
  return quad::length(quad::sub(vec1, vec2));
}

inline quad::scalar_type quad::sqdistance(quad::pref vec1, quad::pref vec2)
{
  return quad::sqlength(quad::sub(vec1, vec2));
}
inline quad::type quad::half(pref a)
{
  return mul(a, 0.5f);
}
inline quad::type quad::set_000w(pref a, std::uint8_t select)
{
#if VML_USE_SSE_AVX
  switch (select)
  {
  case 0:
    return _mm_and_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 2, 1, 0)), clear_xyz());
  case 1:
    return _mm_and_ps(_mm_movelh_ps(a, a), clear_xyz());
  case 2:
    return _mm_and_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 2, 1, 0)), clear_xyz());
  case 3:
    return _mm_and_ps(a, clear_xyz());
  }
  assert(0 && "Not allowed!");
  return type();
#else
  return {0, 0, 0, a[select]};
#endif
}
inline quad::type quad::set_111w(pref a, std::uint8_t select)
{
#if VML_USE_SSE_AVX
  return _mm_or_ps(_mm_set_ps(0.0f, 1.0f, 1.0f, 1.0f), set_000w(a, select));
#else
  return {1.0, 1.0, 1.0, a[select]};
#endif
}

inline auto operator+(quad::pref a, quad::pref b) noexcept
{
  return quad::add(a, b);
}

inline auto operator-(quad::pref a, quad::pref b) noexcept
{
  return quad::sub(a, b);
}

inline auto operator/(quad::pref a, quad::pref b) noexcept
{
  return quad::div(a, b);
}

inline auto operator*(quad::pref a, quad::pref b) noexcept
{
  return quad::mul(a, b);
}

inline auto operator*(quad::pref a, quad::scalar_type b) noexcept
{
  return quad::mul(a, b);
}

inline auto operator*(quad::scalar_type a, quad::pref b) noexcept
{
  return quad::mul(a, b);
}

} // namespace acl

#if VML_USE_SSE_AVX

inline bool operator==(__m128 const& a, __m128 const& b) noexcept
{
  return _mm_movemask_epi8(acl_cast_ps_i128(_mm_cmpneq_ps(a, b))) == 0;
}

inline bool operator!=(__m128 const& a, __m128 const& b) noexcept
{
  return _mm_movemask_epi8(acl_cast_ps_i128(_mm_cmpneq_ps(a, b))) != 0;
}

inline bool operator<(__m128 const& a, __m128 const& b) noexcept
{
  return _mm_movemask_epi8(acl_cast_ps_i128(_mm_cmpnlt_ps(a, b))) != 0;
}

inline bool operator>(__m128 const& a, __m128 const& b) noexcept
{
  return _mm_movemask_epi8(acl_cast_ps_i128(_mm_cmpngt_ps(a, b))) != 0;
}

inline bool operator<=(__m128 const& a, __m128 const& b) noexcept
{
  return _mm_movemask_epi8(acl_cast_ps_i128(_mm_cmpngt_ps(b, a))) == 0;
}

inline bool operator>=(__m128 const& a, __m128 const& b) noexcept
{
  return _mm_movemask_epi8(acl_cast_ps_i128(_mm_cmpnlt_ps(b, a))) == 0;
}

#endif