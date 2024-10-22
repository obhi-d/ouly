#pragma once

#include <acl/utils/utils.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#if defined(__clang__)
#define ACL_FUNC_NAME  __PRETTY_FUNCTION__
#define ACL_FUNC_START "T = "
#define ACL_FUNC_END   "]"
#elif defined(__GNUC__) || defined(__GNUG__)
#define ACL_FUNC_NAME  __PRETTY_FUNCTION__
#define ACL_FUNC_START "T = "
#define ACL_FUNC_END   ";"
#elif defined(_MSC_VER)
#define ACL_FUNC_NAME  __FUNCSIG__
#define ACL_FUNC_START "type_name<"
#define ACL_FUNC_END   ">(void)"
#endif

namespace acl::detail
{
template <typename T>
constexpr std::string_view type_name()
{
  constexpr std::string_view name = ACL_FUNC_NAME;
  // return name;
  constexpr auto start = name.find(ACL_FUNC_START) + sizeof(ACL_FUNC_START) - 1;
  return name.substr(start, name.find(ACL_FUNC_END, start) - start);
}

template <typename T>
constexpr std::uint32_t type_hash()
{
  return fnv1a_32(type_name<T>());
}

template <bool B, typename T>
inline constexpr void typed_static_assert()
{
  static_assert(B, ACL_FUNC_NAME ": assert failed");
}

} // namespace acl::detail
