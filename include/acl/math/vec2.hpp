#pragma once
#include "vec_base.hpp"

namespace acl
{
template <typename scalar_t>
inline vec2_t<scalar_t>& vec2_t<scalar_t>::operator+=(vec2_t<scalar_t> const& s) noexcept
{
  x += s.x;
  y += s.y;
  return *this;
}
template <typename scalar_t>
inline vec2_t<scalar_t>& vec2_t<scalar_t>::operator-=(vec2_t<scalar_t> const& s) noexcept
{
  x -= s.x;
  y -= s.y;
  return *this;
}
template <typename scalar_t>
inline vec2_t<scalar_t>& vec2_t<scalar_t>::operator*=(vec2_t<scalar_t> const& s) noexcept
{
  x *= s.x;
  y *= s.y;
  return *this;
}
template <typename scalar_t>
inline vec2_t<scalar_t>& vec2_t<scalar_t>::operator/=(vec2_t<scalar_t> const& s) noexcept
{
  x /= s.x;
  y /= s.y;
  return *this;
}

template <typename scalar_t>
inline vec2_t<scalar_t>& vec2_t<scalar_t>::operator*=(scalar_t s) noexcept
{
  x *= s;
  y *= s;
  return *this;
}

template <typename scalar_t>
inline vec2_t<scalar_t>& vec2_t<scalar_t>::operator/=(scalar_t s) noexcept
{
  s = 1 / s;
  x *= s;
  y *= s;
  return *this;
}

} // namespace acl
