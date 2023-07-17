#pragma once

#include "vml_commons.hpp"

#include "vml_commons.hpp"
#include <acl/containers/small_vector.hpp>
#include <array>
#include <compare>
#include <cstdint>
#include <type_traits>

namespace acl
{
#ifdef ACL_USE_SSE2
static constexpr bool has_sse = true;
#else
static constexpr bool has_sse   = false;
#endif

#ifdef ACL_USE_SSE3
static constexpr bool has_sse3 = true;
#else
static constexpr bool has_sse3  = false;
#endif

#ifdef ACL_USE_SSE41
static constexpr bool has_sse41 = true;
#else
static constexpr bool has_sse41 = false;
#endif

#ifdef ACL_USE_AVX
static constexpr bool has_avx = true;
#else
static constexpr bool has_avx   = false;
#endif

#ifdef ACL_USE_FMA
static constexpr bool has_fma = true;
#else
static constexpr bool has_fma   = false;
#endif

struct vec4_tag
{};
struct vec3a_tag
{};
struct vec2_tag
{};
struct vec3_tag
{};
struct extends_tag
{};
struct matrix_tag
{};
struct rect_tag
{};
struct quaternion_tag
{};
struct sphere_tag
{};
struct aabb_tag
{};
struct plane_tag
{};
struct color_tag
{};
struct axis_angle_tag
{};

template <typename scalar_t>
concept FloatingType = std::is_floating_point_v<scalar_t>;

template <typename scalar_t>
concept IntegralType = std::is_integral_v<scalar_t>;

template <typename scalar_t>
concept ScalarType = std::is_floating_point_v<scalar_t> || std::is_integral_v<scalar_t>;

template <typename T>
concept Matrix = std::same_as<typename T::tag, matrix_tag>;

template <typename T>
concept GenVector = std::same_as<typename T::tag, vec2_tag> || std::same_as<typename T::tag, vec3_tag>;

template <typename T>
concept Color = std::same_as<typename T::tag, color_tag>;

template <ScalarType scalar_t>
struct quad
{
  using type = std::array<scalar_t, 4>;
};

#ifdef ACL_USE_SSE2
template <>
struct quad<float>
{
  using type = __m128;
};
#endif

template <ScalarType scalar_t>
using quadv_t = typename quad<scalar_t>::type;

template <ScalarType scalar_t, uint32_t N>
struct quadv_array_t
{
  using scalar_type = scalar_t;
  quadv_t<scalar_t> data[N];

  inline quadv_t<scalar_t>& operator[](uint32_t i) noexcept
  {
    return data[i];
  }

  inline quadv_t<scalar_t> const& operator[](uint32_t i) const noexcept
  {
    return data[i];
  }
};

using quadv_f = quadv_t<float>;
using quadv_d = quadv_t<double>;

template <ScalarType scalar_t, typename tag_t>
struct quad_t
{
  using tag                               = tag_t;
  using scalar_type                       = scalar_t;
  static constexpr uint32_t element_count = 4;

  union
  {
    quadv_t<scalar_t>       v;
    std::array<scalar_t, 4> xyzw;
    struct
    {
      scalar_t x;
      scalar_t y;
      scalar_t z;
      scalar_t w;
    };
    struct
    {
      scalar_t r;
      scalar_t g;
      scalar_t b;
      scalar_t a;
    };
  };

  inline constexpr auto operator<=>(quad_t const& other) const noexcept
  {
    return xyzw <=> other.xyzw;
  }

  inline constexpr auto operator==(quad_t const& other) const noexcept
  {
    return xyzw == other.xyzw;
  }

  inline constexpr auto operator!=(quad_t const& other) const noexcept
  {
    return xyzw != other.xyzw;
  }

  inline scalar_t& operator[](int i) noexcept
  {
    return xyzw[i];
  }
  inline constexpr scalar_t operator[](int i) const noexcept
  {
    return xyzw[i];
  }

  static constexpr bool homogenous_vector = std::is_same_v<tag, quaternion_tag> || std::is_same_v<tag, vec4_tag>;

  inline constexpr quad_t(acl::noinit) noexcept {}
  template <typename utag_t>
  inline constexpr quad_t(quad_t<scalar_t, utag_t> const& other, scalar_t w) noexcept : xyzw{other.x, other.y, other.z, w}
  {}
  template <typename utag_t>
  inline constexpr quad_t(quad_t<scalar_t, utag_t> const& other) noexcept : v(other.v)
  {} // implicit conversion allowed for this
  template <typename utag_t>
  inline constexpr quad_t& operator=(quad_t<scalar_t, utag_t> const& other) noexcept
  {
    v = other.v;
    return *this;
  } // implicit conversion allowed for this
  inline constexpr quad_t() noexcept : xyzw{0, 0, 0, homogenous_vector ? 1 : 0} {}

