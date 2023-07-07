#pragma once

#include "mat4.hpp"
#include "plane.hpp"
#include "vec4.hpp"
#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <tuple>

namespace acl
{

struct frustum_t
{
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

  // common frustums will have these planes
  enum plane_type
  {
    k_near,
    k_far,
    k_left,
    k_right,
    k_top,
    k_bottom,
  };

  enum
  {
    k_fixed_plane_count = 6
  };

  frustum_t() : plane_count(0) {}
  frustum_t(frustum_t const& i_other) : plane_count(i_other.plane_count), pplanes(nullptr)
  {
    if (plane_count <= k_fixed_plane_count && plane_count > 0)
    {
      for (std::uint32_t i = 0; i < plane_count; ++i)
        planes[i] = i_other.planes[i];
    }
    else if (plane_count)
    {
      pplanes = acl::allocate<acl::plane_t>(default_allocator<>(), sizeof(acl::plane_t) * plane_count,
                                            acl::alignarg<acl::plane_t>);
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
  frustum_t(plane_t const* i_planes, std::uint32_t i_size) : plane_count(i_size), pplanes(nullptr)
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
      pplanes = acl::allocate<acl::plane_t>(default_allocator<>(), sizeof(acl::plane_t) * plane_count,
                                            acl::alignarg<acl::plane_t>);
      if (!i_planes)
        return;
      for (std::uint32_t i = 0; i < plane_count; ++i)
        pplanes[i] = i_planes[i];
    }
  }
  ~frustum_t()
  {
    if (plane_count > k_fixed_plane_count && pplanes)
      acl::deallocate(default_allocator<>(), pplanes, sizeof(acl::plane_t) * plane_count, acl::alignarg<acl::plane_t>);
  }

  inline frustum_t& operator=(frustum_t const& i_other) noexcept
  {
    if (plane_count > k_fixed_plane_count && pplanes)
      acl::deallocate(default_allocator<>(), pplanes, sizeof(acl::plane_t) * plane_count);
    pplanes     = nullptr;
    plane_count = i_other.plane_count;
    if (plane_count <= k_fixed_plane_count && plane_count > 0)
    {
      for (std::uint32_t i = 0; i < plane_count; ++i)
        planes[i] = i_other.planes[i];
    }
    else if (plane_count)
    {
      pplanes = acl::allocate<acl::plane_t>(default_allocator<>(), sizeof(acl::plane_t) * plane_count,
                                            acl::alignarg<acl::plane_t>);
      for (std::uint32_t i = 0; i < plane_count; ++i)
        pplanes[i] = i_other.pplanes[i];
    }
    return *this;
  }

  inline frustum_t& operator=(frustum_t&& i_other) noexcept
  {
    if (plane_count > k_fixed_plane_count && pplanes)
      acl::deallocate(default_allocator<>(), pplanes, sizeof(acl::plane_t) * plane_count);

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

  inline std::uint32_t count() const noexcept
  {
    return plane_count;
  }

  inline plane_t operator[](std::uint32_t i) const noexcept
  {
    assert(i < plane_count);
    return (plane_count > k_fixed_plane_count) ? pplanes[i] : planes[i];
  }

  inline void modify(std::uint32_t i, plane_t const& p) noexcept
  {
    assert(i < plane_count);
    plane_t* dest = (plane_count > k_fixed_plane_count) ? &pplanes[i] : &planes[i];
    *dest         = p;
  }

  /**
   * @remarks Construct from a transpose(view*projection) matrix
   * @param mat transpose(View*Projection) or transpose(Proj)*transpose(View)
   * matrix
   */
  void build(mat4_t const& combo) noexcept
  {
    // Near clipping planeT
    planes[k_near] = plane::normalize((mat4::row(combo, 2)));
    // Far clipping planeT
    planes[k_far] = plane::normalize(vec4::sub(mat4::row(combo, 3), mat4::row(combo, 2)));
    // Left clipping planeT
    planes[k_left] = plane::normalize((vec4::add(mat4::row(combo, 0), mat4::row(combo, 3))));
    // Right clipping planeT
    planes[k_right] = plane::normalize(vec4::sub(mat4::row(combo, 3), mat4::row(combo, 0)));
    // Top clipping planeT
    planes[k_top] = plane::normalize(vec4::sub(mat4::row(combo, 3), mat4::row(combo, 1)));
    // Bottom clipping planeT
    planes[k_bottom] = plane::normalize((vec4::add(mat4::row(combo, 1), mat4::row(combo, 3))));
    plane_count      = 6;
  }

  plane_t const* get_all() const noexcept
  {
    return (plane_count > k_fixed_plane_count) ? pplanes : planes;
  }
  plane_t* get_all() noexcept
  {
    return (plane_count > k_fixed_plane_count) ? pplanes : planes;
  }

  union
  {
    plane_t  planes[k_fixed_plane_count];
    plane_t* pplanes;
  };
  // if plane_count <= 6, we use planes, otherwise we use planes
  std::uint32_t plane_count;
};

struct frustum
{
  using coherency  = frustum_t::coherency;
  using plane_type = frustum_t::plane_type;

  static inline std::uint32_t count(frustum_t const& _)
  {
    return _.count();
  }
  static inline coherency default_coherency(std::uint32_t plane_count)
  {
    return coherency(plane_count);
  }
  static inline frustum_t from_planes(plane_t const* i_planes, std::uint32_t i_size)
  {
    return frustum_t(i_planes, i_size);
  }

  static inline void set(frustum_t& _, mat4::pref m)
  {
    _.build(m);
  }
  //! From a combined view projection matrix
  static inline frustum_t from_mat4_transpose(mat4::pref m)
  {
    frustum_t _;
    _.build(m);
    return _;
  }
  static inline std::pair<plane_t const*, std::uint32_t> get_planes(frustum_t const& _)
  {
    return std::make_pair(_.get_all(), _.count());
  }
  static inline std::pair<plane_t*, std::uint32_t> get_planes(frustum_t& _)
  {
    return std::make_pair(_.get_all(), _.count());
  }
  static inline plane_t get_plane(frustum_t& _, frustum::plane_type type)
  {
    return _[type];
  }
  static inline void set_plane(frustum_t& _, std::uint32_t i, plane_t const& p)
  {
    _.modify(i, p);
  }
};

} // namespace acl
