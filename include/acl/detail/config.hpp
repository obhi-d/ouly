
#pragma once

#if !defined(NDEBUG) || defined(_DEBUG)
#define ACL_DEBUG 1
#endif

namespace acl
{
namespace detail
{
static inline constexpr bool debug = ACL_DEBUG;
}
} // namespace acl
