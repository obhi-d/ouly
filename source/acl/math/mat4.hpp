#pragma once
#include "aabb.hpp"
#include "mat_base.hpp"
namespace acl
{

template <typename scalar_t>
inline mat4_t<scalar_t> make_identity_mat4() noexcept
{
  return mat4_t<scalar_t>();
}

/// @brief Returns maximum scaling
template <typename scalar_t>
inline scalar_t max_scale(mat4_t<scalar_t> const& m) noexcept
{
  return std::sqrt(std::max(std::max(sqlength(m.r[0].v), sqlength(m.r[1].v)), sqlength(m.r[2].v)));
}

template <typename scalar_t>
inline bool equals(mat4_t<scalar_t> const& a, mat4_t<scalar_t> const& b) noexcept
{
  return equals(a[0], b[0]) && equals(a[1], b[1]) && equals(a[2], b[2]) && equals(a[3], b[3]);
}

/// @brief Full matrix multiplication
template <typename scalar_t>
inline mat4_t<scalar_t> mul(mat4_t<scalar_t> const& m1, mat4_t<scalar_t> const& m2) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    mat4_t<scalar_t> result{noinit_v};
    // Use vw to hold the original row
    __m128 vw = m1.r[0].v;
    // Splat the component x,y,Z then W
    __m128 vx = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 vy = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 vz = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(2, 2, 2, 2));
    vw        = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 3, 3, 3));
    // Perform the opertion on the first row
    vx = _mm_mul_ps(vx, m2.r[0].v);
    vy = _mm_mul_ps(vy, m2.r[1].v);
    vz = _mm_mul_ps(vz, m2.r[2].v);
    vw = _mm_mul_ps(vw, m2.r[3].v);
    // Perform a binary add to reduce cumulative errors
    vx            = _mm_add_ps(vx, vz);
    vy            = _mm_add_ps(vy, vw);
    vx            = _mm_add_ps(vx, vy);
    result.r[0].v = vx;
    // Repeat for the other 3 rows
    vw            = m1.r[1].v;
    vx            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(0, 0, 0, 0));
    vy            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(1, 1, 1, 1));
    vz            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(2, 2, 2, 2));
    vw            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 3, 3, 3));
    vx            = _mm_mul_ps(vx, m2.r[0].v);
    vy            = _mm_mul_ps(vy, m2.r[1].v);
    vz            = _mm_mul_ps(vz, m2.r[2].v);
    vw            = _mm_mul_ps(vw, m2.r[3].v);
    vx            = _mm_add_ps(vx, vz);
    vy            = _mm_add_ps(vy, vw);
    vx            = _mm_add_ps(vx, vy);
    result.r[1].v = vx;
    vw            = m1.r[2].v;
    vx            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(0, 0, 0, 0));
    vy            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(1, 1, 1, 1));
    vz            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(2, 2, 2, 2));
    vw            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 3, 3, 3));
    vx            = _mm_mul_ps(vx, m2.r[0].v);
    vy            = _mm_mul_ps(vy, m2.r[1].v);
    vz            = _mm_mul_ps(vz, m2.r[2].v);
    vw            = _mm_mul_ps(vw, m2.r[3].v);
    vx            = _mm_add_ps(vx, vz);
    vy            = _mm_add_ps(vy, vw);
    vx            = _mm_add_ps(vx, vy);
    result.r[2].v = vx;
    vw            = m1.r[3].v;
    vx            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(0, 0, 0, 0));
    vy            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(1, 1, 1, 1));
    vz            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(2, 2, 2, 2));
    vw            = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 3, 3, 3));
    vx            = _mm_mul_ps(vx, m2.r[0].v);
    vy            = _mm_mul_ps(vy, m2.r[1].v);
    vz            = _mm_mul_ps(vz, m2.r[2].v);
    vw            = _mm_mul_ps(vw, m2.r[3].v);
    vx            = _mm_add_ps(vx, vz);
    vy            = _mm_add_ps(vy, vw);
    vx            = _mm_add_ps(vx, vy);
    result.r[3].v = vx;

    return result;
  }
  else
  {
    mat4_t<scalar_t> res{noinit_v};
    for (int i = 0; i < 4; ++i)
    {
      float x          = m1.m[i * 4 + 0];
      float y          = m1.m[i * 4 + 1];
      float z          = m1.m[i * 4 + 2];
      float w          = m1.m[i * 4 + 3];
      res.m[i * 4 + 0] = (m2.m[0 * 4 + 0] * x) + (m2.m[1 * 4 + 0] * y) + (m2.m[2 * 4 + 0] * z) + (m2.m[3 * 4 + 0] * w);
      res.m[i * 4 + 1] = (m2.m[0 * 4 + 1] * x) + (m2.m[1 * 4 + 1] * y) + (m2.m[2 * 4 + 1] * z) + (m2.m[3 * 4 + 1] * w);
      res.m[i * 4 + 2] = (m2.m[0 * 4 + 2] * x) + (m2.m[1 * 4 + 2] * y) + (m2.m[2 * 4 + 2] * z) + (m2.m[3 * 4 + 2] * w);
      res.m[i * 4 + 3] = (m2.m[0 * 4 + 3] * x) + (m2.m[1 * 4 + 3] * y) + (m2.m[2 * 4 + 3] * z) + (m2.m[3 * 4 + 3] * w);
    }

    return res;
  }
}

