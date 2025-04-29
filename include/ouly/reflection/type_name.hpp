// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/reflection/detail/field_helpers.hpp"

namespace ouly
{
template <typename T>
[[nodiscard]] constexpr auto type_name() -> decltype(auto)
{
#if defined(__clang__)
  std::string_view constexpr start = "T = ";
  std::string_view constexpr end   = "]";
#elif defined(__GNUC__) || defined(__GNUG__)
  std::string_view constexpr start = "T = ";
  std::string_view constexpr end   = ";";
#elif defined(_MSC_VER)
  std::string_view constexpr start = "type_name<";
  std::string_view constexpr end   = ">(void)";
#endif
  constexpr std::string_view name = ouly::detail::function_name_type<T>();
  // return name;
  constexpr auto start_pos = name.find(start) + start.size();
  constexpr auto type      = name.substr(start_pos, name.find(end, start_pos) - start_pos);

  constexpr auto length = type.length();

  // NOLINTNEXTLINE
  return string_literal<length + 1>{type.data()};
}

template <typename T>
constexpr auto type_hash() -> std::uint32_t
{
  return type_name<T>().hash();
}

} // namespace ouly