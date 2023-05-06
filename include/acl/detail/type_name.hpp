#pragma once

#if defined(__clang__)
#define ACL_FUNC_NAME  __PRETTY_FUNCTION__
#define ACL_FUNC_START sizeof("string_literal acl::detail::type_name() [T = ") - 1
#define ACL_FUNC_END   sizeof("]") - 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define ACL_FUNC_NAME __PRETTY_FUNCTION__
#define ACL_FUNC_START                                                                                                 \
  sizeof("constexpr acl::string_literal "                                                                              \
         "acl::detail::type_name() [with T = ") -                                                                      \
    1
#define ACL_FUNC_END sizeof("]") - 1
#elif defined(_MSC_VER)
#define ACL_FUNC_NAME __FUNCSIG__
#define ACL_FUNC_START                                                                                                 \
  sizeof("struct acl::string_literal __cdecl "                                                                         \
         "acl::detail::type_name<") -                                                                                  \
    1
#define ACL_FUNC_END sizeof(">(void)") - 1
#endif

#include "../string_literal.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace acl::detail
{

template <typename T>
constexpr string_literal type_name()
{
  return string_literal{ACL_FUNC_NAME}.substring(ACL_FUNC_START, ACL_FUNC_END);
}

template <typename T>
constexpr std::uint32_t type_hash()
{
  return string_literal{ACL_FUNC_NAME}.substring(ACL_FUNC_START, ACL_FUNC_END).hash();
}

} // namespace acl::detail
