#pragma once
#include "vec3a.hpp"

namespace acl
{

template <ScalarType scalar_t, ScalarType radius_t>
inline sphere_t<scalar_t> make_sphere(vec3a_t<scalar_t> const& v, radius_t radius) noexcept
{
  return sphere_t<scalar_t>{vml::set_w(v.v, static_cast<scalar_t>(radius))};
}

template <ScalarType scalar_t>
inline scalar_t radius(sphere_t<scalar_t> const& v) noexcept
{
  return vml::get_w(v.v);
}

template <ScalarType scalar_t>
inline vec3a_t<scalar_t> vradius(sphere_t<scalar_t> const& v) noexcept
{
  return vml::set_x<3>(v.v);
}

template <ScalarType scalar_t>
inline vec3a_t<scalar_t> center(sphere_t<scalar_t> const& v) noexcept
{
  return vml::clear_w(v.v);
}

template <ScalarType scalar_t>
inline scalar_t max_radius(vec3a_t<scalar_t> const& v) noexcept
{
  return vml::length(v.v);
}

template <ScalarType scalar_t>
inline sphere_t<scalar_t> scale_radius(vec3a_t<scalar_t> const& p, scalar_t scale) noexcept
{
  return vml::mul(p.v, vml::set<scalar_t>(1.0f, 1.0f, 1.0f, scale));
}
} // namespace acl
