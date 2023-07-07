
export module acl.utils:string_literal;

import <algorithm>;
import <cstdint>;
import <string_view>;

export namespace acl
{

template <std::size_t N>
struct string_literal
{
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
