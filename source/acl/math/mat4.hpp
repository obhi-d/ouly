#pragma once
#include "aabb.hpp"
#include "mat_base.hpp"
namespace acl
{
namespace detail
{
struct mat4_traits
{
  using type        = types::mat4_t<float>;
  using ref         = type&;
  using pref        = type const&;
  using cref        = type const&;
  using row_type    = types::vec4_t<float>;
  using row_tag     = vec4;
  using scalar_type = float;

  static constexpr std::uint32_t element_count = 16;
  static constexpr std::uint32_t row_count     = 4;
  static constexpr std::uint32_t column_count  = 4;
};
} // namespace detail

struct mat4 : public mat_base<detail::mat4_traits>
{
  using typename mat_base<detail::mat4_traits>::type;
  using typename mat_base<detail::mat4_traits>::pref;
  using typename mat_base<detail::mat4_traits>::ref;
  using typename mat_base<detail::mat4_traits>::cref;
  using typename mat_base<detail::mat4_traits>::row_type;
  using typename mat_base<detail::mat4_traits>::row_tag;
  using typename mat_base<detail::mat4_traits>::scalar_type;

  //! @brief Returns identity matrix
  static inline mat4_t identity();
  //! @brief Returns maximum scaling
  static inline float max_scale(mat4_t const&);
  //! @brief Full matrix multiplication
  static inline type mul(pref m1, pref m2);
  //! @brief transform vertices assuming orthogonal matrix
  static inline void transform_assume_ortho(pref m, const vec3::type* i_stream, std::uint32_t i_stride,
                                            std::uint32_t count, vec3::type* o_stream, std::uint32_t i_output_stride);
  //! @brief transform vertices in place, assuming orthogonal matrix
  static inline void transform_assume_ortho(pref m, vec3::type* io_stream, std::uint32_t i_stride, std::uint32_t count);
  //! @brief transform vertices and project the w coord as 1.0.
  static inline void transform_and_project(pref m, const vec3::type* i_stream, std::uint32_t i_stride,
                                           std::uint32_t count, vec3::type* o_stream, std::uint32_t i_output_stride);
  //! @brief transform vertices assuming orthogonal matrix
  static inline vec4_t transform_assume_ortho(pref m, vec3a::pref v);
  //! @brief transform vertices and project the w coord as 1.0.
  static inline vec4_t transform_and_project(pref m, vec3a::pref v);
  //! @brief Special transform for AABB bound extends.
  static inline vec3a_t transform_bounds_extends(pref m, vec3a::pref extends);
  //! @brief Special transform for AABB min and max
  static inline aabb_t transform_aabb(pref m, aabb::pref v);

  static inline type from_scale_rotation_translation(scalar_type scale, quat::pref rot, vec3a::pref pos);
  static inline type from_scale(vec3a::pref scale);
  static inline type from_translation(vec3a::pref pos);
  //! @brief Create a rotation matrix from quaternion
  static inline type from_quat(quat::pref rot);
  //! @brief Create a rotation matrix from quaternion
  static inline type from_rotation(quat::pref rot);
  //! @brief Create a view matrix from camera world matrix
  static inline type from_world_to_view(pref m);
  //! @brief mat4::type Creates a camera look at matrix
  static inline type from_look_at(vec3a::pref eye, vec3a::pref look_at, vec3a::pref up);
  //! @brief Orthogonal Projection matrix
  static inline type from_orthographic_projection(scalar_type min_x, scalar_type max_x, scalar_type min_y,
                                                  scalar_type max_y, scalar_type near_plane, scalar_type far_plane);
  //! @brief Orthogonal Projection matrix
  static inline type from_orthographic_projection(scalar_type width, scalar_type height, scalar_type near_plane,
                                                  scalar_type far_plane);
  //! @brief Perspective Projection matrix
  static inline type from_perspective_projection(scalar_type field_of_view, scalar_type aspect_ratio,
                                                 scalar_type near_plane, scalar_type far_plane);

  static inline type   mul(pref m, scalar_type amount);
  static inline type   mul(scalar_type amount, pref m);
  static inline vec4_t mul(vec4::pref v, mat4::pref m);

