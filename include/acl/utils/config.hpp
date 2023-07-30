
#pragma once

#include <cassert>

#if !defined(NDEBUG) || defined(_DEBUG)
#define ACL_DEBUG 1
#endif

namespace acl::detail
{
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

#ifdef ACL_THROW_ON_ERROR
#ifdef ACL_DEBUG
#define ACL_ASSERT(expression) assert(expression)
#else
#include <cstdio>
#include <cstdlib>
#define ACL_STRINGIFY_LINE(x) #x
#define ACL_ASSERT_IMPL(file, line, expression)                                                                        \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!(expression))                                                                                                 \
    {                                                                                                                  \
      std::printf("failed: @" file ":" ACL_STRINGIFY_LINE(line) " expression: " #expression);                          \
      std::abort();                                                                                                    \
    }                                                                                                                  \
  }                                                                                                                    \
  while (0)

#define ACL_ASSERT(expression) ACL_ASSERT_IMPL(__FILE__, __LINE__, expression)
#endif
#else
#define ACL_ASSERT(expression) assert(expression)
#endif

#ifdef _MSC_VER
#if defined(_M_X64) || defined(_M_IA64)
#define ACL_PACK_TAGGED_POINTER
#endif
#endif

#ifdef __GNUC__
#if defined(__x86_64__)
#define ACL_PACK_TAGGED_POINTER
#endif
#endif

/**
 * Before including any config.hpp, if ACL_API is defined from a dll to exp
 */
#ifndef ACL_API
#define ACL_API
#endif
