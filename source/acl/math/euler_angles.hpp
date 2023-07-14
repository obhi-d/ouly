#pragma once
#include "quad.hpp"
#include "quat.hpp"
#include "vec3.hpp"

namespace acl
{

template <typename scalar_t>
inline euler_angles_t<scalar_t> canonize(euler_angles_t<scalar_t> const& m) noexcept
{ // First, wrap r[0] in range -pi ... pi
  euler_angles_t<scalar_t> r;
  r.pitch = wrap_pi(m.pitch);
  r.yaw   = m.yaw;
  r.roll  = m.roll;
  // Now, check for "the back side" of the matrix r[0] outside
  // the canonical range of -pi/2 ... pi/2
  if (r.pitch < -k_pi_by_2)
  {
    r.pitch = -k_pi_by_2 - r.pitch;
    r.yaw += k_pi;
    r.roll += k_pi;
  }
  else if (r.pitch > k_pi_by_2)
  {
    r.pitch = k_pi - r.pitch;
    r.yaw += k_pi;
    r.roll += k_pi;
  }
  // Now check for the gimbel Lock case (within a slight tolerance)
  if (std::abs(r.pitch) > k_pi_by_2 - 1e-4)
  {
    // We are in gimbel Lock. Assign all rotation
    // about the vertical axis to r[1]
    r.yaw += r.roll;
    r.roll = 0.0f;
  }
  else
  {
    // Not in gimbel Lock. Wrap the r[2] angle in
    // canonical range
    r.roll = wrap_pi(r.roll);
  }
  // Wrap r[1] in canonical range
  r.yaw = wrap_pi(r.yaw);
  return r;
}

template <typename scalar_t>
inline euler_angles_t<scalar_t> make_euler_angles(quat_t<scalar_t> const& src) noexcept
{
  euler_angles_t<scalar_t> r;
  float                    src_x = x(src);
  float                    src_y = y(src);
  float                    src_z = z(src);
  float                    src_w = w(src);
  // Extract sin(r[0])
  float sp = -2.0f * (src_y * src_z - src_w * src_x);
  // Check for Gimbel Lock, giving slight tolerance for numerical imprecision
  if (std::abs(sp) > 0.9999f)
  {
    // Looking straight up or down
    r.pitch = k_pi_by_2 * sp;
    // Compute r[1], slam r[2] to zero
    r.yaw  = std::atan2(-src_x * src_z + src_w * src_y, 0.5f - src_y * src_y - src_z * src_z);
    r.roll = 0.0f;
  }
  else
  {
    // Compute angles. We don't have to use the "safe" asin
    // function because we already checked for range errors when
    // checking for Gimbel Lock
    r.pitch = std::asin(sp);
    r.yaw   = std::atan2(src_x * src_z + src_w * src_y, 0.5f - src_x * src_x - src_y * src_y);
    r.roll  = std::atan2(src_x * src_y + src_w * src_z, 0.5f - src_x * src_x - src_z * src_z);
  }
  return r;
}

template <typename scalar_t>
inline euler_angles_t<scalar_t> make_euler_angles_from_quat_conjugate(quat_t<scalar_t> const& src) noexcept
{
  euler_angles_t<scalar_t> r;
  float                    src_x = x(src);
  float                    src_y = y(src);
  float                    src_z = z(src);
  float                    src_w = w(src);
  // Extract sin(r[0])
  float sp = -2.0f * (src_y * src_z + src_w * src_x);
  // Check for Gimbel Lock, giving slight tolerance for numerical imprecision
  if (std::abs(sp) > 0.9999f)
  {
    // Looking straight up or down
    r.pitch = k_pi_by_2 * sp;
    // Compute heading, slam bank to zero
    r.yaw  = std::atan2(-src_x * src_z - src_w * src_y, 0.5f - src_y * src_y - src_z * src_z);
    r.roll = 0.0f;
  }
  else
  {
    // Compute angles. We don't have to use the "safe" asin
    // function because we already checked for range errors when
    // checking for Gimbel Lock
    r.pitch = std::asin(sp);
    r.yaw   = std::atan2(src_x * src_z - src_w * src_y, 0.5f - src_x * src_x - src_y * src_y);
    r.roll  = std::atan2(src_x * src_y - src_w * src_z, 0.5f - src_x * src_x - src_z * src_z);
  }
  return r;
}

template <typename scalar_t>
inline euler_angles_t<scalar_t> make_euler_angles(mat3_t<scalar_t> const& src) noexcept
{
  euler_angles_t<scalar_t> r;
  // Extract sin(r[0]) from e[3][2].
  float sp = -src.e[2][1];
  // Check for Gimbel Lock
  if (std::abs(sp) > 9.99999f)
  {
    // Looking straight up or down
    r.pitch = k_pi_by_2 * sp;
    // Compute r[1], slam r[2] to zero
    r.yaw  = std::atan2(-src.e[1][2], src.e[0][0]);
    r.roll = 0.0f;
  }
  else
  {
    // Compute angles. We don't have to use the "safe" asin
    // function because we already checked for range errors when
    // checking for Gimbel Lock
    r.pitch = std::asin(sp);
    r.yaw   = std::atan2(src.e[2][0], src.e[2][2]);
    r.roll  = std::atan2(src.e[0][1], src.e[1][1]);
  }
  return r;
}

template <typename scalar_t>
inline euler_angles_t<scalar_t> make_euler_angles(mat4_t<scalar_t> const& m) noexcept
{
  return from_mat3(*reinterpret_cast<const mat3_t<scalar_t>*>(&m));
}

} // namespace acl
