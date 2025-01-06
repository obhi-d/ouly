
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
    std::copy_n(static_cast<char const*>(str), N, static_cast<char*>(value_));
  }

  static constexpr auto compute(char const* const s, std::size_t count) -> std::uint32_t
  {
    constexpr uint32_t prime_0 = 2166136261U;
    constexpr uint32_t prime_1 = 16777619U;
    return (((count != 0U) ? compute(s, count - 1) : prime_0) ^ static_cast<std::uint8_t>(s[count])) * prime_1;
  }

  [[nodiscard]] constexpr auto hash() const -> std::uint32_t
  {
    return compute(static_cast<char const*>(value_), N - 1);
  }

  constexpr operator std::string_view() const
  {
    return {static_cast<char const*>(value_), N - 1};
  }

  char value_[N]{};
};

} // namespace acl
