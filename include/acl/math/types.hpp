#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

namespace acl::fallback_types
{

template <typename scalar_t = float>
using quad_t = std::array<scalar_t, 4>;
template <typename scalar_t>
using vec2_t = std::array<scalar_t, 2>;
template <typename scalar_t>
using vec3_t = std::array<scalar_t, 3>;
template <typename scalar_t>
using vec3a_t = quad_t<scalar_t>;
template <typename scalar_t>
using vec4_t = quad_t<scalar_t>;
template <typename scalar_t>
using plane_t = quad_t<scalar_t>;
template <typename scalar_t>
using quat_t = quad_t<scalar_t>;
template <typename scalar_t>
using sphere_t = quad_t<scalar_t>;
template <typename scalar_t>
using axis_angle_t = quad_t<scalar_t>;
template <typename scalar_t>
using polar_coord_t = vec2_t<scalar_t>;
template <typename scalar_t>
using euler_angles_t = vec3_t<scalar_t>;

template <typename scalar_t>
struct rect_t
{
  union
  {
    vec2_t<scalar_t>                r[2];
    std::array<vec2_t<scalar_t>, 2> m;
    scalar_t                        e[2][2];
  };

  rect_t(){};
  template <typename... RowType>
  rect_t(RowType... args) : m{args...}
  {}

  inline constexpr auto operator<=>(rect_t const& other) const noexcept
  {
    return m <=> other.m;
  }
};

template <typename scalar_t>
struct aabb_t
{
  union
  {
    vec3a_t<scalar_t>                r[2];
    std::array<vec3a_t<scalar_t>, 2> m;
    scalar_t                         e[2][4];
  };
  aabb_t(){};
  template <typename... RowType>
  aabb_t(RowType... args) : m{args...}
  {}

  inline constexpr auto operator<=>(aabb_t const& other) const noexcept
  {
    return m <=> other.m;
  }
};

template <typename scalar_t>
struct mat4_t
{
  //! @note This is possibly non isoc++, but knowing it will work for all
  //! compilers is why I am putting it here
  union
  {
    vec4_t<scalar_t>         r[4];
    std::array<scalar_t, 16> m;
    scalar_t                 e[4][4];
  };

  mat4_t(){};
  template <typename... ScalarType>
  mat4_t(ScalarType... args) : m{static_cast<scalar_t>(args)...}
  {}

  inline constexpr auto operator<=>(mat4_t const& other) const noexcept
  {
    return m <=> other.m;
  }
};

template <typename scalar_t>
struct mat3_t
{
  //! @note This is possibly non isoc++, but knowing it will work for all
  //! compilers is why I am putting it here
  union
  {
    vec4_t<scalar_t>         r[3];
    std::array<scalar_t, 12> m;
    scalar_t                 e[3][4];
  };
  mat3_t(){};
  template <typename... ScalarType>
  mat3_t(ScalarType... args) : m{static_cast<scalar_t>(args)...}
  {}

  inline constexpr auto operator<=>(mat3_t const& other) const noexcept
  {
    return m <=> other.m;
  }
};

} // namespace acl::fallback_types
