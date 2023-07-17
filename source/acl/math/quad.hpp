#pragma once

#include "vml_sse.hpp"

namespace acl
{

template <ScalarType scalar_t, typename tag_t>
inline bool isnan(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return vml::isnan(v.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> isnanv(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return vml::isnanv(v.v);
}

template <ScalarType scalar_t, typename tag_t>
inline bool isinf(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return vml::isinf(v.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> isinfv(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return vml::isinfv(v.v);
}

template <ScalarType scalar_t, typename tag_t>
inline scalar_t get_x(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return v.x;
}

template <ScalarType scalar_t, typename tag_t>
inline scalar_t get_y(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return v.y;
}

template <ScalarType scalar_t, typename tag_t>
inline scalar_t get_z(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return v.z;
}

template <ScalarType scalar_t, typename tag_t>
inline scalar_t get_w(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return v.w;
}

template <ScalarType scalar_t, typename tag_t, ScalarType scalar2_t>
inline quad_t<scalar_t, tag_t> set_x(quad_t<scalar_t, tag_t> const& v, scalar2_t val) noexcept
{
  return quad_t<scalar_t, tag_t>{static_cast<scalar_t>(val), v.y, v.z, v.w};
}

template <ScalarType scalar_t, typename tag_t, ScalarType scalar2_t>
inline quad_t<scalar_t, tag_t> set_y(quad_t<scalar_t, tag_t> const& v, scalar2_t val) noexcept
{
  return quad_t<scalar_t, tag_t>{v.x, static_cast<scalar_t>(val), v.z, v.w};
}

template <ScalarType scalar_t, typename tag_t, ScalarType scalar2_t>
inline quad_t<scalar_t, tag_t> set_z(quad_t<scalar_t, tag_t> const& v, scalar2_t val) noexcept
{
  return quad_t<scalar_t, tag_t>{v.x, v.y, static_cast<scalar_t>(val), v.w};
}

template <ScalarType scalar_t, typename tag_t, ScalarType scalar2_t>
inline quad_t<scalar_t, tag_t> set_w(quad_t<scalar_t, tag_t> const& v, scalar2_t val) noexcept
{
  return quad_t<scalar_t, tag_t>{v.x, v.y, v.z, static_cast<scalar_t>(val)};
}

template <ScalarType scalar_t, typename tag_t>
inline bool equals(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::equals(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline bool greater_all(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::greater_all(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline bool greater_any(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::greater_any(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline bool lesser_all(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::lesser_all(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline bool lesser_any(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::lesser_any(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> abs(quad_t<scalar_t, tag_t> const& v) noexcept
{
  return vml::abs(v.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> min(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::min(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> max(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::max(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> quad_t<scalar_t, tag_t>::operator!() noexcept
{
  return vml::conjugate_quat(v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> quad_t<scalar_t, tag_t>::operator-() noexcept
{
  return vml::negate(v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t>& quad_t<scalar_t, tag_t>::operator+=(quad_t<scalar_t, tag_t> const& s) noexcept
{
  v = vml::add(v, s.v);
  return *this;
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t>& quad_t<scalar_t, tag_t>::operator-=(quad_t<scalar_t, tag_t> const& s) noexcept
{
  v = vml::sub(v, s.v);
  return *this;
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t>& quad_t<scalar_t, tag_t>::operator*=(quad_t<scalar_t, tag_t> const& s) noexcept
{
  if constexpr (std::is_same_v<tag, quaternion_tag>)
    v = vml::mul_quat(v, s.v);
  else
    v = vml::mul(v, s.v);
  return *this;
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t>& quad_t<scalar_t, tag_t>::operator/=(quad_t<scalar_t, tag_t> const& s) noexcept
{
  v = vml::div(v, s.v);
  return *this;
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t>& quad_t<scalar_t, tag_t>::operator*=(scalar_t s) noexcept
{
  v = vml::mul_quad_scalar(v, s);
  return *this;
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t>& quad_t<scalar_t, tag_t>::operator/=(scalar_t s) noexcept
{
  v = vml::mul_quad_scalar(v, 1 / s);
  return *this;
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> operator*(quad_t<scalar_t, tag_t> const& a, scalar_t b) noexcept
{
  return vml::mul_quad_scalar(a.v, b);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> operator*(scalar_t a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::mul_quad_scalar(b.v, a);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> operator*(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::mul(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> operator/(quad_t<scalar_t, tag_t> const& a, scalar_t b) noexcept
{
  return vml::mul_quad_scalar(a, 1 / b);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> operator/(scalar_t b, quad_t<scalar_t, tag_t> const& a) noexcept
{
  return vml::mul_quad_scalar(b.v, vml::set<scalar_t>(1 / a));
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> operator/(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::div(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> operator+(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::add(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> operator-(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::sub(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline quad_t<scalar_t, tag_t> madd(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b,
                                    quad_t<scalar_t, tag_t> const& c) noexcept
{
  return vml::madd(a.v, b.v, c.v);
}

template <ScalarType scalar_t, typename tag_t, FloatingType blend_t>
inline quad_t<scalar_t, tag_t> lerp(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b,
                                    blend_t t) noexcept
{
  return vml::lerp(a.v, b.v, static_cast<scalar_t>(t));
}

template <ScalarType scalar_t, typename tag_t>
inline scalar_t distance(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::distance(a.v, b.v);
}

template <ScalarType scalar_t, typename tag_t>
inline scalar_t length(quad_t<scalar_t, tag_t> const& vec) noexcept
{
  return vml::length(vec.v);
}

template <ScalarType scalar_t, typename tag_t>
inline scalar_t sqlength(quad_t<scalar_t, tag_t> const& vec) noexcept
{
  return vml::sqlength(vec.v);
}

template <ScalarType scalar_t, typename tag_t>
inline scalar_t sqdistance(quad_t<scalar_t, tag_t> const& a, quad_t<scalar_t, tag_t> const& b) noexcept
{
  return vml::sqdistance(a.v, b.v);
}
} // namespace acl
