#pragma once

#include "mat4.hpp"
#include "quad.hpp"
#include "quat.hpp"
#include "sphere.hpp"
#include "transform.hpp"
#include "vec3.hpp"

namespace acl
{

/// @brief Appends the 'info' bounding box to 'dest'.
template <typename scalar_t>
static inline bounds_info_t<scalar_t>& append(bounds_info_t<scalar_t>&       dest,
                                              bounds_info_t<scalar_t> const& src) noexcept
{
  if (src.radius > 0)
  {
    if (dest.radius <= 0)
      dest = src;
    else
    {
      vec3_t min_p      = min(sub(dest.center, dest.half_extends), sub(src.center, src.half_extends));
      vec3_t max_p      = max(add(dest.center, dest.half_extends), add(src.center, src.half_extends));
      vec3_t a          = abs(sub(dest.center, src.center));
      dest.center       = half(add(min_p, max_p));
      dest.half_extends = half(sub(max_p, min_p));
      dest.radius += (src.radius + std::sqrt(dot(a, a)));
      dest.radius *= 0.5f;
    }
  }
  return dest;
}

/// @brief Returns the bounding box center
template <typename scalar_t>
inline vec3a_t<scalar_t> center(bounding_volume_t<scalar_t> const& v) noexcept
{
  return center(v.spherical_vol);
}

/// @brief Returns the bounding box half size
template <typename scalar_t>
inline vec3a_t<scalar_t> half_extends(bounding_volume_t<scalar_t> const& v) noexcept
{
  return v.half_extends;
}

/// @brief Returns the bounding sphere radius
template <typename scalar_t>
inline scalar_t radius(bounding_volume_t<scalar_t> const& v) noexcept
{
  return radius(v.spherical_vol);
}

/// @brief Returns the bounding sphere radius
template <typename scalar_t>
inline auto vradius(bounding_volume_t<scalar_t> const& v) noexcept
{
  return vradius(v.spherical_vol);
}

template <typename scalar_t>
inline void nullify(bounding_volume_t<scalar_t>& v) noexcept
{
  v.spherical_vol = v.half_extends = zero<scalar_t, default_tag>();
}

/// @brief Compute from axis aliogned bounding box
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
  return bounding_volume_t(sphere, halfextends);
}

template <typename scalar_t>
inline auto mul(bounding_volume_t<scalar_t> const& bv, scalar_t scale, quat_t<scalar_t> const& rot,
                vec3a_t<scalar_t> const& translation) noexcept
{
  return bounding_volume_t<scalar_t>(
    scale_radius(set_w(add(mul(center(bv.spherical_vol), rot), translation), radius(bv.spherical_vol)), scale),
    rotate_bounds_extends(mul(bv.half_extends, scale), rot));
}

template <typename scalar_t>
inline auto mul(bounding_volume_t<scalar_t> const& bv, transform_t<scalar_t> const& tf) noexcept
{
  return mul(bv, scale(tf), rotation(tf), translation(tf));
}

template <typename scalar_t>
inline auto operator*(bounding_volume_t<scalar_t> const& bv, transform_t<scalar_t> const& tf) noexcept
{
  return mul(bv, tf);
}

template <typename scalar_t>
inline auto& append(bounding_volume_t<scalar_t>& bv, vec3a_t<scalar_t> const* points, uint32_t count) noexcept
{
  auto box = aabb_t<scalar_t>(center(bv), half_extends(bv));
  for (std::uint32_t i = 0; i < count; i++)
    box = append(box, points[i]);
  return make_bounding_volume(center(box), half_size(box));
}

template <typename scalar_t>
inline auto& append(bounding_volume_t<scalar_t>& bv, bounding_volume_t<scalar_t> const& vol) noexcept
{
  auto center_this  = center(bv);
  auto center_other = center(vol);

  auto a     = abs(sub(center_this, center_other));
  auto min_p = min(sub(center_this, bv.half_extends), sub(center_other, vol.half_extends));
  auto max_p = max(add(center_this, bv.half_extends), add(center_other, vol.half_extends));

  bv.spherical_vol =
    set_w(half(add(min_p, max_p)),
          half_x(add_x(add_x(vradius(vol.spherical_vol), vradius(bv.spherical_vol)), sqrt_x(vdot(a, a)))));
  bv.half_extends = half(sub(max_p, min_p));
  return bv;
}

/// @brief Given a matrix, update the bounding volume using the original extends and
/// @brief radius
template <typename scalar_t>
inline bounding_volume_t<scalar_t> mul(bounding_volume_t<scalar_t> const& bv, mat4_t<scalar_t> const& m) noexcept
{
  // TODO test which is tighter avro's bound transform or this one
  return {
    scale_radius(set_w(transform_assume_ortho(m, center(bv.spherical_vol)), radius(bv.spherical_vol)), max_scale(m)),
    transform_bounds_extends(m, bv.half_extends)};
}

template <typename scalar_t>
inline bounding_volume_t<scalar_t> operator*(bounding_volume_t<scalar_t> const& bv, mat4_t<scalar_t> const& m) noexcept
{
  return mul(bv, m);
}

} // namespace acl
