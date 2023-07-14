#pragma once

#include "mat4.hpp"
#include "quad.hpp"
#include "quat.hpp"
#include "vec4.hpp"

namespace acl
{
template <typename scalar_t>
inline void set_identity_transform(transform_t<scalar_t>& t) noexcept
{
    t = transform_t<scalar_t>();
}

template <typename scalar_t>
inline void make_mat4(transform_t<scalar_t> const& t) noexcept
{
  return make_mat4(t.translation_and_scale.w, t.rotation, make_vec3a(t.translation_and_scale));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> translation(transform_t<scalar_t> const& t) noexcept
{
  return make_vec3a(t.translation_and_scale);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> rotation(transform_t<scalar_t> const& t) noexcept
{
  return t.rotation;
}

template <typename scalar_t>
inline scalar_t scale(transform_t<scalar_t> const& t) noexcept
{
  return t.translation_and_scale.w;
}

template <typename scalar_t>
inline void set_translation(transform_t<scalar_t>& t, vec3a_t<scalar_t> const& v) noexcept
{
  if constexpr (has_sse)
  t.translation_and_scale = _mm_or_ps(_mm_and_ps(t.translation_and_scale, clear_xyz()), v);
else
  t.translation_and_scale = {v[0], v[1], v[2], t.translation_and_scale[3]};

}

template <typename scalar_t>
inline void set_rotation(transform_t<scalar_t>& t, quat_t<scalar_t> const& v) noexcept
{
  t.rotation = v;
}

template <typename scalar_t>
inline void set_scale(transform_t<scalar_t>& t, scalar_t v) noexcept
{
  t.translation_and_scale.w = v;
}

template <typename scalar_t>
inline auto concat(transform_t<scalar_t> const& parent_combined, transform_t<scalar_t> const& local) noexcept
{
  auto ret = transform_t<scalar_t>( mul(parent_combined.rotation, local.rotation),
                        rotate(mul(translation(local), splat_w(parent_combined.translation_and_scale)), parent_combined.rotation));

  ret.translation_and_scale = mul(add(parent_combined.translation_and_scale, ret.translation_and_scale),
                                      set_111w(local.translation_and_scale, 3));
  return ret;
}

template <typename scalar_t>
inline vec3a_t<scalar_t> mul(vec3a_t<scalar_t> const& v, transform_t<scalar_t> const& t)
{
  auto rotated_trans = mul(mul(v, splat_w(t.translation_and_scale)), t.rotation);
  return make_vec3a(add(t.translation_and_scale, rotated_trans));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> operator*(vec3a_t<scalar_t> const& v, transform_t<scalar_t> const& t)
{
  return mul(v, t);
}

template <typename scalar_t>
inline auto operator*(transform_t<scalar_t> const& parent_combined, transform_t<scalar_t> const& local) noexcept
{
  return concat(parent_combined, local);
}

} // namespace acl
