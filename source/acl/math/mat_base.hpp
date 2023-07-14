#pragma once
#include "multi_dim.hpp"
#include "quat.hpp"
#include "vec3.hpp"
#include "vec3a.hpp"
#include "vec4.hpp"

namespace acl
{
/// @brief Create a matrix from vector mapping that can rotate the vector axis1 to axis2 when post multiplied to axis1.
template <TransformMatrix M, typename scalar_t>
inline M make_rotation_from_vector_mapping(vec3_t<scalar_t> const& axis1, vec3_t<scalar_t> const& axis2)
{
  /** \todo sse **/
  auto cs = dot(axis1, axis2);

  auto axis = cross(axis1, axis2);
  // OPTIMIZE: we can also check the angle to
  // see if its a multiple of Pi.
  if (std::abs(axis.x) < acl::k_const_epsilon_med && std::abs(axis.y) < acl::k_const_epsilon_med &&
      std::abs(axis.z) < acl::k_const_epsilon_med)
  {
    // take a cross for that
    axis = cross(axis1, vec3_t<scalar_t>(0.0f, 1.0f, 0.0f));
    if (std::abs(axis.x) < acl::k_const_epsilon_med && std::abs(axis.y) < acl::k_const_epsilon_med &&
        std::abs(axis.z) < acl::k_const_epsilon_med)
    {
      axis = cross(axis1, vec3_t<scalar_t>(1.0f, 0.0f, 0.0f));
    }
  }
  auto _1_c   = 1.0f - cs;
  auto xyzs   = mul(axis, -std::sqrt(1 - cs * cs));
  auto mstr   = mul(axis, axis);
  mstr        = mstr * _1_c;
  auto xy_1_c = axis.x * axis.y * _1_c;
  auto xz_1_c = axis.x * axis.z * _1_c;
  auto yz_1_c = axis.y * axis.z * _1_c;

  M m{noinit_v};
  m.r[0].v = vec4_t<scalar_t>(cs + mstr.x, xy_1_c - xyzs.z, xz_1_c + xyzs.y, 0);
  m.r[1].v = vec4_t<scalar_t>(xy_1_c + xyzs.z, cs + mstr.y, yz_1_c - xyzs.x, 0);
  m.r[2].v = vec4_t<scalar_t>(xz_1_c - xyzs.y, yz_1_c + xyzs.x, cs + mstr.z, 0);

  return m;
}

/// @brief rotate vector in place
template <TransformMatrix M, typename scalar_t>
inline void rotate(M const& m, vec3_t<scalar_t>* io_stream, std::uint32_t i_stride, std::uint32_t i_count)
{
  assert(io_stream);
  const std::uint8_t* inout_vec = (const std::uint8_t*)io_stream;
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    for (std::uint32_t i = 0; i < i_count; i++)
    {
      __m128 x                  = _mm_load_ps1(reinterpret_cast<const scalar_t*>(inout_vec));
      __m128 y                  = _mm_load_ps1(reinterpret_cast<const scalar_t*>(inout_vec + 4));
      __m128 res                = _mm_load_ps1(reinterpret_cast<const scalar_t*>(inout_vec + 8));
      res                       = _mm_mul_ps(res, m.r[2].v);
      y                         = _mm_mul_ps(y, m.r[1].v);
      res                       = _mm_add_ps(res, y);
      x                         = _mm_mul_ps(x, m.r[0].v);
      res                       = _mm_add_ps(res, x);
      auto vres                 = normalize(vec3a_t<scalar_t>(res));
      ((scalar_t*)inout_vec)[0] = vres.xyzw[0];
      ((scalar_t*)inout_vec)[1] = vres.s[1];
      ((scalar_t*)inout_vec)[2] = vres.s[2];

      inout_vec += i_stride;
    }
  }
  else
  {

    for (std::uint32_t i = 0; i < i_count; i++)
    {
      auto x = vec3a_t<scalar_t>(((scalar_t*)inout_vec)[0]);
      auto y = vec3a_t<scalar_t>(((scalar_t*)inout_vec)[1]);
      auto z = vec3a_t<scalar_t>(((scalar_t*)inout_vec)[2]);

      auto r = mul(z, row(m, 2));
      r      = madd(y, row(m, 1), r);
      r      = normalize(madd(x, row(m, 0), r));

      ((scalar_t*)inout_vec)[0] = r[0];
      ((scalar_t*)inout_vec)[1] = r[1];
      ((scalar_t*)inout_vec)[2] = r[2];

      inout_vec += i_stride;
    }
  }
}

template <TransformMatrix M, typename scalar_t>
inline vec3a_t<scalar_t> rotate(vec3a_t<scalar_t> v, M const& m)
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    auto v_res  = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    v_res       = _mm_mul_ps(v_res, m.r[0].v);
    auto v_temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    v_temp      = _mm_mul_ps(v_temp, m.r[1].v);
    v_res       = _mm_add_ps(v_res, v_temp);
    v_temp      = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
    v_temp      = _mm_mul_ps(v_temp, m.r[2].v);
    v_res       = _mm_add_ps(v_res, v_temp);
    return v_res;
  }
  else
  {
    auto r = mul(splat_z(v), row(m, 2));
    r      = madd(splat_y(v), row(m, 1), r);
    r      = madd(splat_x(v), row(m, 0), r);
    return r;
  }
}

template <TransformMatrix M, typename scalar_t>
inline void set_rotation(M& m, quat_t<scalar_t> const& rot)
{
  scalar_t q[4] = {x(rot), y(rot), z(rot), w(rot)};
  m.e[0][3]     = 0.0f;
  m.e[1][3]     = 0.0f;
  m.e[2][3]     = 0.0f;
  scalar_t x2   = q[0] + q[0];
  scalar_t y2   = q[1] + q[1];
  scalar_t z2   = q[2] + q[2];
  {
    scalar_t xx2 = q[0] * x2;
    scalar_t yy2 = q[1] * y2;
    scalar_t zz2 = q[2] * z2;
    m.e[0][0]    = 1.0f - yy2 - zz2;
    m.e[1][1]    = 1.0f - xx2 - zz2;
    m.e[2][2]    = 1.0f - xx2 - yy2;
  }
  {
    scalar_t yz2 = q[1] * z2;
    scalar_t wx2 = q[3] * x2;
    m.e[2][1]    = yz2 - wx2;
    m.e[1][2]    = yz2 + wx2;
  }
  {
    scalar_t xy2 = q[0] * y2;
    scalar_t wz2 = q[3] * z2;
    m.e[1][0]    = xy2 - wz2;
    m.e[0][1]    = xy2 + wz2;
  }
  {
    scalar_t xz2 = q[0] * z2;
    scalar_t wy2 = q[3] * y2;
    m.e[0][2]    = xz2 - wy2;
    m.e[2][0]    = xz2 + wy2;
  }
}

template <TransformMatrix M, typename scalar_t>
inline void set_as_view(M& ret, vec3a_t<scalar_t> const& view_dir, vec3a_t<scalar_t> const& up_dir)
{
  // TODO needs validation
  ret.r[2].v = normalize(view_dir);
  ret.r[0].v = normalize(cross(view_dir, up_dir));
  ret.r[1].v = cross(ret.r[0].v, ret.r[2].v);
}
} // namespace acl
