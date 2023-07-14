#pragma once

#include "vml_commons.hpp"
#include <array>
#include <compare>
#include <cstdint>
#include <type_traits>

namespace acl
{

struct vector_tag
{};
struct extended_tag
{};
struct matrix_tag
{};
struct default_tag
{};
struct nonquad_tag
{};
struct quaternion_tag
{};
struct sphere_tag
{};
struct transform_matrix_tag
{};
struct plane_tag
{};


template <typename scalar_t = float, typename stag_t = default_tag>
union quad_t
{
  using tag                               = vector_tag;
  using stag                              = stag_t;
  using scalar_type                       = scalar_t;
  static constexpr uint32_t element_count = 4;

  std::array<scalar_t, 4> v; // same as xyzw, just ensure v exists for vector types
  std::array<scalar_t, 4> xyzw;
  struct
  {
    scalar_t x;
    scalar_t y;
    scalar_t z;
    scalar_t w;
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

  inline constexpr quad_t(acl::noinit) noexcept {}
  template <typename utag_t>
  inline constexpr quad_t(quad_t<scalar_t, utag_t> const& other) noexcept : v(other.v)
  {} // implicit conversion allowed for this
  template <typename utag_t>
  inline constexpr quad_t& operator=(quad_t<scalar_t, utag_t> const& other) noexcept
  {
    v = other.v;
    return *this;
  } // implicit conversion allowed for this
  inline constexpr quad_t() noexcept requires(!std::is_same_v<stag, quaternion_tag>) : xyzw{0, 0, 0, 0} {}
  inline constexpr quad_t() noexcept requires(std::is_same_v<stag, quaternion_tag>) : xyzw{0, 0, 0, 1} {}
  inline constexpr explicit quad_t(std::array<scalar_t, 4> s) noexcept : xyzw(s) {}
  inline constexpr explicit quad_t(scalar_t s) noexcept : xyzw{s, s, s, s} {}
  inline constexpr quad_t(scalar_t vx, scalar_t vy, scalar_t vz, scalar_t vw) noexcept : xyzw{vx, vy, vz, vw} {}
  inline constexpr quad_t(scalar_t vx, scalar_t vy, scalar_t vz) noexcept : xyzw{vx, vy, vz, 0} {}
};

template <typename scalar_t>
union vec2_t
{
  using tag                               = vector_tag;
  using stag                              = nonquad_tag;
  using scalar_type                       = scalar_t;
  static constexpr uint32_t element_count = 2;

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

  inline constexpr vec2_t(acl::noinit) noexcept {}
  inline constexpr vec2_t() noexcept : xy{0, 0} {}
  inline constexpr explicit vec2_t(std::array<scalar_t, 2> s) noexcept : xy(s) {}
  inline constexpr explicit vec2_t(scalar_t v) noexcept : xy{v, v} {}
  inline constexpr vec2_t(scalar_t vx, scalar_t vy) noexcept : xy{vx, vy} {}
};

template <typename scalar_t>
union vec3_t
{
  using tag                               = vector_tag;
  using stag                              = nonquad_tag;
  using scalar_type                       = scalar_t;
  static constexpr uint32_t element_count = 3;

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

  inline constexpr vec3_t(acl::noinit) noexcept {}
  inline constexpr vec3_t() noexcept : xyz{0, 0, 0} {}
  inline constexpr explicit vec3_t(std::array<scalar_t, 3> s) noexcept : xyz(s) {}
  inline constexpr explicit vec3_t(scalar_t v) noexcept : xyz{v, v, v} {}
  inline constexpr vec3_t(scalar_t vx, scalar_t vy, scalar_t vz) noexcept : xyz{vx, vy, vz} {}
};

template <typename scalar_t>
using vec3a_t = quad_t<scalar_t, extended_tag>;
template <typename scalar_t>
using vec4_t = quad_t<scalar_t>;
template <typename scalar_t>
using plane_t = quad_t<scalar_t, plane_tag>;
template <typename scalar_t>
using quat_t = quad_t<scalar_t, quaternion_tag>;
template <typename scalar_t>
using sphere_t = quad_t<scalar_t, sphere_tag>;
template <typename scalar_t>
using axis_angle_t = quad_t<scalar_t>;
template <typename scalar_t>
using polar_coord_t = vec2_t<scalar_t>;
template <typename scalar_t>
using euler_angles_t = vec3_t<scalar_t>;

template <typename scalar_t>
union rect_t
{
  using tag         = matrix_tag;
  using scalar_type = scalar_t;

  std::array<vec2_t<scalar_t>, 2>        r;
  std::array<scalar_t, 4>                m;
  std::array<std::array<scalar_t, 2>, 2> e[2][2];

  inline constexpr rect_t(acl::noinit) noexcept : r{acl::noinit_v, acl::noinit_v} {};

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
};

template <typename scalar_t>
union aabb_t
{
  using tag = matrix_tag;

