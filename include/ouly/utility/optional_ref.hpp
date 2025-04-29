// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/utility/user_config.hpp"
#include <compare>
#include <optional>
#include <type_traits>

namespace ouly
{

/**
 * @brief A lightweight optional reference wrapper similar to std::optional but for references
 *
 * This class provides a way to represent an optional reference to an object. Unlike
 * std::optional, it does not own the referenced object and has no copy/move semantics
 * of the underlying value_type. It simply holds a pointer that can be null.
 *
 * @tparam T The value_type of the referenced object
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
 * optional_ref<int> cfg(x);
 * if(cfg) {
 *     // Use *cfg or cfg.get()
 * }
 * ```
 */
template <class T>
struct optional_ref
{
  using value_type = std::remove_reference_t<T>;

  constexpr optional_ref() = default;
  constexpr optional_ref(value_type& iv) noexcept : value_(&iv) {}
  constexpr optional_ref(value_type* iv) noexcept : value_(iv) {}
  constexpr optional_ref(std::nullopt_t /*value*/) noexcept {}
  constexpr optional_ref(std::nullptr_t /*value*/) noexcept {}

  constexpr operator bool() const noexcept
  {
    return value_ != nullptr;
  }

  constexpr auto operator*() const noexcept -> value_type&
  {
    OULY_ASSERT(value_);
    return *value_;
  }

  constexpr auto get() const noexcept -> value_type&
  {
    OULY_ASSERT(value_);
    return *value_;
  }

  constexpr auto operator->() const noexcept -> value_type*
  {
    return value_;
  }

  constexpr explicit operator value_type*() const noexcept
  {
    return value_;
  }

  constexpr explicit operator value_type&() const noexcept
  {
    OULY_ASSERT(value_);
    return *value_;
  }

  [[nodiscard]] constexpr auto has_value() const noexcept -> bool
  {
    return value_ != nullptr;
  }

  [[nodiscard]] constexpr auto value() -> value_type&
  {
    OULY_ASSERT(value_);
    return *value_;
  }

  [[nodiscard]] constexpr auto value() const -> value_type const&
  {
    OULY_ASSERT(value_);
    return *value_;
  }

  [[nodiscard]] constexpr auto value_or(value_type& default_value) -> value_type&
  {
    return value_ ? *value_ : default_value;
  }

  [[nodiscard]] constexpr auto value_or(value_type const& default_value) const -> value_type const&
  {
    return value_ ? *value_ : default_value;
  }

  constexpr void reset() const noexcept
  {
    value_ = nullptr;
  }

  constexpr auto release() const noexcept -> value_type*
  {
    auto r = value_;
    value_ = nullptr;
    return r;
  }

  constexpr auto operator<=>(const optional_ref& other) const noexcept = default;

  value_type* value_ = nullptr;
};

// Deduction guide for optional ref constructed from pointer
template <class T>
optional_ref(T*) -> optional_ref<T>;

template <class T>
optional_ref(T&) -> optional_ref<T>;

} // namespace ouly