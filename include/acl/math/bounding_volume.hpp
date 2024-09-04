#pragma once

#include "mat4.hpp"
#include "quad.hpp"
#include "quat.hpp"
#include "sphere.hpp"
#include "transform.hpp"
#include "vec3.hpp"

namespace acl
{

/**
 * @brief Appends the 'info' bounding box to 'dest'.
 */
template <typename scalar_t>
inline bounds_info_t<scalar_t>& bounds_info_t<scalar_t>::operator+=(bounds_info_t<scalar_t> const& src) noexcept
{
  if (src.radius > 0)
  {
    if (radius <= 0)
      *this = src;
    else
    {
      auto min_p   = min(center - half_extends, src.center - src.half_extends);
      auto max_p   = max(center + half_extends, src.center + src.half_extends);
      auto a       = abs(center - src.center);
      center       = half(max_p + min_p);
      half_extends = half(max_p - min_p);
      radius += (src.radius + std::sqrt(dot(a, a)));
      radius *= 0.5f;
    }
  }
  return *this;
}

template <typename scalar_t>
inline bounds_info_t<scalar_t> operator+(bounds_info_t<scalar_t> const& a, bounds_info_t<scalar_t> const& b) noexcept
{
  if (a.radius == 0)
    return b;
  if (b.radius == 0)
    return a;

  auto                    min_p = min(a.center - a.half_extends, b.center - b.half_extends);
  auto                    max_p = max(a.center + a.half_extends, b.center + b.half_extends);
  auto                    v     = abs(a.center - b.center);
  bounds_info_t<scalar_t> r;
  r.center       = half(min_p + max_p);
  r.half_extends = half(max_p - min_p);
  r.radius       = (a.radius + (b.radius + std::sqrt(dot(v, v)))) * .5f;
  return r;
}

/**
 * @brief Returns the bounding box center
 */
template <typename scalar_t>
inline vec3a_t<scalar_t> center(bounding_volume_t<scalar_t> const& v) noexcept
{
  return center(v.spherical_vol);
}

/**
 * @brief Returns the bounding box half size
 */
template <typename scalar_t>
inline vec3a_t<scalar_t> half_extends(bounding_volume_t<scalar_t> const& v) noexcept
{
  return v.half_extends;
}

/**
 * @brief Returns the bounding sphere radius
 */
template <typename scalar_t>
inline scalar_t radius(bounding_volume_t<scalar_t> const& v) noexcept
{
  return radius(v.spherical_vol);
}

/**
 * @brief Returns the bounding sphere radius
 */
template <typename scalar_t>
inline auto vradius(bounding_volume_t<scalar_t> const& v) noexcept
{
  return vec3a_t<scalar_t>(v.spherical_vol.w);
}

template <typename scalar_t>
inline void nullify(bounding_volume_t<scalar_t>& v) noexcept
{
  v.spherical_vol = v.half_extends = vml::zero<scalar_t>();
}

/**
 * @brief Compute from axis aliogned bounding box
 */
template <typename scalar_t>
inline bounding_volume_t<scalar_t> make_bounding_volume(vec3a_t<scalar_t> const& center,
                                                        vec3a_t<scalar_t> const& half_extends) noexcept
{
  return bounding_volume_t<scalar_t>(make_sphere(center, max_radius(half_extends)), half_extends);
}

template <typename scalar_t>
inline bounding_volume_t<scalar_t> make_bounding_volume(vec3a_t<scalar_t> const& center,
                                                        vec3a_t<scalar_t> const& half_extends, float radius) noexcept
{
  return bounding_volume_t<scalar_t>(make_sphere(center, radius), half_extends);
}

template <typename scalar_t>
inline bounding_volume_t<scalar_t> make_bounding_volume(sphere_t<scalar_t> const& sphere,
                                                        vec3a_t<scalar_t> const&  halfextends)
{
  return bounding_volume_t<scalar_t>(sphere, halfextends);
}

template <typename scalar_t>
inline auto make_bounding_volume(bounding_volume_t<scalar_t> const& bv, scalar_t scale, quat_t<scalar_t> const& rot,
                                 vec3a_t<scalar_t> const& translation) noexcept
{
  return bounding_volume_t<scalar_t>(
    vml::mul(
      vml::set_w(vml::add(vml::mul_quat(center(bv.spherical_vol).v, rot.v), translation.v), radius(bv.spherical_vol)),
      vml::set<scalar_t>(1.0f, 1.0f, 1.0f, scale)),
    ((bv.half_extends * scale) * rot));
}

template <typename scalar_t>
inline auto operator*(bounding_volume_t<scalar_t> const& bv, transform_t<scalar_t> const& tf) noexcept
{
  return make_bounding_volume(bv, scale(tf), rotation(tf), translation(tf));
}

template <typename scalar_t>
inline bounding_volume_t<scalar_t> make_bounding_volume(vec3a_t<scalar_t> const* points, uint32_t count) noexcept
{
  auto box = aabb_t<scalar_t>(points[0], extends3d_t<scalar_t>());
  for (std::uint32_t i = 1; i < count; i++)
    box = box + points[i];
  return make_bounding_volume(center(box), half_size(box));
}

template <typename scalar_t>
inline bounding_volume_t<scalar_t> operator+(bounding_volume_t<scalar_t> const& op1,
                                             bounding_volume_t<scalar_t> const& op2) noexcept
{
  auto center_this  = center(op1);
  auto center_other = center(op2);

  auto a     = vml::abs(vml::sub(center_this.v, center_other.v));
  auto min_p = vml::min(vml::sub(center_this.v, op1.half_extends.v), vml::sub(center_other.v, op2.half_extends.v));
  auto max_p = vml::max(vml::add(center_this.v, op1.half_extends.v), vml::add(center_other.v, op2.half_extends.v));

  bounding_volume_t<scalar_t> r{noinit_v};
  r.spherical_vol = vml::set_w(vml::half(vml::add(min_p, max_p)),
                               ((radius(op2.spherical_vol) + radius(op1.spherical_vol)) + std::sqrt(vml::dot(a, a))) *
                                 static_cast<scalar_t>(0.5));
  r.half_extends  = vml::half(vml::sub(max_p, min_p));
  return r;
}

template <typename scalar_t>
inline bounding_volume_t<scalar_t>& bounding_volume_t<scalar_t>::operator+=(
  bounding_volume_t<scalar_t> const& vol) noexcept
{
  *this = *this + vol;
  return *this;
}

/**
 * @brief Given a matrix, update the bounding volume using the original extends and
 * @brief radius
 */
template <typename scalar_t>
inline bounding_volume_t<scalar_t> operator*(bounding_volume_t<scalar_t> const& bv, mat4_t<scalar_t> const& m) noexcept
{
  auto recenter = center(bv.spherical_vol) * m;
  recenter.w    = radius(bv.spherical_vol) * max_scale(m);
  return {recenter, bv.half_extends * m};
}

} // namespace acl