  std::array<vec3a_t<scalar_t>, 2>       r;
  std::array<scalar_t, 8>                m;
  std::array<std::array<scalar_t, 4>, 2> e;

  inline constexpr aabb_t(acl::noinit) noexcept : m{acl::noinit_v, acl::noinit_v} {};

  inline constexpr aabb_t() noexcept : m{} {};

  template <typename... RowType>
  inline constexpr aabb_t(RowType... args) noexcept : m{args...}
  {}

  inline constexpr aabb_t(vec3a_t<scalar_t> const& min_v, vec3a_t<scalar_t> const& max_v) noexcept : r{min_v, max_v}
  {}

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
};

template <typename scalar_t>
union mat4_t
{
  using tag = matrix_tag;
  using stag = transform_matrix_tag;

  std::array<vec4_t<scalar_t>, 4>        r;
  std::array<scalar_t, 16>               m;
  std::array<std::array<scalar_t, 4>, 4> e;

  inline constexpr mat4_t(acl::noinit) noexcept : r{acl::noinit_v, acl::noinit_v, acl::noinit_v, acl::noinit_v} {}

  inline constexpr mat4_t() noexcept : r{
          vec4_t<scalar_t>(1, 0, 0, 0),
          vec4_t<scalar_t>(0, 1, 0, 0),
          vec4_t<scalar_t>(0, 0, 1, 0),
          vec4_t<scalar_t>(0, 0, 0, 1)
      } {}

  template <typename... ScalarType>
  inline constexpr mat4_t(ScalarType... args) noexcept : m{static_cast<scalar_t>(args)...}
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
};

template <typename scalar_t>
union mat3_t
{
  using tag = matrix_tag;
  using stag = transform_matrix_tag;

  std::array<vec4_t<scalar_t>, 3>        r;
  std::array<scalar_t, 12>               m;
  std::array<std::array<scalar_t, 4>, 3> e;

  inline constexpr mat3_t(acl::noinit) noexcept : r{acl::noinit_v, acl::noinit_v, acl::noinit_v, acl::noinit_v} {}

  inline constexpr mat3_t() noexcept : r{
          vec4_t<scalar_t>(1, 0, 0, 0),
          vec4_t<scalar_t>(0, 1, 0, 0),
          vec4_t<scalar_t>(0, 0, 1, 0)
      } {}

  template <typename... ScalarType>
  inline constexpr mat3_t(ScalarType... args) noexcept : m{static_cast<scalar_t>(args)...}
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
};

template <typename scalar_t>
struct bounds_info_t
{
  vec3_t<scalar_t> center       = {};
  vec3_t<scalar_t> half_extends = {};
  scalar_t         radius       = {};
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
  vec3a_t<scalar_t> half_extends;

  bounding_volume_t() noexcept = default;
  bounding_volume_t(sphere_t<scalar_t> const& sphere, vec3a_t<scalar_t> const& halfextends) noexcept
      : spherical_vol(sphere), half_extends(halfextends)
  {}
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
  inline transform_t(quat_t<scalar_t> const& r, vec4_t<scalar_t> const& ts) noexcept : rotation(r), translation_and_scale(ts) {}
  inline transform_t(quat_t<scalar_t> const& r, vec3a_t<scalar_t> const& ts, float scale) noexcept : rotation(r), translation_and_scale(ts.xy, ts.y, ts.z, scale) {}
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

  frustum_t() noexcept = default;
  frustum_t(frustum_t const& i_other) noexcept : plane_count(i_other.plane_count), pplanes(nullptr)
  {
    if (plane_count <= k_fixed_plane_count && plane_count > 0)
    {
      for (std::uint32_t i = 0; i < plane_count; ++i)
        planes[i] = i_other.planes[i];
    }
    else if (plane_count)
    {
      pplanes = acl::allocate<acl::plane_t<scalar_t>>(
        default_allocator<>(), sizeof(acl::plane_t<scalar_t>) * plane_count, acl::alignarg<acl::plane_t<scalar_t>>);
      for (std::uint32_t i = 0; i < plane_count; ++i)
        pplanes[i] = i_other.pplanes[i];
    }
  }
  frustum_t(frustum_t&& i_other) : plane_count(i_other.plane_count), pplanes(nullptr)
  {
    if (plane_count <= k_fixed_plane_count && plane_count > 0)
    {
      for (std::uint32_t i = 0; i < plane_count; ++i)
        planes[i] = i_other.planes[i];
    }
    else if (plane_count)
    {
      pplanes         = i_other.pplanes;
      i_other.pplanes = nullptr;
    }
  }
  frustum_t(plane_t<scalar_t> const* i_planes, std::uint32_t i_size) : plane_count(i_size), pplanes(nullptr)
  {
    if (plane_count <= k_fixed_plane_count && plane_count > 0)
    {
      if (!i_planes)
        return;
      for (std::uint32_t i = 0; i < plane_count; ++i)
        planes[i] = i_planes[i];
    }
    else if (plane_count)
    {
      pplanes = acl::allocate<acl::plane_t<scalar_t>>(
        default_allocator<>(), sizeof(acl::plane_t<scalar_t>) * plane_count, acl::alignarg<acl::plane_t<scalar_t>>);
      if (!i_planes)
        return;
      for (std::uint32_t i = 0; i < plane_count; ++i)
        pplanes[i] = i_planes[i];
    }
  }
  ~frustum_t() noexcept
  {
    if (plane_count > k_fixed_plane_count && pplanes)
      acl::deallocate(default_allocator<>(), pplanes, sizeof(acl::plane_t<scalar_t>) * plane_count,
                      acl::alignarg<acl::plane_t<scalar_t>>);
  }

