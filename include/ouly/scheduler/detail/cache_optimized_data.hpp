// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/utility/config.hpp"
#include <atomic>
#include <concepts>
#include <cstddef>

namespace ouly::detail
{

static constexpr std::size_t cache_line_size = ouly_cache_line_size; /*std::hardware_destructive_interference_size*/

// Simple padding structure to prevent false sharing
template <typename T>
struct alignas(cache_line_size) cache_optimized_data
{
  struct plain_layout
  {
    T value_ = {};

    template <typename... Args>
    plain_layout(Args&&... args)
      requires(std::constructible_from<T, Args...>)
        : value_(std::forward<Args>(args)...)
    {}

    plain_layout() = default;
    plain_layout(const T& val)
      requires(std::copy_constructible<T>)
        : value_(val)
    {}

    plain_layout(T&& val)
      requires(std::move_constructible<T>)
        : value_(std::move(val))
    {}
  };

  constexpr static size_t padding_size = cache_line_size - (sizeof(T) % cache_line_size);
  struct padded_layout
  {
    T         value_                 = {};
    std::byte padding_[padding_size] = {};

    template <typename... Args>
    padded_layout(Args&&... args)
      requires(std::constructible_from<T, Args...>)
        : value_(std::forward<Args>(args)...)
    {}

    padded_layout() = default;
    padded_layout(const T& val)
      requires(std::copy_constructible<T>)
        : value_(val)
    {}

    padded_layout(T&& val)
      requires(std::move_constructible<T>)

        : value_(std::move(val))
    {}
  };

  std::conditional_t<padding_size != 0, padded_layout, plain_layout> value_;

  template <typename... Args>
  cache_optimized_data(Args&&... args)
    requires(std::constructible_from<T, Args...>)
      : value_(std::forward<Args>(args)...)
  {}

  cache_optimized_data() = default;
  cache_optimized_data(const T& val)
    requires(std::copy_constructible<T>)
      : value_(val)
  {}

  cache_optimized_data(T&& val)
    requires(std::move_constructible<T>)
      : value_(std::move(val))
  {}

  [[nodiscard]] auto get() -> T&
  {
    return value_.value_;
  }

  [[nodiscard]] auto get() const -> const T&
  {
    return value_.value_;
  }

  explicit operator T&()
  {
    return value_.value_;
  }

  explicit operator const T&() const
  {
    return value_.value_;
  }
};

// Atomic type with cache line alignment to prevent false sharing
template <typename T>
using cache_aligned_atomic = cache_optimized_data<std::atomic<T>>;

template <typename T>
struct cache_aligned_padding
{
  static constexpr size_t padding_size = cache_line_size - (sizeof(T) % cache_line_size);
  std::byte padding_[padding_size] = {};
};

} // namespace ouly::detail