/// @brief transform vertices assuming orthogonal matrix
template <typename scalar_t>
inline void transform_assume_ortho(mat4_t<scalar_t> const& m, vec3_t<scalar_t> const* inpstream,
                                   std::uint32_t inpstride, std::uint32_t count, vec3_t<scalar_t>* outstream,
                                   std::uint32_t outstride) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    assert(outstream);
    assert(inpstream);
    const std::uint8_t* inp_vec = (const std::uint8_t*)inpstream;
    std::uint8_t*       out_vec = (std::uint8_t*)outstream;

    for (std::uint32_t i = 0; i < count; i++)
    {
      __m128 x   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec));
      __m128 y   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 1);
      __m128 res = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 2);
      res        = _mm_mul_ps(res, m.r[2].v);
      res        = _mm_add_ps(res, m.r[3].v);
      y          = _mm_mul_ps(y, m.r[1].v);
      res        = _mm_add_ps(res, y);
      x          = _mm_mul_ps(x, m.r[0].v);
      res        = _mm_add_ps(res, x);

      ((float*)out_vec)[0] = _mm_cvtss_f32(res);
      ((float*)out_vec)[1] = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(1, 1, 1, 1)));
      ((float*)out_vec)[2] = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(2, 2, 2, 2)));

      inp_vec += inpstride;
      out_vec += outstride;
    }
  }
  else
  {
    assert(outstream);
    assert(inpstream);
    const std::uint8_t* inp_vec = (const std::uint8_t*)inpstream;
    std::uint8_t*       out_vec = (std::uint8_t*)outstream;

    for (std::uint32_t i = 0; i < count; i++)
    {
      auto x = make_vec3a(((float*)inp_vec)[0]);
      auto y = make_vec3a(((float*)inp_vec)[1]);
      auto z = make_vec3a(((float*)inp_vec)[2]);

      auto r = madd(z, row(m, 2), row(m, 3));
      r      = madd(y, row(m, 1), r);
      r      = madd(x, row(m, 0), r);

      ((float*)out_vec)[0] = r.xyzw[0];
      ((float*)out_vec)[1] = r.xyzw[1];
      ((float*)out_vec)[2] = r.xyzw[2];

      inp_vec += inpstride;
      out_vec += outstride;
    }
  }
}

template <typename scalar_t>
inline void transform_assume_ortho(mat4_t<scalar_t> const& m, vec3_t<scalar_t>* io_stream, std::uint32_t i_stride,
                                   std::uint32_t count) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    assert(io_stream);
    std::uint8_t* inp_vec = (std::uint8_t*)io_stream;

    for (std::uint32_t i = 0; i < count; i++)
    {
      __m128 x   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec));
      __m128 y   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 1);
      __m128 res = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 2);
      res        = _mm_mul_ps(res, m.r[2].v);
      res        = _mm_add_ps(res, m.r[3].v);
      y          = _mm_mul_ps(y, m.r[1].v);
      res        = _mm_add_ps(res, y);
      x          = _mm_mul_ps(x, m.r[0].v);
      res        = _mm_add_ps(res, x);

      ((float*)inp_vec)[0] = _mm_cvtss_f32(res);
      ((float*)inp_vec)[1] = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(1, 1, 1, 1)));
      ((float*)inp_vec)[2] = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(2, 2, 2, 2)));

      inp_vec += i_stride;
    }
  }
  else
  {
    std::uint8_t* inp_vec = (std::uint8_t*)io_stream;

    for (std::uint32_t i = 0; i < count; i++)
    {
      auto x = make_vec3a(((float*)inp_vec)[0]);
      auto y = make_vec3a(((float*)inp_vec)[1]);
      auto z = make_vec3a(((float*)inp_vec)[2]);

      auto r = madd(z, row(m, 2), row(m, 3));
      r      = madd(y, row(m, 1), r);
      r      = madd(x, row(m, 0), r);

      ((float*)inp_vec)[0] = r.xyzw[0];
      ((float*)inp_vec)[1] = r.xyzw[1];
      ((float*)inp_vec)[2] = r.xyzw[2];

      inp_vec += i_stride;
    }
  }
}

/// @brief transform vertices and project the w coord as 1.0.
template <typename scalar_t>
inline void transform_and_project(mat4_t<scalar_t> const& m, vec3_t<scalar_t> const* inpstream, std::uint32_t inpstride,
                                  std::uint32_t count, vec3_t<scalar_t>* outstream, std::uint32_t outstride) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    assert(outstream);
    assert(inpstream);
    const std::uint8_t* inp_vec = (std::uint8_t*)inpstream;
    std::uint8_t*       out_vec = (std::uint8_t*)outstream;

    for (std::uint32_t i = 0; i < count; i++)
    {
      __m128 x   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec));
      __m128 y   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 1);
      __m128 res = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 2);
      res        = _mm_mul_ps(res, m.r[2].v);
      res        = _mm_add_ps(res, m.r[3].v);
      y          = _mm_mul_ps(y, m.r[1].v);
      res        = _mm_add_ps(res, y);
      x          = _mm_mul_ps(x, m.r[0].v);
      res        = _mm_add_ps(res, x);

      x   = _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 3, 3, 3));
      res = _mm_div_ps(res, x);
      _mm_store_ss(reinterpret_cast<float*>(out_vec), res);
      res = _mm_shuffle_ps(res, res, _MM_SHUFFLE(0, 3, 2, 1));
      _mm_store_ss(reinterpret_cast<float*>(out_vec) + 1, res);
      res = _mm_shuffle_ps(res, res, _MM_SHUFFLE(0, 3, 2, 1));
      _mm_store_ss(reinterpret_cast<float*>(out_vec) + 2, res);
      inp_vec += inpstride;
      out_vec += outstride;
    }
  }
  else
  {

    assert(outstream);
    assert(inpstream);
    const std::uint8_t* inp_vec = (const std::uint8_t*)inpstream;
    std::uint8_t*       out_vec = (std::uint8_t*)outstream;

    for (std::uint32_t i = 0; i < count; i++)
    {
      auto x = quad_t<scalar_t>(((float*)inp_vec)[0]);
      auto y = quad_t<scalar_t>(((float*)inp_vec)[1]);
      auto z = quad_t<scalar_t>(((float*)inp_vec)[2]);

      auto r = madd(z, row(m, 2), row(m, 3));
      r      = madd(y, row(m, 1), r);
      r      = madd(x, row(m, 0), r);
      r      = mul(r, 1.f / r[3]);

      ((float*)out_vec)[0] = r[0];
      ((float*)out_vec)[1] = r[1];
      ((float*)out_vec)[2] = r[2];

      inp_vec += inpstride;
      out_vec += outstride;
    }
  }
}

