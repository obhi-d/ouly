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
  if (r.pitch < -static_cast<scalar_t>(k_pi_by_2_d))
  {
    r.pitch = -static_cast<scalar_t>(k_pi_by_2_d) - r.pitch;
    r.yaw += static_cast<scalar_t>(k_pi_d);
    r.roll += static_cast<scalar_t>(k_pi_d);
  }
  else if (r.pitch > static_cast<scalar_t>(k_pi_by_2_d))
  {
    r.pitch = static_cast<scalar_t>(k_pi_d) - r.pitch;
    r.yaw += static_cast<scalar_t>(k_pi_d);
    r.roll += static_cast<scalar_t>(k_pi_d);
  }
  // Now check for the gimbel Lock case (within a slight tolerance)
  if (std::abs(r.pitch) > static_cast<scalar_t>(k_pi_by_2_d) - 1e-4)
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
inline euler_angles_t<scalar_t> make_euler_angles(quat_t<scalar_t> const& q) noexcept
{
  euler_angles_t<scalar_t> euler;

  // Pitch (X-axis rotation)
  scalar_t sinr_cosp = static_cast<scalar_t>(2.0) * (q.w * q.x + q.y * q.z);
  scalar_t cosr_cosp = static_cast<scalar_t>(1.0) - static_cast<scalar_t>(2.0) * (q.x * q.x + q.y * q.y);
  euler.pitch        = std::atan2(sinr_cosp, cosr_cosp);

  // Yaw (Y-axis rotation)
  scalar_t sinp = static_cast<scalar_t>(2.0) * (q.w * q.y - q.z * q.x);
  if (std::abs(sinp) >= static_cast<scalar_t>(1.0))
    euler.yaw =
      std::copysign(static_cast<scalar_t>(k_pi_d) / static_cast<scalar_t>(2.0), sinp); // Use 90 degrees if out of range
  else
    euler.yaw = std::asin(sinp);

  // Roll (Z-axis rotation)
  scalar_t siny_cosp = static_cast<scalar_t>(2.0) * (q.w * q.z + q.x * q.y);
  scalar_t cosy_cosp = static_cast<scalar_t>(1.0) - static_cast<scalar_t>(2.0) * (q.y * q.y + q.z * q.z);
  euler.roll         = std::atan2(siny_cosp, cosy_cosp);

  return euler;
}

template <typename scalar_t>
inline euler_angles_t<scalar_t> make_euler_angles(mat3_t<scalar_t> const& src) noexcept
{
  euler_angles_t<scalar_t> r;
  // Extract sin(r[0]) from e[3][2].
  auto sp = -src.e[2][1];
  // Check for Gimbel Lock
  if (std::abs(sp) > 0.99999f)
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
  return make_euler_angles(*reinterpret_cast<const mat3_t<scalar_t>*>(&m));
}

} // namespace acl
