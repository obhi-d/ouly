
#pragma once

namespace acl::detail
{

template <typename T>
concept HasMemoryManager = requires { typename T::manager_t; };

template <typename T>
concept HasAllocStrategy = requires { typename T::strategy_t; };

template <typename O>
concept HasBsearchAlgo = requires { O::bsearch_algo; };

} // namespace acl::detail