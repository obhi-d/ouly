// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/config.hpp"
#include "ouly/allocators/detail/memory_tracker.hpp"
#include "ouly/allocators/tags.hpp"

namespace ouly::detail
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
concept HasProtection = requires { typename T::protection_t; };

template <typename T>
struct debug_tracer
{
  using type = ouly::detail::dummy_debug_tracer;
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

template <typename T>
struct has_protection
{
  static constexpr bool value = false;
};

template <HasProtection T>
struct has_protection<T>
{
  static constexpr bool value = true;
};

template <typename T>
constexpr bool has_protection_v = has_protection<T>::value;

template <typename T>
struct protection
{
  // Default protection - will be overridden by specialization
  using type = void;
};

template <HasProtection T>
struct protection<T>
{
  using type                  = typename T::protection_t;
  static constexpr auto value = T::protection_v;
};

template <typename T>
using protection_t = typename protection<T>::type;

template <typename T>
constexpr auto protection_v = protection<T>::value;

} // namespace ouly::detail
