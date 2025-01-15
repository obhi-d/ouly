#pragma once

#include <acl/allocators/default_allocator.hpp>

namespace acl::detail
{
template <typename T>
struct custom_allocator
{
  using type = default_allocator<>;
};
template <acl::detail::HasAllocatorAttribs T>
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

template <acl::detail::HasUnderlyingAllocator T>
struct underlying_allocator<T>
{
  using type = typename T::underlying_allocator_t;
};

template <typename Traits>
using underlying_allocator_t = typename underlying_allocator<Traits>::type;
} // namespace acl::detail