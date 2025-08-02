#pragma once
// SPDX-License-Identifier: MIT

#include "ouly/scheduler/auto_parallel_for.hpp"
#include "ouly/scheduler/default_parallel_for.hpp"

namespace ouly
{
template <typename L, typename FwIt, TaskContext WC, typename Traits = default_partitioner_traits>
void parallel_for(L lambda, FwIt&& range, WC const& this_context, Traits /*unused*/ = {})
{
  if constexpr (AutoParitionerTraits<Traits>)
  {
    auto_parallel_for(lambda, std::forward<FwIt>(range), this_context, Traits{});
    return;
  }

  default_parallel_for(lambda, std::forward<FwIt>(range), this_context, Traits{});
}
} // namespace ouly