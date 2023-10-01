
#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace acl
{

template <std::size_t N>
struct string_literal
{
  static constexpr std::size_t length = N - 1;

  constexpr string_literal(const char (&str)[N])
  {
    std::copy_n(str, N, value);
  }

  static constexpr std::uint32_t compute(char const* const s, std::size_t count)
  {
    return ((count ? compute(s, count - 1) : 2166136261u) ^ static_cast<std::uint8_t>(s[count])) * 16777619u;
  }

  constexpr inline std::uint32_t hash() const
  {
    return compute(value, N - 1);
  }

  inline constexpr explicit operator std::string_view() const
  {
    return std::string_view(value, N - 1);
  }

  char value[N];
};

} // namespace acl
