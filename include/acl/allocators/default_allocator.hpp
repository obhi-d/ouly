#pragma once
#include <acl/allocators/allocator.hpp>
#include <acl/allocators/memory_stats.hpp>
#include <acl/allocators/memory_tracker.hpp>
#include <acl/allocators/std_allocator_wrapper.hpp>
#include <acl/utils/common.hpp>
#include <acl/utils/type_traits.hpp>
#include <new>

namespace acl
{
struct default_allocator_tag
{};

template <>
struct allocator_traits<default_allocator_tag>
{
  using is_always_equal                        = std::true_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_swap            = std::false_type;
};

namespace detail
{

template <>
struct is_static<default_allocator_tag>
{
  constexpr static bool value = true;
};
} // namespace detail

// ----------------- Allocator Options -----------------
namespace opt
{

struct track_memory
{
  static constexpr bool track_memory_v = true;
};

template <typename T>
struct debug_tracer
{
  using debug_tracer_t = T;
};

template <std::size_t N>
struct min_alignment
{
  static constexpr std::size_t min_alignment_v = N;
};

template <typename T>
struct underlying_allocator
{
  using underlying_allocator_t = T;
};

template <typename T>
struct allocator_type
{
  using allocator_t = T;
};

} // namespace opt

// ----------------- Allocator Options -----------------
namespace detail
{

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
  using type = detail::dummy_debug_tracer;
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

} // namespace detail
// ----------------- Allocator Options -----------------

template <typename Options = acl::options<>>
struct ACL_EMPTY_BASES default_allocator
    : detail::memory_tracker<default_allocator_tag, detail::debug_tracer_t<Options>, detail::HasTrackMemory<Options>>
{
  using tag       = default_allocator_tag;
  using address   = void*;
  using size_type = detail::choose_size_t<std::size_t, Options>;
  using tracker =
   detail::memory_tracker<default_allocator_tag, detail::debug_tracer_t<Options>, detail::HasTrackMemory<Options>>;

  static constexpr auto align = detail::min_alignment_v<Options>;

  template <typename Alignment = alignment<align>>
  [[nodiscard]] static auto allocate(size_type size, Alignment alignment = {}) -> address
  {
    if constexpr (alignment)
    {
      return tracker::when_allocate(::operator new(size, std::align_val_t{(std::size_t)alignment}), size);
    }
    else
    {
      return tracker::when_allocate(::operator new(size), size);
    }
  }

  template <typename Alignment = alignment<align>>
  [[nodiscard]] static auto zero_allocate(size_type size, Alignment alignment = {}) -> address
  {
    void* ptr = allocate(size, alignment);
    std::memset(ptr, 0, size);
    return ptr;
  }

  template <typename Alignment = alignment<align>>
  static void deallocate(address addr, size_type size, Alignment alignment = {})
  {
    void* fixup = tracker::when_deallocate(addr, size);
    if constexpr (alignment)
    {
      ::operator delete(fixup, std::align_val_t{(std::size_t)alignment});
    }
    else
    {
      ::operator delete(fixup);
    }
  }

  static constexpr auto null() -> void*
  {
    return nullptr;
  }

  constexpr auto operator==(default_allocator const& /*unused*/) const -> bool
  {
    return true;
  }

  constexpr auto operator!=(default_allocator const& /*unused*/) const -> bool
  {
    return false;
  }
};

namespace detail
{
template <typename T>
struct custom_allocator
{
  using type = default_allocator<>;
};
template <detail::HasAllocatorAttribs T>
struct custom_allocator<T>
{
  using type = typename T::allocator_t;
};

template <typename Traits>
using custom_allocator_t = typename custom_allocator<Traits>::type;

template <typename T>
struct underlying_allocator
{
  using type = default_allocator<>;
};

template <detail::HasUnderlyingAllocator T>
struct underlying_allocator<T>
{
  using type = typename T::underlying_allocator_t;
};

template <typename Traits>
using underlying_allocator_t = typename underlying_allocator<Traits>::type;

} // namespace detail

template <typename T, typename UA = default_allocator<std::size_t>>
using vector = std::vector<T, acl::allocator_wrapper<T, UA>>;
} // namespace acl
