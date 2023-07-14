#pragma once

#include "mat4.hpp"
#include "plane.hpp"
#include "vec4.hpp"
#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <span>
#include <tuple>

namespace acl
{

template <typename scalar_t>
static inline std::uint32_t size(frustum_t<scalar_t> const& v)
{
  return v.size();
}

static inline auto default_coherency(std::uint32_t plane_count)
{
  return coherency(plane_count);
}

/// @brief Construct from a transpose(view*projection) matrix
template <typename scalar_t>
static inline frustum_t<scalar_t> make_frustum(mat4_t<scalar_t> const& m)
{
  frustum_t<scalar_t> f;
  f.build(m);
  return f;
}

template <typename scalar_t>
static inline std::span<plane_t<scalar_t> const> get_planes(frustum_t<scalar_t> const& m)
{
  return std::span<plane_t<scalar_t> const>(m.get_all(), m.size());
}

template <typename scalar_t>
static inline std::span<plane_t<scalar_t>> get_planes(frustum_t<scalar_t>& m)
{
  return std::span<plane_t<scalar_t>>(m.get_all(), m.size());
}

template <typename scalar_t>
static inline void set_plane(frustum_t<scalar_t>& f, std::uint32_t i, plane_t<scalar_t> const& p)
{
  f.modify(i, p);
}

} // namespace acl
