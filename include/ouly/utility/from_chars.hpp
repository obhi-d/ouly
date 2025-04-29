// SPDX-License-Identifier: MIT
#pragma once

#include <charconv>
#include <concepts>
#include <stdexcept>
#include <string_view>
#include <system_error>

#if __has_include(<fast_float/fast_float.h>)
#include <fast_float/fast_float.h>
namespace ouly::detail
{
constexpr bool has_fast_float = true;
}
#else
namespace ouly::detail
{
constexpr bool has_fast_float = false;
}
namespace fast_float
{
template <typename T>
void from_chars(const char* first, const char* last, T& value, int base = 0);
}
#endif

namespace ouly
{

template <std::floating_point T>
void from_chars(std::string_view sv, T& value)
{
  using namespace std::string_view_literals;

  if (sv == ".nan"sv || sv == "nan"sv)
  {
    value = std::numeric_limits<T>::quiet_NaN();
  }
  else if (sv == ".inf"sv || sv == "inf"sv)
  {
    value = std::numeric_limits<T>::infinity();
  }
  else if (sv == "-.inf"sv || sv == "-inf"sv)
  {
    value = -std::numeric_limits<T>::infinity();
  }
  else
  {
    const char* begin = sv.data();
    const char* end   = begin + sv.size();

    if constexpr (ouly::detail::has_fast_float)
    {
      // Use fast_float for floating point types if available.
      auto result = fast_float::from_chars(begin, end, value);
      if (result.ec != std::errc{})
      {
        throw std::runtime_error("fast_float conversion error");
      }
    }
    else
    {
      auto result = std::from_chars(begin, end, value);
      if (result.ec != std::errc{})
      {
        throw std::runtime_error("std::from_chars conversion error");
      }
    }
  }
}

template <std::integral T>
void from_chars(std::string_view sv, T& value)
{
  using namespace std::string_view_literals;

  constexpr int default_base = 10;
  constexpr int base_16      = 16;
  constexpr int base_8       = 8;

  int base = default_base;
  if (sv.starts_with("0x"sv))
  {
    base = base_16;
    sv.remove_prefix(2);
  }
  else if (sv.starts_with("0"sv) && sv.size() > 1)
  {
    base = base_8;
    sv.remove_prefix(1);
  }

  const char* begin = sv.data();
  const char* end   = begin + sv.size();

  if constexpr (ouly::detail::has_fast_float)
  {
    // Use fast_float for floating point types if available.
    auto result = fast_float::from_chars(begin, end, value, base);
    if (result.ec != std::errc{})
    {
      throw std::runtime_error("fast_float conversion error");
    }
  }
  else
  {
    auto result = std::from_chars(begin, end, value, base);
    if (result.ec != std::errc{})
    {
      throw std::runtime_error("std::from_chars conversion error");
    }
  }
}
} // namespace ouly