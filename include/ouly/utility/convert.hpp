// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/utility/from_chars.hpp"
#include "ouly/utility/string_literal.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace ouly
{

template <typename T>
struct convert; // Specialization examples are given

// Variant type transform
template <typename T>
struct index_transform
{
  static auto to_index(std::string_view ref) -> std::size_t
  {
    uint32_t index = std::numeric_limits<uint32_t>::max();
    from_chars(ref, index);
    return index;
  }

  static auto from_index(std::size_t ref) -> std::string
  {
    return std::to_string(ref);
  }
};

template <typename T>
  requires(requires {
    T(std::declval<std::string_view>());
    static_cast<std::string_view>(std::declval<T const>());
  })
struct convert<T>
{
  static auto to_type(T const& ref) -> std::string_view
  {
    return static_cast<std::string_view>(ref);
  }

  static auto from_type(T& ref, std::string_view v) -> void
  {
    ref = T(v);
  }
};

template <typename T>
  requires(requires {
    T(std::declval<std::string_view>());
    static_cast<std::string>(std::declval<T const>());
  })
struct convert<T>
{
  static auto to_type(T const& ref) -> std::string
  {
    return static_cast<std::string>(ref);
  }

  static auto from_type(T& ref, std::string_view v) -> void
  {
    ref = T(v);
  }
};

template <>
struct convert<std::string>
{
  static auto to_type(std::string const& ref) -> std::string_view
  {
    return ref;
  }

  static auto from_type(std::string& ref, std::string_view v) -> void
  {
    ref = std::string(v);
  }

  static auto from_type(std::string& ref, std::string&& v) -> void
  {
    ref = std::move(v);
  }
};

template <>
struct convert<std::unique_ptr<char[]>>
{
  static auto to_type(std::unique_ptr<char[]> const& ref) -> std::string_view
  {
    return {ref.get()};
  }

  static auto from_type(std::unique_ptr<char[]>& ref, std::string_view v) -> void
  {
    ref = std::make_unique<char[]>(v.size());
    std::ranges::copy(v, ref.get());
  }
};

template <>
struct convert<std::string_view>
{
  static auto to_type(std::string_view const& ref) -> std::string_view
  {
    return ref;
  }

  static auto from_type(std::string_view& ref, std::string_view v) -> void
  {
    ref = v;
  }
};

struct pass_through_transform
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string_view
  {
    return name;
  }
};

template <std::size_t N>
struct remove_prefix
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string_view
  {
    return name.substr(N);
  }
};
template <std::size_t N>
struct remove_suffix
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string_view
  {
    return name.substr(0, name.length() - N);
  }
};

template <string_literal Target>
struct remove_first
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result{name};
    auto        pos = result.find(static_cast<std::string_view>(Target));
    if (pos != std::string::npos)
    {
      result.erase(pos, Target.length);
    }
    return result;
  }
};

template <string_literal Target>
struct remove_last
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result{name};
    auto        pos = result.rfind(static_cast<std::string_view>(Target));
    if (pos != std::string::npos)
    {
      result.erase(pos, Target.length);
    }
    return result;
  }
};

template <string_literal Target>
struct append_first
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result;
    result.reserve(name.length() + Target.length);
    result.append(Target);
    result.append(name);
    return result;
  }
};

template <string_literal Target>
struct append_last
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result;
    result.reserve(name.length() + Target.length);
    result.append(name);
    result.append(Target);
    return result;
  }
};

template <string_literal From, string_literal To>
struct replace_all
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result{name};
    size_t      pos = 0;
    while ((pos = result.find(static_cast<std::string_view>(From), pos)) != std::string::npos)
    {
      result.replace(pos, From.length, static_cast<std::string_view>(To));
      pos += To.length;
    }
    return result;
  }
};

struct trim
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string_view
  {
    auto start = name.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos)
    {
      return {};
    }

    auto end = name.find_last_not_of(" \t\r\n");
    return name.substr(start, end - start + 1);
  }
};

constexpr static auto toupper(char a) -> char
{
  // convert to upper
  constexpr char value = 32;
  return static_cast<char>((a >= 'a' && a <= 'z') ? (a - value) : a);
}

constexpr static auto tolower(char a) -> char
{
  // convert to lower
  constexpr char value = 32;
  return static_cast<char>((a >= 'A' && a <= 'Z') ? (a + value) : a);
}

constexpr static auto isalnum(char c) -> bool
{
  return c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

constexpr static auto isupper(char c) -> bool
{
  return c >= 'A' && c <= 'Z';
}

constexpr static auto islower(char c) -> bool
{
  return c >= 'a' && c <= 'z';
}

struct to_upper
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result{name};
    std::ranges::transform(result, result.begin(), toupper);
    return result;
  }
};

struct to_lower
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result{name};
    std::ranges::transform(result, result.begin(), tolower);
    return result;
  }
};

struct pascal_case
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result;
    bool        capitalize = true;

    for (char c : name)
    {
      if (isalnum(c))
      {
        if (capitalize)
        {
          result += toupper(c);
          capitalize = false;
        }
        else
        {
          result += tolower(c);
        }
      }
      else
      {
        capitalize = true;
      }
    }
    return result;
  }
};

struct snake_case
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result;
    bool        first = true;

    for (char c : name)
    {
      if (isalnum(c))
      {
        if ((isupper(c)) && !first)
        {
          result += '_';
        }
        result += tolower(c);
        first = false;
      }
      else
      {
        result += '_';
      }
    }
    return result;
  }
};

struct lower_pascal_case
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result = pascal_case::transform(name);
    if (!result.empty())
    {
      result[0] = tolower(result[0]);
    }
    return result;
  }
};

template <typename... Transformers>
struct chain
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result{name};
    (void)std::initializer_list<int>{(result = Transformers::transform(result), 0)...};
    return result;
  }
};

template <typename Transform, string_literal Target>
auto cache_key()
{
  static auto key_str = std::string{Transform::transform(Target)};
  return key_str;
}

} // namespace ouly