  inline quad_t(quadv_t<scalar_t> const& s) noexcept : v(s) {}
  inline constexpr explicit quad_t(std::array<scalar_t, 4> const& s) noexcept
    requires(!std::is_same_v<std::array<scalar_t, 4>, quadv_t<scalar_t>>)
      : xyzw(s)
  {}
  inline constexpr explicit quad_t(scalar_t s) noexcept : xyzw{s, s, s, s} {}
  inline constexpr explicit quad_t(scalar_t const v[4]) noexcept : xyzw{v[0], v[0], v[0], v[0]} {}
  inline constexpr quad_t(scalar_t vx, scalar_t vy, scalar_t vz, scalar_t vw) noexcept : xyzw{vx, vy, vz, vw} {}
  inline constexpr quad_t(scalar_t vx, scalar_t vy, scalar_t vz) noexcept : xyzw{vx, vy, vz, homogenous_vector ? 1 : 0}
  {}

  inline operator quadv_t<scalar_t> const&() const noexcept
  {
    return v;
  }

  inline quad_t& operator=(quad_t const& s) noexcept
  {
    v = s.v;
    return *this;
  }

  inline quad_t& operator=(quadv_t<scalar_t> const& s) noexcept
  {
    v = s;
    return *this;
  }

  inline quad_t operator!() noexcept;
  inline quad_t operator-() noexcept;

  inline quad_t& operator+=(quad_t const& s) noexcept;
  inline quad_t& operator-=(quad_t const& s) noexcept;
  inline quad_t& operator*=(quad_t const& s) noexcept;
  inline quad_t& operator/=(quad_t const& s) noexcept;

  inline quad_t& operator*=(scalar_t s) noexcept;
  inline quad_t& operator/=(scalar_t s) noexcept;
};

template <typename scalar_t>
struct vec2_t
{
  using tag                               = vec2_tag;
  using scalar_type                       = scalar_t;
  static constexpr uint32_t element_count = 2;

  union
  {
    std::array<scalar_t, 2> v;
    std::array<scalar_t, 2> xy;
    struct
    {
      scalar_t x;
      scalar_t y;
    };
    struct
    {
      scalar_t theta;
      scalar_t phi;
    };
  };

  inline constexpr auto operator<=>(vec2_t const& other) const noexcept
  {
    return xy <=> other.xy;
  }

  inline constexpr auto operator==(vec2_t const& other) const noexcept
  {
    return xy == other.xy;
  }

  inline constexpr auto operator!=(vec2_t const& other) const noexcept
  {
    return xy != other.xy;
  }

  inline scalar_t& operator[](int i) noexcept
  {
    return xy[i];
  }
  inline constexpr scalar_t operator[](int i) const noexcept
  {
    return xy[i];
  }

  inline vec2_t operator-() noexcept;

  inline vec2_t& operator+=(vec2_t const& s) noexcept;
  inline vec2_t& operator-=(vec2_t const& s) noexcept;
  inline vec2_t& operator*=(vec2_t const& s) noexcept;
  inline vec2_t& operator/=(vec2_t const& s) noexcept;

  inline vec2_t& operator*=(scalar_t s) noexcept;
  inline vec2_t& operator/=(scalar_t s) noexcept;

  inline constexpr vec2_t(acl::noinit) noexcept {}
  inline constexpr vec2_t() noexcept : xy{0, 0} {}
  inline constexpr explicit vec2_t(std::array<scalar_t, 2> s) noexcept : xy(s) {}
  inline constexpr explicit vec2_t(scalar_t v) noexcept : xy{v, v} {}
  inline constexpr vec2_t(scalar_t vx, scalar_t vy) noexcept : xy{vx, vy} {}
};

template <typename scalar_t>
struct vec3_t
{
  using tag                               = vec3_tag;
  using scalar_type                       = scalar_t;
  static constexpr uint32_t element_count = 3;

  union
  {
    std::array<scalar_t, 3> v;
    std::array<scalar_t, 3> xyz;
    struct
    {
      scalar_t x;
      scalar_t y;
      scalar_t z;
    };
    struct
    {
      scalar_t pitch;
      scalar_t yaw;
      scalar_t roll;
    };
  };

