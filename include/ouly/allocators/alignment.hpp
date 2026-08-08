// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <type_traits>

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

namespace detail
{
/**
 * @brief Round `offset` up to the given power-of-two `alignment`.
 *
 * @return The aligned offset, or `std::nullopt` when rounding up would wrap around the value range.
 */
template <typename T>
  requires(std::is_unsigned_v<T>)
constexpr auto align_offset(T offset, T alignment) noexcept -> std::optional<T>
{
  auto const mask = static_cast<T>(alignment - 1);
  if (offset > static_cast<T>(std::numeric_limits<T>::max() - mask))
  {
    return std::nullopt;
  }
  return static_cast<T>((offset + mask) & ~mask);
}

/**
 * @brief Normalize an alignment argument to a plain value.
 *
 * Allocators accept the alignment as an `ouly::alignment<N>` tag, which carries the value in the
 * type, as a `std::align_val_t` when it is only known at runtime, or as any integral value. All
 * three spellings convert to `std::size_t`, and a value of 0 or 1 means "no alignment requirement".
 */
template <typename Alignment>
constexpr auto alignment_of(Alignment i_alignment) noexcept -> std::size_t
{
  return static_cast<std::size_t>(i_alignment);
}

/** @brief Restate any alignment argument as the type the standard uses for over-aligned new/delete */
template <typename Alignment>
constexpr auto align_val_of(Alignment i_alignment) noexcept -> std::align_val_t
{
  return std::align_val_t{alignment_of(i_alignment)};
}

/** @brief True when the alignment argument is known at compile time to be trivial (0 or 1) */
template <typename Alignment>
constexpr bool is_trivial_alignment_v = false;

template <std::size_t Value>
constexpr bool is_trivial_alignment_v<alignment<Value>> = (Value <= 1);

/**
 * @brief Number of padding bytes needed to bring `address` up to `i_alignment`
 *
 * Returns 0 when the address already satisfies the alignment, which is the common case for arenas
 * whose page boundary is at least as aligned as the request; no extra bytes are then consumed.
 */
constexpr auto align_padding(std::uintptr_t address, std::size_t i_alignment) noexcept -> std::size_t
{
  if (i_alignment <= 1)
  {
    return 0;
  }
  return static_cast<std::size_t>((~address + 1U) & static_cast<std::uintptr_t>(i_alignment - 1));
}

/** @brief Round `value` up to a multiple of the power-of-two `i_alignment` */
constexpr auto align_size(std::size_t value, std::size_t i_alignment) noexcept -> std::size_t
{
  if (i_alignment <= 1)
  {
    return value;
  }
  auto const mask = i_alignment - 1U;
  return (value + mask) & ~mask;
}
} // namespace detail

/** @brief True when `value` is a non-zero power of two */
constexpr auto is_power_of_two(std::size_t value) noexcept -> bool
{
  return value != 0U && (value & (value - 1U)) == 0U;
}

/** @brief Align a pointer up to the given power-of-two alignment. */
inline auto align(void* ptr, std::size_t alignment) -> void*
{
  auto const mask  = alignment - 1;
  auto const value = (reinterpret_cast<uintptr_t>(ptr) + mask) & ~static_cast<uintptr_t>(mask); // NOLINT
  return reinterpret_cast<void*>(value);                                                        // NOLINT
}

} // namespace ouly