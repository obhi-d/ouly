#pragma once

#include "mat4.hpp"
#include "quad.hpp"
#include "sphere.hpp"
#include "transform.hpp"
#include "vec3.hpp"

namespace acl
{
struct bounds_info_t
{
  vec3_t center       = {};
  vec3_t half_extends = {};
  float  radius       = {};
};

struct bounds_info
{
  //! Appends the 'info' bounding box to '_' this one.
  static inline bounds_info_t& update(bounds_info_t& _, bounds_info_t const& info)
  {
    if (info.radius > 0)
    {
      if (_.radius <= 0)
        _ = info;
      else
      {
        vec3_t min_p   = vec3::min(vec3::sub(_.center, _.half_extends), vec3::sub(info.center, info.half_extends));
        vec3_t max_p   = vec3::max(vec3::add(_.center, _.half_extends), vec3::add(info.center, info.half_extends));
        vec3_t a       = vec3::abs(vec3::sub(_.center, info.center));
        _.center       = vec3::half(vec3::add(min_p, max_p));
        _.half_extends = vec3::half(vec3::sub(max_p, min_p));
        _.radius += (info.radius + acl::sqrt(vec3::dot(a, a)));
        _.radius *= 0.5f;
      }
    }
    return _;
  }
};

//! Represents a bounding volume, consisting of
//! an AABB and a bounding sphere, the AABB shares
//! the center with the sphere
struct bounding_volume_t
{
  //! center wrt some coordinate system
  sphere_t spherical_vol;
  //! half half_extends dx/2, dy/2, dz/2
  vec3a_t half_extends;
  //! center wrt some coordinate system mostly 0,0,0
  sphere_t orig_spherical_vol;
  //! half half_extends dx/2, dy/2, dz/2
  vec4_t orig_half_extends;
};

struct bounding_volume
{
  using type = bounding_volume_t;
  using pref = bounding_volume_t const&;
  using ref  = bounding_volume_t&;