  static inline type transpose(pref m);
  static inline void transpose_in_place(ref m);
  static inline type inverse(pref m);
  //! @brief inverse for orthogonal matrix
  static inline type          inverse_assume_ortho(pref m);
  static inline mat3_t const& as_mat3(mat4_t const& m);
  static inline mat3_t&       as_mat3(mat4_t& m);
};

inline mat4_t mat4::identity()
{
  return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}
inline float mat4::max_scale(mat4_t const& m)
{
  return acl::sqrt(std::max(std::max(quad::sqlength(m.r[0]), quad::sqlength(m.r[1])), quad::sqlength(m.r[2])));
}

inline mat4::type mat4::mul(pref m1, pref m2)
{
#if VML_USE_SSE_AVX
  mat4_t result;
  // Use vw to hold the original row
  quad_t vw = m1.r[0];
  // Splat the component x,y,Z then W
  quad_t vx = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(0, 0, 0, 0));
  quad_t vy = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(1, 1, 1, 1));
  quad_t vz = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(2, 2, 2, 2));
  vw        = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 3, 3, 3));
  // Perform the opertion on the first row
  vx = _mm_mul_ps(vx, m2.r[0]);
  vy = _mm_mul_ps(vy, m2.r[1]);
  vz = _mm_mul_ps(vz, m2.r[2]);
  vw = _mm_mul_ps(vw, m2.r[3]);
  // Perform a binary add to reduce cumulative errors
  vx          = _mm_add_ps(vx, vz);
  vy          = _mm_add_ps(vy, vw);
  vx          = _mm_add_ps(vx, vy);
  result.r[0] = vx;
  // Repeat for the other 3 rows
  vw          = m1.r[1];
  vx          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(0, 0, 0, 0));
  vy          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(1, 1, 1, 1));
  vz          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(2, 2, 2, 2));
  vw          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 3, 3, 3));
  vx          = _mm_mul_ps(vx, m2.r[0]);
  vy          = _mm_mul_ps(vy, m2.r[1]);
  vz          = _mm_mul_ps(vz, m2.r[2]);
  vw          = _mm_mul_ps(vw, m2.r[3]);
  vx          = _mm_add_ps(vx, vz);
  vy          = _mm_add_ps(vy, vw);
  vx          = _mm_add_ps(vx, vy);
  result.r[1] = vx;
  vw          = m1.r[2];
  vx          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(0, 0, 0, 0));
  vy          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(1, 1, 1, 1));
  vz          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(2, 2, 2, 2));
  vw          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 3, 3, 3));
  vx          = _mm_mul_ps(vx, m2.r[0]);
  vy          = _mm_mul_ps(vy, m2.r[1]);
  vz          = _mm_mul_ps(vz, m2.r[2]);
  vw          = _mm_mul_ps(vw, m2.r[3]);
  vx          = _mm_add_ps(vx, vz);
  vy          = _mm_add_ps(vy, vw);
  vx          = _mm_add_ps(vx, vy);
  result.r[2] = vx;
  vw          = m1.r[3];
  vx          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(0, 0, 0, 0));
  vy          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(1, 1, 1, 1));
  vz          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(2, 2, 2, 2));
  vw          = _mm_shuffle_ps(vw, vw, _MM_SHUFFLE(3, 3, 3, 3));
  vx          = _mm_mul_ps(vx, m2.r[0]);
  vy          = _mm_mul_ps(vy, m2.r[1]);
  vz          = _mm_mul_ps(vz, m2.r[2]);
  vw          = _mm_mul_ps(vw, m2.r[3]);
  vx          = _mm_add_ps(vx, vz);
  vy          = _mm_add_ps(vy, vw);
  vx          = _mm_add_ps(vx, vy);
  result.r[3] = vx;
  return result;
#else
  mat4_t res;
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
#endif
}

inline void mat4::transform_assume_ortho(pref m, const vec3::type* inpstream, std::uint32_t inpstride,
                                         std::uint32_t count, vec3::type* outstream, std::uint32_t outstride)
{
#if VML_USE_SSE_AVX
  assert(outstream);
  assert(inpstream);
  const std::uint8_t* inp_vec = (const std::uint8_t*)inpstream;
  std::uint8_t*       out_vec = (std::uint8_t*)outstream;

  for (std::uint32_t i = 0; i < count; i++)
  {
    quad_t x   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec));
    quad_t y   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 1);
    quad_t res = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 2);
    res        = _mm_mul_ps(res, m.r[2]);
    res        = _mm_add_ps(res, m.r[3]);
    y          = _mm_mul_ps(y, m.r[1]);
    res        = _mm_add_ps(res, y);
    x          = _mm_mul_ps(x, m.r[0]);
    res        = _mm_add_ps(res, x);

    ((float*)out_vec)[0] = quad::x(res);
    ((float*)out_vec)[1] = quad::y(res);
    ((float*)out_vec)[2] = quad::z(res);

    inp_vec += inpstride;
    out_vec += outstride;
  }
#else
  assert(outstream);
  assert(inpstream);
  const std::uint8_t* inp_vec = (const std::uint8_t*)inpstream;
  std::uint8_t*       out_vec = (std::uint8_t*)outstream;

  vec3a::type x, y, z, r;

  for (std::uint32_t i = 0; i < count; i++)
  {
    x = vec3a::set(((float*)inp_vec)[0]);
    y = vec3a::set(((float*)inp_vec)[1]);
    z = vec3a::set(((float*)inp_vec)[2]);

    r = vec3a::madd(z, mat4::row(m, 2), mat4::row(m, 3));
    r = vec3a::madd(y, row(m, 1), r);
    r = vec3a::madd(x, row(m, 0), r);

    ((float*)out_vec)[0] = r[0];
    ((float*)out_vec)[1] = r[1];
    ((float*)out_vec)[2] = r[2];

    inp_vec += inpstride;
    out_vec += outstride;
  }
#endif
}

