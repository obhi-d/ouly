
#pragma once

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
