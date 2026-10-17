// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/allocation_id.hpp"
#include "ouly/allocators/config.hpp"
#include "ouly/allocators/detail/tlsf_bins.hpp"
#include "ouly/utility/user_config.hpp"
#include <concepts>
#include <cstdint>
#include <limits>
#include <vector>

namespace ouly
{

using coalescing_allocator_size_type = std::conditional_t<cfg::coalescing_allocator_large_size, uint64_t, uint32_t>;

/**
 * @brief O(1) offset-range suballocator with free coalescing and an optional compaction pass.
 *
 * Manages the offsets of a single range `[0, capacity)` (by default the whole size type range, so a
 * user can grow the backing buffer as high offsets are handed out). No memory is touched; the
 * allocator is meant for virtual allocations such as GPU buffer suballocation.
 *
 * Only occupied blocks are stored. Each block owns the free gap behind it and is linked to its
 * neighbours in address order, so freeing coalesces by adding the block and its gap to its
 * predecessor's gap. Gaps are filed in TLSF size bins, which makes both `allocate` and `deallocate`
 * constant time regardless of how many allocations are live.
 *
 * `allocate` returns a stable `allocation_id`; query `get_offset` for the placement. Ids remain valid
 * across `compact`, which slides live allocations towards offset zero.
 */
class coalescing_allocator
{
public:
  using size_type = coalescing_allocator_size_type;

  struct compact_result
  {
    size_type bytes_moved_       = 0;
    uint32_t  moves_             = 0;
    uint32_t  allocations_moved_ = 0;
    bool      completed_         = true;
  };

  OULY_API coalescing_allocator();
  OULY_API explicit coalescing_allocator(size_type capacity);

  /**
   * @brief Allocate `size` bytes at a power of two `alignment`.
   * @return A null (default) `allocation_id` when no gap can hold the request.
   */
  OULY_API auto allocate(size_type size, size_type alignment = 1) -> allocation_id;

  OULY_API void deallocate(allocation_id id);

  /** @brief Current offset of an allocation */
  [[nodiscard]] auto get_offset(allocation_id id) const noexcept -> size_type
  {
    return ouly::detail::vector_access(blocks_, id.get()).offset_;
  }

  /** @brief Size of an allocation */
  [[nodiscard]] auto get_size(allocation_id id) const noexcept -> size_type
  {
    return ouly::detail::vector_access(blocks_, id.get()).size_;
  }

  /**
   * @brief Slide live allocations towards offset zero, closing the gaps between them.
   *
   * Allocations are processed in offset order, so every move shifts data to a lower offset and the
   * emitted sequence never overwrites data that has not been moved yet. Adjacent moves are merged,
   * so an individual `move_memory(from, to, size)` call can self-overlap and must behave like
   * `memmove`. Allocation ids remain valid; query `get_offset` for the new placements.
   *
   * @param move_memory Callback invoked as `move_memory(from, to, size)` for each range to move.
   * @param max_bytes_to_move Optional budget; once moving another allocation would exceed it, all
   * remaining allocations stay in place and `completed_` is false in the result.
   */
  template <typename MoveMemory>
    requires(std::invocable<MoveMemory, size_type, size_type, size_type>)
  auto compact(MoveMemory move_memory, size_type max_bytes_to_move = std::numeric_limits<size_type>::max())
   -> compact_result
  {
    struct move
    {
      size_type from_;
      size_type to_;
      size_type size_;
    };

    compact_result    result;
    std::vector<move> moves;
    size_type         cursor = 0;

    for (auto index = head_block().next_; index != null_index;
         index      = ouly::detail::vector_access(blocks_, index).next_)
    {
      auto&      block = ouly::detail::vector_access(blocks_, index);
      auto const from  = block.offset_;
      if (result.completed_ && from != cursor && result.bytes_moved_ + block.size_ > max_bytes_to_move)
      {
        result.completed_ = false;
      }
      if (result.completed_ && from != cursor)
      {
        result.bytes_moved_ += block.size_;
        ++result.allocations_moved_;
        if (!moves.empty() && moves.back().from_ + moves.back().size_ == from &&
            moves.back().to_ + moves.back().size_ == cursor)
        {
          moves.back().size_ += block.size_;
        }
        else
        {
          moves.push_back({.from_ = from, .to_ = cursor, .size_ = block.size_});
        }
        block.offset_ = cursor;
      }
      cursor = block.offset_ + block.size_;
    }

    if (!moves.empty())
    {
      rebuild_gaps();
    }
    result.moves_ = static_cast<uint32_t>(moves.size());
    for (auto const& m : moves)
    {
      move_memory(m.from_, m.to_, m.size_);
    }
    return result;
  }

  OULY_API void validate_integrity() const;

private:
  static constexpr uint32_t null_index = std::numeric_limits<uint32_t>::max();
  static constexpr uint32_t head_index = 0;

  /** @brief An allocation followed by the free gap it owns; block 0 owns the leading gap */
  struct block_node
  {
    size_type offset_   = 0;
    size_type size_     = 0;
    size_type gap_      = 0;
    uint32_t  prev_     = null_index;
    uint32_t  next_     = null_index;
    uint32_t  bin_prev_ = null_index;
    uint32_t  bin_next_ = null_index;
    uint16_t  bin_      = std::numeric_limits<uint16_t>::max();
    bool      live_     = false;
  };

  using bins_type = ouly::detail::tlsf_bins<block_node>;

  [[nodiscard]] auto head_block() const noexcept -> block_node const&
  {
    return ouly::detail::vector_access(blocks_, head_index);
  }

  OULY_API auto make_block() -> uint32_t;
  OULY_API void rebuild_gaps();

  std::vector<block_node> blocks_;
  bins_type          bins_;
  size_type          capacity_   = 0;
  uint32_t           free_block_ = null_index;
};

} // namespace ouly