inline void mat4::transform_assume_ortho(pref m, vec3::type* io_stream, std::uint32_t i_stride, std::uint32_t count)
{
#if VML_USE_SSE_AVX
  assert(io_stream);
  std::uint8_t* inp_vec = (std::uint8_t*)io_stream;

  for (std::uint32_t i = 0; i < count; i++)
  {
    quad_t x             = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec));
    quad_t y             = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 1);
    quad_t res           = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 2);
    res                  = _mm_mul_ps(res, m.r[2]);
    res                  = _mm_add_ps(res, m.r[3]);
    y                    = _mm_mul_ps(y, m.r[1]);
    res                  = _mm_add_ps(res, y);
    x                    = _mm_mul_ps(x, m.r[0]);
    res                  = _mm_add_ps(res, x);
    ((float*)inp_vec)[0] = quad::x(res);
    ((float*)inp_vec)[1] = quad::y(res);
    ((float*)inp_vec)[2] = quad::z(res);

    inp_vec += i_stride;
  }
#else
  std::uint8_t* inp_vec = (std::uint8_t*)io_stream;

  quad_t x, y, z, r;

  for (std::uint32_t i = 0; i < count; i++)
  {
    x = vec3a::set(((float*)inp_vec)[0]);
    y = vec3a::set(((float*)inp_vec)[1]);
    z = vec3a::set(((float*)inp_vec)[2]);

    r = vec3a::madd(z, row(m, 2), row(m, 3));
    r = vec3a::madd(y, row(m, 1), r);
    r = vec3a::madd(x, row(m, 0), r);

    ((float*)inp_vec)[0] = r[0];
    ((float*)inp_vec)[1] = r[1];
    ((float*)inp_vec)[2] = r[2];

    inp_vec += i_stride;
  }
#endif
}

inline void mat4::transform_and_project(pref m, const vec3::type* inpstream, std::uint32_t inpstride,
                                        std::uint32_t count, vec3::type* outstream, std::uint32_t outstride)
{
#if VML_USE_SSE_AVX
  assert(outstream);
  assert(inpstream);
  const std::uint8_t* inp_vec = (std::uint8_t*)inpstream;
  std::uint8_t*       out_vec = (std::uint8_t*)outstream;

  for (std::uint32_t i = 0; i < count; i++)
  {
    quad_t x   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec));
    quad_t y   = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 1);
    quad_t res = _mm_load_ps1(reinterpret_cast<const float*>(inp_vec) + 2);
    res        = _mm_mul_ps(res, m.r[2]);
    res        = _mm_add_ps(res, m.r[3]);
    y          = _mm_mul_ps(y, m.r[1]);
    res        = _mm_add_ps(res, y);
    x          = _mm_mul_ps(x, m.r[0]);
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
#else

  assert(outstream);
  assert(inpstream);
  const std::uint8_t* inp_vec = (const std::uint8_t*)inpstream;
  std::uint8_t*       out_vec = (std::uint8_t*)outstream;

  quad_t x, y, z, r;

  for (std::uint32_t i = 0; i < count; i++)
  {
    x = vec4::set(((float*)inp_vec)[0]);
    y = vec4::set(((float*)inp_vec)[1]);
    z = vec4::set(((float*)inp_vec)[2]);

    r = vec4::madd(z, row(m, 2), row(m, 3));
    r = vec4::madd(y, row(m, 1), r);
    r = vec4::madd(x, row(m, 0), r);
    r = vec4::mul(r, 1.f / r[3]);

    ((float*)out_vec)[0] = r[0];
    ((float*)out_vec)[1] = r[1];
    ((float*)out_vec)[2] = r[2];

    inp_vec += inpstride;
    out_vec += outstride;
  }
#endif
}

inline vec4::type mat4::transform_assume_ortho(pref m, vec3a::pref v)
{
#if VML_USE_SSE_AVX
  quad_t ret;
  ret           = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
  ret           = _mm_mul_ps(ret, m.r[0]);
  quad_t v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
  v_temp        = _mm_mul_ps(v_temp, m.r[1]);
  ret           = _mm_add_ps(ret, v_temp);
  v_temp        = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
  v_temp        = _mm_mul_ps(v_temp, m.r[2]);
  ret           = _mm_add_ps(ret, v_temp);
  ret           = _mm_add_ps(ret, m.r[3]);
  return ret;
#else
  quad_t r, x, y, z;

  x = vec3a::splat_x(v);
  y = vec3a::splat_y(v);
  z = vec3a::splat_z(v);

  r = vec3a::madd(z, row(m, 2), row(m, 3));
  r = vec3a::madd(y, row(m, 1), r);
  r = vec3a::madd(x, row(m, 0), r);
  return r;
#endif
}

