#pragma once

#include "types.hpp"
#include "types_sse.hpp"
#include "vml_fcn.hpp"

namespace acl
{
#if VML_USE_SSE_AVX
namespace types = acl::sse_types;
#else
namespace types = acl::fallback_types;
#endif

using real_t         = float;
using quad_t         = types::quad_t<float>;
using vec2_t         = types::vec2_t<float>;
using vec3_t         = types::vec3_t<float>;
using vec3a_t        = types::vec3a_t<float>;
using vec4_t         = types::vec4_t<float>;
using plane_t        = types::plane_t<float>;
using quat_t         = types::quat_t<float>;
using axis_angle_t   = types::axis_angle_t<float>;
using polar_coord_t  = types::polar_coord_t<float>;
using euler_angles_t = types::euler_angles_t<float>;
using mat4_t         = types::mat4_t<float>;
using mat3_t         = types::mat3_t<float>;
using rect_t         = types::rect_t<float>;
using aabb_t         = types::aabb_t<float>;
using ivec2_t        = types::vec2_t<int>;
using ivec3_t        = types::vec3_t<int>;
using ivec4_t        = types::vec4_t<int>;
using irect_t        = types::rect_t<int>;
using sphere_t       = types::sphere_t<float>;
} // namespace acl