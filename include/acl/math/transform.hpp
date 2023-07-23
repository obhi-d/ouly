#pragma once

#include "mat4.hpp"
#include "quad.hpp"
#include "quat.hpp"
#include "vec4.hpp"

namespace acl
{
template <ScalarType scalar_t>
inline void set_identity_transform(transform_t<scalar_t>& t) noexcept
{
  t = transform_t<scalar_t>();
}

template <ScalarType scalar_t>
inline auto make_mat4(transform_t<scalar_t> const& t) noexcept
{
  return make_mat4(t.translation_and_scale.w, t.rotation, make_vec3a(t.translation_and_scale));
}

template <ScalarType scalar_t>
inline auto make_transform(mat4_t<scalar_t> const& m) noexcept
{

  scalar_t         scale  = length(m[0]);
  scalar_t         rscale = static_cast<scalar_t>(1) / scale;
  mat3_t<scalar_t> r{m[0] * rscale, m[1] * rscale, m[2] * rscale};

  return transform_t<scalar_t>(make_quaternion(r), m[3], scale);
}

/// @brief Returns maximum scaling
template <ScalarType scalar_t>
inline bool equals(transform_t<scalar_t> const& a, transform_t<scalar_t> const& b) noexcept
{
  return equals(a.rotation, b.rotation) && equals(a.translation_and_scale, b.translation_and_scale);
}

template <ScalarType scalar_t>
inline vec3a_t<scalar_t> translation(transform_t<scalar_t> const& t) noexcept
{
  return make_vec3a(t.translation_and_scale);
}

template <ScalarType scalar_t>
inline quat_t<scalar_t> rotation(transform_t<scalar_t> const& t) noexcept
{
  return t.rotation;
}

template <ScalarType scalar_t>
inline scalar_t scale(transform_t<scalar_t> const& t) noexcept
{
  return t.translation_and_scale.w;
}

template <ScalarType scalar_t>
inline void set_translation(transform_t<scalar_t>& t, vec3a_t<scalar_t> const& v) noexcept
{
  t.translation_and_scale = vec4_t<scalar_t>{v[0], v[1], v[2], t.translation_and_scale[3]};
}

template <ScalarType scalar_t>
inline void set_rotation(transform_t<scalar_t>& t, quat_t<scalar_t> const& v) noexcept
{
  t.rotation = v;
}

template <ScalarType scalar_t, ScalarType mult_t>
inline void set_scale(transform_t<scalar_t>& t, mult_t v) noexcept
{
  t.translation_and_scale.w = static_cast<scalar_t>(v);
}

template <ScalarType scalar_t>
inline vec3a_t<scalar_t> operator*(vec3a_t<scalar_t> const& v, transform_t<scalar_t> const& t) noexcept
{
  auto rotated_trans = vml::mul_vec3a_quat(vml::mul(v.v, vml::splat_w(t.translation_and_scale.v)), t.rotation.v);
  return vml::clear_w(vml::add(t.translation_and_scale.v, rotated_trans));
}

template <ScalarType scalar_t>
inline auto operator*(transform_t<scalar_t> const& parent_combined, transform_t<scalar_t> const& local) noexcept
{
  auto r   = (parent_combined.rotation * local.rotation);
  auto ret = transform_t<scalar_t>(
    r, vml::mul_vec3a_quat(
         vml::clear_w(vml::mul(local.translation_and_scale.v, vml::splat_w(parent_combined.translation_and_scale.v))),
         parent_combined.rotation.v));

  ret.translation_and_scale = vml::mul(vml::add(parent_combined.translation_and_scale.v, ret.translation_and_scale.v),
                                       vml::set_111w(local.translation_and_scale.v, 3));
  return ret;
}

} // namespace acl