inline vec4::type mat4::transform_and_project(pref m, vec3a::pref v)
{
#if VML_USE_SSE_AVX
  quad_t ret;
  ret           = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
  ret           = _mm_mul_ps(ret, m.r[0]);
  quad_t v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
  v_temp        = _mm_mul_ps(v_temp, m.r[1]);
  ret           = _mm_add_ps(ret, v_temp);
  v_temp        = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
  v_temp        = _mm_mul_ps(v_temp, m.r[2]);
  ret           = _mm_add_ps(ret, v_temp);
  ret           = _mm_add_ps(ret, m.r[3]);
  v_temp        = _mm_shuffle_ps(ret, ret, _MM_SHUFFLE(3, 3, 3, 3));
  ret           = _mm_div_ps(ret, v_temp);
  return ret;
#else
  quad_t r, x, y, z;

  x = vec3a::splat_x(v);
  y = vec3a::splat_y(v);
  z = vec3a::splat_z(v);

  r = vec3a::madd(z, row(m, 2), row(m, 3));
  r = vec3a::madd(y, row(m, 1), r);
  r = vec3a::madd(x, row(m, 0), r);
  r = vec3a::mul(r, 1.f / r[3]);

  return r;
#endif
}

inline vec3a::type mat4::transform_bounds_extends(pref m, vec3a::pref v)
{
#if VML_USE_SSE_AVX
  quad_t ret        = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
  quad_t clear_sign = acl_cast_i128_ps(_mm_set1_epi32(0x7fffffff));
  ret               = _mm_mul_ps(ret, m.r[0]);
  ret               = _mm_and_ps(ret, clear_sign);
  quad_t v_temp     = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
  v_temp            = _mm_mul_ps(v_temp, m.r[1]);
  v_temp            = _mm_and_ps(v_temp, clear_sign);
  ret               = _mm_add_ps(ret, v_temp);
  v_temp            = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
  v_temp            = _mm_mul_ps(v_temp, m.r[2]);
  v_temp            = _mm_and_ps(v_temp, clear_sign);
  ret               = _mm_add_ps(ret, v_temp);
  return ret;
#else
  vec3a_t ret;
  for (int i = 0; i < 3; i++)
  {
    ret[i] = 0.0f;
    for (int j = 0; j < 3; j++)
    {
      ret[i] += v[j] * acl::abs(m.m[i + j * 4]);
    }
  }
  return ret;
#endif
}

inline aabb_t mat4::transform_aabb(pref m, aabb::pref box)
{
#if VML_USE_SSE_AVX
  aabb_t ret;
  quad_t max0 = _mm_shuffle_ps(box.r[1], box.r[1], _MM_SHUFFLE(0, 0, 0, 0));
  max0        = _mm_mul_ps(max0, m.r[0]);
  quad_t min0 = _mm_shuffle_ps(box.r[0], box.r[0], _MM_SHUFFLE(0, 0, 0, 0));
  min0        = _mm_mul_ps(min0, m.r[0]);
  quad_t max1 = _mm_shuffle_ps(box.r[1], box.r[1], _MM_SHUFFLE(1, 1, 1, 1));
  max1        = _mm_mul_ps(max1, m.r[1]);
  quad_t min1 = _mm_shuffle_ps(box.r[0], box.r[0], _MM_SHUFFLE(1, 1, 1, 1));
  min1        = _mm_mul_ps(min1, m.r[1]);
  quad_t max2 = _mm_shuffle_ps(box.r[1], box.r[1], _MM_SHUFFLE(2, 2, 2, 2));
  max2        = _mm_mul_ps(max2, m.r[2]);
  quad_t min2 = _mm_shuffle_ps(box.r[0], box.r[0], _MM_SHUFFLE(2, 2, 2, 2));
  min2        = _mm_mul_ps(min2, m.r[2]);

  ret.r[0] = vec3a::from_vec4(
    _mm_add_ps(_mm_add_ps(_mm_min_ps(max0, min0), _mm_add_ps(_mm_min_ps(max1, min1), _mm_min_ps(max2, min2))), m.r[3]));
  ret.r[1] = vec3a::from_vec4(
    _mm_add_ps(_mm_add_ps(_mm_max_ps(max0, min0), _mm_add_ps(_mm_max_ps(max1, min1), _mm_max_ps(max2, min2))), m.r[3]));
  return ret;
#else
  aabb_t ret;
  for (int i = 0; i < 3; i++)
  {
    ret.r[0][i] = std::min(box.r[0][0] * m.m[i + 0 * 4], box.r[1][0] * m.m[i + 0 * 4]) +
                  std::min(box.r[0][1] * m.m[i + 1 * 4], box.r[1][1] * m.m[i + 1 * 4]) +
                  std::min(box.r[0][2] * m.m[i + 2 * 4], box.r[1][2] * m.m[i + 2 * 4]) + m.m[i + 3 * 4];
  }
  ret.r[0][3] = 0;
  for (int i = 0; i < 3; i++)
  {
    ret.r[1][i] = std::max(box.r[0][0] * m.m[i + 0 * 4], box.r[1][0] * m.m[i + 0 * 4]) +
                  std::max(box.r[0][1] * m.m[i + 1 * 4], box.r[1][1] * m.m[i + 1 * 4]) +
                  std::max(box.r[0][2] * m.m[i + 2 * 4], box.r[1][2] * m.m[i + 2 * 4]) + m.m[i + 3 * 4];
  }
  ret.r[1][3] = 0;
  return ret;

#endif
}