  inline constexpr auto operator<=>(vec3_t const& other) const noexcept
  {
    return xyz <=> other.xyz;
  }

  inline constexpr auto operator==(vec3_t const& other) const noexcept
  {
    return xyz == other.xyz;
  }

  inline constexpr auto operator!=(vec3_t const& other) const noexcept
  {
    return xyz != other.xyz;
  }

  inline scalar_t& operator[](int i) noexcept
  {
    return xyz[i];
  }
  inline constexpr scalar_t operator[](int i) const noexcept
  {
    return xyz[i];
  }

  inline vec3_t operator-() noexcept;

  inline vec3_t& operator+=(vec3_t const& s) noexcept;
  inline vec3_t& operator-=(vec3_t const& s) noexcept;
  inline vec3_t& operator*=(vec3_t const& s) noexcept;
  inline vec3_t& operator/=(vec3_t const& s) noexcept;

  inline vec3_t& operator*=(scalar_t s) noexcept;
  inline vec3_t& operator/=(scalar_t s) noexcept;

  inline constexpr vec3_t(acl::noinit) noexcept {}
  inline constexpr vec3_t() noexcept : xyz{0, 0, 0} {}
  inline constexpr explicit vec3_t(std::array<scalar_t, 3> s) noexcept : xyz(s) {}
  inline constexpr explicit vec3_t(scalar_t v) noexcept : xyz{v, v, v} {}
  inline constexpr vec3_t(scalar_t vx, scalar_t vy, scalar_t vz) noexcept : xyz{vx, vy, vz} {}

  inline constexpr vec3_t& operator =(vec3_t const& other) noexcept { xyz = other.xyz; return *this; }
};

template <typename scalar_t>
using vec3a_t = quad_t<scalar_t, vec3a_tag>;
template <typename scalar_t>
using vec4_t = quad_t<scalar_t, vec4_tag>;
template <typename scalar_t>
using plane_t = quad_t<scalar_t, plane_tag>;
template <typename scalar_t>
using quat_t = quad_t<scalar_t, quaternion_tag>;
template <typename scalar_t>
using sphere_t = quad_t<scalar_t, sphere_tag>;
template <typename scalar_t>
using axis_angle_t = quad_t<scalar_t, axis_angle_tag>;
template <typename scalar_t>
using polar_coord_t = vec2_t<scalar_t>;
template <typename scalar_t>
using euler_angles_t = vec3_t<scalar_t>;
template <typename scalar_t>
using color_t = quad_t<scalar_t, color_tag>;
template <typename scalar_t>
using extends_t = quad_t<scalar_t, extends_tag>;

template <typename scalar_t>
struct rect_t
{
  using tag = rect_tag;

  union
  {
    std::array<vec2_t<scalar_t>, 2>        r;
    std::array<scalar_t, 4>                m;
    std::array<std::array<scalar_t, 2>, 2> e;
  };

  inline constexpr rect_t(acl::noinit) noexcept : r{acl::noinit_v, acl::noinit_v} {};
  inline constexpr rect_t(std::array<vec2_t<scalar_t>, 2> const& rv) noexcept : r{rv} {};
  inline constexpr rect_t() noexcept : m{} {};

  template <typename... RowType>
  inline constexpr rect_t(RowType... args) noexcept : m{args...}
  {}

  inline constexpr auto operator<=>(rect_t const& other) const noexcept
  {
    return r <=> other.r;
  }

  inline constexpr auto operator==(rect_t const& other) const noexcept
  {
    return r == other.r;
  }

  inline constexpr auto operator!=(rect_t const& other) const noexcept
  {
    return r != other.r;
  }

  inline auto& operator[](int i) noexcept
  {
    return r[i];
  }

  inline auto const& operator[](int i) const noexcept
  {
    return r[i];
  }

  inline constexpr rect_t& operator =(rect_t const& other) noexcept { r = other.r; return *this; }
};

template <typename scalar_t>
struct aabb_t
{
  using tag = aabb_tag;

  union
  {
    quadv_array_t<scalar_t, 2>             v;
    std::array<vec3a_t<scalar_t>, 2>       r;
    std::array<scalar_t, 8>                m;
    std::array<std::array<scalar_t, 4>, 2> e;
  };

