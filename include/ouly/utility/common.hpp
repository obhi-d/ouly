// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/utility/type_traits.hpp"
#include <cstdint>
#include <cstring>
#include <string>

#ifdef _MSC_VER
#include <xmmintrin.h>
#endif

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

#ifndef OULY_PRINT_ERROR
#define OULY_PRINT_ERROR ouly::print_error
#endif

#define OULY_EXTERN extern "C"

namespace ouly
{
constexpr std::uint32_t safety_offset = alignof(void*);

inline void print_debug_info(std::string_view /*s*/)
{
  /*ignored*/
}
inline void print_error(std::string_view /*msg*/)
{
  /*ignored */
}

template <bool B, typename T>
constexpr void typed_static_assert()
{
  static_assert(B, "static assert failed - check below ->");
}

/**
 * @brief Prefetch memory for better cache performance
 * @param addr Memory address to prefetch
 */
inline void prefetch_for_write(void* addr) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
  __builtin_prefetch(addr, 1, 3); // prefetch for write, high temporal locality
#elif defined(_MSC_VER)
  _mm_prefetch(static_cast<const char*>(addr), 0);
#endif
}
/**
 * @brief Prefetch memory for better cache performance
 * @param addr Memory address to prefetch
 */
inline void prefetch_for_read(void* addr) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
  __builtin_prefetch(addr, 0, 1); // prefetch for read, high temporal locality
#elif defined(_MSC_VER)
  _mm_prefetch(static_cast<const char*>(addr), 0);
#endif
}

/**
 * @brief Align a size value up to the allocator's alignment boundary
 * @param value Size to align
 * @return Aligned size (multiple of alignment)
 */
template <std::size_t Alignment = alignof(std::max_align_t)>
static constexpr auto align_up(std::size_t value) noexcept -> std::size_t
{
  return (value + Alignment - 1U) & ~(Alignment - 1U);
}

/**
 * @brief Check if a size is already aligned
 * @param value Size to check
 * @return true if already aligned, false otherwise
 */
template <std::size_t Alignment = alignof(std::max_align_t)>
static constexpr auto is_aligned(std::size_t value) noexcept -> bool
{
  return (value & (Alignment - 1U)) == 0;
}

} // namespace ouly
