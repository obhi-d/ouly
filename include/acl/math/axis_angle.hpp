#pragma once

#include "quad.hpp"
#include "vec3a.hpp"

namespace acl
{

template <typename scalar_t>
static inline axis_angle_t<scalar_t> make_axis_angle(quat_t<scalar_t> const& q) noexcept
{
  float angle = 2 * std::acos(q.w);
  auto  qf    = std::sqrt(1 - q.w * q.w);
  if (qf < k_const_epsilon_med)
  {
    return axis_angle_t<scalar_t>(1, 0, 0, 0);
  }
  qf = 1 / qf;
  return vml::set_w(vml::mul_quad_scalar(q, qf), angle);
}

template <typename scalar_t>
static inline axis_angle_t<scalar_t> make_axis_angle(vec3a_t<scalar_t> const& axis, scalar_t angle) noexcept
{
  return vml::set_w(axis.v, angle);
}

template <typename scalar_t>
static inline vec3a_t<scalar_t> axis(axis_angle_t<scalar_t> const& q) noexcept
{
  return make_vec3a(q);
}

template <typename scalar_t>
static inline auto angle(axis_angle_t<scalar_t> const& q) noexcept
{
  return q.w;
}

template <typename scalar_t>
static inline vec3a_t<scalar_t> vangle(axis_angle_t<scalar_t> const& v) noexcept
{
  return vml::set_x<3>(v.v);
}

} // namespace acl
