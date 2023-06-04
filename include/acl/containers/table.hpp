#pragma once

#include <acl/detail/table.hpp>

namespace acl
{

template <typename Type, bool IsPOD = std::is_trivial_v<Type>>
using table = detail::table<Type, IsPOD>;

} // namespace acl