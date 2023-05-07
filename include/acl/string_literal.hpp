
#pragma once

#include <string_view>
#include <algorithm>

namespace acl
{

struct string_literal
{
  template <std::size_t N>
  constexpr string_literal(const char (&a)[N]) : p(a), sz(N - 1)
  {}
  constexpr string_literal(char const* a, std::size_t const N) : p(a), sz(N) {}

  constexpr char operator[](std::size_t n) const
  {
    return n < sz ? p[n] : (char)0;
  }

  [[nodiscard]] constexpr std::size_t size() const
  {
    return sz;
  }
  [[nodiscard]] constexpr char const* name() const
  {
    return p;
  }
  [[nodiscard]] constexpr string_literal substring(const std::size_t start, const std::size_t end) const
  {
    return string_literal(p + start, size() - (end + start));
  }
  [[nodiscard]] constexpr std::uint32_t hash() const
  {
    return compute(p, sz - 1);
  }

  static constexpr std::uint32_t compute(char const* const s, std::size_t count)
  {
    return ((count ? compute(s, count - 1) : 2166136261u) ^ static_cast<std::uint8_t>(s[count])) * 16777619u;
  }

  inline constexpr explicit operator std::string_view() const
  {
    return std::string_view(p, sz);
  }

  char const*       p;
  std::size_t const sz;
};

template <std::size_t N>
struct fixed_string
{
  constexpr fixed_string(const char (&str)[N]) {
    std::copy_n(str, N, value);
  }

  inline constexpr explicit operator std::string_view() const
  {
    return std::string_view(value, N-1);
  }

  char value[N];
};

} // namespace acl