  inline constexpr aabb_t(quadv_array_t<scalar_t, 2> const& vv) noexcept : v{vv} {};
  inline constexpr aabb_t(std::array<vec3a_t<scalar_t>, 2> const& rv) noexcept : r{rv} {};
  inline constexpr aabb_t(acl::noinit) noexcept : m{acl::noinit_v, acl::noinit_v} {};

  inline constexpr aabb_t() noexcept : m{} {};

  template <ScalarType... RowType>
  inline constexpr aabb_t(RowType... args) noexcept : m{args...}
  {}

  inline constexpr aabb_t(vec3a_t<scalar_t> const& min_v, vec3a_t<scalar_t> const& max_v) noexcept : r{min_v, max_v} {}

  inline constexpr auto operator<=>(aabb_t const& other) const noexcept
  {
    return m <=> other.m;
  }

  inline constexpr auto operator==(aabb_t const& other) const noexcept
  {
    return m == other.m;
  }

  inline constexpr auto operator!=(aabb_t const& other) const noexcept
  {
    return m != other.m;
  }

  inline auto& operator[](int i) noexcept
  {
    return r[i];
  }

  inline auto const& operator[](int i) const noexcept
  {
    return r[i];
  }

  inline constexpr aabb_t& operator =(aabb_t const& other) noexcept { r = other.r; return *this; }
};

template <typename scalar_t>
struct mat4_t
{
  using tag = matrix_tag;

  union
  {
    quadv_array_t<scalar_t, 4>             v;
    std::array<vec4_t<scalar_t>, 4>        r;
    std::array<scalar_t, 16>               m;
    std::array<std::array<scalar_t, 4>, 4> e;
  };

  inline constexpr mat4_t(quadv_array_t<scalar_t, 4> const& vv) noexcept : v{vv} {};
  inline constexpr mat4_t(std::array<vec4_t<scalar_t>, 4> const& rv) noexcept : r{rv} {};
  inline constexpr mat4_t(acl::noinit) noexcept : r{acl::noinit_v, acl::noinit_v, acl::noinit_v, acl::noinit_v} {}

  inline constexpr mat4_t() noexcept
      : r{vec4_t<scalar_t>(1, 0, 0, 0), vec4_t<scalar_t>(0, 1, 0, 0), vec4_t<scalar_t>(0, 0, 1, 0),
          vec4_t<scalar_t>(0, 0, 0, 1)}
  {}

  template <ScalarType... setter_t>
  inline constexpr mat4_t(setter_t... args) noexcept : m{static_cast<scalar_t>(args)...}
  {}

  inline constexpr mat4_t(vec4_t<scalar_t> r0, vec4_t<scalar_t> r1, vec4_t<scalar_t> r2, vec4_t<scalar_t> r3) noexcept
      : r{r0, r1, r2, r3}
  {}

  inline constexpr auto operator<=>(mat4_t const& other) const noexcept
  {
    return m <=> other.m;
  }

  inline constexpr auto operator!=(mat4_t const& other) const noexcept
  {
    return m != other.m;
  }

  inline constexpr auto operator==(mat4_t const& other) const noexcept
  {
    return m == other.m;
  }

  inline auto& operator[](int i) noexcept
  {
    return r[i];
  }

  inline auto const& operator[](int i) const noexcept
  {
    return r[i];
  }

  inline constexpr mat4_t& operator =(mat4_t const& other) noexcept { r = other.r; return *this; }
};

template <typename scalar_t>
struct mat3_t
{
  using tag = matrix_tag;

  union
  {
    quadv_array_t<scalar_t, 3>             v;
    std::array<vec4_t<scalar_t>, 3>        r;
    std::array<scalar_t, 12>               m;
    std::array<std::array<scalar_t, 4>, 3> e;
  };

  inline constexpr mat3_t(quadv_array_t<scalar_t, 3> const& vv) noexcept : v{vv} {};
  inline constexpr mat3_t(std::array<vec4_t<scalar_t>, 3> const& rv) noexcept : r{rv} {};
  inline constexpr mat3_t(acl::noinit) noexcept : r{acl::noinit_v, acl::noinit_v, acl::noinit_v} {}

  inline constexpr mat3_t() noexcept
      : r{vec4_t<scalar_t>(1, 0, 0, 0), vec4_t<scalar_t>(0, 1, 0, 0), vec4_t<scalar_t>(0, 0, 1, 0)}
  {}

  template <ScalarType... setter_t>
  inline constexpr mat3_t(setter_t... args) noexcept : m{static_cast<scalar_t>(args)...}
  {}

