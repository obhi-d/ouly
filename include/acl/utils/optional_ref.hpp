#pragma once
#include <cassert>
#include <compare>
#include <type_traits>

namespace acl
{

/**
 * @brief A lightweight optional reference wrapper similar to std::optional but for references
 *
 * This class provides a way to represent an optional reference to an object. Unlike
 * std::optional, it does not own the referenced object and has no copy/move semantics
 * of the underlying type. It simply holds a pointer that can be null.
 *
 * @tparam T The type of the referenced object
 *
 * @note The class removes references from the template parameter using std::remove_reference_t
 *
 * Key features:
 * - Lightweight (sizeof(T*))
 * - Can be used as a boolean to check if it contains a value
 * - Provides pointer-like dereferencing operations
 * - Supports comparison operations
 * - Non-owning semantics
 *
 * Example usage:
 * ```
 * int x = 42;
 * optional_ref<int> opt(x);
 * if(opt) {
 *     // Use *opt or opt.get()
 * }
 * ```
 */
template <class T>
struct optional_ref
{
  using type               = std::remove_reference_t<T>;
  constexpr optional_ref() = default;
  constexpr explicit optional_ref(type& iv) noexcept : value_(&iv) {}
  constexpr explicit optional_ref(type* iv) noexcept : value_(iv) {}

  constexpr operator bool() const noexcept
  {
    return value_ != nullptr;
  }

  constexpr auto operator*() const noexcept -> type&
  {
    assert(value_);
    return *value_;
  }

  constexpr auto get() const noexcept -> type&
  {
    assert(value_);
    return *value_;
  }

  constexpr auto operator->() const noexcept -> type*
  {
    return value_;
  }

  constexpr explicit operator type*() const noexcept
  {
    return value_;
  }

  constexpr explicit operator type&() const noexcept
  {
    assert(value_);
    return *value_;
  }

  [[nodiscard]] constexpr auto has_value() const noexcept -> bool
  {
    return value_ != nullptr;
  }

  constexpr void reset() const noexcept
  {
    value_ = nullptr;
  }

  constexpr auto release() const noexcept -> type*
  {
    auto r = value_;
    value_ = nullptr;
    return r;
  }

  constexpr auto operator<=>(const optional_ref& other) const noexcept = default;

  type* value_ = nullptr;
};
} // namespace acl