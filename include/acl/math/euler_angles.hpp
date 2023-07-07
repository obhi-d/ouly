#pragma once
#include "quad.hpp"
#include "quat.hpp"
#include "vec3.hpp"

namespace acl
{

struct euler_angles : public vec3
{
  using vec3::cref;
  using vec3::pref;
  using vec3::ref;
  using vec3::row_type;
  using vec3::scalar_type;
  using vec3::type;

  //! Ensure angles are in canonical state
  static inline type canonize(pref m);
  static inline type from_quat(quat_t const& m);
  static inline type from_quat_conjugate(quat_t const& m);
  static inline type from_mat4(mat4_t const& m);
  static inline type from_mat3(mat3_t const& m);
};

inline euler_angles::type euler_angles::canonize(pref m)
{ // First, wrap r[0] in range -pi ... pi
  type r;
  r[0] = wrap_pi(m[0]);
  r[1] = m[1];
  r[2] = m[2];
  // Now, check for "the back side" of the matrix r[0] outside
  // the canonical range of -pi/2 ... pi/2
  if (r[0] < -k_pi_by_2)
  {
    r[0] = -k_pi_by_2 - r[0];
    r[1] += k_pi;
    r[2] += k_pi;
  }
  else if (r[0] > k_pi_by_2)
  {
    r[0] = k_pi - r[0];
    r[1] += k_pi;
    r[2] += k_pi;
  }
  // Now check for the gimbel Lock case (within a slight tolerance)
  if (acl::abs(r[0]) > k_pi_by_2 - 1e-4)
  {
    // We are in gimbel Lock. Assign all rotation
    // about the vertical axis to r[1]
    r[1] += r[2];
    r[2] = 0.0f;
  }
  else
  {
    // Not in gimbel Lock. Wrap the r[2] angle in
    // canonical range
    r[2] = wrap_pi(r[2]);
  }
  // Wrap r[1] in canonical range
  r[1] = wrap_pi(r[1]);
  return r;
}

inline euler_angles::type euler_angles::from_quat(quat_t const& src)
{
  type  r;
  float src_x = quad::x(src);
  float src_y = quad::y(src);
  float src_z = quad::z(src);
  float src_w = quad::w(src);
  // Extract sin(r[0])
  float sp = -2.0f * (src_y * src_z - src_w * src_x);
  // Check for Gimbel Lock, giving slight tolerance for numerical imprecision
  if (acl::abs(sp) > 0.9999f)
  {
    // Looking straight up or down
    r[0] = k_pi_by_2 * sp;
    // Compute r[1], slam r[2] to zero
    r[1] = arc_tan2(-src_x * src_z + src_w * src_y, 0.5f - src_y * src_y - src_z * src_z);
    r[2] = 0.0f;
  }
  else
  {
    // Compute angles. We don't have to use the "safe" asin
    // function because we already checked for range errors when
    // checking for Gimbel Lock
    r[0] = arc_sin(sp);
    r[1] = arc_tan2(src_x * src_z + src_w * src_y, 0.5f - src_x * src_x - src_y * src_y);
    r[2] = arc_tan2(src_x * src_y + src_w * src_z, 0.5f - src_x * src_x - src_z * src_z);
  }
  return r;
}

inline euler_angles::type euler_angles::from_quat_conjugate(quat_t const& src)
{
  type  r;
  float src_x = quat::x(src);
  float src_y = quat::y(src);
  float src_z = quat::z(src);
  float src_w = quat::w(src);
  // Extract sin(r[0])
  float sp = -2.0f * (src_y * src_z + src_w * src_x);
  // Check for Gimbel Lock, giving slight tolerance for numerical imprecision
  if (acl::abs(sp) > 0.9999f)
  {
    // Looking straight up or down
    r[0] = k_pi_by_2 * sp;
    // Compute heading, slam bank to zero
    r[1] = arc_tan2(-src_x * src_z - src_w * src_y, 0.5f - src_y * src_y - src_z * src_z);
    r[2] = 0.0f;
  }
  else
  {
    // Compute angles. We don't have to use the "safe" asin
    // function because we already checked for range errors when
    // checking for Gimbel Lock
    r[0] = arc_sin(sp);
    r[1] = arc_tan2(src_x * src_z - src_w * src_y, 0.5f - src_x * src_x - src_y * src_y);
    r[2] = arc_tan2(src_x * src_y - src_w * src_z, 0.5f - src_x * src_x - src_z * src_z);
  }
  return r;
}

inline euler_angles::type euler_angles::from_mat4(mat4_t const& m)
{
  return from_mat3(*reinterpret_cast<const mat3_t*>(&m));
}

inline euler_angles::type euler_angles::from_mat3(mat3_t const& src)
{
  type r;
  // Extract sin(r[0]) from e[3][2].
  float sp = -src.e[2][1];
  // Check for Gimbel Lock
  if (acl::abs(sp) > 9.99999f)
  {
    // Looking straight up or down
    r[0] = k_pi_by_2 * sp;
    // Compute r[1], slam r[2] to zero
    r[1] = arc_tan2(-src.e[1][2], src.e[0][0]);
    r[2] = 0.0f;
  }
  else
  {
    // Compute angles. We don't have to use the "safe" asin
    // function because we already checked for range errors when
    // checking for Gimbel Lock
    r[1] = arc_tan2(src.e[2][0], src.e[2][2]);
    r[0] = arc_sin(sp);
    r[2] = arc_tan2(src.e[0][1], src.e[1][1]);
  }
  return r;
}
} // namespace acl