  inline constexpr mat3_t(vec4_t<scalar_t> r0, vec4_t<scalar_t> r1, vec4_t<scalar_t> r2) noexcept : r{r0, r1, r2} {}

  inline constexpr auto operator<=>(mat3_t const& other) const noexcept
  {
    return m <=> other.m;
  }

  inline constexpr auto operator!=(mat3_t const& other) const noexcept
  {
    return m != other.m;
  }

  inline constexpr auto operator==(mat3_t const& other) const noexcept
  {
    return m == other.m;
  }

  inline auto& operator[](int i) noexcept
  {
    return r[i];
  }

  inline auto const& operator[](int i) const noexcept
  {
    return r[i];
  }

  inline constexpr mat3_t& operator =(mat3_t const& other) noexcept { r = other.r; return *this; }
};

template <typename scalar_t>
struct bounds_info_t
{
  vec3_t<scalar_t> center       = {};
  vec3_t<scalar_t> half_extends = {};
  scalar_t         radius       = {};

  inline bounds_info_t& operator+=(bounds_info_t const& other) noexcept;
};

//! Represents a bounding volume, consisting of
//! an AABB and a bounding sphere, the AABB shares
//! the center with the sphere
template <typename scalar_t>
struct bounding_volume_t
{
  //! center wrt some coordinate system
  sphere_t<scalar_t> spherical_vol;
  //! half half_extends dx/2, dy/2, dz/2
  extends_t<scalar_t> half_extends;

  bounding_volume_t() noexcept = default;
  bounding_volume_t(sphere_t<scalar_t> const& sphere, extends_t<scalar_t> const& halfextends) noexcept
      : spherical_vol(sphere), half_extends(halfextends)
  {}
  inline constexpr bounding_volume_t(acl::noinit) noexcept
      : spherical_vol{acl::noinit_v}, half_extends{acl::noinit_v} {};
  inline bounding_volume_t& operator+=(bounding_volume_t const& other) noexcept;
};

template <typename scalar_t>
struct transform_t
{
  //! Rotation
  quat_t<scalar_t> rotation = quat_t<scalar_t>(0.0f, 0.0f, 0.0f, 1.0f);
  //! Translation and scale (w = scale)
  vec4_t<scalar_t> translation_and_scale = vec4_t<scalar_t>(0.0f, 0.0f, 0.0f, 1.0f);

  inline transform_t() noexcept = default;
  inline transform_t(noinit) noexcept : rotation(noinit_v), translation_and_scale(noinit_v) {}
  inline transform_t(quat_t<scalar_t> const& r, vec4_t<scalar_t> const& ts) noexcept
      : rotation(r), translation_and_scale(ts)
  {}
  inline transform_t(quat_t<scalar_t> const& r, vec3a_t<scalar_t> const& ts, scalar_t scale) noexcept
      : rotation(r), translation_and_scale(ts.x, ts.y, ts.z, scale)
  {}
};

struct coherency
{
  std::uint32_t mask_hierarchy = 0xffffffff;
  std::uint32_t plane          = 0;
#ifndef NDEBUG
  std::uint32_t iterations = 0;
#endif
  coherency() = default;
  coherency(std::uint32_t plane_count) : mask_hierarchy((1 << plane_count) - 1) {}
};

template <typename scalar_t>
struct frustum_t
{
  // common frustums will have these planes
  static constexpr uint32_t k_near              = 0;
  static constexpr uint32_t k_far               = 1;
  static constexpr uint32_t k_left              = 2;
  static constexpr uint32_t k_right             = 3;
  static constexpr uint32_t k_top               = 4;
  static constexpr uint32_t k_bottom            = 5;
  static constexpr uint32_t k_fixed_plane_count = 6;

  inline auto& operator[](int i) noexcept
  {
    return planes[i];
  }

  inline auto const& operator[](int i) const noexcept
  {
    return planes[i];
  }

  inline auto size() const noexcept
  {
    return planes.size();
  }

  inline frustum_t() noexcept = default;
  inline frustum_t(plane_t<scalar_t> const* iplanes, uint32_t nb) noexcept : planes(iplanes, iplanes + nb) {}

  acl::small_vector<plane_t<scalar_t>, 6> planes;
};

//

// template <typename T>
// concept TransformMatrix =
//   std::same_as<typename T::tag, matrix_tag> && std::same_as<typename T::stag, transform_matrix_tag>;

} // namespace acl