inline mat4::type mat4::from_scale_rotation_translation(scalar_type scale, quat::pref rot, vec3a::pref pos)
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
  r0 = _mm_and_ps(_mm_sub_ps(r0, v1), fff0);

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

  quad_t scaleQ = quad::set(scale);
  mat4_t ret;
  ret.r[0] = _mm_mul_ps(scaleQ, q1);
  q1       = _mm_shuffle_ps(r0, v0, _MM_SHUFFLE(3, 2, 3, 1));
  q1       = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 3, 0, 2));
  ret.r[1] = _mm_mul_ps(scaleQ, q1);
  q1       = _mm_shuffle_ps(v1, r0, _MM_SHUFFLE(3, 2, 1, 0));
  ret.r[2] = _mm_mul_ps(scaleQ, q1);
  ret.r[3] = _mm_or_ps(_mm_and_ps(pos, fff0), _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
  return ret;
#else
  mat4_t ret;
  float  xx = rot[0] * rot[0];
  float  yy = rot[1] * rot[1];
  float  zz = rot[2] * rot[2];
  float  xy = rot[0] * rot[1];
  float  xz = rot[0] * rot[2];
  float  yz = rot[1] * rot[2];
  float  wx = rot[3] * rot[0];
  float  wy = rot[3] * rot[1];
  float  wz = rot[3] * rot[2];

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
#endif
}

inline mat4::type mat4::from_scale(vec3a::pref scale)
{
#if VML_USE_SSE_AVX
  mat4_t ret;
  ret.r[0] = _mm_and_ps(scale, acl_cast_i128_ps(_mm_set_epi32(0, 0, 0, 0xFFFFFFFF)));
  ret.r[1] = _mm_and_ps(scale, acl_cast_i128_ps(_mm_set_epi32(0, 0, 0xFFFFFFFF, 0)));
  ret.r[2] = _mm_and_ps(scale, acl_cast_i128_ps(_mm_set_epi32(0, 0xFFFFFFFF, 0, 0)));
  ret.r[3] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
  return ret;
#else
  mat4_t ret;
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
#endif
}

