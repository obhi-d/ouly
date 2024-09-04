#pragma once

#include "types.hpp"
#include "vml_fcn.hpp"

namespace acl
{

using vec2            = vec2_t<float>;
using vec3            = vec3_t<float>;
using vec3a           = vec3a_t<float>;
using vec4            = vec4_t<float>;
using plane           = plane_t<float>;
using quat            = quat_t<float>;
using axis_angle      = axis_angle_t<float>;
using polar_coord     = polar_coord_t<float>;
using euler_angles    = euler_angles_t<float>;
using mat4            = mat4_t<float>;
using mat3            = mat3_t<float>;
using rect            = rect_t<float>;
using aabb            = aabb_t<float>;
using uvec2           = vec2_t<uint32_t>;
using uvec3           = vec3_t<uint32_t>;
using ivec2           = vec2_t<int>;
using ivec3           = vec3_t<int>;
using ivec3a          = vec3a_t<int>;
using ivec4           = vec4_t<int>;
using irect           = rect_t<int>;
using sphere          = sphere_t<float>;
using bounds_info     = bounds_info_t<float>;
using bounding_volume = bounding_volume_t<float>;
using transform       = transform_t<float>;
using fixed_frustum   = fixed_frustum_t<float>;
using frustum         = frustum_t<float>;
using color           = color_t<float>;
using rgba8           = color_t<uint8_t>;
using extends3d       = extends3d_t<float>;

#ifdef ACL_DOUBLE_PRECISION
using real = float;
#else
using real = double;
#endif

using f64vec2            = vec2_t<double>;
using f64vec3            = vec3_t<double>;
using f64vec3a           = vec3a_t<double>;
using f64vec4            = vec4_t<double>;
using f64plane           = plane_t<double>;
using f64quat            = quat_t<double>;
using f64axis_angle      = axis_angle_t<double>;
using f64polar_coord     = polar_coord_t<double>;
using f64euler_angles    = euler_angles_t<double>;
using f64mat4            = mat4_t<double>;
using f64mat3            = mat3_t<double>;
using f64rect            = rect_t<double>;
using f64aabb            = aabb_t<double>;
using f64sphere          = sphere_t<double>;
using f64bounds_info     = bounds_info_t<double>;
using f64bounding_volume = bounding_volume_t<double>;
using f64transform       = transform_t<double>;
using f64fixed_frustum   = fixed_frustum_t<double>;
using f64frustum         = frustum_t<double>;
using f64extends3d       = extends3d_t<double>;

using u16vec2  = vec2_t<int16_t>;
using u16vec3  = vec3_t<int16_t>;
using u16vec3a = vec3a_t<int16_t>;
using u16vec4  = vec4_t<int16_t>;

using u16extends2d = extends2d_t<uint16_t>;
using u16rect      = rect_t<int16_t>;
} // namespace acl
