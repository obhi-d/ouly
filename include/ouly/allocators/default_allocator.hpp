// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/allocator.hpp"
#include "ouly/allocators/detail/default_allocator_defs.hpp"
#include "ouly/allocators/detail/memory_stats.hpp"
#include "ouly/allocators/detail/memory_tracker.hpp"
#include "ouly/allocators/std_allocator_wrapper.hpp"
#include "ouly/utility/common.hpp"
#include "ouly/utility/detail/concepts.hpp"
#include "ouly/utility/type_traits.hpp"
#include <new>

namespace ouly
{

template <typename Ty = std::void_t<>>
struct allocator_traits
{
  using is_always_equal                        = std::false_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_swap            = std::true_type;
};

template <>
struct allocator_traits<default_allocator_tag>
{
  using is_always_equal                        = std::true_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_swap            = std::false_type;
};

// ----------------- Allocator Config -----------------

template <typename Config = ouly::config<>>
struct OULY_EMPTY_BASES default_allocator
    : ouly::detail::memory_tracker<default_allocator_tag, ouly::detail::debug_tracer_t<Config>,
                                   ouly::detail::HasTrackMemory<Config>>
{
  using tag       = default_allocator_tag;
  using address   = void*;
  using size_type = ouly::detail::choose_size_t<std::size_t, Config>;
  using tracker   = ouly::detail::memory_tracker<default_allocator_tag, ouly::detail::debug_tracer_t<Config>,
                                                 ouly::detail::HasTrackMemory<Config>>;

  static constexpr auto align = ouly::detail::min_alignment_v<Config>;

  template <typename Alignment = alignment<align>>
  [[nodiscard]] static auto allocate(size_type size, Alignment alignment = {}) -> address
  {
    if constexpr (alignment)
    {
      return tracker::when_allocate(::operator new(size, std::align_val_t{static_cast<std::size_t>(alignment)}), size);
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
      ::operator delete(fixup, std::align_val_t{static_cast<std::size_t>(alignment)});
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

template <typename T, typename UA = default_allocator<std::size_t>>
using vector = std::vector<T, ouly::allocator_wrapper<T, UA>>;
} // namespace ouly
