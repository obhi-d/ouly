#pragma once

#include "types.hpp"
#include "vml_fcn.hpp"

namespace acl
{

using real            = float;
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
using ivec2           = vec2_t<int>;
using ivec3           = vec3_t<int>;
using ivec4           = vec4_t<int>;
using irect           = rect_t<int>;
using sphere          = sphere_t<float>;
using bounds_info     = bounds_info_t<float>;
using bounding_volume = bounding_volume_t<float>;
using transform       = transform_t<float>;
using frustum         = frustum_t<float>;
using color           = color_t<float>;
using rgba8           = color_t<uint8_t>;
using extends         = extends_t<float>;

using real_d            = double;
using vec2_d            = vec2_t<double>;
using vec3_d            = vec3_t<double>;
using vec3a_d           = vec3a_t<double>;
using vec4_d            = vec4_t<double>;
using plane_d           = plane_t<double>;
using quat_d            = quat_t<double>;
using axis_angle_d      = axis_angle_t<double>;
using polar_coord_d     = polar_coord_t<double>;
using euler_angles_d    = euler_angles_t<double>;
using mat4_d            = mat4_t<double>;
using mat3_d            = mat3_t<double>;
using rect_d            = rect_t<double>;
using aabb_d            = aabb_t<double>;
using ivec2_d           = vec2_t<double>;
using ivec3_d           = vec3_t<double>;
using ivec4_d           = vec4_t<double>;
using irect_d           = rect_t<double>;
using sphere_d          = sphere_t<double>;
using bounds_info_d     = bounds_info_t<double>;
using bounding_volume_d = bounding_volume_t<double>;
using transform_d       = transform_t<double>;
using frustum_d         = frustum_t<double>;
using extends_d         = extends_t<double>;

using vec2_i  = vec2_t<int>;
using vec3_i  = vec3_t<int>;
using vec3a_i = vec3a_t<int>;
using vec4_i  = vec4_t<int>;

using vec2_s  = vec2_t<int16_t>;
using vec3_s  = vec3_t<int16_t>;
using vec3a_s = vec3a_t<int16_t>;
using vec4_s  = vec4_t<int16_t>;

} // namespace acl
