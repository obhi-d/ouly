// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/alignment.hpp"
#include <concepts>
#include <cstddef>
#include <new>

namespace ouly
{

/**
 * @brief Allocates memory based on the allocator provided and casts the memory to a type for use. Note that this
 * function **does not** call the constructor for the given type. If the caller needs to call in-place new on the type,
 * it is probaly better to stick to `allocator.allocate(...)`
 */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
[[nodiscard]] auto allocate(Allocator& allocator, typename Allocator::size_type size_in_bytes, Alignment alignment = {})
 -> Ty*
{
  return static_cast<Ty*>(allocator.allocate(size_in_bytes, alignment));
}

/**
 * @brief Allocates zeroed out memory based on the allocator provided and casts the memory to a type for use. Note that
 * this function **does not** call the constructor for the given type. If the caller needs to call in-place new on the
 * type, it is probaly better to stick to `allocator.allocate(...)`
 */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
[[nodiscard]] auto zallocate(Allocator& allocator, typename Allocator::size_type size_in_bytes,
                             Alignment alignment = {}) -> Ty*
{
  return static_cast<Ty*>(allocator.zero_allocate(size_in_bytes, alignment));
}

/** @brief Deallocates memory allocated by allocate or zallocate */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
void deallocate(Allocator& allocator, Ty* data, typename Allocator::size_type size_in_bytes, Alignment alignment = {})
{
  allocator.deallocate(data, size_in_bytes, alignment);
}

/**
 * @brief Allocates memory based on the allocator provided and casts the memory to a type for use. The allocator can be
 * a constant allocator, like the default_allocator. Note that this function **does not** call the constructor for the
 * given type. If the caller needs to call in-place new on the type, it is probaly better to stick to
 * `allocator.allocate(...)`
 */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
[[nodiscard]] auto allocate(Allocator const& allocator, typename Allocator::size_type size_in_bytes,
                            Alignment alignment = {}) -> Ty*
{
  return static_cast<Ty*>(allocator.allocate(size_in_bytes, alignment));
}

/**
 * @brief Allocates zeroed out memory based on the allocator provided and casts the memory to a type for use. The
 * allocator can be a constant allocator, like the default_allocator. Note that this function **does not** call the
 * constructor for the given type. If the caller needs to call in-place new on the type, it is probaly better to stick
 * to `allocator.allocate(...)`
 */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
[[nodiscard]] auto zallocate(Allocator const& allocator, typename Allocator::size_type size_in_bytes,
                             Alignment alignment = {}) -> Ty*
{
  return static_cast<Ty*>(allocator.zero_allocate(size_in_bytes, alignment));
}

/**
 * @brief Deallocates memory allocated by allocate or zallocate. The allocator can be a constant allocator, like the
 * default_allocator */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
void deallocate(Allocator const& allocator, Ty* data, typename Allocator::size_type size_in_bytes,
                Alignment alignment = {})
{
  allocator.deallocate(data, size_in_bytes, alignment);
}

} // namespace ouly
