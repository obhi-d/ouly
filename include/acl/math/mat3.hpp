#pragma once
#include "deduced_types.hpp"
#include "mat_base.hpp"
#include "quat.hpp"

namespace acl
{
namespace detail
{
struct mat3_traits
{
  using type        = acl::types::mat3_t<float>;
  using ref         = type&;
  using pref        = type const&;
  using cref        = type const&;
  using row_type    = acl::types::vec4_t<float>;
  using row_tag     = vec4;
  using scalar_type = float;

  static constexpr std::uint32_t element_count = 12;
  static constexpr std::uint32_t row_count     = 3;
  static constexpr std::uint32_t column_count  = 4;
};
} // namespace detail

struct mat3 : public mat_base<detail::mat3_traits>
{

  using typename mat_base<detail::mat3_traits>::type;
  using typename mat_base<detail::mat3_traits>::pref;

  static inline type transpose(pref m);
  // @brief Create a rotation matrix from quaternion
  static inline type from_quat(quat::pref rot);
  static inline type from_rotation(quat::pref rot);
};

inline mat3::type mat3::transpose(pref m)
{
#if VML_USE_SSE_AVX
  mat3_t ret;
  //    std::swap(m.e[0][1], m.e[1][0]);
  //    std::swap(m.e[0][2], m.e[2][0]);
  //    std::swap(m.e[1][2], m.e[2][1]);
  ret.r[0] = _mm_move_ss(_mm_shuffle_ps(m.r[1], m.r[2], _MM_SHUFFLE(3, 0, 0, 3)), m.r[0]);
  ret.r[1] = _mm_shuffle_ps(_mm_shuffle_ps(m.r[0], m.r[1], _MM_SHUFFLE(3, 1, 3, 1)), m.r[2], _MM_SHUFFLE(3, 1, 2, 0));
  ret.r[2] = _mm_shuffle_ps(_mm_shuffle_ps(m.r[0], m.r[1], _MM_SHUFFLE(3, 2, 3, 2)), m.r[2], _MM_SHUFFLE(3, 2, 2, 0));
  return ret;

#else
  mat3_t ret = m;
  std::swap(ret.e[0][1], ret.e[1][0]);
  std::swap(ret.e[1][2], ret.e[2][1]);
  std::swap(ret.e[0][2], ret.e[2][0]);
  return ret;
#endif
}

inline mat3::type mat3::from_quat(quat::pref rot)
{
  type ret;
  set_rotation(ret, rot);
  return ret;
}

inline mat3::type mat3::from_rotation(quat::pref rot)
{
  return from_quat(rot);
}

inline auto operator+(mat3_t const& a, mat3_t const& b) noexcept
{
  return mat3::add(a, b);
}

inline auto operator-(mat3_t const& a, mat3_t const& b) noexcept
{
  return mat3::sub(a, b);
}

inline auto operator*(mat3_t const& a, mat3::scalar_type b) noexcept
{
  return mat3::mul(a, b);
}

inline auto operator*(mat3::scalar_type a, mat3_t const& b) noexcept
{
  return mat3::mul(a, b);
}

} // namespace acl