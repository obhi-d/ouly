#pragma once
#include "multi_dim.hpp"
#include "quat.hpp"
#include "vec3.hpp"
#include "vec3a.hpp"
#include "vec4.hpp"

namespace acl
{
/// @brief Create a matrix from vector mapping that can rotate the vector axis1 to axis2 when post multiplied to axis1.
template <Matrix M, typename scalar_t>
inline M make_rotation_from_vector_mapping(vec3_t<scalar_t> const& axis1, vec3_t<scalar_t> const& axis2) noexcept
{
  /** @todo sse **/
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
  auto xyzs   = axis * -std::sqrt(1 - cs * cs);
  auto mstr   = axis * axis;
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

template <Matrix M, typename scalar_t>
inline void set_rotation(M& m, quat_t<scalar_t> const& q) noexcept
{
  m.e[0][3]   = 0.0f;
  m.e[1][3]   = 0.0f;
  m.e[2][3]   = 0.0f;
  scalar_t x2 = q[0] + q[0];
  scalar_t y2 = q[1] + q[1];
  scalar_t z2 = q[2] + q[2];
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

template <Matrix M, typename scalar_t>
inline void set_view_matrix(M& ret, vec3a_t<scalar_t> const& view_dir, vec3a_t<scalar_t> const& up_dir) noexcept
{
  // TODO needs validation
  ret.r[2].v = normalize(view_dir);
  ret.r[0].v = normalize(cross(view_dir, up_dir));
  ret.r[1].v = vml::cross(ret.r[0].v, ret.r[2].v);
}
} // namespace acl
