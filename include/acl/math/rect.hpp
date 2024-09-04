#pragma once
#include "multi_dim.hpp"
#include "vec2.hpp"

namespace acl
{
template <typename scalar_t>
static inline auto size(rect_t<scalar_t> const& box)
{
  return sub(box.r[1], box.r[0]);
}

template <typename scalar_t>
static inline auto half_size(rect_t<scalar_t> const& box)
{
  return half(size(box));
}

template <typename scalar_t>
static inline auto center(rect_t<scalar_t> const& box)
{
  return half(add(box.r[1], box.r[0]));
}

template <typename scalar_t>
static inline auto left_top(rect_t<scalar_t> const& box)
{
  return box.r[0];
}

template <typename scalar_t>
static inline auto right_bottom(rect_t<scalar_t> const& box)
{
  return box.r[1];
}

template <typename scalar_t>
static inline auto left(rect_t<scalar_t> const& box)
{
  return box.r[0].x;
}

template <typename scalar_t>
static inline auto top(rect_t<scalar_t> const& box)
{
  return box.r[0].y;
}

template <typename scalar_t>
static inline auto right(rect_t<scalar_t> const& box)
{
  return box.r[1].x;
}

template <typename scalar_t>
static inline auto bottom(rect_t<scalar_t> const& box)
{
  return box.r[1].y;
}

template <typename scalar_t>
static inline auto width(rect_t<scalar_t> const& box)
{
  return box.r[1].x - box.r[0].x;
}

template <typename scalar_t>
static inline auto height(rect_t<scalar_t> const& box)
{
  return box.r[1].y - box.r[0].y;
}

template <typename scalar_t>
inline bool is_valid(rect_t<scalar_t> const& box)
{
  return !vml::greater_any(box.r[0], box.r[1]);
}

template <typename scalar_t>
inline bool is_empty(rect_t<scalar_t> const& box)
{
  return !vml::lesser_all(box.r[0], box.r[1]);
}

template <typename scalar_t>
inline rect_t<scalar_t> union_of(rect_t<scalar_t> const& b, vec2_t<scalar_t> const& point) noexcept
{
  return {vec2_t<scalar_t>(acl::min(b.r[0], point.v)), vec2_t<scalar_t>(acl::max(b.r[1], point.v))};
}

template <typename scalar_t>
inline rect_t<scalar_t> union_of(rect_t<scalar_t> const& box, rect_t<scalar_t> const& other) noexcept
{
  return {acl::min(box.r[0], other.r[0]), acl::max(box.r[1], other.r[1])};
}

template <typename scalar_t>
inline rect_t<scalar_t> intersection_of(rect_t<scalar_t> const& box, rect_t<scalar_t> const& other) noexcept
{
  return rect_t<scalar_t>{acl::max(box.r[0], other.r[0]), acl::min(box.r[1], other.r[1])};
}

template <typename scalar_t>
inline bool is_intersecting(rect_t<scalar_t> const& r, vec2_t<scalar_t> const& point) noexcept
{
  return (point.x >= r.r[0].x && point.x <= r.r[1].x && point.y >= r.r[0].y && point.y <= r.r[1].y);
}

template <typename scalar_t>
inline bool is_intersecting(rect_t<scalar_t> const& r1, rect_t<scalar_t> const& r2) noexcept
{
  return r1.r[0].x <= r2.r[1].x && r1.r[1].x >= r2.r[0].x && r1.r[0].y <= r2.r[1].y && r1.r[1].y >= r2.r[0].y;
}

template <typename scalar_t>
inline auto area(rect_t<scalar_t> const& r)
{
  return width(r) * height(r);
}

template <typename scalar_t>
inline rect_t<scalar_t>& rect_t<scalar_t>::operator+=(rect_t<scalar_t> const& s) noexcept
{
  return (*this = intersection_of(*this, s));
}

template <typename scalar_t>
inline rect_t<scalar_t>& rect_t<scalar_t>::operator+=(vec2_t<scalar_t> const& s) noexcept
{
  r[0] += s;
  r[1] += s;
  return *this;
}
template <typename scalar_t>
inline rect_t<scalar_t>& rect_t<scalar_t>::operator-=(vec2_t<scalar_t> const& s) noexcept
{
  r[0] -= s;
  r[1] -= s;
  return *this;
}
template <typename scalar_t>
inline rect_t<scalar_t>& rect_t<scalar_t>::operator*=(scalar_t s) noexcept
{
  r[0] *= s;
  r[1] *= s;
  return *this;
}

template <typename scalar_t>
inline rect_t<scalar_t>& rect_t<scalar_t>::operator/=(scalar_t s) noexcept
{
  s = 1 / s;
  r[0] *= s;
  r[1] *= s;
  return *this;
}

} // namespace acl
