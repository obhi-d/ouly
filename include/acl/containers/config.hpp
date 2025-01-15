
#pragma once

#include <acl/containers/detail/blackboard_defs.hpp>
#include <typeindex>

namespace acl::cfg
{
/**
 * Provide a custom blackboard hash map lookup implementation.
 */
template <acl::detail::BlackboardHashMap H>
struct name_val_map
{
  using name_map_type = H;
};

/**
 * This option allows blackboard to customize the <type key> by using std::type_index, you are free to provide
 * the hash map implementation and replace std::unordered_map by your own class if it satisfies
 *
 */
template <template <typename K, typename V> typename H>
struct map
{
  using name_map_type = H<std::type_index, blackboard_offset>;
};

/**
 * This option allows you to implement your own key type, the offset is always required to be blackboard_offset type and
 * provided by blackboard.
 */
template <template <typename V> typename H>
struct name_map
{
  using name_map_type = H<blackboard_offset>;
};

} // namespace acl::cfg
