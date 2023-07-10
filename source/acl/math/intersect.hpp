#pragma once
#include "bounding_volume.hpp"
#include "frustum.hpp"
#include "sphere.hpp"

namespace acl
{

enum class result_t : std::uint32_t
{
  k_outside = 0,
  k_inside,
  k_intersecting
};

/** @remarks Test bounding volume bounding volume */
inline result_t bounding_volumes(bounding_volume_t const& i_vol1, bounding_volume_t const& i_vol2);

/**
 * @remarks Test bounding volume frustum_t intersection using coherency
 *          and masking.
 */
inline result_t bounding_volume_frustum_coherent(bounding_volume_t const& i_vol, frustum_t const& i_frustum,
                                                 frustum_t::coherency& io_coherency);

/** @remarks Test bounding volume frustum_t intersection */
inline result_t bounding_volume_frustum(bounding_volume_t const& i_vol, frustum_t const& i_frustum);

/** @remarks Intersect sphere with frustum_t */
inline result_t bounding_sphere_frustum(sphere::pref i_sphere, frustum_t const& i_frustum);

inline result_t bounding_volumes(bounding_volume_t const& vol1, bounding_volume_t const& vol2)
{

  vec3a::type d = vec4::sub(vol1.spherical_vol, quad::negate_w(vol2.spherical_vol));
  if (quad::hadd(quad::negate_w(vec4::mul(d, d))) > 0.0f)
    return result_t::k_outside;
  else
    return (vec3a::greater_any(vec3a::abs(vec3a::from_vec4(d)), vec3a::add(vol1.half_extends, vol2.half_extends)))
             ? result_t::k_outside
             : result_t::k_intersecting;
}

inline result_t bounding_volume_frustum_coherent(bounding_volume_t const& i_vol, frustum_t const& i_frustum,
                                                 frustum_t::coherency& i_coherency)
{

  result_t      result   = result_t::k_inside;
  auto          planes   = frustum::get_planes(i_frustum);
  std::uint32_t out_mask = 0;
#ifndef NDEBUG
  i_coherency.iterations = 0;
#endif

  for (std::uint32_t i = 0; i < planes.second; i++
#ifndef NDEBUG
                                               ,
                     i_coherency.iterations++
#endif
  )
  {
    std::uint32_t plane = (i + i_coherency.plane) % planes.second;
    std::uint32_t k     = 1 << plane;
    if ((k & i_coherency.mask_hierarchy))
    {
      vec3a_t abs_norm = plane::abs_normal(planes.first[plane]);
      auto    m        = plane::vdot(planes.first[plane], sphere::center(i_vol.spherical_vol));
      auto    n        = vec3a::vdot(abs_norm, i_vol.half_extends);
      if (quad::isnegative_x(quad::add_x(m, n)))
      {
        i_coherency.plane = plane;
        return result_t::k_outside;
      }
      if (quad::isnegative_x(quad::sub_x(m, n)))
      {
        out_mask |= k;
        result = result_t::k_intersecting;
      }
    }
  }
  i_coherency.mask_hierarchy = out_mask;
  return result;
}

inline result_t bounding_volume_frustum(bounding_volume_t const& i_vol, frustum_t const& i_frustum)
{

  auto planes = frustum::get_planes(i_frustum);

  for (std::uint32_t i = 0; i < planes.second; i++)
  {
    std::uint32_t plane    = i;
    vec3a_t       abs_norm = plane::abs_normal(planes.first[plane]);
    auto          m        = plane::vdot(planes.first[plane], sphere::center(i_vol.spherical_vol));
    auto          n        = vec3a::vdot(abs_norm, i_vol.half_extends);
    if (quad::isnegative_x(quad::add_x(m, n)))
      return result_t::k_outside;

    if (quad::isnegative_x(quad::sub_x(m, n)))
      return result_t::k_intersecting;
  }
  return result_t::k_inside;
}

inline result_t bounding_sphere_frustum(sphere::pref i_sphere, frustum_t const& i_frustum)
{

  auto   planes = frustum::get_planes(i_frustum);
  quad_t vrad   = vec3a::negate(sphere::vradius(i_sphere));
  for (std::uint32_t i = 0; i < planes.second; i++)
  {
    std::uint32_t plane = i;
    auto          m     = plane::vdot(planes.first[plane], sphere::center(i_sphere));
    if (quad::islesser_x(m, vrad))
      return result_t::k_outside;
    if (quad::isnegative_x(quad::add_x(m, vrad)))
      return result_t::k_intersecting;
  }
  return result_t::k_inside;
}

} // namespace acl
