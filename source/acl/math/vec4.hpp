#pragma once

#include "quad.hpp"

namespace acl
{

template <typename scalar_t>
inline vec4_t<scalar_t> homogonize(vec4_t<scalar_t> const& v) noexcept
{
  return vml::div(v.v, vml::splat_w(v));
}

template <typename scalar_t>
inline vec4_t<scalar_t> normalize(vec4_t<scalar_t> const& v) noexcept
{
  return vml::normalize(v.v);
}

} // namespace acl
