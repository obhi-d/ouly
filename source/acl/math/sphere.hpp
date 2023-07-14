#pragma once
#include "vec3a.hpp"

namespace acl
{

template <typename scalar_t>
inline sphere_t<scalar_t> make_sphere(vec3a_t<scalar_t> const& v, float radius) noexcept
{
  return sphere_t<scalar_t>{set_w(v, radius).v};
}

template <typename scalar_t>
inline scalar_t radius(sphere_t<scalar_t> const& v) noexcept
{
  return w(v);
}

template <typename scalar_t>
inline quad_t<scalar_t> vradius(sphere_t<scalar_t> const& v) noexcept
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return quad_t<scalar_t>(_mm_shuffle_ps(v.v, v.v, _MM_SHUFFLE(0, 0, 0, 3)));
  else
    return quad_t<scalar_t>{w(v), 0, 0, 0};
}

template <typename scalar_t>
inline vec3a_t<scalar_t> center(sphere_t<scalar_t> const& v) noexcept
{
  return make_vec3a(v);
}

template <typename scalar_t>
inline scalar_t max_radius(vec3a_t<scalar_t> const& v) noexcept
{
  return length(v);
}

template <typename scalar_t>
inline sphere_t<scalar_t> scale_radius(vec3a_t<scalar_t> const& p, scalar_t scale) noexcept
{
  return sphere_t<scalar_t>(mul(p, vec3a_t<scalar_t>(1.0f, 1.0f, 1.0f, scale)));
}
} // namespace acl