template <typename scalar_t>
inline vec4_t<scalar_t> transform_assume_ortho(mat4_t<scalar_t> const& m, vec3a_t<scalar_t> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto ret    = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    ret         = _mm_mul_ps(ret, m.r[0].v);
    auto v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    v_temp      = _mm_mul_ps(v_temp, m.r[1].v);
    ret         = _mm_add_ps(ret, v_temp);
    v_temp      = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
    v_temp      = _mm_mul_ps(v_temp, m.r[2].v);
    ret         = _mm_add_ps(ret, v_temp);
    ret         = _mm_add_ps(ret, m.r[3].v);
    return ret;
  }
  else
  {
    auto x = splat_x(v);
    auto y = splat_y(v);
    auto z = splat_z(v);

    auto r = madd(z, row(m, 2), row(m, 3));
    r      = madd(y, row(m, 1), r);
    r      = madd(x, row(m, 0), r);
    return r;
  }
}

template <typename scalar_t>
inline vec4_t<scalar_t> transform_and_project(mat4_t<scalar_t> const& m, vec3a_t<scalar_t> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto ret    = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    ret         = _mm_mul_ps(ret, m.r[0].v);
    auto v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    v_temp      = _mm_mul_ps(v_temp, m.r[1].v);
    ret         = _mm_add_ps(ret, v_temp);
    v_temp      = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
    v_temp      = _mm_mul_ps(v_temp, m.r[2].v);
    ret         = _mm_add_ps(ret, v_temp);
    ret         = _mm_add_ps(ret, m.r[3].v);
    v_temp      = _mm_shuffle_ps(ret, ret, _MM_SHUFFLE(3, 3, 3, 3));
    ret         = _mm_div_ps(ret, v_temp);
    return vec4_t<scalar_t>{ret};
  }
  else
  {
    auto x = splat_x(v);
    auto y = splat_y(v);
    auto z = splat_z(v);

    auto r = madd(z, row(m, 2), row(m, 3));
    r      = madd(y, row(m, 1), r);
    r      = madd(x, row(m, 0), r);
    r      = mul(r, 1.f / r[3]);

    return vec4_t<scalar_t>{r};
  }
}

template <typename scalar_t>
inline vec3a_t<scalar_t> mul(vec3a_t<scalar_t> const& v, mat4_t<scalar_t> const& m) noexcept
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

template <typename scalar_t>
inline vec3a_t<scalar_t> operator*(vec3a_t<scalar_t> const& v, mat4_t<scalar_t> const& m) noexcept
{
  return mul(v, m);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> transform_bounds_extends(mat4_t<scalar_t> const& m, vec3a_t<scalar_t> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto ret        = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    auto clear_sign = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
    ret             = _mm_mul_ps(ret, m.r[0].v);
    ret             = _mm_and_ps(ret, clear_sign);
    auto v_temp     = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    v_temp          = _mm_mul_ps(v_temp, m.r[1].v);
    v_temp          = _mm_and_ps(v_temp, clear_sign);
    ret             = _mm_add_ps(ret, v_temp);
    v_temp          = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
    v_temp          = _mm_mul_ps(v_temp, m.r[2].v);
    v_temp          = _mm_and_ps(v_temp, clear_sign);
    ret             = _mm_add_ps(ret, v_temp);
    return ret;
  }
  else
  {
    vec3a_t<scalar_t> ret{noinit_v};
    for (int i = 0; i < 3; i++)
    {
      ret[i] = 0.0f;
      for (int j = 0; j < 3; j++)
      {
        ret[i] += v[j] * std::abs(m.m[i + j * 4]);
      }
    }
    return ret;
  }
}

template <typename scalar_t>
inline aabb_t<scalar_t> mul(aabb_t<scalar_t> const& box, mat4_t<scalar_t> const& m) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    aabb_t<scalar_t> ret;
    auto             max0 = _mm_shuffle_ps(box.r[1].v, box.r[1].v, _MM_SHUFFLE(0, 0, 0, 0));
    max0                  = _mm_mul_ps(max0, m.r[0].v);
    auto min0             = _mm_shuffle_ps(box.r[0].v, box.r[0].v, _MM_SHUFFLE(0, 0, 0, 0));
    min0                  = _mm_mul_ps(min0, m.r[0].v);
    auto max1             = _mm_shuffle_ps(box.r[1].v, box.r[1].v, _MM_SHUFFLE(1, 1, 1, 1));
    max1                  = _mm_mul_ps(max1, m.r[1].v);
    auto min1             = _mm_shuffle_ps(box.r[0].v, box.r[0].v, _MM_SHUFFLE(1, 1, 1, 1));
    min1                  = _mm_mul_ps(min1, m.r[1].v);
    auto max2             = _mm_shuffle_ps(box.r[1].v, box.r[1].v, _MM_SHUFFLE(2, 2, 2, 2));
    max2                  = _mm_mul_ps(max2, m.r[2].v);
    auto min2             = _mm_shuffle_ps(box.r[0].v, box.r[0].v, _MM_SHUFFLE(2, 2, 2, 2));
    min2                  = _mm_mul_ps(min2, m.r[2].v);

    ret.r[0].v = make_vec3a(_mm_add_ps(
      _mm_add_ps(_mm_min_ps(max0, min0), _mm_add_ps(_mm_min_ps(max1, min1), _mm_min_ps(max2, min2))), m.r[3].v));
    ret.r[1].v = make_vec3a(_mm_add_ps(
      _mm_add_ps(_mm_max_ps(max0, min0), _mm_add_ps(_mm_max_ps(max1, min1), _mm_max_ps(max2, min2))), m.r[3].v));
    return ret;
  }
  else
  {
    aabb_t<scalar_t> ret{noinit_v};
    for (int i = 0; i < 3; i++)
    {
      ret.r[0].v[i] = std::min(box.r[0].v[0] * m.m[i + 0 * 4], box.r[1].v[0] * m.m[i + 0 * 4]) +
                      std::min(box.r[0].v[1] * m.m[i + 1 * 4], box.r[1].v[1] * m.m[i + 1 * 4]) +
                      std::min(box.r[0].v[2] * m.m[i + 2 * 4], box.r[1].v[2] * m.m[i + 2 * 4]) + m.m[i + 3 * 4];
    }
    ret.r[0].v[3] = 0;
    for (int i = 0; i < 3; i++)
    {
      ret.r[1].v[i] = std::max(box.r[0].v[0] * m.m[i + 0 * 4], box.r[1].v[0] * m.m[i + 0 * 4]) +
                      std::max(box.r[0].v[1] * m.m[i + 1 * 4], box.r[1].v[1] * m.m[i + 1 * 4]) +
                      std::max(box.r[0].v[2] * m.m[i + 2 * 4], box.r[1].v[2] * m.m[i + 2 * 4]) + m.m[i + 3 * 4];
    }
    ret.r[1].v[3] = 0;
    return ret;
  }
}

