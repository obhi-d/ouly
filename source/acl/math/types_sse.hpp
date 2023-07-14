#pragma once

#include "types.hpp"
#define USE_SSE2
#include "sse_mathfun.h"
#include "vml_commons.hpp"

namespace acl
{

#ifdef ACL_USE_SSE2

template <typename stag_t>
union quad_t<float, stag_t>
{
  using tag  = vector_tag;
  using stag = stag_t;

  __m128               v;
  std::array<float, 4> xyzw;
  struct
  {
    float x;
    float y;
    float z;
    float w;
  };

  inline constexpr bool operator<=(quad_t const& other) noexcept
  {
    return _mm_movemask_epi8(_mm_castps_si128(_mm_cmpngt_ps(other.v, v))) == 0;
  }

  inline constexpr bool operator>=(quad_t const& other) noexcept
  {
    return _mm_movemask_epi8(_mm_castps_si128(_mm_cmpnlt_ps(other.v, v))) == 0;
  }

  inline constexpr bool operator>(quad_t const& other) noexcept
  {
    return _mm_movemask_epi8(_mm_castps_si128(_mm_cmpngt_ps(v, other.v))) != 0;
  }

  inline constexpr bool operator<(quad_t const& other) const noexcept
  {
    return _mm_movemask_epi8(_mm_castps_si128(_mm_cmpnlt_ps(v, other.v))) != 0;
  }

  inline constexpr bool operator==(quad_t const& other) const noexcept
  {
    return _mm_movemask_epi8(_mm_castps_si128(_mm_cmpneq_ps(v, other.v))) == 0;
  }

  inline constexpr bool operator!=(quad_t const& other) const noexcept
  {
    return _mm_movemask_epi8(_mm_castps_si128(_mm_cmpneq_ps(v, other.v))) != 0;
  }

  inline operator const __m128&() const noexcept
  {
    return v;
  }
  inline operator __m128&() noexcept
  {
    return v;
  }

  inline float& operator[](int i) noexcept
  {
    return xyzw[i];
  }
  inline constexpr float operator[](int i) const noexcept
  {
    return xyzw[i];
  }

  inline constexpr quad_t(acl::noinit) noexcept {}
  inline constexpr quad_t() noexcept requires(!std::is_same_v<stag, quaternion_tag>) : xyzw{0, 0, 0, 0} {}
  inline constexpr quad_t() noexcept requires(std::is_same_v<stag, quaternion_tag>) : xyzw{0, 0, 0, 1} {}
  inline constexpr explicit quad_t(std::array<float, 4> s) noexcept : xyzw(s) {}
  inline constexpr explicit quad_t(float s) noexcept : xyzw{s, s, s, s} {}
  inline constexpr quad_t(float vx, float vy, float vz, float vw) noexcept : xyzw{vx, vy, vz, vw} {}
  inline quad_t(__m128 vv) noexcept : v{vv} {}
  template <typename utag_t>
  inline constexpr quad_t(quad_t<float, utag_t> const& other) noexcept : v(other.v)
  {} // implicit conversion allowed for this
  template <typename utag_t>
  inline constexpr quad_t& operator=(quad_t<float, utag_t> const& other) noexcept
  {
    v = other.v;
    return *this;
  } // implicit conversion allowed for this
};

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
template <typename stag_t>
union quad_t<double, stag_t>
{
  using tag  = vector_tag;
  using stag = stag_t;

  std::array<__m128d, 2> v;
  std::array<double, 4>  xyzw;
  struct
  {
    double x;
    double y;
    double z;
    double w;
  };

  inline operator const std::array<__m128d, 2>&() const noexcept
  {
    return v;
  }
  inline operator std::array<__m128d, 2>&() noexcept
  {
    return v;
  }

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

  inline double& operator[](int i) noexcept
  {
    return xyzw[i];
  }
  inline constexpr double operator[](int i) const noexcept
  {
    return xyzw[i];
  }

  inline constexpr quad_t(acl::noinit) noexcept {}
  inline constexpr quad_t() noexcept requires(!std::is_same_v<stag, quaternion_tag>) : xyzw{0, 0, 0, 0} {}
  inline constexpr quad_t() noexcept requires(std::is_same_v<stag, quaternion_tag>) : xyzw{0, 0, 0, 1} {}
  inline constexpr explicit quad_t(std::array<float, 4> s) noexcept : xyzw(s) {}
  inline constexpr explicit quad_t(float s) noexcept : xyzw{s, s, s, s} {}
  inline constexpr quad_t(float vx, float vy, float vz, float vw) noexcept : xyzw{vx, vy, vz, vw} {}
  inline explicit quad_t(__m128d xy, __m128d zw) noexcept : v{xy, zw} {}
  template <typename utag_t>
  inline constexpr quad_t(quad_t<double, utag_t> const& other) noexcept : v(other.v)
  {} // implicit conversion allowed for this
  template <typename utag_t>
  inline constexpr quad_t& operator=(quad_t<double, utag_t> const& other) noexcept
  {
    v = other.v;
    return *this;
  } // implicit conversion allowed for this
};

static constexpr bool has_avx = true;

#else
static constexpr bool has_avx   = false;
#endif

} // namespace acl
