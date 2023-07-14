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

/// @brief Test bounding volume bounding volume intersection
template <typename scalar_t>
inline result_t test_intersection(bounding_volume_t<scalar_t> const& i_vol1,
                                  bounding_volume_t<scalar_t> const& i_vol2) noexcept;

/// @brief Test bounding volume frustum_t intersection using coherency and masking.
template <typename scalar_t>
inline result_t test_intersection(bounding_volume_t<scalar_t> const& i_vol, frustum_t<scalar_t> const& i_frustum,
                                  coherency& io_coherency) noexcept;

/// @brief Test bounding volume frustum_t intersection
template <typename scalar_t>
inline result_t test_intersection(bounding_volume_t<scalar_t> const& i_vol,
                                  frustum_t<scalar_t> const&         i_frustum) noexcept;

/// @brief Intersect sphere with frustum_t
template <typename scalar_t>
inline result_t test_intersection(sphere_t<scalar_t> i_sphere, frustum_t<scalar_t> const& i_frustum) noexcept;

template <typename scalar_t>
inline result_t test_intersection(bounding_volume_t<scalar_t> const& vol1,
                                  bounding_volume_t<scalar_t> const& vol2) noexcept
{
  auto d = sub(vol1.spherical_vol, negate_w(vol2.spherical_vol));
  if (hadd(negate_w(mul(d, d))) > 0.0f)
    return result_t::k_outside;
  else
    return (greater_any(abs(make_vec3a(d)), add(vol1.half_extends, vol2.half_extends))) ? result_t::k_outside
                                                                                        : result_t::k_intersecting;
}

template <typename scalar_t>
inline result_t test_intersection(bounding_volume_t<scalar_t> const& i_vol, frustum_t<scalar_t> const& i_frustum,
                                  coherency& io_coherency) noexcept
{

  result_t      result    = result_t::k_inside;
  auto          planes    = i_frustum.get_all();
  uint32_t      nb_planes = i_frustum.size();
  std::uint32_t out_mask  = 0;
#ifndef NDEBUG
  io_coherency.iterations = 0;
#endif

  for (std::uint32_t i = 0; i < nb_planes; i++
#ifndef NDEBUG
                                           ,
                     io_coherency.iterations++
#endif
  )
  {
    std::uint32_t plane = (i + io_coherency.plane) % nb_planes;
    std::uint32_t k     = 1 << plane;
    if ((k & io_coherency.mask_hierarchy))
    {
      auto abs_norm = abs_normal(planes[plane]);
      auto m        = vdot(planes[plane], center(i_vol.spherical_vol));
      auto n        = vdot(abs_norm, i_vol.half_extends);
      if (isnegative_x(add_x(m, n)))
      {
        io_coherency.plane = plane;
        return result_t::k_outside;
      }
      if (isnegative_x(sub_x(m, n)))
      {
        out_mask |= k;
        result = result_t::k_intersecting;
      }
    }
  }
  io_coherency.mask_hierarchy = out_mask;
  return result;
}

template <typename scalar_t>
inline result_t test_intersection(bounding_volume_t<scalar_t> const& i_vol,
                                  frustum_t<scalar_t> const&         i_frustum) noexcept
{
  auto     planes    = i_frustum.get_all();
  uint32_t nb_planes = i_frustum.size();

  for (std::uint32_t i = 0; i < nb_planes; i++)
  {
    std::uint32_t plane    = i;
    auto          abs_norm = abs_normal(planes[plane]);
    auto          m        = vdot(planes[plane], center(i_vol.spherical_vol));
    auto          n        = vdot(abs_norm, i_vol.half_extends);
    if (isnegative_x(add_x(m, n)))
      return result_t::k_outside;

    if (isnegative_x(sub_x(m, n)))
      return result_t::k_intersecting;
  }
  return result_t::k_inside;
}

template <typename scalar_t>
inline result_t test_intersection(sphere_t<scalar_t> i_sphere, frustum_t<scalar_t> const& i_frustum) noexcept
{

  auto     planes    = i_frustum.get_all();
  uint32_t nb_planes = i_frustum.size();
  auto     vrad      = negate(vradius(i_sphere));
  for (std::uint32_t i = 0; i < nb_planes; i++)
  {
    std::uint32_t plane = i;
    auto          m     = vdot(planes[plane], center(i_sphere));
    if (islesser_x(m, vrad))
      return result_t::k_outside;
    if (isnegative_x(add_x(m, vrad)))
      return result_t::k_intersecting;
  }
  return result_t::k_inside;
}

} // namespace acl
