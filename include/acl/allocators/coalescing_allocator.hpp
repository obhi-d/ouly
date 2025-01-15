
#pragma once

#include <acl/allocators/config.hpp>
#include <limits>
#include <vector>

namespace acl
{

using coalescing_allocator_size_type = std::conditional_t<cfg::coalescing_allocator_large_size, uint64_t, uint32_t>;

/**
 * @brief This allocator grows the buffer size, and merges free sizes.
 */
/**
 * @brief A memory allocator that merges adjacent free blocks to reduce fragmentation
 *
 * The coalescing allocator maintains a list of free memory blocks and combines
 * adjacent free blocks when memory is deallocated to prevent memory fragmentation.
 * It tracks blocks using offset and size pairs.
 *
 * @tparam size_type The type used for memory sizes and offsets
 *
 * Key features:
 * - Merges adjacent free blocks on deallocation
 * - Tracks memory using offset/size pairs
 * - Manages a sorted list of free blocks
 * - Suitable for scenarios requiring defragmented memory allocation
 *
 * @note The allocator starts with one maximum-sized free block
 */
class coalescing_allocator
{
public:
  using size_type = coalescing_allocator_size_type;

  auto allocate(size_type size) -> size_type;
  void deallocate(size_type offset, size_type size);

private:
  // Free blocks
  std::vector<size_type> offsets_ = {0};
  std::vector<size_type> sizes_   = {std::numeric_limits<size_type>::max()};
};

} // namespace acl
