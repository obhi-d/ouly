#pragma once

#include "vec3a.hpp"

namespace acl
{

template <typename scalar_t>
inline plane_t<scalar_t> make_plane(vec3a_t<scalar_t> const& i_normal, float d) noexcept
{
  return vml::set_w(i_normal.v, d);
}

template <typename scalar_t>
inline plane_t<scalar_t> normalize(plane_t<scalar_t> const& i_plane) noexcept
{
  return vml::normalize(i_plane.v, vml::clear_w(i_plane.v));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> vdot(plane_t<scalar_t> const& p, vec3a_t<scalar_t> const& v) noexcept
{
  return vml::vdot(p.v, v.v);
}

template <typename scalar_t>
inline scalar_t dot(plane_t<scalar_t> const& p, vec3a_t<scalar_t> const& v) noexcept
{
  return vml::dot(p.v, v.v);
}

template <typename scalar_t>
inline scalar_t dot_with_normal(plane_t<scalar_t> const& p, vec3a_t<scalar_t> const& v) noexcept
{
  return vml::dot(make_vec3a(p).v, v.v);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> abs_normal(plane_t<scalar_t> const& p) noexcept
{
  return abs(make_vec3a(p));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> get_normal(plane_t<scalar_t> const& p) noexcept
{
  return make_vec3a(p);
}

} // namespace acl
