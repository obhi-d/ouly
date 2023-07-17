#pragma once
#include "quad.hpp"

namespace acl
{

template <typename scalar_t>
inline vec3a_t<scalar_t> make_vec3a(scalar_t x, scalar_t y, scalar_t z) noexcept
{
  return vec3a_t<scalar_t>(x, y, z, 0.0f);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> make_vec3a(scalar_t p) noexcept
{
  return vec3a_t<scalar_t>(p, p, p, 0.0f);
}

template <typename scalar_t, typename stag>
inline vec3a_t<scalar_t> make_vec3a(quad_t<scalar_t, stag> const& p) noexcept
{
  return vml::clear_w(p.v);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> normalize(vec3a_t<scalar_t> const& vec) noexcept
{
  return vml::normalize(vec.v);
}

template <typename scalar_t>
inline scalar_t dot(vec3a_t<scalar_t> const& q1, vec3a_t<scalar_t> const& q2) noexcept
{
  if constexpr (has_sse)
    return vml::dot(q1.v, q2.v);
  else
    return (q1[0] * q2[0]) + (q1[1] * q2[1]) + (q1[2] * q2[2]);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> vdot(vec3a_t<scalar_t> const& vec1, vec3a_t<scalar_t> const& vec2) noexcept
{
  if constexpr (has_sse)
    return vml::vdot(vec1.v, vec1.v);
  else
    return vec3a_t<scalar_t>(dot(vec1, vec2), 0, 0, 0);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> cross(vec3a_t<scalar_t> const& vec1, vec3a_t<scalar_t> const& vec2) noexcept
{
  return vml::cross(vec1.v, vec2.v);
}

template <typename scalar_t>
inline bool greater_all(vec3a_t<scalar_t> const& q1, vec3a_t<scalar_t> const& q2) noexcept
{
  return vml::greater_all_3(q1.v, q2.v);
}

template <typename scalar_t>
inline bool greater_any(vec3a_t<scalar_t> const& q1, vec3a_t<scalar_t> const& q2) noexcept
{
  if constexpr (has_sse)
    return vml::greater_any(q1.v, q2.v);
  else
    return (q1[0] > q2[0] || q1[1] > q2[1] || q1[2] > q2[2]) != 0;
}

template <typename scalar_t>
inline bool lesser_all(vec3a_t<scalar_t> const& q1, vec3a_t<scalar_t> const& q2) noexcept
{
  return vml::lesser_all_3(q1.v, q2.v);
}

template <typename scalar_t>
inline bool lesser_any(vec3a_t<scalar_t> const& q1, vec3a_t<scalar_t> const& q2) noexcept
{
  if constexpr (has_sse)
    return vml::lesser_any(q1.v, q2.v);
  else
    return q1[0] < q2[0] || q1[1] < q2[1] || q1[2] < q2[2];
}

} // namespace acl
