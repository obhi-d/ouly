
#pragma once

#include <acl/utils/config.hpp>
#include <compare>
#include <cstdint>
#include <limits>
#include <vector>

namespace acl
{

using coalescing_allocator_size_type = std::conditional_t<detail::coalescing_allocator_large_size, uint64_t, uint32_t>;

/**
 * @brief This allocator grows the buffer size, and merges free sizes.
 */
class coalescing_allocator
{
public:
  using size_type = coalescing_allocator_size_type;

  size_type allocate(size_type size);
  void      deallocate(size_type offset, size_type size);

private:
  // Free blocks
  std::vector<size_type> offsets_ = {0};
  std::vector<size_type> sizes_   = {std::numeric_limits<size_type>::max()};
};

} // namespace acl
