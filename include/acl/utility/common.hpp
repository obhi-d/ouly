#pragma once

#include <acl/utility/type_traits.hpp>
#include <cstdint>
#include <cstring>
#include <string>

#if defined(_MSC_VER)
#include <intrin.h>
#define ACL_EXPORT      __declspec(dllexport)
#define ACL_IMPORT      __declspec(dllimport)
#define ACL_EMPTY_BASES __declspec(empty_bases)
#else
#define ACL_EXPORT __attribute__((visibility("default")))
#define ACL_IMPORT __attribute__((visibility("default")))
#define ACL_EMPTY_BASES
#endif

#ifdef ACL_DLL_IMPL
#ifdef ACL_EXPORT_SYMBOLS
#define ACL_API ACL_EXPORT
#else
#define ACL_API ACL_IMPORT
#endif
#else
#define ACL_API
#endif

#if _DEBUG
#define ACL_VALIDITY_CHECKS
#endif

#ifndef ACL_PRINT_DEBUG
#define ACL_PRINT_DEBUG acl::print_debug_info
#endif

#define ACL_EXTERN extern "C"

namespace acl
{
constexpr std::uint32_t safety_offset = alignof(void*);

inline void print_debug_info(std::string const& s)
{
  /*ignored*/
}
template <bool B, typename T>
constexpr void typed_static_assert()
{
  static_assert(B, "static assert failed - check below ->");
}
} // namespace acl
