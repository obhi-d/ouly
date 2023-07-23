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

} // namespace acl
