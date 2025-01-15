
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

  constexpr explicit string_literal(const char* str)
  {
    std::copy_n(str, length, static_cast<char*>(value_));
  }

  constexpr string_literal(const char (&str)[N])
  {
    std::copy_n(static_cast<char const*>(str), length, static_cast<char*>(value_));
  }

  static constexpr auto compute(char const* const s, std::size_t count) -> std::uint32_t
  {
    constexpr uint32_t prime_0 = 2166136261U;
    constexpr uint32_t prime_1 = 16777619U;

    std::uint32_t hash = prime_0;
    for (std::size_t i = 0; i <= count; ++i)
    {
      hash = (hash ^ static_cast<std::uint8_t>(s[i])) * prime_1;
    }
    return hash;
  }

  [[nodiscard]] constexpr auto hash() const -> std::uint32_t
  {
    return compute(static_cast<char const*>(value_), N - 1);
  }

  constexpr explicit(false) operator std::string_view() const
  {
    return {static_cast<char const*>(value_), N - 1};
  }

  char value_[N]{};
};

} // namespace acl
