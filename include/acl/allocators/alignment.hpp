#pragma once
#include <cstdint>
#include <new>

namespace acl
{

/**
 * @brief Alignment for allocations is passed using tag dispatching. It is strictly a type when the alignment can be
 * determined at compile time, otherwise a generic unsigned value is accepted as a parameter and the alignment type is
 * assumed to be the underlying unsigned type.
 * @example Allocate using alignarg
 * auto pointer = acl::allocate<std::string>(allocator, sizeof(std::string), alignarg<std::string>);
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
    return Value > alignof(void*);
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

inline auto align(void* ptr, std::size_t alignment) -> void*
{
  auto off = static_cast<std::size_t>(reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)); // NOLINT
  return static_cast<char*>(ptr) + off;
}

} // namespace acl