template <typename scalar_t>
inline aabb_t<scalar_t> operator*(aabb_t<scalar_t> const& box, mat4_t<scalar_t> const& m) noexcept
{
  return mul(box, m);
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_mat4(scalar_t scale, quat_t<scalar_t> const& rot, vec3a_t<scalar_t> const& pos) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    const __m128 fff0 = _mm_castsi128_ps(_mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));

    auto q0 = _mm_add_ps(rot, rot);
    auto q1 = _mm_mul_ps(rot, q0);

    auto v0 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 0, 0, 1));
    auto v1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 1, 2, 2));
    auto r0 = _mm_sub_ps(_mm_set_ps(0.0f, 1.0f, 1.0f, 1.0f), v0);
    r0      = _mm_and_ps(_mm_sub_ps(r0, v1), fff0);

    v0 = _mm_shuffle_ps(rot, rot, _MM_SHUFFLE(3, 1, 0, 0));
    v1 = _mm_shuffle_ps(q0, q0, _MM_SHUFFLE(3, 2, 1, 2));
    v0 = _mm_mul_ps(v0, v1);

    v1      = _mm_shuffle_ps(rot, rot, _MM_SHUFFLE(3, 3, 3, 3));
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

    auto scaleQ = _mm_set_ps(scale);

    mat4_t<scalar_t> ret{noinit_v};
    ret.r[0].v = _mm_mul_ps(scaleQ, q1);
    q1         = _mm_shuffle_ps(r0, v0, _MM_SHUFFLE(3, 2, 3, 1));
    q1         = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 3, 0, 2));
    ret.r[1].v = _mm_mul_ps(scaleQ, q1);
    q1         = _mm_shuffle_ps(v1, r0, _MM_SHUFFLE(3, 2, 1, 0));
    ret.r[2].v = _mm_mul_ps(scaleQ, q1);
    ret.r[3].v = _mm_or_ps(_mm_and_ps(pos, fff0), _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
    return ret;
  }
  else
  {
    mat4_t<scalar_t> ret{noinit_v};
    float            xx = rot[0] * rot[0];
    float            yy = rot[1] * rot[1];
    float            zz = rot[2] * rot[2];
    float            xy = rot[0] * rot[1];
    float            xz = rot[0] * rot[2];
    float            yz = rot[1] * rot[2];
    float            wx = rot[3] * rot[0];
    float            wy = rot[3] * rot[1];
    float            wz = rot[3] * rot[2];

    ret.e[0][0] = scale * (1 - 2 * (yy + zz));
    ret.e[0][1] = scale * (2 * (xy + wz));
    ret.e[0][2] = scale * (2 * (xz - wy));

    ret.e[1][0] = scale * (2 * (xy - wz));
    ret.e[1][1] = scale * (1 - 2 * (xx + zz));
    ret.e[1][2] = scale * (2 * (yz + wx));

    ret.e[2][0] = scale * (2 * (xz + wy));
    ret.e[2][1] = scale * (2 * (yz - wx));
    ret.e[2][2] = scale * (1 - 2 * (xx + yy));

    ret.e[0][3] = ret.e[1][3] = ret.e[2][3] = 0.0f;
    ret.e[3][0]                             = pos[0];
    ret.e[3][1]                             = pos[1];
    ret.e[3][2]                             = pos[2];
    ret.e[3][3]                             = 1.0f;

    return ret;
  }
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_mat4_from_scale(vec3a_t<scalar_t> const& scale) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    mat4_t<scalar_t> ret{noinit_v};
    ret.r[0].v = _mm_and_ps(scale, _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, 0xFFFFFFFF)));
    ret.r[1].v = _mm_and_ps(scale, _mm_castsi128_ps(_mm_set_epi32(0, 0, 0xFFFFFFFF, 0)));
    ret.r[2].v = _mm_and_ps(scale, _mm_castsi128_ps(_mm_set_epi32(0, 0xFFFFFFFF, 0, 0)));
    ret.r[3].v = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
    return ret;
  }
  else
  {
    mat4_t<scalar_t> ret{noinit_v};
    ret.e[0][0] = scale[0];
    ret.e[0][1] = 0;
    ret.e[0][2] = 0;
    ret.e[0][3] = 0;
    ret.e[1][0] = 0;
    ret.e[1][1] = scale[1];
    ret.e[1][2] = 0;
    ret.e[1][3] = 0;
    ret.e[2][0] = 0;
    ret.e[2][1] = 0;
    ret.e[2][2] = scale[2];
    ret.e[2][3] = 0;
    ret.e[3][0] = 0;
    ret.e[3][1] = 0;
    ret.e[3][2] = 0;
    ret.e[3][3] = 1;
    return ret;
  }
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_mat4_from_translation(vec3a_t<scalar_t> const& pos) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    mat4_t<scalar_t> ret{noinit_v};
    ret.r[0].v = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
    ret.r[1].v = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
    ret.r[2].v = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
    ret.r[3].v = _mm_or_ps(pos.v, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
    return ret;
  }
  else
  {
    mat4_t<scalar_t> ret{noinit_v};
    ret.e[0][0] = 1;
    ret.e[0][1] = 0;
    ret.e[0][2] = 0;
    ret.e[0][3] = 0;
    ret.e[1][0] = 0;
    ret.e[1][1] = 1;
    ret.e[1][2] = 0;
    ret.e[1][3] = 0;
    ret.e[2][0] = 0;
    ret.e[2][1] = 0;
    ret.e[2][2] = 1;
    ret.e[2][3] = 0;
    ret.e[3][0] = pos[0];
    ret.e[3][1] = pos[1];
    ret.e[3][2] = pos[2];
    ret.e[3][3] = 1;
    return ret;
  }
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_mat4_form_quaternion(quat_t<scalar_t> const& rot) noexcept
{
  mat4_t<scalar_t> ret{noinit_v};
  set_rotation(ret, rot);

  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    ret.r[3].v = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
  }
  else
  {
    ret.e[3][0] = 0;
    ret.e[3][1] = 0;
    ret.e[3][2] = 0;
    ret.e[3][3] = 1.0f;
  }
  return ret;
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_view_from_world_mat4(mat4_t<scalar_t> const& m) noexcept
{
  return inverse_assume_ortho(m);
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_view_from_look_at(vec3a_t<scalar_t> const& eye, vec3a_t<scalar_t> const& look_at,
                                               vec3a_t<scalar_t> const& vup) noexcept
{
  mat4_t<scalar_t> m{noinit_v};
  auto             zaxis = m.r[2].v = normalize(sub(look_at, eye));
  auto             xaxis = m.r[0].v = normalize(cross(vup, m.r[2].v));
  auto             yaxis = m.r[1].v = cross(m.r[2].v, m.r[0].v);
  m.r[3].v                          = vec4_t<scalar_t>(0.0f, 0.0f, 0.0f, 1.0f);
  transpose_in_place(m);

  auto neg_eye = negate(eye);
  m.r[3].v     = vec4_t<scalar_t>(dot(xaxis, neg_eye), dot(yaxis, neg_eye), dot(zaxis, neg_eye), 1);

  return m;
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_orthographic_projection(scalar_t min_x, scalar_t max_x, scalar_t min_y, scalar_t max_y,
                                                     scalar_t zn, scalar_t zf) noexcept
{
  float dx_recip = 1.0f / (zf - zn);
  // clang-format off
    return mat4_t<scalar_t>{
		2.0f / (max_x - min_x),              0.0f,   	                          0.0f,             0.0f,
	    0.0f,                                2.0f / (max_y - min_y),              0.0f,             0.0f,
	    0.0f,                                0.0f,                                dx_recip,         0.0f,
	    (max_x + min_x) / (min_x - max_x),   (max_y + min_y) / (max_y - min_y),   -(zn) * dx_recip, 1.0f
	};
  // clang-format on
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_orthographic_projection(scalar_t w, scalar_t h, scalar_t zn, scalar_t zf) noexcept
{
  float dx_recip = 1.0f / (zf - zn);
  // clang-format off
    return mat4_t<scalar_t>{
		2.0f / w,  0.0f,   	   0.0f,                0.0f,
	    0.0f,      2.0f / h,   0.0f,                0.0f,
	    0.0f,      0.0f,       dx_recip,            0.0f,
	    0.0f,      0.0f,      -(zn) * dx_recip,     1.0f
	};
  // clang-format on
}

template <typename scalar_t>
inline mat4_t<scalar_t> make_perspective_projection(scalar_t field_of_view, scalar_t aspect_ratio, scalar_t zn,
                                                    scalar_t zf) noexcept
{
  field_of_view *= 0.5f;

  float yscale = 1 / std::tan(field_of_view);
  float q      = zf / (zf - zn);
  // clang-format off
    return mat4_t<scalar_t>{
		(yscale / aspect_ratio),        0,        0,        0,
	     0,                        yscale,        0,        0,
		 0,                             0,        q,        1,
		 0,                             0,  -q * zn,        0
	};
  // clang-format on
}

template <typename scalar_t>
inline mat4_t<scalar_t> mul(mat4_t<scalar_t> const& m, scalar_t scale) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    mat4_t<scalar_t> ret{noinit_v};
    auto             scaleQ = _mm_set_ps1(scale);
    ret.r[0].v              = _mm_mul_ps(scaleQ, m.r[0].v);
    ret.r[1].v              = _mm_mul_ps(scaleQ, m.r[1].v);
    ret.r[2].v              = _mm_mul_ps(scaleQ, m.r[2].v);
    ret.r[3].v              = _mm_mul_ps(scaleQ, m.r[3].v);
    return ret;
  }
  else
  {
    mat4_t<scalar_t> ret = m;
    for (int i = 0; i < 4; ++i)
    {
      ret.e[i][0] *= scale;
      ret.e[i][1] *= scale;
      ret.e[i][2] *= scale;
      ret.e[i][3] *= scale;
    }
    return ret;
  }
}

template <typename scalar_t>
inline mat4_t<scalar_t> mul(scalar_t scale, mat4_t<scalar_t> const& m) noexcept
{
  return mul(m, scale);
}

template <typename scalar_t>
inline mat4_t<scalar_t> transpose(mat4_t<scalar_t> const& m) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto result = m;
    _MM_TRANSPOSE4_PS(result.r[0].v, result.r[1].v, result.r[2].v, result.r[3].v);
    return result;
  }
  else
  {
    mat4_t<scalar_t> ret{noinit_v};
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        ret.m[i * 4 + j] = m.m[j * 4 + i];

    return ret;
  }
}