inline mat4::type mat4::from_translation(vec3a::pref pos)
{
#if VML_USE_SSE_AVX
  mat4_t ret;
  ret.r[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
  ret.r[1] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
  ret.r[2] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
  ret.r[3] = _mm_or_ps(vec3a::from_vec4(pos), _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
  return ret;
#else
  mat4_t ret;
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
#endif
}

inline mat4::type mat4::from_quat(quat::pref rot)
{
  type ret;
  set_rotation(ret, rot);

#if VML_USE_SSE_AVX
  ret.r[3] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
#else
  ret.e[3][0] = 0;
  ret.e[3][1] = 0;
  ret.e[3][2] = 0;
  ret.e[3][3] = 1.0f;
#endif
  return ret;
}

inline mat4::type mat4::from_rotation(quat::pref rot)
{
  return from_quat(rot);
}

inline mat4::type mat4::from_world_to_view(pref m)
{
  return inverse_assume_ortho(m);
}

inline mat4::type mat4::from_look_at(vec3a::pref eye, vec3a::pref look_at, vec3a::pref vup)
{
  mat4_t  m;
  vec3a_t zaxis = m.r[2] = vec3a::normalize(vec3a::sub(look_at, eye));
  vec3a_t xaxis = m.r[0] = vec3a::normalize(vec3a::cross(vup, m.r[2]));
  vec3a_t yaxis = m.r[1] = vec3a::cross(m.r[2], m.r[0]);
  m.r[3]                 = vec4::set(0.0f, 0.0f, 0.0f, 1.0f);
  transpose_in_place(m);

  vec3a_t neg_eye = vec3a::negate(eye);
  m.r[3]          = vec4::set(vec3a::dot(xaxis, neg_eye), vec3a::dot(yaxis, neg_eye), vec3a::dot(zaxis, neg_eye), 1);

  return m;
}

inline mat4::type mat4::from_orthographic_projection(scalar_type min_x, scalar_type max_x, scalar_type min_y,
                                                     scalar_type max_y, scalar_type zn, scalar_type zf)
{
  float dx_recip = 1.0f / (zf - zn);
  // clang-format off
	return {
		2.0f / (max_x - min_x),              0.0f,   	                          0.0f,             0.0f,
	    0.0f,                                2.0f / (max_y - min_y),              0.0f,             0.0f,
	    0.0f,                                0.0f,                                dx_recip,         0.0f,
	    (max_x + min_x) / (min_x - max_x),   (max_y + min_y) / (max_y - min_y),   -(zn) * dx_recip, 1.0f
	};
  // clang-format on
}

inline mat4::type mat4::from_orthographic_projection(scalar_type w, scalar_type h, scalar_type zn, scalar_type zf)
{
  float dx_recip = 1.0f / (zf - zn);
  // clang-format off
	return {
		2.0f / w,  0.0f,   	   0.0f,                0.0f,
	    0.0f,      2.0f / h,   0.0f,                0.0f,
	    0.0f,      0.0f,       dx_recip,            0.0f,
	    0.0f,      0.0f,      -(zn) * dx_recip,     1.0f
	};
  // clang-format on
}

inline mat4::type mat4::from_perspective_projection(scalar_type field_of_view, scalar_type aspect_ratio, scalar_type zn,
                                                    scalar_type zf)
{
  field_of_view *= 0.5f;

  float yscale = 1 / acl::tan(field_of_view);
  float q      = zf / (zf - zn);
  // clang-format off
	return {
		(yscale / aspect_ratio),        0,        0,        0,
	     0,                        yscale,        0,        0,
		 0,                             0,        q,        1,
		 0,                             0,  -q * zn,        0
	};
  // clang-format on
}

inline mat4::type mat4::mul(pref m, scalar_type scale)
{
#if VML_USE_SSE_AVX
  mat4_t ret;
  quad_t scaleQ = quad::set(scale);
  ret.r[0]      = _mm_mul_ps(scaleQ, m.r[0]);
  ret.r[1]      = _mm_mul_ps(scaleQ, m.r[1]);
  ret.r[2]      = _mm_mul_ps(scaleQ, m.r[2]);
  ret.r[3]      = _mm_mul_ps(scaleQ, m.r[3]);
  return ret;
#else
  mat4_t ret  = m;
  for (int i = 0; i < 4; ++i)
  {
    ret.e[i][0] *= scale;
    ret.e[i][1] *= scale;
    ret.e[i][2] *= scale;
    ret.e[i][3] *= scale;
  }
  return ret;
#endif
}

inline mat4::type mat4::mul(scalar_type scale, pref m)
{
  return mul(m, scale);
}

inline vec4_t mat4::mul(vec4::pref v, mat4::pref m)
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

  x = vec3a::splat_x(v);
  y = vec3a::splat_y(v);
  z = vec3a::splat_z(v);
  w = vec3a::splat_w(v);

  r = vec3a::mul(w, row(m, 3));
  r = vec3a::madd(z, row(m, 2), r);
  r = vec3a::madd(y, row(m, 1), r);
  r = vec3a::madd(x, row(m, 0), r);

  return r;
#endif
}

inline mat4::type mat4::transpose(pref m)
{
#if VML_USE_SSE_AVX
  mat4_t result = m;
  _MM_TRANSPOSE4_PS(result.r[0], result.r[1], result.r[2], result.r[3]);
  return result;
#else
  mat4_t ret;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      ret.m[i * 4 + j] = m.m[j * 4 + i];

  return ret;
#endif
}

inline void mat4::transpose_in_place(ref m)
{
#if VML_USE_SSE_AVX
  _MM_TRANSPOSE4_PS(m.r[0], m.r[1], m.r[2], m.r[3]);
#else
  std::swap(m.e[0][1], m.e[1][0]);
  std::swap(m.e[0][2], m.e[2][0]);
  std::swap(m.e[0][3], m.e[3][0]);
  std::swap(m.e[1][2], m.e[2][1]);
  std::swap(m.e[1][3], m.e[3][1]);
  std::swap(m.e[2][3], m.e[3][2]);
#endif
}
inline mat4::type mat4::inverse(pref m)
{
#if VML_USE_SSE_AVX
  mat4_t MT  = transpose(m);
  quad_t V00 = _mm_shuffle_ps(MT.r[2], MT.r[2], _MM_SHUFFLE(1, 1, 0, 0));
  quad_t V10 = _mm_shuffle_ps(MT.r[3], MT.r[3], _MM_SHUFFLE(3, 2, 3, 2));
  quad_t V01 = _mm_shuffle_ps(MT.r[0], MT.r[0], _MM_SHUFFLE(1, 1, 0, 0));
  quad_t V11 = _mm_shuffle_ps(MT.r[1], MT.r[1], _MM_SHUFFLE(3, 2, 3, 2));
  quad_t V02 = _mm_shuffle_ps(MT.r[2], MT.r[0], _MM_SHUFFLE(2, 0, 2, 0));
  quad_t V12 = _mm_shuffle_ps(MT.r[3], MT.r[1], _MM_SHUFFLE(3, 1, 3, 1));

  quad_t D0 = _mm_mul_ps(V00, V10);
  quad_t D1 = _mm_mul_ps(V01, V11);
  quad_t D2 = _mm_mul_ps(V02, V12);

  V00 = _mm_shuffle_ps(MT.r[2], MT.r[2], _MM_SHUFFLE(3, 2, 3, 2));
  V10 = _mm_shuffle_ps(MT.r[3], MT.r[3], _MM_SHUFFLE(1, 1, 0, 0));
  V01 = _mm_shuffle_ps(MT.r[0], MT.r[0], _MM_SHUFFLE(3, 2, 3, 2));
  V11 = _mm_shuffle_ps(MT.r[1], MT.r[1], _MM_SHUFFLE(1, 1, 0, 0));
  V02 = _mm_shuffle_ps(MT.r[2], MT.r[0], _MM_SHUFFLE(3, 1, 3, 1));
  V12 = _mm_shuffle_ps(MT.r[3], MT.r[1], _MM_SHUFFLE(2, 0, 2, 0));

  V00 = _mm_mul_ps(V00, V10);
  V01 = _mm_mul_ps(V01, V11);
  V02 = _mm_mul_ps(V02, V12);
  D0  = _mm_sub_ps(D0, V00);
  D1  = _mm_sub_ps(D1, V01);
  D2  = _mm_sub_ps(D2, V02);
  // V11 = D0Y,D0W,D2Y,D2Y
  V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 1, 3, 1));
  V00 = _mm_shuffle_ps(MT.r[1], MT.r[1], _MM_SHUFFLE(1, 0, 2, 1));
  V10 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(0, 3, 0, 2));
  V01 = _mm_shuffle_ps(MT.r[0], MT.r[0], _MM_SHUFFLE(0, 1, 0, 2));
  V11 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(2, 1, 2, 1));
  // V13 = D1Y,D1W,D2W,D2W
  quad_t V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 3, 3, 1));
  V02        = _mm_shuffle_ps(MT.r[3], MT.r[3], _MM_SHUFFLE(1, 0, 2, 1));
  V12        = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(0, 3, 0, 2));
  quad_t V03 = _mm_shuffle_ps(MT.r[2], MT.r[2], _MM_SHUFFLE(0, 1, 0, 2));
  V13        = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(2, 1, 2, 1));

  quad_t C0 = _mm_mul_ps(V00, V10);
  quad_t C2 = _mm_mul_ps(V01, V11);
  quad_t C4 = _mm_mul_ps(V02, V12);
  quad_t C6 = _mm_mul_ps(V03, V13);

  // V11 = D0X,D0Y,D2X,D2X
  V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(0, 0, 1, 0));
  V00 = _mm_shuffle_ps(MT.r[1], MT.r[1], _MM_SHUFFLE(2, 1, 3, 2));
  V10 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(2, 1, 0, 3));
  V01 = _mm_shuffle_ps(MT.r[0], MT.r[0], _MM_SHUFFLE(1, 3, 2, 3));
  V11 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(0, 2, 1, 2));
  // V13 = D1X,D1Y,D2Z,D2Z
  V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(2, 2, 1, 0));
  V02 = _mm_shuffle_ps(MT.r[3], MT.r[3], _MM_SHUFFLE(2, 1, 3, 2));
  V12 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(2, 1, 0, 3));
  V03 = _mm_shuffle_ps(MT.r[2], MT.r[2], _MM_SHUFFLE(1, 3, 2, 3));
  V13 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(0, 2, 1, 2));

  V00 = _mm_mul_ps(V00, V10);
  V01 = _mm_mul_ps(V01, V11);
  V02 = _mm_mul_ps(V02, V12);
  V03 = _mm_mul_ps(V03, V13);
  C0  = _mm_sub_ps(C0, V00);
  C2  = _mm_sub_ps(C2, V01);
  C4  = _mm_sub_ps(C4, V02);
  C6  = _mm_sub_ps(C6, V03);

  V00 = _mm_shuffle_ps(MT.r[1], MT.r[1], _MM_SHUFFLE(0, 3, 0, 3));
  // V10 = D0Z,D0Z,D2X,D2Y
  V10 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 2, 2));
  V10 = _mm_shuffle_ps(V10, V10, _MM_SHUFFLE(0, 2, 3, 0));
  V01 = _mm_shuffle_ps(MT.r[0], MT.r[0], _MM_SHUFFLE(2, 0, 3, 1));
  // V11 = D0X,D0W,D2X,D2Y
  V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 3, 0));
  V11 = _mm_shuffle_ps(V11, V11, _MM_SHUFFLE(2, 1, 0, 3));
  V02 = _mm_shuffle_ps(MT.r[3], MT.r[3], _MM_SHUFFLE(0, 3, 0, 3));
  // V12 = D1Z,D1Z,D2Z,D2W
  V12 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 2, 2));
  V12 = _mm_shuffle_ps(V12, V12, _MM_SHUFFLE(0, 2, 3, 0));
  V03 = _mm_shuffle_ps(MT.r[2], MT.r[2], _MM_SHUFFLE(2, 0, 3, 1));
  // V13 = D1X,D1W,D2Z,D2W
  V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 3, 0));
  V13 = _mm_shuffle_ps(V13, V13, _MM_SHUFFLE(2, 1, 0, 3));

  V00       = _mm_mul_ps(V00, V10);
  V01       = _mm_mul_ps(V01, V11);
  V02       = _mm_mul_ps(V02, V12);
  V03       = _mm_mul_ps(V03, V13);
  quad_t C1 = _mm_sub_ps(C0, V00);
  C0        = _mm_add_ps(C0, V00);
  quad_t C3 = _mm_add_ps(C2, V01);
  C2        = _mm_sub_ps(C2, V01);
  quad_t C5 = _mm_sub_ps(C4, V02);
  C4        = _mm_add_ps(C4, V02);
  quad_t C7 = _mm_add_ps(C6, V03);
  C6        = _mm_sub_ps(C6, V03);

  C0 = _mm_shuffle_ps(C0, C1, _MM_SHUFFLE(3, 1, 2, 0));
  C2 = _mm_shuffle_ps(C2, C3, _MM_SHUFFLE(3, 1, 2, 0));
  C4 = _mm_shuffle_ps(C4, C5, _MM_SHUFFLE(3, 1, 2, 0));
  C6 = _mm_shuffle_ps(C6, C7, _MM_SHUFFLE(3, 1, 2, 0));
  C0 = _mm_shuffle_ps(C0, C0, _MM_SHUFFLE(3, 1, 2, 0));
  C2 = _mm_shuffle_ps(C2, C2, _MM_SHUFFLE(3, 1, 2, 0));
  C4 = _mm_shuffle_ps(C4, C4, _MM_SHUFFLE(3, 1, 2, 0));
  C6 = _mm_shuffle_ps(C6, C6, _MM_SHUFFLE(3, 1, 2, 0));
  // get the determinate
