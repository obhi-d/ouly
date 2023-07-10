#pragma once
#include "multi_dim.hpp"
#include "quat.hpp"
#include "vec3.hpp"
#include "vec3a.hpp"
#include "vec4.hpp"

namespace acl
{
template <typename concrete>
struct mat_base : public multi_dim<concrete>
{
  using typename multi_dim<concrete>::type;
  using typename multi_dim<concrete>::ref;
  using typename multi_dim<concrete>::pref;
  using typename multi_dim<concrete>::scalar_type;
  using typename multi_dim<concrete>::row_tag;
  using typename multi_dim<concrete>::row_type;

  //! @brief Create a matrix from vector mapping that
  //! can rotate the vector axis1 to axis2 when post multiplied to axis1.
  static inline type from_vector_mapping(vec3::pref v1, vec3::pref v2);
  //! @brief rotate vector in place
  static inline void rotate(pref m, vec3::type* io_stream, std::uint32_t i_stride, std::uint32_t i_count);
  //! @brief rotate vector
  static inline vec3a_t rotate(pref m, vec3a::pref v);
  //! @brief set a rotation for a given matrix
  static inline void set_rotation(ref _, quat::pref rot);
  //! @brief Returns a look at matrix based on a look at vector and up direction
  static inline void set_as_view(ref m, vec3a::pref view_dir, vec3a::pref up_dir);
};

template <typename concrete>
inline typename mat_base<concrete>::type mat_base<concrete>::from_vector_mapping(vec3::pref axis1, vec3::pref axis2)
{
  /** \todo sse **/
  type        m;
  scalar_type cs, xy_1_c, xz_1_c, yz_1_c;
  cs = vec3::dot(axis1, axis2);
  scalar_type _1_c;
  vec3_t      axis = vec3::cross(axis1, axis2);
  // OPTIMIZE: we can also check the angle to
  // see if its a multiple of Pi.
  if (acl::abs(axis[0]) < acl::k_const_epsilon_med && acl::abs(axis[1]) < acl::k_const_epsilon_med &&
      acl::abs(axis[2]) < acl::k_const_epsilon_med)
  {
    // take a cross for that
    axis = vec3::cross(axis1, vec3::set(0.0f, 1.0f, 0.0f));
    if (acl::abs(axis[0]) < acl::k_const_epsilon_med && acl::abs(axis[1]) < acl::k_const_epsilon_med &&
        acl::abs(axis[2]) < acl::k_const_epsilon_med)
    {
      axis = vec3::cross(axis1, vec3::set(1.0f, 0.0f, 0.0f));
    }
  }
  _1_c        = 1.0f - cs;
  vec3_t xyzs = vec3::mul(axis, -acl::sqrt(1 - cs * cs));
  vec3_t mstr = vec3::mul(axis, axis);
  mstr        = mstr * _1_c;
  xy_1_c      = axis[0] * axis[1] * _1_c;
  xz_1_c      = axis[0] * axis[2] * _1_c;
  yz_1_c      = axis[1] * axis[2] * _1_c;

  m.r[0] = row_tag::set(cs + mstr[0], xy_1_c - xyzs[2], xz_1_c + xyzs[1], 0);
  m.r[1] = row_tag::set(xy_1_c + xyzs[2], cs + mstr[1], yz_1_c - xyzs[0], 0);
  m.r[2] = row_tag::set(xz_1_c - xyzs[1], yz_1_c + xyzs[0], cs + mstr[2], 0);

  return m;
}

template <typename concrete>
inline void mat_base<concrete>::rotate(pref m, vec3::type* io_stream, std::uint32_t i_stride, std::uint32_t i_count)
{
  assert(io_stream);
  const std::uint8_t* inout_vec = (const std::uint8_t*)io_stream;
#if VML_USE_SSE_AVX
  std::uint8_t* out_vec = (std::uint8_t*)io_stream;
  union
  {
    quad_t      v;
    scalar_type s[4];
  } store;

  for (std::uint32_t i = 0; i < i_count; i++)
  {
    quad_t x   = _mm_load_ps1(reinterpret_cast<const scalar_type*>(inout_vec));
    quad_t y   = _mm_load_ps1(reinterpret_cast<const scalar_type*>(inout_vec + 4));
    quad_t res = _mm_load_ps1(reinterpret_cast<const scalar_type*>(inout_vec + 8));
    res        = _mm_mul_ps(res, m.r[2]);
    y          = _mm_mul_ps(y, m.r[1]);
    res        = _mm_add_ps(res, y);
    x          = _mm_mul_ps(x, m.r[0]);
    res        = _mm_add_ps(res, x);
    res        = vec3a::normalize(res);
    _mm_store_ps(store.s, res);
    ((scalar_type*)inout_vec)[0] = store.s[0];
    ((scalar_type*)inout_vec)[1] = store.s[1];
    ((scalar_type*)inout_vec)[2] = store.s[2];

    inout_vec += i_stride;
  }
#else
  quad_t x, y, z, r;
  for (std::uint32_t i = 0; i < i_count; i++)
  {
    x = quad::set(((scalar_type*)inout_vec)[0]);
    y = quad::set(((scalar_type*)inout_vec)[1]);
    z = quad::set(((scalar_type*)inout_vec)[2]);

    r = vec3a::mul(z, row(m, 2));
    r = vec3a::madd(y, row(m, 1), r);
    r = vec3a::normalize(vec3a::madd(x, row(m, 0), r));

    ((scalar_type*)inout_vec)[0] = r[0];
    ((scalar_type*)inout_vec)[1] = r[1];
    ((scalar_type*)inout_vec)[2] = r[2];

    inout_vec += i_stride;
  }
#endif
}

template <typename concrete>
inline vec3a_t mat_base<concrete>::rotate(pref m, vec3a::pref v)
{
#if VML_USE_SSE_AVX
  quad_t v_res  = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
  v_res         = _mm_mul_ps(v_res, m.r[0]);
  quad_t v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
  v_temp        = _mm_mul_ps(v_temp, m.r[1]);
  v_res         = _mm_add_ps(v_res, v_temp);
  v_temp        = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
  v_temp        = _mm_mul_ps(v_temp, m.r[2]);
  v_res         = _mm_add_ps(v_res, v_temp);
  return v_res;
#else
  quad_t r = vec3a::mul(vec3a::splat_z(v), row(m, 2));
  r        = vec3a::madd(vec3a::splat_y(v), row(m, 1), r);
  r        = vec3a::madd(vec3a::splat_x(v), row(m, 0), r);
  return r;
#endif
}

template <typename concrete>
inline void mat_base<concrete>::set_rotation(ref m, quat::pref rot)
{
  scalar_type q[4] = {quat::x(rot), quat::y(rot), quat::z(rot), quat::w(rot)};
  m.e[0][3]        = 0.0f;
  m.e[1][3]        = 0.0f;
  m.e[2][3]        = 0.0f;
  scalar_type x2   = q[0] + q[0];
  scalar_type y2   = q[1] + q[1];
  scalar_type z2   = q[2] + q[2];
  {
    scalar_type xx2 = q[0] * x2;
    scalar_type yy2 = q[1] * y2;
    scalar_type zz2 = q[2] * z2;
    m.e[0][0]       = 1.0f - yy2 - zz2;
    m.e[1][1]       = 1.0f - xx2 - zz2;
    m.e[2][2]       = 1.0f - xx2 - yy2;
  }
  {
    scalar_type yz2 = q[1] * z2;
    scalar_type wx2 = q[3] * x2;
    m.e[2][1]       = yz2 - wx2;
    m.e[1][2]       = yz2 + wx2;
  }
  {
    scalar_type xy2 = q[0] * y2;
    scalar_type wz2 = q[3] * z2;
    m.e[1][0]       = xy2 - wz2;
    m.e[0][1]       = xy2 + wz2;
  }
  {
    scalar_type xz2 = q[0] * z2;
    scalar_type wy2 = q[3] * y2;
    m.e[0][2]       = xz2 - wy2;
    m.e[2][0]       = xz2 + wy2;
  }
}

template <typename concrete>
inline void mat_base<concrete>::set_as_view(ref ret, vec3a::pref view_dir, vec3a::pref up_dir)
{
  // TODO needs validation
  ret.r[2] = vec3a::normalize(view_dir);
  ret.r[0] = vec3a::normalize(vec3a::cross(view_dir, up_dir));
  ret.r[1] = vec3a::cross(ret.r[0], ret.r[2]);
}
} // namespace acl