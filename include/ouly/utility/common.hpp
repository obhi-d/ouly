#pragma once

#include "ouly/utility/type_traits.hpp"
#include <cstdint>
#include <cstring>
#include <string>

#if defined(_MSC_VER)
#include <intrin.h>
#define OULY_EXPORT      __declspec(dllexport)
#define OULY_IMPORT      __declspec(dllimport)
#define OULY_EMPTY_BASES __declspec(empty_bases)
#else
#define OULY_EXPORT __attribute__((visibility("default")))
#define OULY_IMPORT __attribute__((visibility("default")))
#define OULY_EMPTY_BASES
#endif

#ifdef OULY_DLL_IMPL
#ifdef OULY_EXPORT_SYMBOLS
#define OULY_API OULY_EXPORT
#else
#define OULY_API OULY_IMPORT
#endif
#else
#define OULY_API
#endif

#ifdef _DEBUG
#define OULY_VALIDITY_CHECKS
#endif

#ifndef OULY_PRINT_DEBUG
#define OULY_PRINT_DEBUG ouly::print_debug_info
#endif

#define OULY_EXTERN extern "C"

namespace ouly
{
constexpr std::uint32_t safety_offset = alignof(void*);

inline void print_debug_info(std::string const& /*s*/)
{
  /*ignored*/
}
template <bool B, typename T>
constexpr void typed_static_assert()
{
  static_assert(B, "static assert failed - check below ->");
}
} // namespace ouly
