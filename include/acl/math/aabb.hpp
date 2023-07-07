#pragma once
#include "mat_base.hpp"
#include "vec3a.hpp"

namespace acl
{
namespace detail
{
struct aabb_traits
{
  using type        = types::aabb_t<float>;
  using ref         = type&;
  using pref        = type const&;
  using cref        = type const&;
  using scalar_type = float;
  using row_type    = types::vec3a_t<scalar_type>;
  using row_tag     = vec3a;

  static constexpr std::uint32_t element_count = 8;
  static constexpr std::uint32_t row_count     = 2;
  static constexpr std::uint32_t column_count  = 4;
};
} // namespace detail

struct aabb : public mat_base<detail::aabb_traits>
{
  //! Returns true if AABB is valid
  static inline bool is_valid(pref box);
  //! Returns the AABB center
  static inline vec3a_t center(pref box);
  //! Returns the AABB full-size
  static inline vec3a_t size(pref box);
  //! Returns the AABB half-size
  static inline vec3a_t half_size(pref box);
  //! Returns an AABB corner point, i must be between (0, 8]
  static inline vec3a_t corner(pref box, unsigned int i);
  //! Append a point to an existing box to return a new box
  static inline type append(pref box, vec3a::pref point);
  //! Append a box to an existing box to return a new box
  static inline type append(pref box, pref other);
  //! Set an AABB using center and half-size
  static inline type set(vec3a::pref center, vec3a::pref extends);
  //! Set min and max point for AABB
  static inline type set_min_max(vec3a::pref i_min, vec3a::pref i_max);
};

inline bool acl::aabb::is_valid(pref box)
{
#if VML_USE_SSE_AVX
  return _mm_movemask_epi8(_mm_castps_si128(_mm_cmplt_ps(box.r[1], box.r[0]))) == 0;
#else
  for (int i = 0; i < 3; ++i)
    if (box.r[1][i] < box.r[0][i])
      return false;
  return true;
#endif
}
inline vec3a_t aabb::center(pref box)
{
  return vec3a::half(vec3a::add(box.r[1], box.r[0]));
}
inline vec3a_t aabb::size(pref box)
{
  return vec3a::sub(box.r[1], box.r[0]);
}
inline vec3a_t aabb::half_size(pref box)
{
  return vec3a::half(vec3a::sub(box.r[1], box.r[0]));
}
inline vec3a_t aabb::corner(pref box, unsigned int i)
{
  return vec3a::set(vec3a::x(box.r[((i >> 2) & 1)]), vec3a::y(box.r[((i >> 1) & 1)]), vec3a::z(box.r[((i >> 0) & 1)]));
}
inline aabb::type aabb::append(pref b, vec3a::pref point)
{
#if VML_USE_SSE_AVX
  return {_mm_min_ps(b.r[0], point), _mm_max_ps(b.r[1], point)};
#else
  aabb_t box = b;
  for (int i = 0; i < 3; ++i)
  {
    if (box.r[0][i] > point[i])
      box.r[0][i] = point[i];
    if (box.r[1][i] < point[i])
      box.r[1][i] = point[i];
  }
  return box;
#endif
}
inline aabb::type aabb::append(pref box, pref other)
{
#if VML_USE_SSE_AVX
  return {_mm_min_ps(box.r[0], other.r[0]), _mm_max_ps(box.r[1], other.r[1])};
#else
  aabb_t ret;
  for (int i = 0; i < 3; ++i)
  {
    ret.r[0][i] = (box.r[0][i] > other.r[0][i]) ? other.r[0][i] : box.r[0][i];
    ret.r[1][i] = (box.r[1][i] < other.r[1][i]) ? other.r[1][i] : box.r[1][i];
  }
  return ret;
#endif
}
inline aabb::type aabb::set(vec3a::pref center, vec3a::pref extends)
{
  return {vec3a::sub(center, extends), vec3a::add(center, extends)};
}
inline aabb::type aabb::set_min_max(vec3a::pref i_min, vec3a::pref i_max)
{
  return {i_min, i_max};
}
} // namespace acl