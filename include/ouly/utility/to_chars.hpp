// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/utility/user_config.hpp"
#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

#if __has_include(<fast_float/to_chars.h>)
#include <fast_float/to_chars.h>
#define OULY_DETAIL_HAS_FAST_FLOAT_TO_CHARS 1
#else
#define OULY_DETAIL_HAS_FAST_FLOAT_TO_CHARS 0
#endif

namespace ouly::detail
{
constexpr bool     has_fast_float_to_chars         = OULY_DETAIL_HAS_FAST_FLOAT_TO_CHARS != 0;
constexpr uint32_t reserved_buffer_size_for_floats = 16;
constexpr int      default_base                    = 10;

template <typename T>
constexpr auto to_chars_buffer_size() -> std::size_t
{
  if constexpr (std::integral<T>)
  {
    return static_cast<std::size_t>(std::numeric_limits<T>::digits) + 2U;
  }
  else
  {
    return static_cast<std::size_t>(std::numeric_limits<T>::max_digits10) + reserved_buffer_size_for_floats;
  }
}
} // namespace ouly::detail

namespace ouly
{

template <std::floating_point T>
auto to_chars(char* first, char* last, T value) -> std::to_chars_result
{
#if OULY_DETAIL_HAS_FAST_FLOAT_TO_CHARS
  auto result = fast_float::to_chars(first, last, value);
  return {result.ptr, result.ec};
#else
  return std::to_chars(first, last, value);
#endif
}

template <std::integral T>
auto to_chars(char* first, char* last, T value, int base = detail::default_base) -> std::to_chars_result
{
  return std::to_chars(first, last, value, base);
}

template <std::floating_point T>
void to_chars(std::string& stream, T value)
{
  std::array<char, ouly::detail::to_chars_buffer_size<T>()> buffer;
  auto const [ptr, ec] = ouly::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  if (ec != std::errc{})
  {
    if constexpr (ouly::detail::has_fast_float_to_chars)
    {
      throw std::runtime_error("fast_float conversion error");
    }
    else
    {
      throw std::runtime_error("std::to_chars conversion error");
    }
  }
  stream.append(buffer.data(), ptr);
}

template <std::integral T>
void to_chars(std::string& stream, T value, int base = detail::default_base)
{
  std::array<char, ouly::detail::to_chars_buffer_size<T>()> buffer;
  auto const [ptr, ec] = ouly::to_chars(buffer.data(), buffer.data() + buffer.size(), value, base);
  if (ec != std::errc{})
  {
    throw std::runtime_error("std::to_chars conversion error");
  }
  stream.append(buffer.data(), ptr);
}

} // namespace ouly

#undef OULY_DETAIL_HAS_FAST_FLOAT_TO_CHARS
