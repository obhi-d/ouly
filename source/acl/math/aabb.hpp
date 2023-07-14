#pragma once
#include "mat_base.hpp"
#include "vec3a.hpp"

namespace acl
{
template <typename scalar_t>
inline bool is_valid(aabb_t<scalar_t> const& box) noexcept
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return _mm_movemask_epi8(_mm_castps_si128(_mm_cmplt_ps(box.r[1].v, box.r[0].v))) == 0;
  else
  {
    for (int i = 0; i < 3; ++i)
      if (box.r[1].xyzw[i] < box.r[0].xyzw[i])
        return false;
    return true;
  }
}

template <typename scalar_t>
inline vec3a_t<scalar_t> center(aabb_t<scalar_t> const& box) noexcept
{
  return half(add(box.r[1], box.r[0]));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> size(aabb_t<scalar_t> const& box) noexcept
{
  return sub(box.r[1], box.r[0]);
}

template <typename scalar_t>
inline vec3a_t<scalar_t> half_size(aabb_t<scalar_t> const& box) noexcept
{
  return half(sub(box.r[1], box.r[0]));
}

template <typename scalar_t>
inline vec3a_t<scalar_t> corner(aabb_t<scalar_t> const& box, unsigned int i) noexcept
{
  return set(x(box.r[((i >> 2) & 1)]), y(box.r[((i >> 1) & 1)]), z(box.r[((i >> 0) & 1)]));
}

template <typename scalar_t>
inline aabb_t<scalar_t> append(aabb_t<scalar_t> const& b, vec3a_t<scalar_t> const& point) noexcept
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return {_mm_min_ps(b.r[0].v, point.v), _mm_max_ps(b.r[1].v, point.v)};
  else
  {
    aabb_t<scalar_t> box = b;
    for (int i = 0; i < 3; ++i)
    {
      if (box.r[0].xyzw[i] > point.xyzw[i])
        box.r[0].xyzw[i] = point.xyzw[i];
      if (box.r[1].xyzw[i] < point.xyzw[i])
        box.r[1].xyzw[i] = point.xyzw[i];
    }
    return box;
  }
}

template <typename scalar_t>
inline aabb_t<scalar_t> append(aabb_t<scalar_t> const& box, aabb_t<scalar_t> const& other) noexcept
{
  if constexpr (has_sse && std::is_same_v<scalar_t, float>)
    return {_mm_min_ps(box.r[0].v, other.r[0].v), _mm_max_ps(box.r[1].v, other.r[1].v)};
  else
  {
    aabb_t<scalar_t> ret;
    for (int i = 0; i < 3; ++i)
    {
      ret.r[0].xyzw[i] = (box.r[0].xyzw[i] > other.r[0].xyzw[i]) ? other.r[0].xyzw[i] : box.r[0].xyzw[i];
      ret.r[1].xyzw[i] = (box.r[1].xyzw[i] < other.r[1].xyzw[i]) ? other.r[1].xyzw[i] : box.r[1].xyzw[i];
    }
    return ret;
  }
}

template <typename scalar_t>
inline aabb_t<scalar_t> make_aabb_from_center_extends(vec3a_t<scalar_t> const& center,
                                                      vec3a_t<scalar_t> const& extends) noexcept
{
  return aabb_t<scalar_t>{sub(center, extends), add(center, extends)};
}

template <typename scalar_t>
inline aabb_t<scalar_t> make_aabb_from_min_max(vec3a_t<scalar_t> const& i_min, vec3a_t<scalar_t> const& i_max) noexcept
{
  return aabb_t<scalar_t>{i_min, i_max};
}
} // namespace acl
