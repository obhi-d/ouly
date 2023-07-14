#pragma once
#include "deduced_types.hpp"
#include "mat_base.hpp"
#include "quat.hpp"

namespace acl
{

template <typename scalar_t>
inline mat3_t<scalar_t> transpose(mat3_t<scalar_t> m)
{
  if constexpr (has_sse && std::is_same_v<float, scalar_t>)
  {
    mat3_t<scalar_t> ret{noinit_v};
    ret.r[0].v = _mm_move_ss(_mm_shuffle_ps(m.r[1].v, m.r[2].v, _MM_SHUFFLE(3, 0, 0, 3)), m.r[0].v);
    ret.r[1].v =
      _mm_shuffle_ps(_mm_shuffle_ps(m.r[0].v, m.r[1].v, _MM_SHUFFLE(3, 1, 3, 1)), m.r[2].v, _MM_SHUFFLE(3, 1, 2, 0));
    ret.r[2].v =
      _mm_shuffle_ps(_mm_shuffle_ps(m.r[0].v, m.r[1].v, _MM_SHUFFLE(3, 2, 3, 2)), m.r[2].v, _MM_SHUFFLE(3, 2, 2, 0));
    return ret;
  }
  else
  {
    mat3_t ret = m;
    std::swap(ret.e[0][1], ret.e[1][0]);
    std::swap(ret.e[1][2], ret.e[2][1]);
    std::swap(ret.e[0][2], ret.e[2][0]);
    return ret;
  }
}

template <typename scalar_t>
inline mat3_t<scalar_t> make_mat3(quat_t<scalar_t> rot)
{
  mat3_t<scalar_t> ret{noinit_v};
  set_rotation(ret, rot);
  return ret;
}

template <typename scalar_t>
inline auto operator+(mat3_t<scalar_t> const& a, mat3_t<scalar_t> const& b) noexcept
{
  return add(a, b);
}

template <typename scalar_t>
inline auto operator-(mat3_t<scalar_t> const& a, mat3_t<scalar_t> const& b) noexcept
{
  return sub(a, b);
}

template <typename scalar_t>
inline auto operator*(mat3_t<scalar_t> const& a, scalar_t b) noexcept
{
  return mul(a, b);
}

template <typename scalar_t>
inline auto operator*(scalar_t a, mat3_t<scalar_t> const& b) noexcept
{
  return mul(a, b);
}

} // namespace acl
