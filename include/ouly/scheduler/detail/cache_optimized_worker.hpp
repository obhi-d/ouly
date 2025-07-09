// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace ouly::detail
{

// Cache line size - typically 64 bytes on modern CPUs
static constexpr std::size_t cache_line_size = 64;

// Simple padding structure to prevent false sharing
template <typename T>
struct alignas(cache_line_size) cache_aligned
{
  T value;

  cache_aligned() = default;
  cache_aligned(const T& val) : value(val) {}
  cache_aligned(T&& val) : value(std::move(val)) {}

  T& get()
  {
    return value;
  }
  const T& get() const
  {
    return value;
  }

  operator T&()
  {
    return value;
  }
  operator const T&() const
  {
    return value;
  }
};

// Atomic type with cache line alignment to prevent false sharing
template <typename T>
using cache_aligned_atomic = cache_aligned<std::atomic<T>>;

} // namespace ouly::detail
