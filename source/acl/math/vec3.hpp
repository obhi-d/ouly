#pragma once
#include "vec_base.hpp"

namespace acl
{
template <typename scalar_t>
inline vec3_t<scalar_t> cross(vec3_t<scalar_t> const& v1, vec3_t<scalar_t> const& v2) noexcept
{
  return vec3_t<scalar_t>(v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0]);
}

template <typename scalar_t>
inline vec3_t<scalar_t>& vec3_t<scalar_t>::operator+=(vec3_t<scalar_t> const& s) noexcept
{
  x += s.x;
  y += s.y;
  z += s.z;
  return *this;
}
template <typename scalar_t>
inline vec3_t<scalar_t>& vec3_t<scalar_t>::operator-=(vec3_t<scalar_t> const& s) noexcept
{
  x -= s.x;
  y -= s.y;
  z -= s.z;
  return *this;
}
template <typename scalar_t>
inline vec3_t<scalar_t>& vec3_t<scalar_t>::operator*=(vec3_t<scalar_t> const& s) noexcept
{
  x *= s.x;
  y *= s.y;
  z *= s.z;
  return *this;
}
template <typename scalar_t>
inline vec3_t<scalar_t>& vec3_t<scalar_t>::operator/=(vec3_t<scalar_t> const& s) noexcept
{
  x /= s.x;
  y /= s.y;
  z /= s.z;
  return *this;
}

template <typename scalar_t>
inline vec3_t<scalar_t>& vec3_t<scalar_t>::operator*=(scalar_t s) noexcept
{
  x *= s;
  y *= s;
  z *= s;
  return *this;
}

template <typename scalar_t>
inline vec3_t<scalar_t>& vec3_t<scalar_t>::operator/=(scalar_t s) noexcept
{
  s = 1 / s;
  x *= s;
  y *= s;
  z *= s;
  return *this;
}

} // namespace acl
