
#pragma once

#include <concepts>
#include <cstdint>

/**
 * @file arena_options.hpp
 * @brief Configuration options for arena allocator.
 *
 * This header defines configuration options that can be used to customize the behavior
 * of arena allocators. These options are implemented as template structs that hold
 * compile-time constants or types.
 *
 * Available options:
 * @struct granularity
 * Defines the minimum allocation size/alignment. All allocations will be rounded up
 * to the nearest multiple of this value.
 *
 * @struct max_bucket
 * Specifies the maximum size category/bucket that the arena will handle.
 * Requests larger than this will be handled by a fallback allocator.
 *
 * @struct search_window
 * Determines how many slots the allocator will search through before giving up
 * and trying a different strategy. Affects allocation performance and memory utilization.
 *
 * @struct fallback_start
 * Specifies the fallback allocation strategy to use when the primary allocation
 * method fails or when requests exceed max_bucket size.
 *
 * @struct fixed_max_per_slot
 * Defines the maximum number of allocations that can be stored in a single slot.
 * Used for fixed-size allocation strategies.
 */

namespace acl::opt
{
template <std::size_t Value>
struct granularity
{
  static constexpr std::size_t granularity_v = Value;
};

template <std::size_t Value>
struct max_bucket
{
  static constexpr std::size_t max_bucket_v = Value;
};

template <std::size_t Value>
struct search_window
{
  static constexpr std::size_t search_window_v = Value;
};

template <typename T>
struct fallback_start
{
  using fallback_strat_t = T;
};

template <std::size_t Value>
struct fixed_max_per_slot
{
  static constexpr std::size_t fixed_max_per_slot_v = Value;
};

} // namespace acl::opt
