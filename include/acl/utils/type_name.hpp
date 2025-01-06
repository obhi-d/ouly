#pragma once

#include <acl/utils/utils.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#if defined(__clang__)
#define ACL_FUNC_NAME  __PRETTY_FUNCTION__ // NOLINT
#define ACL_FUNC_START "T = "              // NOLINT
#define ACL_FUNC_END   "]"                 // NOLINT
#elif defined(__GNUC__) || defined(__GNUG__)
#define ACL_FUNC_NAME  __PRETTY_FUNCTION__ // NOLINT
#define ACL_FUNC_START "T = "              // NOLINT
#define ACL_FUNC_END   ";"                 // NOLINT
#elif defined(_MSC_VER)
#define ACL_FUNC_NAME  __FUNCSIG__  // NOLINT
#define ACL_FUNC_START "type_name<" // NOLINT
#define ACL_FUNC_END   ">(void)"    // NOLINT
#endif

namespace acl::detail
{
template <typename T>
constexpr auto type_name() -> std::string_view
{
  constexpr std::string_view name = ACL_FUNC_NAME;
  // return name;
  constexpr auto start = name.find(ACL_FUNC_START) + sizeof(ACL_FUNC_START) - 1;
  return name.substr(start, name.find(']', start) - start);
}

template <typename T>
constexpr auto type_hash() -> std::uint32_t
{
  return fnv1a_32(type_name<T>());
}

template <bool B, typename T>
constexpr void typed_static_assert()
{
  static_assert(B, "static assert failed - check below ->");
}

} // namespace acl::detail
