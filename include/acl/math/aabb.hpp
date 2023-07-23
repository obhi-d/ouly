#pragma once
#include "mat_base.hpp"
#include "vec3a.hpp"

namespace acl
{

template <typename scalar_t>
inline bool equals(aabb_t<scalar_t> const& a, aabb_t<scalar_t> const& b) noexcept
{
  return equals(a[0], b[0]) && equals(a[1], b[1]);
}

template <typename scalar_t>
inline bool is_valid(aabb_t<scalar_t> const& box) noexcept
{
  return greater_all(box.r[1], box.r[0]);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> center(aabb_t<scalar_t> const& box) noexcept
{
  return vml::half(vml::add(box.v[1], box.v[0]));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> size(aabb_t<scalar_t> const& box) noexcept
{
  return vml::sub(box.v[1], box.v[0]);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> half_size(aabb_t<scalar_t> const& box) noexcept
{
  return vml::half(vml::sub(box.v[1], box.v[0]));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> corner(aabb_t<scalar_t> const& box, unsigned int i) noexcept
{
  return vec3a_t<scalar_t>(vml::get_x(box.v[((i >> 2) & 1)]), vml::get_y(box.v[((i >> 1) & 1)]),
                           vml::get_z(box.v[((i >> 0) & 1)]));
}

template <typename scalar_t>
inline aabb_t<scalar_t> operator+(aabb_t<scalar_t> const& b, vec3a_t<scalar_t> const& point) noexcept
{
  return {vec3a_t<scalar_t>(vml::min(b.v[0], point.v)), vec3a_t<scalar_t>(vml::max(b.v[1], point.v))};
}

template <typename scalar_t>
inline aabb_t<scalar_t> operator+(aabb_t<scalar_t> const& box, aabb_t<scalar_t> const& other) noexcept
{
  return {vml::min(box.v[0], other.v[0]), vml::max(box.v[1], other.v[1])};
}

template <typename scalar_t>
inline aabb_t<scalar_t> make_aabb_from_center_extends(vec3a_t<scalar_t> const& center,
                                                      vec3a_t<scalar_t> const& extends) noexcept
{
  return aabb_t<scalar_t>{vml::sub(center.v, extends.v), vml::add(center.v, extends.v)};
}

template <typename scalar_t>
inline aabb_t<scalar_t> make_aabb_from_min_max(vec3a_t<scalar_t> const& i_min, vec3a_t<scalar_t> const& i_max) noexcept
{
  return aabb_t<scalar_t>{i_min, i_max};
}
} // namespace acl
