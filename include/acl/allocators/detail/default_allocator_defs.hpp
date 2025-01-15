
#pragma once

#include <acl/allocators/config.hpp>
#include <acl/allocators/detail/memory_tracker.hpp>
#include <acl/allocators/tags.hpp>

namespace acl::detail
{

template <typename UnderlyingAllocatorTag>
struct is_static
{
  constexpr static bool value = false;
};

template <typename UnderlyingAllocatorTag>
constexpr static bool is_static_v = is_static<UnderlyingAllocatorTag>::value;

template <>
struct is_static<default_allocator_tag>
{
  constexpr static bool value = true;
};

// defaults

template <typename O>
concept HasTrackMemory = O::track_memory_v;

template <typename O>
concept HasDebugTracer = requires { typename O::debug_tracer_t; };

template <typename O>
concept HasMinAlignment = requires {
  { O::min_alignment_v } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept HasUnderlyingAllocator = requires { typename T::underlying_allocator_t; };

template <typename T>
struct debug_tracer
{
  using type = acl::detail::dummy_debug_tracer;
};

template <HasDebugTracer T>
struct debug_tracer<T>
{
  using type = typename T::debug_tracer_t;
};

template <typename T>
using debug_tracer_t = typename debug_tracer<T>::type;

template <typename T>
struct min_alignment
{
  static constexpr auto value = alignof(std::max_align_t);
};

template <HasMinAlignment T>
struct min_alignment<T>
{
  static constexpr auto value = T::min_alignment_v;
};

template <typename T>
constexpr auto min_alignment_v = min_alignment<T>::value;

} // namespace acl::detail
