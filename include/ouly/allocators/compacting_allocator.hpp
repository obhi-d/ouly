// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/allocation_id.hpp"
#include "ouly/allocators/coalescing_allocator.hpp"
#include "ouly/utility/user_config.hpp"
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <limits>
#include <vector>

namespace ouly
{

/**
 * @brief `coalescing_allocator` with stable allocation ids and a compaction pass.
 *
 * A single growable arena: like `coalescing_allocator`, the offset space starts as one
 * maximum-sized free block and the user grows the backing buffer as high offsets get handed out.
 * Free blocks are kept in two parallel vectors sorted by offset, so deallocation coalesces
 * neighbors with a binary search and allocation is a first-fit scan.
 *
 * Unlike `coalescing_allocator`, `allocate` returns an `allocation_id` handle; the current offset
 * of an allocation is queried with `get_offset`. This is what allows `compact` to slide live
 * allocations towards offset zero: ids remain stable across compaction, only offsets change.
 *
 * @note This class is meant for virtual allocations, for example GPU memory management. No actual
 * memory is touched by the allocator; `compact` reports the ranges to move through a callback.
 */
class compacting_allocator
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

  /** @brief Allocate `size` bytes. Returns a null (default) `allocation_id` on exhaustion. */
  OULY_API auto allocate(size_type size) -> allocation_id;

  OULY_API void deallocate(allocation_id id);

  /** @brief Given an allocation_id return the current offset of the allocation */
  [[nodiscard]] OULY_API auto get_offset(allocation_id id) const noexcept -> size_type;

  /** @brief Given an allocation_id return the allocation size */
  [[nodiscard]] OULY_API auto get_size(allocation_id id) const noexcept -> size_type;

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
    compact_result result;

    struct item
    {
      size_type offset_;
      uint32_t  id_;
    };
    std::vector<item> items;
    items.reserve(entry_offsets_.size());
    for (uint32_t id = 1; id < static_cast<uint32_t>(entry_offsets_.size()); ++id)
    {
      if (ouly::detail::vector_access(entry_live_, id))
      {
        items.push_back({.offset_ = ouly::detail::vector_access(entry_offsets_, id), .id_ = id});
      }
    }
    std::ranges::sort(items,
                      [](item const& a, item const& b) -> bool
                      {
                        return a.offset_ < b.offset_;
                      });

    struct move
    {
      size_type from_;
      size_type to_;
      size_type size_;
    };
    std::vector<move> moves;
    size_type         cursor      = 0;
    bool              budget_left = true;

    free_offsets_.clear();
    free_sizes_.clear();

    for (auto const& it : items)
    {
      auto const size = ouly::detail::vector_access(entry_sizes_, it.id_);
      auto       to   = budget_left ? cursor : it.offset_;

      if (to != it.offset_ && result.bytes_moved_ + size > max_bytes_to_move)
      {
        result.completed_ = false;
        budget_left       = false;
        to                = it.offset_;
      }
      if (to != it.offset_)
      {
        result.bytes_moved_ += size;
        result.allocations_moved_++;
        if (!moves.empty() && moves.back().from_ + moves.back().size_ == it.offset_ &&
            moves.back().to_ + moves.back().size_ == to)
        {
          moves.back().size_ += size;
        }
        else
        {
          moves.push_back({.from_ = it.offset_, .to_ = to, .size_ = size});
        }
        ouly::detail::vector_access(entry_offsets_, it.id_) = to;
      }
      // rebuild the free list from the gap left before this allocation's final position
      if (to > cursor)
      {
        free_offsets_.push_back(cursor);
        free_sizes_.push_back(to - cursor);
      }
      cursor = to + size;
    }
    if (cursor != std::numeric_limits<size_type>::max())
    {
      free_offsets_.push_back(cursor);
      free_sizes_.push_back(std::numeric_limits<size_type>::max() - cursor);
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
  OULY_API auto push_entry(size_type offset, size_type size) -> uint32_t;

  OULY_API void free_entry(uint32_t id);

  // free blocks sorted by offset; the offset space starts as one maximum-sized block
  std::vector<size_type> free_offsets_ = {0};
  std::vector<size_type> free_sizes_   = {std::numeric_limits<size_type>::max()};

  // allocation table; slot 0 is a sentinel so id 0 can terminate the free chain
  std::vector<size_type> entry_offsets_ = {0};
  std::vector<size_type> entry_sizes_   = {0};
  std::vector<bool>      entry_live_    = {false};
  uint32_t               free_entry_    = 0;
};

} // namespace ouly
