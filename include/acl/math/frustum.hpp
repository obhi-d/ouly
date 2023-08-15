#pragma once

#include "mat4.hpp"
#include "plane.hpp"
#include "vec4.hpp"
#include <span>
#include <tuple>

namespace acl
{

template <typename scalar_t>
static inline std::uint32_t size(frustum_t<scalar_t> const& v) noexcept
{
  return v.planes.size();
}

static inline auto default_coherency(std::uint32_t plane_count) noexcept
{
  return coherency(plane_count);
}

/**
 * @brief Construct from a transpose(view*projection) matrix
 * @param mat transpose(View*Projection) or transpose(Proj)*transpose(View) matrix
 */
template <typename scalar_t>
static inline frustum_t<scalar_t> make_frustum(mat4_t<scalar_t> const& m) noexcept
{
  frustum_t<scalar_t> ft;
  ft.planes.resize(6);
  // Near clipping planeT
  ft.planes[frustum_t<scalar_t>::k_near] = make_plane(m[2]);
  // Far clipping planeT
  ft.planes[frustum_t<scalar_t>::k_far] = make_plane(m[3] - m[2]);
  // Left clipping planeT
  ft.planes[frustum_t<scalar_t>::k_left] = make_plane(m[0] + m[3]);
  // Right clipping planeT
  ft.planes[frustum_t<scalar_t>::k_right] = make_plane(m[3] - m[0]);
  // Top clipping planeT
  ft.planes[frustum_t<scalar_t>::k_top] = make_plane(m[3] - m[1]);
  // Bottom clipping planeT
  ft.planes[frustum_t<scalar_t>::k_bottom] = make_plane(m[1] + m[3]);

  return ft;
}

/**
 * @brief Construct from a transpose(view*projection) matrix
 * @param mat transpose(View*Projection) or transpose(Proj)*transpose(View) matrix
 */
template <typename scalar_t>
static inline fixed_frustum_t<scalar_t> make_fixed_frustum(mat4_t<scalar_t> const& m) noexcept
{
  fixed_frustum_t<scalar_t> ft;
  // Near clipping planeT
  ft.planes[frustum_t<scalar_t>::k_near] = make_plane(m[2]);
  // Far clipping planeT
  ft.planes[frustum_t<scalar_t>::k_far] = make_plane(m[3] - m[2]);
  // Left clipping planeT
  ft.planes[frustum_t<scalar_t>::k_left] = make_plane(m[0] + m[3]);
  // Right clipping planeT
  ft.planes[frustum_t<scalar_t>::k_right] = make_plane(m[3] - m[0]);
  // Top clipping planeT
  ft.planes[frustum_t<scalar_t>::k_top] = make_plane(m[3] - m[1]);
  // Bottom clipping planeT
  ft.planes[frustum_t<scalar_t>::k_bottom] = make_plane(m[1] + m[3]);

  return ft;
}

/**
 * @brief Construct from a transpose(view*projection) matrix
 */
template <typename scalar_t>
static inline frustum_t<scalar_t> make_frustum(plane_t<scalar_t> const* planes, uint32_t size) noexcept
{
  return frustum_t<scalar_t>{planes, size};
}

} // namespace acl