  //! Returns the bounding box center
  inline static vec3a_t center(bounding_volume_t const&);
  //! Returns the bounding box half size
  inline static vec3a_t half_extends(bounding_volume_t const&);
  //! Returns the bounding sphere radius
  inline static float radius(bounding_volume_t const&);
  //! Returns the bounding sphere radius
  inline static quad_t vradius(bounding_volume_t const&);
  //! Nullify bounding volume (set it to (0,0,0),0)
  inline static void nullify(bounding_volume_t&);
  //! Compute from axis aliogned bounding box
  inline static type from_box(vec3a_t const& center, vec3a_t const& half_extends);
  inline static type set(vec3a_t const& center, vec3a_t const& half_extends, float radius);
  inline static type set(sphere_t const& sphere, vec3a_t const& half_extends);
  //! Given a matrix, update the bounding volume using the original extends and
  //! radius
  inline static void update(bounding_volume_t& _, mat4::pref m);
  //! Given a scale, rotation and translation, update the bounding volume using
  //! the original extends and radius
  inline static void update(bounding_volume_t& _, float scale, quat::pref rot, vec3a::pref translation);
  //! Given a transform, rotation and translation, update the bounding volume
  //! using the original extends and radius
  inline static void update(bounding_volume_t& _, transform_t const& tf);
  //! Compute the bounding volume from a set of points
  inline static void update(bounding_volume_t& _, vec3a_t const* points, std::uint32_t count);
  //! Compute the bounding volume by appending another bounding volume to it
  inline static void update(bounding_volume_t& _, bounding_volume_t const&);
};

inline vec3a_t bounding_volume::center(bounding_volume_t const& _)
{
  return sphere::center(_.spherical_vol);
}

inline vec3a_t bounding_volume::half_extends(bounding_volume_t const& _)
{
  return _.half_extends;
}

inline float bounding_volume::radius(bounding_volume_t const& _)
{
  return sphere::radius(_.spherical_vol);
}

inline quad_t bounding_volume::vradius(bounding_volume_t const& _)
{
  return sphere::vradius(_.spherical_vol);
}

inline void bounding_volume::nullify(bounding_volume_t& _)
{
  _.spherical_vol = _.orig_spherical_vol = _.half_extends = _.orig_half_extends = vec4::zero();
}

inline bounding_volume::type bounding_volume::from_box(vec3a_t const& center, vec3a_t const& half_extends)
{
  bounding_volume_t _;
  _.spherical_vol = _.orig_spherical_vol = sphere::set(center, sphere::max_radius(half_extends));
  _.half_extends = _.orig_half_extends = half_extends;
  return _;
}

inline bounding_volume::type bounding_volume::set(vec3a_t const& center, vec3a_t const& half_extends, float radius)
{
  bounding_volume_t _;
  _.spherical_vol = _.orig_spherical_vol = sphere::set(center, radius);
  _.half_extends = _.orig_half_extends = half_extends;
  return _;
}

inline bounding_volume::type bounding_volume::set(sphere_t const& sphere_arg, vec3a_t const& half_extends)
{
  bounding_volume_t _;
  _.spherical_vol = _.orig_spherical_vol = sphere_arg;
  _.half_extends = _.orig_half_extends = half_extends;
  return _;
}

inline void bounding_volume::update(bounding_volume_t& _, mat4::pref m)
{
  // TODO test which is tighter avro's bound transform or this one
  _.spherical_vol =
    sphere::scale_radius(sphere::set(mat4::transform_assume_ortho(m, sphere::center(_.orig_spherical_vol)),
                                     sphere::radius(_.orig_spherical_vol)),
                         mat4::max_scale(m));

  _.half_extends = mat4::transform_bounds_extends(m, _.orig_half_extends);
}

inline void bounding_volume::update(bounding_volume_t& _, float scale, quat::pref rot, vec3a::pref translation)
{

  _.spherical_vol = sphere::scale_radius(
    sphere::set(vec3a::add(quat::transform(rot, sphere::center(_.orig_spherical_vol)), translation),
                sphere::radius(_.spherical_vol)),
    scale);
  _.half_extends = quat::transform_bounds_extends(rot, vec3a::mul(_.orig_half_extends, scale));
}

inline void bounding_volume::update(bounding_volume_t& _, transform_t const& tf)
{
  update(_, transform::scale(tf), transform::rotation(tf), transform::translation(tf));
}

inline void bounding_volume::update(bounding_volume_t& _, vec3a_t const* points, std::uint32_t count)
{
  aabb_t box = aabb::set(center(_), half_extends(_));
  for (std::uint32_t i = 0; i < count; i++)
    box = aabb::append(box, points[i]);
  _ = from_box(aabb::center(box), aabb::half_size(box));
}

inline void bounding_volume::update(bounding_volume_t& _, bounding_volume_t const& vol)
{
  vec3a_t center_this  = center(_);
  vec3a_t center_other = center(vol);

  vec3a_t a       = vec3a::abs(vec3a::sub(center_this, center_other));
  vec3a_t min_p   = vec3a::min(vec3a::sub(center_this, _.half_extends), vec3a::sub(center_other, vol.half_extends));
  vec3a_t max_p   = vec3a::max(vec3a::add(center_this, _.half_extends), vec3a::add(center_other, vol.half_extends));
  _.half_extends  = vec3a::half(vec3a::sub(max_p, min_p));
  _.spherical_vol = vec3a::set_w(
    vec3a::half(vec3a::add(min_p, max_p)),
    vec3a::half_x(vec3a::add_x(vec3a::add_x(sphere::vradius(vol.spherical_vol), sphere::vradius(_.spherical_vol)),
                               vec3a::sqrt_x(vec3a::vdot(a, a)))));
  _.orig_spherical_vol = _.spherical_vol;
  _.orig_half_extends  = _.half_extends;
}
} // namespace acl