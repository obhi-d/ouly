// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <new>

namespace ouly
{

/**
 * @brief Alignment for allocations is passed using tag dispatching. It is strictly a type when the alignment can be
 * determined at compile time, otherwise a generic unsigned value is accepted as a parameter and the alignment type is
 * assumed to be the underlying unsigned type.
 * @example Allocate using alignarg
 * auto pointer = ouly::allocate<std::string>(allocator, sizeof(std::string), alignarg<std::string>);
 *
 * @note alignarg is an alias for a constexpr declaration of alignment.
 */
template <std::size_t Value = 0>
struct alignment
{
  static constexpr std::align_val_t value = std::align_val_t{Value};
  constexpr alignment() noexcept          = default;

  constexpr explicit operator bool() const noexcept
  {
    // Any non-trivial alignment must be honored: allocators do not round allocation sizes up, so
    // even small alignments (<= alignof(void*)) are not implicitly guaranteed by previous
    // allocations.
    return Value > 1;
  }

  constexpr operator std::size_t() const noexcept
  {
    return Value;
  }

  static constexpr auto log2() noexcept
  {
    auto constexpr half = Value >> 1;
    return (Value != 0U) ? 1 + alignment<half>::log2() : -1;
  }
};

/** @brief constexpr value of alignment for a given type using alignof */
template <typename T>
constexpr auto alignarg = alignment<alignof(T)>();

/** @brief Align a pointer up to the given power-of-two alignment. */
inline auto align(void* ptr, std::size_t alignment) -> void*
{
  auto const mask  = alignment - 1;
  auto const value = (reinterpret_cast<uintptr_t>(ptr) + mask) & ~static_cast<uintptr_t>(mask); // NOLINT
  return reinterpret_cast<void*>(value);                                                        // NOLINT
}

} // namespace ouly