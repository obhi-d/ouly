
#pragma once
#include "axis_angle.hpp"
#include "quad.hpp"
#include "vec3.hpp"
#include "vec4.hpp"
#include "vml_fcn.hpp"

namespace acl
{
template <ScalarType scalar_t>
inline quat_t<scalar_t> normalize(quat_t<scalar_t> const& r) noexcept
{
  return vml::normalize(r.v);
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> make_identity_quaternion() noexcept
{
  return quat_t<scalar_t>(0.0f, 0.0f, 0.0f, 1.0f);
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> conjugate(quat_t<scalar_t> const& q) noexcept
{
  return vml::conjugate_quat(q.v);
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> make_quaternion(vec3a_t<scalar_t> const& axis, scalar_t ang) noexcept
{
#ifndef NDEBUG
  scalar_t len = length(axis);
  assert(approx_equals(len, static_cast<scalar_t>(1), static_cast<scalar_t>(acl::k_const_epsilon_med)));
#endif
  auto sc = acl::sin_cos(ang * static_cast<scalar_t>(.5f));
  return quat_t<scalar_t>(sc.first * axis, sc.second);
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> make_quaternion(axis_angle_t<scalar_t> const& ax) noexcept
{
  return make_quaternion<scalar_t>(axis(ax), angle(ax));
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> make_quaternion(mat3_t<scalar_t> const& m) noexcept
{

  auto trace = m.e[0][0] + m.e[1][1] + m.e[2][2] + 1.0f;
  if (trace > 0.0f)
  {
    return quat_t<scalar_t>((m.e[1][2] - m.e[2][1]) / (2.0f * std::sqrt(trace)),
                            (m.e[2][0] - m.e[0][2]) / (2.0f * std::sqrt(trace)),
                            (m.e[0][1] - m.e[1][0]) / (2.0f * std::sqrt(trace)), std::sqrt(trace) / 2.0f);
  }
  int  maxi    = 0;
  auto maxdiag = m.e[0][0];

  if (m.e[1][1] > maxdiag)
  {
    maxdiag = m.e[1][1];
    maxi    = 1;
  }

  if (m.e[2][2] > maxdiag)
  {
    maxdiag = m.e[2][2];
    maxi    = 2;
  }

  switch (maxi)
  {
  case 0:
  {
    auto s    = 2.0f * std::sqrt(1.0f + m.e[0][0] - m.e[1][1] - m.e[2][2]);
    auto invS = 1 / s;
    return normalize(quat_t<scalar_t>(0.25f * s, (m.e[0][1] + m.e[1][0]) * invS, (m.e[0][2] + m.e[2][0]) * invS,
                                      (m.e[1][2] - m.e[2][1]) * invS));
  }
  case 1:
  {
    auto s    = 2.0f * std::sqrt(1.0f + m.e[1][1] - m.e[0][0] - m.e[2][2]);
    auto invS = 1 / s;
    return normalize(quat_t<scalar_t>((m.e[0][1] + m.e[1][0]) * invS, 0.25f * s, (m.e[1][2] + m.e[2][1]) * invS,
                                      (m.e[2][0] - m.e[0][2]) * invS));
  }
  case 2:
  default:
  {
    auto s    = 2.0f * std::sqrt(1.0f + m.e[2][2] - m.e[0][0] - m.e[1][1]);
    auto invS = 1 / s;
    return normalize(quat_t<scalar_t>((m.e[0][2] + m.e[2][0]) * invS, (m.e[1][2] + m.e[2][1]) * invS, 0.25f * s,
                                      (m.e[0][1] - m.e[1][0]) * invS));
  }
  }
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> make_quaternion(mat4_t<scalar_t> const& m) noexcept
{
  return make_quaternion(*reinterpret_cast<const mat3_t<scalar_t>*>(&m));
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> operator*(quat_t<scalar_t> const& q1, quat_t<scalar_t> const& q2) noexcept
{
  return vml::mul_quat(q1.v, q2.v);
}

template <ScalarType scalar_t>
inline vec3a_t<scalar_t> operator*(vec3a_t<scalar_t> const& v, quat_t<scalar_t> const& q) noexcept
{
  return vml::mul_vec3a_quat(v.v, q.v);
}

template <ScalarType scalar_t>
inline extends3d_t<scalar_t> operator*(extends3d_t<scalar_t> const& v, quat_t<scalar_t> const& rot) noexcept
{
  return vml::mul_extends_quat(v.v, rot.v);
}

template <ScalarType scalar_t, FloatingType blend_t>
inline quat_t<scalar_t> slerp(quat_t<scalar_t> const& from, quat_t<scalar_t> const& to, blend_t t) noexcept
{
  scalar_t sinom, omega, scale0, scale1;
  auto     cosom     = vml::dot(from.v, to.v);
  auto     abs_cosom = std::abs(cosom);
  if ((1.0f - abs_cosom) > acl::k_const_epsilon)
  {
    omega  = std::acos(abs_cosom);
    sinom  = 1.0f / std::sin(omega);
    scale0 = std::sin((1.0f - t) * omega) * sinom;
    scale1 = std::sin(t * omega) * sinom;
  }
  else
  {
    scale0 = 1.0f - t;
    scale1 = t;
  }

  scale1 = (cosom >= 0.0f) ? scale1 : -scale1;
  return vml::normalize(vml::add(vml::mul_quad_scalar(from.v, scale0), vml::mul_quad_scalar(to.v, scale1)));
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> inverse(quat_t<scalar_t> const& q) noexcept
{
  return conjugate(q);
}

} // namespace acl