  inline frustum_t& operator=(frustum_t const& i_other) noexcept
  {
    if (plane_count > k_fixed_plane_count && pplanes)
      acl::deallocate(default_allocator<>(), pplanes, sizeof(acl::plane_t<scalar_t>) * plane_count);
    pplanes     = nullptr;
    plane_count = i_other.plane_count;
    if (plane_count <= k_fixed_plane_count && plane_count > 0)
    {
      for (std::uint32_t i = 0; i < plane_count; ++i)
        planes[i] = i_other.planes[i];
    }
    else if (plane_count)
    {
      pplanes = acl::allocate<acl::plane_t<scalar_t>>(
        default_allocator<>(), sizeof(acl::plane_t<scalar_t>) * plane_count, acl::alignarg<acl::plane_t<scalar_t>>);
      for (std::uint32_t i = 0; i < plane_count; ++i)
        pplanes[i] = i_other.pplanes[i];
    }
    return *this;
  }

  inline frustum_t& operator=(frustum_t&& i_other) noexcept
  {
    if (plane_count > k_fixed_plane_count && pplanes)
      acl::deallocate(default_allocator<>(), pplanes, sizeof(acl::plane_t<scalar_t>) * plane_count);

    pplanes     = nullptr;
    plane_count = i_other.plane_count;
    if (plane_count <= k_fixed_plane_count && plane_count > 0)
    {
      for (std::uint32_t i = 0; i < plane_count; ++i)
        planes[i] = i_other.planes[i];
    }
    else if (plane_count)
    {
      pplanes         = i_other.pplanes;
      i_other.pplanes = nullptr;
    }
    return *this;
  }

  inline std::uint32_t size() const noexcept
  {
    return plane_count;
  }

  inline plane_t<scalar_t> operator[](std::uint32_t i) const noexcept
  {
    assert(i < plane_count);
    return (plane_count > k_fixed_plane_count) ? pplanes[i] : planes[i];
  }

  inline void modify(std::uint32_t i, plane_t<scalar_t> const& p) noexcept
  {
    assert(i < plane_count);
    plane_t<scalar_t>* dest = (plane_count > k_fixed_plane_count) ? &pplanes[i] : &planes[i];
    *dest                   = p;
  }

  /// @brief Construct from a transpose(view*projection) matrix
  /// @param mat transpose(View*Projection) or transpose(Proj)*transpose(View) matrix
  void build(mat4_t<scalar_t> const& combo) noexcept
  {
    // Near clipping planeT
    planes[k_near] = normalize(row(combo, 2));
    // Far clipping planeT
    planes[k_far] = normalize(sub(row(combo, 3), row(combo, 2)));
    // Left clipping planeT
    planes[k_left] = normalize((add(row(combo, 0), row(combo, 3))));
    // Right clipping planeT
    planes[k_right] = normalize(sub(row(combo, 3), row(combo, 0)));
    // Top clipping planeT
    planes[k_top] = normalize(sub(row(combo, 3), row(combo, 1)));
    // Bottom clipping planeT
    planes[k_bottom] = normalize((add(row(combo, 1), row(combo, 3))));
    plane_count      = 6;
  }

  plane_t<scalar_t> const* get_all() const noexcept
  {
    return (plane_count > k_fixed_plane_count) ? pplanes : planes;
  }
  plane_t<scalar_t>* get_all() noexcept
  {
    return (plane_count > k_fixed_plane_count) ? pplanes : planes;
  }

  union
  {
    plane_t<scalar_t>  planes[k_fixed_plane_count];
    plane_t<scalar_t>* pplanes;
  };
  // if plane_count <= 6, we use planes, otherwise we use planes
  std::uint32_t plane_count;
};

template <typename T>
concept Matrix = std::same_as<typename T::tag, matrix_tag>;

template <typename T>
concept Vector = std::same_as<typename T::tag, vector_tag>;

template <typename T>
concept NonQuadVector = std::same_as<typename T::tag, vector_tag> && std::same_as<typename T::stag, nonquad_tag>;

template <typename T>
concept TransformMatrix = std::same_as<typename T::tag, matrix_tag> && std::same_as<typename T::stag, transform_matrix_tag>;

} // namespace acl
