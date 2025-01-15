
#pragma once

#include <cstddef>
#include <type_traits>

namespace acl::detail
{
template <typename T>
struct allocator_common
{
  using value_type      = T;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference       = value_type&;
  using const_reference = value_type const&;
  using pointer         = value_type*;
  using const_pointer   = value_type const*;

  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap            = std::true_type;
};
} // namespace acl::detail