#pragma once

#include <acl/utility/detail/compressed_ptr.hpp>

namespace acl
{

#ifdef ACL_PACK_TAGGED_POINTER
template <typename T>
using tagged_ptr = acl::detail::compressed_ptr<T>;
#else
template <typename T>
using tagged_ptr = acl::detail::tagged_ptr<T>;
#endif

} // namespace acl
