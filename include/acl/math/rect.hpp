#pragma once
#include "multi_dim.hpp"
#include "vec2.hpp"

namespace acl
{
namespace detail
{
struct rect_traits
{
  using type        = types::rect_t<float>;
  using ref         = type&;
  using pref        = type const&;
  using cref        = type const&;
  using row_type    = types::vec2_t<float>;
  using row_tag     = vec2;
  using scalar_type = float;

  static constexpr std::uint32_t element_count = 8;
  static constexpr std::uint32_t row_count     = 2;
  static constexpr std::uint32_t column_count  = 2;
};

template <typename traits>
struct rect_base : public multi_dim<traits>
{
  using typename multi_dim<traits>::type;
  using typename multi_dim<traits>::scalar_type;
  using typename multi_dim<traits>::row_type;
  using typename multi_dim<traits>::row_tag;
  using typename multi_dim<traits>::pref;

  static inline type set(scalar_type left, scalar_type top, scalar_type right, scalar_type bottom)
  {
    return {row_type{left, top}, row_type{right, bottom}};
  }
  static inline row_type half_size(pref box)
  {
    return row_tag::half(row_tag::sub(box.r[1], box.r[0]));
  }
  static inline row_type size(pref box)
  {
    return row_tag::sub(box.r[1], box.r[0]);
  }
  static inline row_type center(pref box)
  {
    return row_tag::half(row_tag::add(box.r[1], box.r[0]));
  }
  static inline scalar_type left(pref box)
  {
    return row_tag::x(box.r[0]);
  }
  static inline scalar_type top(pref box)
  {
    return row_tag::y(box.r[0]);
  }
  static inline scalar_type right(pref box)
  {
    return row_tag::x(box.r[1]);
  }
  static inline scalar_type bottom(pref box)
  {
    return row_tag::y(box.r[1]);
  }
  static inline scalar_type width(pref box)
  {
    return row_tag::x(box.r[1]) - row_tag::x(box.r[0]);
  }
  static inline scalar_type height(pref box)
  {
    return row_tag::y(box.r[1]) - row_tag::y(box.r[0]);
  }
};
} // namespace detail

struct rect : public detail::rect_base<detail::rect_traits>
{
  template <typename... Args>
  rect(Args&&... args) : detail::rect_base<detail::rect_traits>(std::forward<Args>(args)...)
  {}
};
} // namespace acl