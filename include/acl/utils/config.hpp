
#pragma once

#include <cassert>

#if !defined(NDEBUG) || defined(_DEBUG)
#define ACL_DEBUG
#endif

namespace acl::detail
{
inline static constexpr bool coalescing_allocator_large_size = false;
#ifdef ACL_DEBUG
inline static constexpr bool debug = true;
#else
inline static constexpr bool debug = false;
#endif
} // namespace acl::detail

#ifdef _MSC_VER
#define ACL_POTENTIAL_EMPTY_MEMBER [[msvc::no_unique_address]]
#else
#define ACL_POTENTIAL_EMPTY_MEMBER [[no_unique_address]]
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>

#ifdef _MSC_VER
#if defined(_M_X64) || defined(_M_IA64)
#define ACL_PACK_TAGGED_POINTER
#endif // defined(_M_X64) || defined(_M_IA64)
#endif // _MSC_VER

#ifdef __GNUC__
#if defined(__x86_64__)
#define ACL_PACK_TAGGED_POINTER
#endif // __GNUC__
#endif // defined(__x86_64__)

/**
 * Before including any config.hpp, if ACL_API is defined from a dll to exp
 */
#ifndef ACL_API
#define ACL_API
#endif
