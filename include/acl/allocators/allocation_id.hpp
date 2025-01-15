
#pragma once

#include <compare>
#include <cstdint>
#include <limits>

#include <acl/allocators/config.hpp>

namespace acl
{

/**
 * @brief This allocator grows the buffer size, and merges free sizes_.
 */
struct allocation_id
{
  uint32_t id_ = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] auto get() const noexcept -> uint32_t
  {
    return id_;
  }

  auto operator<=>(allocation_id const&) const noexcept = default;
};

struct arena_id
{
  uint16_t id_ = std::numeric_limits<uint16_t>::max();

  [[nodiscard]] auto get() const noexcept -> uint16_t
  {
    return id_;
  }
  auto operator<=>(arena_id const&) const noexcept = default;
};

using allocation_size_type = std::conditional_t<cfg::coalescing_allocator_large_size, uint64_t, uint32_t>;

} // namespace acl