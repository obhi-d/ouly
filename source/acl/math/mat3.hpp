#pragma once
#include "deduced_types.hpp"
#include "mat_base.hpp"
#include "quat.hpp"

namespace acl
{

template <typename scalar_t>
inline bool equals(mat3_t<scalar_t> const& a, mat3_t<scalar_t> const& b) noexcept
{
  return equals(a[0], b[0]) && equals(a[1], b[1]) && equals(a[2], b[2]);
}

template <typename scalar_t>
inline mat3_t<scalar_t> transpose(mat3_t<scalar_t> const& m) noexcept
{
  return vml::transpose(m.v);
}

template <typename scalar_t>
inline mat3_t<scalar_t> make_mat3(quat_t<scalar_t> rot) noexcept
{
  mat3_t<scalar_t> ret{noinit_v};
  set_rotation(ret, rot);
  return ret;
}

template <typename scalar_t>
inline auto operator+(mat3_t<scalar_t> const& a, mat3_t<scalar_t> const& b) noexcept
{
  mat3_t<scalar_t> r{noinit_v};
  for (int i = 0; i < 3; ++i)
    r.r[i] = a.r[i] + b.r[i];
  return r;
}

template <typename scalar_t>
inline auto operator-(mat3_t<scalar_t> const& a, mat3_t<scalar_t> const& b) noexcept
{
  mat3_t<scalar_t> r{noinit_v};
  for (int i = 0; i < 3; ++i)
    r.r[i] = a.r[i] - b.r[i];
  return r;
}

template <FloatingType scalar_t, ScalarType mult_t>
inline auto operator*(mat3_t<scalar_t> const& a, mult_t b) noexcept
{
  mat3_t<scalar_t> r{noinit_v};
  vec4_t<scalar_t> m{static_cast<scalar_t>(b)};
  for (int i = 0; i < 3; ++i)
    r.r[i] = a.r[i] * m;
  return r;
}

template <FloatingType scalar_t, ScalarType mult_t>
inline auto operator*(mult_t a, mat3_t<scalar_t> const& b) noexcept
{
  return b * a;
}

template <typename scalar_t>
inline bool test_orthogonal(mat3_t<scalar_t> const& m) noexcept
{
  vec3a_t<scalar_t> x = make_vec3a(m.r[0]);
  vec3a_t<scalar_t> y = make_vec3a(m.r[0]);
  vec3a_t<scalar_t> z = make_vec3a(m.r[0]);

  return equals(sqlength(cross(x, y)), static_cast<scalar_t>(0)) &&
         equals(sqlength(cross(y, z)), static_cast<scalar_t>(0)) &&
         equals(sqlength(cross(z, x)), static_cast<scalar_t>(0));
}

} // namespace acl
