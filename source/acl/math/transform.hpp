#pragma once

#include "mat4.hpp"
#include "quad.hpp"
#include "quat.hpp"
#include "vec4.hpp"

namespace acl
{
struct transform_t
{
  //! Rotation
  quat_t rotation;
  //! Translation and scale (w = scale)
  vec4_t translation_and_scale;
};

struct transform
{
  static inline void        identity(transform_t& _);
  static inline transform_t identity();
  static inline void        matrix(transform_t const& _, mat4_t& out);
  static inline vec3a_t     translation(transform_t const& _);
  static inline quat_t      rotation(transform_t const& _);
  static inline float       scale(transform_t const& _);
  static inline void        set_translation(transform_t& _, vec3a_t const&);
  static inline void        set_rotation(transform_t& _, quat_t const&);
  static inline void        set_scale(transform_t& _, float);
  static inline transform_t combine(transform_t const& parent_combined, transform_t const& local);
  static inline vec3a_t     mul(vec3a::pref p, transform_t const& _);
};

static_assert(sizeof(transform_t) == sizeof(vec4_t) * 2, "Fix size");

inline void transform::identity(transform_t& _)
{
  _.rotation              = quat::identity();
  _.translation_and_scale = vec4::set(0, 0, 0, 1);
}
inline transform_t transform::identity()
{
  transform_t tmp;
  identity(tmp);
  return tmp;
}
inline void transform::matrix(transform_t const& _, mat4_t& out)
{
  out = mat4::from_scale_rotation_translation(vec4::w(_.translation_and_scale), _.rotation,
                                              vec3a::from_vec4(_.translation_and_scale));
}
inline vec3a_t transform::translation(transform_t const& _)
{
  return vec3a::from_vec4(_.translation_and_scale);
}
inline quat_t transform::rotation(transform_t const& _)
{
  return _.rotation;
}
inline float transform::scale(transform_t const& _)
{
  return vec4::w(_.translation_and_scale);
}
inline void transform::set_translation(transform_t& _, vec3a_t const& v)
{
#if VML_USE_SSE_AVX
  _.translation_and_scale = _mm_or_ps(_mm_and_ps(_.translation_and_scale, clear_xyz()), v);
#else
  _.translation_and_scale = {v[0], v[1], v[2], _.translation_and_scale[3]};
#endif
}
inline void transform::set_rotation(transform_t& _, quat_t const& r)
{
  _.rotation = r;
}
inline void transform::set_scale(transform_t& _, float scale)
{
  _.translation_and_scale = vec4::set_w(_.translation_and_scale, scale);
}
inline transform_t transform::combine(transform_t const& parent_combined, transform_t const& local)
{
  transform_t _;
  _.rotation            = quat::mul(parent_combined.rotation, local.rotation);
  vec3a_t rotated_trans = quat::transform(
    parent_combined.rotation, quad::mul(translation(local), quad::splat_w(parent_combined.translation_and_scale)));

  _.translation_and_scale = quad::mul(quad::add(parent_combined.translation_and_scale, rotated_trans),
                                      quad::set_111w(local.translation_and_scale, 3));
  return _;
}
inline vec3a_t transform::mul(vec3a::pref v, transform_t const& _)
{
  vec3a_t rotated_trans = quat::transform(_.rotation, quad::mul(v, quad::splat_w(_.translation_and_scale)));

  return vec3a::from_vec4(vec4::add(_.translation_and_scale, rotated_trans));
}

} // namespace acl