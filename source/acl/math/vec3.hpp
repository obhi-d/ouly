#pragma once
#include "vec_base.hpp"

namespace acl
{
template <typename scalar_t>
inline vec3_t<scalar_t> cross(vec3_t<scalar_t> const& v1, vec3_t<scalar_t> const& v2)
{
  return set(v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0]);
}
} // namespace acl
