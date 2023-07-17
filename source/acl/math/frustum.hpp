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

/// @brief Construct from a transpose(view*projection) matrix
/// @param mat transpose(View*Projection) or transpose(Proj)*transpose(View) matrix
template <typename scalar_t>
static inline frustum_t<scalar_t> make_frustum(mat4_t<scalar_t> const& m) noexcept
{
  frustum_t<scalar_t> ft;
  ft.planes.resize(6);
  // Near clipping planeT
  ft.planes[frustum_t<scalar_t>::k_near] = normalize(m[2]);
  // Far clipping planeT
  ft.planes[frustum_t<scalar_t>::k_far] = normalize(m[3] - m[2]);
  // Left clipping planeT
  ft.planes[frustum_t<scalar_t>::k_left] = normalize(m[0] + m[3]);
  // Right clipping planeT
  ft.planes[frustum_t<scalar_t>::k_right] = normalize(m[3] - m[0]);
  // Top clipping planeT
  ft.planes[frustum_t<scalar_t>::k_top] = normalize(m[3] - m[1]);
  // Bottom clipping planeT
  ft.planes[frustum_t<scalar_t>::k_bottom] = normalize(m[1] + m[3]);

  return ft;
}

/// @brief Construct from a transpose(view*projection) matrix
template <typename scalar_t>
static inline frustum_t<scalar_t> make_frustum(plane_t<scalar_t> const* planes, uint32_t size) noexcept
{
  return frustum_t<scalar_t>{planes, size};
}

template <typename scalar_t>
static inline std::span<plane_t<scalar_t> const> get_planes(frustum_t<scalar_t> const& m) noexcept
{
  return std::span<plane_t<scalar_t> const>(m.get_all(), m.size());
}

template <typename scalar_t>
static inline std::span<plane_t<scalar_t>> get_planes(frustum_t<scalar_t>& m) noexcept
{
  return std::span<plane_t<scalar_t>>(m.planes.data(), m.size());
}

template <typename scalar_t>
static inline void set_plane(frustum_t<scalar_t>& f, std::uint32_t i, plane_t<scalar_t> const& p) noexcept
{
  f.modify(i, p);
}

} // namespace acl
