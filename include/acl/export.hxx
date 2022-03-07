//! A single file must include this one time, this contains mostly debug info collectors
#pragma once
#include "allocator.hpp"
#include <sstream>

namespace acl::detail
{

#ifdef ACL_REC_STATS

detail::statistics<default_allocator_tag, true> default_allocator_statistics_instance;

#endif

} // namespace acl::detail