template <typename scalar_t>
inline void transpose_in_place(mat4_t<scalar_t>& m) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    _MM_TRANSPOSE4_PS(m.r[0].v, m.r[1].v, m.r[2].v, m.r[3].v);
  }
  else
  {
    std::swap(m.e[0][1], m.e[1][0]);
    std::swap(m.e[0][2], m.e[2][0]);
    std::swap(m.e[0][3], m.e[3][0]);
    std::swap(m.e[1][2], m.e[2][1]);
    std::swap(m.e[1][3], m.e[3][1]);
    std::swap(m.e[2][3], m.e[3][2]);
  }
}

/// @brief Matrix full inverse computation
template <typename scalar_t>
inline mat4_t<scalar_t> inverse(mat4_t<scalar_t> const& m) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto MT  = transpose(m);
    auto V00 = _mm_shuffle_ps(MT.r[2].v, MT.r[2].v, _MM_SHUFFLE(1, 1, 0, 0));
    auto V10 = _mm_shuffle_ps(MT.r[3].v, MT.r[3].v, _MM_SHUFFLE(3, 2, 3, 2));
    auto V01 = _mm_shuffle_ps(MT.r[0].v, MT.r[0].v, _MM_SHUFFLE(1, 1, 0, 0));
    auto V11 = _mm_shuffle_ps(MT.r[1].v, MT.r[1].v, _MM_SHUFFLE(3, 2, 3, 2));
    auto V02 = _mm_shuffle_ps(MT.r[2].v, MT.r[0].v, _MM_SHUFFLE(2, 0, 2, 0));
    auto V12 = _mm_shuffle_ps(MT.r[3].v, MT.r[1].v, _MM_SHUFFLE(3, 1, 3, 1));

    auto D0 = _mm_mul_ps(V00, V10);
    auto D1 = _mm_mul_ps(V01, V11);
    auto D2 = _mm_mul_ps(V02, V12);

    V00 = _mm_shuffle_ps(MT.r[2].v, MT.r[2].v, _MM_SHUFFLE(3, 2, 3, 2));
    V10 = _mm_shuffle_ps(MT.r[3].v, MT.r[3].v, _MM_SHUFFLE(1, 1, 0, 0));
    V01 = _mm_shuffle_ps(MT.r[0].v, MT.r[0].v, _MM_SHUFFLE(3, 2, 3, 2));
    V11 = _mm_shuffle_ps(MT.r[1].v, MT.r[1].v, _MM_SHUFFLE(1, 1, 0, 0));
    V02 = _mm_shuffle_ps(MT.r[2].v, MT.r[0].v, _MM_SHUFFLE(3, 1, 3, 1));
    V12 = _mm_shuffle_ps(MT.r[3].v, MT.r[1].v, _MM_SHUFFLE(2, 0, 2, 0));

    V00 = _mm_mul_ps(V00, V10);
    V01 = _mm_mul_ps(V01, V11);
    V02 = _mm_mul_ps(V02, V12);
    D0  = _mm_sub_ps(D0, V00);
    D1  = _mm_sub_ps(D1, V01);
    D2  = _mm_sub_ps(D2, V02);
    // V11 = D0Y,D0W,D2Y,D2Y
    V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 1, 3, 1));
    V00 = _mm_shuffle_ps(MT.r[1].v, MT.r[1].v, _MM_SHUFFLE(1, 0, 2, 1));
    V10 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(0, 3, 0, 2));
    V01 = _mm_shuffle_ps(MT.r[0].v, MT.r[0].v, _MM_SHUFFLE(0, 1, 0, 2));
    V11 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(2, 1, 2, 1));
    // V13 = D1Y,D1W,D2W,D2W
    auto V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 3, 3, 1));
    V02      = _mm_shuffle_ps(MT.r[3].v, MT.r[3].v, _MM_SHUFFLE(1, 0, 2, 1));
    V12      = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(0, 3, 0, 2));
    auto V03 = _mm_shuffle_ps(MT.r[2].v, MT.r[2].v, _MM_SHUFFLE(0, 1, 0, 2));
    V13      = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(2, 1, 2, 1));

    auto C0 = _mm_mul_ps(V00, V10);
    auto C2 = _mm_mul_ps(V01, V11);
    auto C4 = _mm_mul_ps(V02, V12);
    auto C6 = _mm_mul_ps(V03, V13);

    // V11 = D0X,D0Y,D2X,D2X
    V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(0, 0, 1, 0));
    V00 = _mm_shuffle_ps(MT.r[1].v, MT.r[1].v, _MM_SHUFFLE(2, 1, 3, 2));
    V10 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(2, 1, 0, 3));
    V01 = _mm_shuffle_ps(MT.r[0].v, MT.r[0].v, _MM_SHUFFLE(1, 3, 2, 3));
    V11 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(0, 2, 1, 2));
    // V13 = D1X,D1Y,D2Z,D2Z
    V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(2, 2, 1, 0));
    V02 = _mm_shuffle_ps(MT.r[3].v, MT.r[3].v, _MM_SHUFFLE(2, 1, 3, 2));
    V12 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(2, 1, 0, 3));
    V03 = _mm_shuffle_ps(MT.r[2].v, MT.r[2].v, _MM_SHUFFLE(1, 3, 2, 3));
    V13 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(0, 2, 1, 2));

    V00 = _mm_mul_ps(V00, V10);
    V01 = _mm_mul_ps(V01, V11);
    V02 = _mm_mul_ps(V02, V12);
    V03 = _mm_mul_ps(V03, V13);
    C0  = _mm_sub_ps(C0, V00);
    C2  = _mm_sub_ps(C2, V01);
    C4  = _mm_sub_ps(C4, V02);
    C6  = _mm_sub_ps(C6, V03);

    V00 = _mm_shuffle_ps(MT.r[1].v, MT.r[1].v, _MM_SHUFFLE(0, 3, 0, 3));
    // V10 = D0Z,D0Z,D2X,D2Y
    V10 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 2, 2));
    V10 = _mm_shuffle_ps(V10, V10, _MM_SHUFFLE(0, 2, 3, 0));
    V01 = _mm_shuffle_ps(MT.r[0].v, MT.r[0].v, _MM_SHUFFLE(2, 0, 3, 1));
    // V11 = D0X,D0W,D2X,D2Y
    V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 3, 0));
    V11 = _mm_shuffle_ps(V11, V11, _MM_SHUFFLE(2, 1, 0, 3));
    V02 = _mm_shuffle_ps(MT.r[3].v, MT.r[3].v, _MM_SHUFFLE(0, 3, 0, 3));
    // V12 = D1Z,D1Z,D2Z,D2W
    V12 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 2, 2));
    V12 = _mm_shuffle_ps(V12, V12, _MM_SHUFFLE(0, 2, 3, 0));
    V03 = _mm_shuffle_ps(MT.r[2].v, MT.r[2].v, _MM_SHUFFLE(2, 0, 3, 1));
    // V13 = D1X,D1W,D2Z,D2W
    V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 3, 0));
    V13 = _mm_shuffle_ps(V13, V13, _MM_SHUFFLE(2, 1, 0, 3));

    V00     = _mm_mul_ps(V00, V10);
    V01     = _mm_mul_ps(V01, V11);
    V02     = _mm_mul_ps(V02, V12);
    V03     = _mm_mul_ps(V03, V13);
    auto C1 = _mm_sub_ps(C0, V00);
    C0      = _mm_add_ps(C0, V00);
    auto C3 = _mm_add_ps(C2, V01);
    C2      = _mm_sub_ps(C2, V01);
    auto C5 = _mm_sub_ps(C4, V02);
    C4      = _mm_add_ps(C4, V02);
    auto C7 = _mm_add_ps(C6, V03);
    C6      = _mm_sub_ps(C6, V03);

    C0 = _mm_shuffle_ps(C0, C1, _MM_SHUFFLE(3, 1, 2, 0));
    C2 = _mm_shuffle_ps(C2, C3, _MM_SHUFFLE(3, 1, 2, 0));
    C4 = _mm_shuffle_ps(C4, C5, _MM_SHUFFLE(3, 1, 2, 0));
    C6 = _mm_shuffle_ps(C6, C7, _MM_SHUFFLE(3, 1, 2, 0));
    C0 = _mm_shuffle_ps(C0, C0, _MM_SHUFFLE(3, 1, 2, 0));
    C2 = _mm_shuffle_ps(C2, C2, _MM_SHUFFLE(3, 1, 2, 0));
    C4 = _mm_shuffle_ps(C4, C4, _MM_SHUFFLE(3, 1, 2, 0));
    C6 = _mm_shuffle_ps(C6, C6, _MM_SHUFFLE(3, 1, 2, 0));
    // get the determinate
    auto             v_temp = splat_x(_mm_div_ss(_mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f), vdot(C0, MT.r[0].v)));
    mat4_t<scalar_t> result{noinit_v};
    result.r[0].v = _mm_mul_ps(C0, v_temp);
    result.r[1].v = _mm_mul_ps(C2, v_temp);
    result.r[2].v = _mm_mul_ps(C4, v_temp);
    result.r[3].v = _mm_mul_ps(C6, v_temp);
    return result;
  }
  else
  {
    mat4_t<scalar_t> inv{noinit_v};

    inv.m[0] = m.m[5] * m.m[10] * m.m[15] - m.m[5] * m.m[11] * m.m[14] - m.m[9] * m.m[6] * m.m[15] +
               m.m[9] * m.m[7] * m.m[14] + m.m[13] * m.m[6] * m.m[11] - m.m[13] * m.m[7] * m.m[10];

    inv.m[4] = -m.m[4] * m.m[10] * m.m[15] + m.m[4] * m.m[11] * m.m[14] + m.m[8] * m.m[6] * m.m[15] -
               m.m[8] * m.m[7] * m.m[14] - m.m[12] * m.m[6] * m.m[11] + m.m[12] * m.m[7] * m.m[10];

    inv.m[8] = m.m[4] * m.m[9] * m.m[15] - m.m[4] * m.m[11] * m.m[13] - m.m[8] * m.m[5] * m.m[15] +
               m.m[8] * m.m[7] * m.m[13] + m.m[12] * m.m[5] * m.m[11] - m.m[12] * m.m[7] * m.m[9];

    inv.m[12] = -m.m[4] * m.m[9] * m.m[14] + m.m[4] * m.m[10] * m.m[13] + m.m[8] * m.m[5] * m.m[14] -
                m.m[8] * m.m[6] * m.m[13] - m.m[12] * m.m[5] * m.m[10] + m.m[12] * m.m[6] * m.m[9];

    inv.m[1] = -m.m[1] * m.m[10] * m.m[15] + m.m[1] * m.m[11] * m.m[14] + m.m[9] * m.m[2] * m.m[15] -
               m.m[9] * m.m[3] * m.m[14] - m.m[13] * m.m[2] * m.m[11] + m.m[13] * m.m[3] * m.m[10];

    inv.m[5] = m.m[0] * m.m[10] * m.m[15] - m.m[0] * m.m[11] * m.m[14] - m.m[8] * m.m[2] * m.m[15] +
               m.m[8] * m.m[3] * m.m[14] + m.m[12] * m.m[2] * m.m[11] - m.m[12] * m.m[3] * m.m[10];

    inv.m[9] = -m.m[0] * m.m[9] * m.m[15] + m.m[0] * m.m[11] * m.m[13] + m.m[8] * m.m[1] * m.m[15] -
               m.m[8] * m.m[3] * m.m[13] - m.m[12] * m.m[1] * m.m[11] + m.m[12] * m.m[3] * m.m[9];

    inv.m[13] = m.m[0] * m.m[9] * m.m[14] - m.m[0] * m.m[10] * m.m[13] - m.m[8] * m.m[1] * m.m[14] +
                m.m[8] * m.m[2] * m.m[13] + m.m[12] * m.m[1] * m.m[10] - m.m[12] * m.m[2] * m.m[9];

    inv.m[2] = m.m[1] * m.m[6] * m.m[15] - m.m[1] * m.m[7] * m.m[14] - m.m[5] * m.m[2] * m.m[15] +
               m.m[5] * m.m[3] * m.m[14] + m.m[13] * m.m[2] * m.m[7] - m.m[13] * m.m[3] * m.m[6];

    inv.m[6] = -m.m[0] * m.m[6] * m.m[15] + m.m[0] * m.m[7] * m.m[14] + m.m[4] * m.m[2] * m.m[15] -
               m.m[4] * m.m[3] * m.m[14] - m.m[12] * m.m[2] * m.m[7] + m.m[12] * m.m[3] * m.m[6];

    inv.m[10] = m.m[0] * m.m[5] * m.m[15] - m.m[0] * m.m[7] * m.m[13] - m.m[4] * m.m[1] * m.m[15] +
                m.m[4] * m.m[3] * m.m[13] + m.m[12] * m.m[1] * m.m[7] - m.m[12] * m.m[3] * m.m[5];

    inv.m[14] = -m.m[0] * m.m[5] * m.m[14] + m.m[0] * m.m[6] * m.m[13] + m.m[4] * m.m[1] * m.m[14] -
                m.m[4] * m.m[2] * m.m[13] - m.m[12] * m.m[1] * m.m[6] + m.m[12] * m.m[2] * m.m[5];

    inv.m[3] = -m.m[1] * m.m[6] * m.m[11] + m.m[1] * m.m[7] * m.m[10] + m.m[5] * m.m[2] * m.m[11] -
               m.m[5] * m.m[3] * m.m[10] - m.m[9] * m.m[2] * m.m[7] + m.m[9] * m.m[3] * m.m[6];

    inv.m[7] = m.m[0] * m.m[6] * m.m[11] - m.m[0] * m.m[7] * m.m[10] - m.m[4] * m.m[2] * m.m[11] +
               m.m[4] * m.m[3] * m.m[10] + m.m[8] * m.m[2] * m.m[7] - m.m[8] * m.m[3] * m.m[6];

    inv.m[11] = -m.m[0] * m.m[5] * m.m[11] + m.m[0] * m.m[7] * m.m[9] + m.m[4] * m.m[1] * m.m[11] -
                m.m[4] * m.m[3] * m.m[9] - m.m[8] * m.m[1] * m.m[7] + m.m[8] * m.m[3] * m.m[5];

    inv.m[15] = m.m[0] * m.m[5] * m.m[10] - m.m[0] * m.m[6] * m.m[9] - m.m[4] * m.m[1] * m.m[10] +
                m.m[4] * m.m[2] * m.m[9] + m.m[8] * m.m[1] * m.m[6] - m.m[8] * m.m[2] * m.m[5];

    float det = m.m[0] * inv.m[0] + m.m[1] * inv.m[4] + m.m[2] * inv.m[8] + m.m[3] * inv.m[12];

    if (det == 0)
      return inv;

    det = 1.0f / det;

    for (int i = 0; i < 16; i++)
      inv.m[i] = inv.m[i] * det;
    return inv;
  }
}

