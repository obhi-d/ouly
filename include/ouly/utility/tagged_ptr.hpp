// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/utility/detail/compressed_ptr.hpp"

namespace ouly
{

#ifdef OULY_PACK_TAGGED_POINTER
template <typename T>
using tagged_ptr = ouly::detail::compressed_ptr<T>;
#else
template <typename T>
using tagged_ptr = ouly::detail::tagged_ptr<T>;
#endif

} // namespace ouly