#ifdef L_USE_FAST_DIVISION
  quad_t v_temp = quad::splat_x(_mm_rcp_ss(quad::vdot(C0, MT.r[0])));
#else
  quad_t v_temp = quad::splat_x(_mm_div_ss(_mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f), quad::vdot(C0, MT.r[0])));
#endif
  mat4_t result;
  result.r[0] = _mm_mul_ps(C0, v_temp);
  result.r[1] = _mm_mul_ps(C2, v_temp);
  result.r[2] = _mm_mul_ps(C4, v_temp);
  result.r[3] = _mm_mul_ps(C6, v_temp);
  return result;

#else
  mat4_t inv;

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
#endif
}

inline mat4::type mat4::inverse_assume_ortho(pref m)
{
#if VML_USE_SSE_AVX
  // inverse = [ T(R)       0 ]
  //        [ -vpos*T(R) 1 ]
  mat4_t ret;
  ret.r[0] = _mm_shuffle_ps(m.r[1], m.r[2], _MM_SHUFFLE(3, 0, 0, 3));
  ret.r[0] = _mm_move_ss(ret.r[0], m.r[0]);
  ret.r[1] = _mm_shuffle_ps(m.r[0], m.r[1], _MM_SHUFFLE(3, 1, 3, 1));
  ret.r[1] = _mm_shuffle_ps(ret.r[1], m.r[2], _MM_SHUFFLE(3, 1, 2, 0));
  ret.r[2] = _mm_shuffle_ps(m.r[0], m.r[1], _MM_SHUFFLE(3, 2, 3, 2));
  ret.r[2] = _mm_shuffle_ps(ret.r[2], m.r[2], _MM_SHUFFLE(3, 2, 2, 0));

  quad_t v_temp = _mm_shuffle_ps(m.r[3], m.r[3], _MM_SHUFFLE(0, 0, 0, 0));
  ret.r[3]      = _mm_mul_ps(v_temp, ret.r[0]);
  v_temp        = _mm_shuffle_ps(m.r[3], m.r[3], _MM_SHUFFLE(1, 1, 1, 1));
  v_temp        = _mm_mul_ps(v_temp, ret.r[1]);
  ret.r[3]      = _mm_add_ps(ret.r[3], v_temp);
  v_temp        = _mm_shuffle_ps(m.r[3], m.r[3], _MM_SHUFFLE(2, 2, 2, 2));
  v_temp        = _mm_mul_ps(v_temp, ret.r[2]);
  ret.r[3]      = _mm_add_ps(ret.r[3], v_temp);
  ret.r[3]      = _mm_xor_ps(ret.r[3], acl_cast_i128_ps(_mm_set1_epi32(0x80000000)));
  // and with 0001
  ret.r[3] = _mm_or_ps(_mm_and_ps(ret.r[3], clear_w_mask()), xyz0_w1());

  assert(ret.e[0][3] == 0.f && ret.e[1][3] == 0.f && ret.e[2][3] == 0.f && ret.e[3][3] == 1.f);
  return ret;
#else
  mat4_t ret;
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
      ret.m[i * 4 + j] = m.m[j * 4 + i];
    ret.m[i * 4 + 3] = 0;
  }

  ret.e[3][0] = -vec3a::dot(row(m, 3), row(m, 0));
  ret.e[3][1] = -vec3a::dot(row(m, 3), row(m, 1));
  ret.e[3][2] = -vec3a::dot(row(m, 3), row(m, 2));
  ret.e[3][3] = 1;
  return ret;

#endif
}
inline mat3_t const& mat4::as_mat3(mat4_t const& m)
{
  return reinterpret_cast<mat3_t const&>(m);
}

inline mat3_t& mat4::as_mat3(mat4_t& m)
{
  return reinterpret_cast<mat3_t&>(m);
}

inline auto operator+(mat4_t const& a, mat4_t const& b) noexcept
{
  return mat4::add(a, b);
}

inline auto operator-(mat4_t const& a, mat4_t const& b) noexcept
{
  return mat4::sub(a, b);
}

inline auto operator*(mat4_t const& a, mat4_t const& b) noexcept
{
  return mat4::mul(a, b);
}

inline auto operator*(mat4_t const& a, mat4::scalar_type b) noexcept
{
  return mat4::mul(a, b);
}

inline auto operator*(mat4::scalar_type a, mat4_t const& b) noexcept
{
  return mat4::mul(a, b);
}

} // namespace acl