template <typename scalar_t>
inline mat4_t<scalar_t> inverse_assume_ortho(mat4_t<scalar_t> const& m) noexcept
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    // inverse = [ T(R)       0 ]
    //        [ -vpos*T(R) 1 ]
    mat4_t<scalar_t> ret{noinit_v};
    ret.r[0].v = _mm_shuffle_ps(m.r[1].v, m.r[2].v, _MM_SHUFFLE(3, 0, 0, 3));
    ret.r[0].v = _mm_move_ss(ret.r[0].v, m.r[0].v);
    ret.r[1].v = _mm_shuffle_ps(m.r[0].v, m.r[1].v, _MM_SHUFFLE(3, 1, 3, 1));
    ret.r[1].v = _mm_shuffle_ps(ret.r[1].v, m.r[2].v, _MM_SHUFFLE(3, 1, 2, 0));
    ret.r[2].v = _mm_shuffle_ps(m.r[0].v, m.r[1].v, _MM_SHUFFLE(3, 2, 3, 2));
    ret.r[2].v = _mm_shuffle_ps(ret.r[2].v, m.r[2].v, _MM_SHUFFLE(3, 2, 2, 0));

    auto v_temp = _mm_shuffle_ps(m.r[3].v, m.r[3].v, _MM_SHUFFLE(0, 0, 0, 0));
    ret.r[3].v  = _mm_mul_ps(v_temp, ret.r[0].v);
    v_temp      = _mm_shuffle_ps(m.r[3].v, m.r[3].v, _MM_SHUFFLE(1, 1, 1, 1));
    v_temp      = _mm_mul_ps(v_temp, ret.r[1].v);
    ret.r[3].v  = _mm_add_ps(ret.r[3].v, v_temp);
    v_temp      = _mm_shuffle_ps(m.r[3].v, m.r[3].v, _MM_SHUFFLE(2, 2, 2, 2));
    v_temp      = _mm_mul_ps(v_temp, ret.r[2].v);
    ret.r[3].v  = _mm_add_ps(ret.r[3].v, v_temp);
    ret.r[3].v  = _mm_xor_ps(ret.r[3].v, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
    // and with 0001
    ret.r[3].v = _mm_or_ps(_mm_and_ps(ret.r[3].v, clear_w_mask()), xyz0_w1());

    assert(ret.e[0][3] == 0.f && ret.e[1][3] == 0.f && ret.e[2][3] == 0.f && ret.e[3][3] == 1.f);
    return ret;
  }
  else
  {
    mat4_t<scalar_t> ret;
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
        ret.m[i * 4 + j] = m.m[j * 4 + i];
      ret.m[i * 4 + 3] = 0;
    }

    ret.e[3][0] = -dot(row(m, 3), row(m, 0));
    ret.e[3][1] = -dot(row(m, 3), row(m, 1));
    ret.e[3][2] = -dot(row(m, 3), row(m, 2));
    ret.e[3][3] = 1;
    return ret;
  }
}
template <typename scalar_t>
inline mat3_t<scalar_t> const& as_mat3(mat4_t<scalar_t> const& m) noexcept
{
  return reinterpret_cast<mat3_t<scalar_t> const&>(m);
}

template <typename scalar_t>
inline mat3_t<scalar_t>& as_mat3(mat4_t<scalar_t>& m) noexcept
{
  return reinterpret_cast<mat3_t<scalar_t>&>(m);
}

template <typename scalar_t>
inline auto operator+(mat4_t<scalar_t> const& a, mat4_t<scalar_t> const& b) noexcept
{
  return add(a, b);
}

template <typename scalar_t>
inline auto operator-(mat4_t<scalar_t> const& a, mat4_t<scalar_t> const& b) noexcept
{
  return sub(a, b);
}

template <typename scalar_t>
inline auto operator*(mat4_t<scalar_t> const& a, mat4_t<scalar_t> const& b) noexcept
{
  return mul(a, b);
}

template <typename scalar_t>
inline auto operator*(mat4_t<scalar_t> const& a, scalar_t b) noexcept
{
  return mul(a, b);
}

template <typename scalar_t>
inline auto operator*(scalar_t a, mat4_t<scalar_t> const& b) noexcept
{
  return mul(a, b);
}

} // namespace acl
