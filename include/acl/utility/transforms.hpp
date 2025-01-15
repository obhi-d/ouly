#pragma once

#include <acl/utility/string_literal.hpp>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace acl
{

template <typename T>
struct convert; // Specialization examples are given

// Variant type transform
template <typename T>
struct index_transform
{
  static auto to_index(std::string_view ref) -> std::size_t
  {
    uint32_t index  = std::numeric_limits<uint32_t>::max();
    auto     result = std::from_chars(ref.data(), ref.data() + ref.size(), index);
    return (result.ec != std::errc()) ? std::numeric_limits<std::size_t>::max() : index;
  }

  static auto from_index(std::size_t ref) -> std::string
  {
    return std::to_string(ref);
  }
};

template <typename T>
  requires(requires {
    T(std::declval<std::string_view>());
    (std::string_view) std::declval<T const>();
  })
struct convert<T>
{
  static auto to_string(T const& ref) -> std::string_view
  {
    return (std::string_view)ref;
  }

  static auto from_string(T& ref, std::string_view v) -> void
  {
    ref = T(v);
  }
};

template <typename T>
  requires(requires {
    T(std::declval<std::string_view>());
    (std::string) std::declval<T const>();
  })
struct convert<T>
{
  static auto to_string(T const& ref) -> std::string
  {
    return (std::string)ref;
  }

  static auto from_string(T& ref, std::string_view v) -> void
  {
    ref = T(v);
  }
};

template <>
struct convert<std::string>
{
  static auto to_string(std::string const& ref) -> std::string_view
  {
    return ref;
  }

  static auto from_string(std::string& ref, std::string_view v) -> void
  {
    ref = std::string(v);
  }

  static auto from_string(std::string& ref, std::string&& v) -> void
  {
    ref = std::move(v);
  }
};

template <>
struct convert<std::unique_ptr<char[]>>
{
  static auto to_string(std::unique_ptr<char[]> const& ref) -> std::string_view
  {
    return {ref.get()};
  }

  static auto from_string(std::unique_ptr<char[]>& ref, std::string_view v) -> void
  {
    ref = std::make_unique<char[]>(v.size());
    std::ranges::copy(v, ref.get());
  }
};

template <>
struct convert<std::string_view>
{
  static auto to_string(std::string_view const& ref) -> std::string_view
  {
    return ref;
  }

  static auto from_string(std::string_view& ref, std::string_view v) -> void
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
    auto        pos = result.find((std::string_view)Target);
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
    auto        pos = result.rfind((std::string_view)Target);
    if (pos != std::string::npos)
    {
      result.erase(pos, Target.length);
    }
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
    while ((pos = result.find((std::string_view)From, pos)) != std::string::npos)
    {
      result.replace(pos, From.length, (std::string_view)To);
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

struct to_upper
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result{name};
    std::ranges::transform(result, result.begin(),
                           [](char a)
                           {
                             return static_cast<char>(std::toupper(a));
                           });
    return result;
  }
};

struct to_lower
{
  using is_string_transform = std::true_type;

  constexpr static auto transform(std::string_view name) -> std::string
  {
    std::string result{name};
    std::ranges::transform(result, result.begin(),
                           [](char a)
                           {
                             return static_cast<char>(std::tolower(a));
                           });
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
      if (std::isalnum(c) != 0)
      {
        if (capitalize)
        {
          result += static_cast<char>(std::toupper(c));
          capitalize = false;
        }
        else
        {
          result += static_cast<char>(std::tolower(c));
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
      if (std::isalnum(c) != 0)
      {
        if ((std::isupper(c) != 0) && !first)
        {
          result += '_';
        }
        result += static_cast<char>(std::tolower(c));
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
      result[0] = static_cast<char>(std::tolower(result[0]));
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

} // namespace acl