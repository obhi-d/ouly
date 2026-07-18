// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/coalescing_arena_allocator.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <vector>

namespace ouly
{

/**
 * @brief Best-fit arena allocator (`coalescing_arena_allocator`) extended with alignment support
 * and defragmentation.
 *
 * Allocation and deallocation use the base allocator's best-fit-by-size machinery. Alignment is
 * handled the way `arena_allocator` does it: the block is over-allocated by `alignment - 1` bytes and
 * the returned offset is rounded up, so `get_offset` on the base class returns the raw block offset
 * while `ca_allocation::get_offset()` and `get_adjusted_offset` return the usable, aligned offset.
 *
 * `defragment` compacts allocations towards the front of earlier arenas, drops emptied arenas and
 * reports moves and relocations through the manager. Allocation ids remain stable across
 * defragmentation, only their arena/offset change.
 *
 * @note Always allocate through this class (not through a base class reference) so that
 * per-allocation alignment is recorded for defragmentation.
 * @note This class is meant for virtual allocations, for example GPU memory management.
 */
class best_fit_defrag_allocator : public coalescing_arena_allocator
{
public:
  using size_type = allocation_size_type;

  using coalescing_arena_allocator::coalescing_arena_allocator;

  /** @brief Given an allocation_id return the aligned offset of the allocation within its arena */
  [[nodiscard]] OULY_API auto get_adjusted_offset(allocation_id id) const noexcept -> size_type;

  /** @brief Given an allocation_id return the usable size (block size minus alignment slack) */
  [[nodiscard]] OULY_API auto get_adjusted_size(allocation_id id) const noexcept -> size_type;

  /** @brief Given an allocation_id return its alignment in bytes */
  [[nodiscard]] OULY_API auto get_alignment(allocation_id id) const noexcept -> size_type;

  /**
   * @brief Allocate `size` bytes with optional alignment. The returned offset is aligned; the
   * underlying block is over-allocated by `alignment - 1` bytes to guarantee the fit.
   */
  template <CoalescingMemoryManager M, typename Alignment = ouly::alignment<>, typename Dedicated = std::false_type>
  auto allocate(size_type size, M& manager, Alignment alignment = {}, Dedicated dedicated = {}) -> ca_allocation
  {
    auto const align_value = static_cast<size_type>(alignment);
    auto const mask        = align_value > 1 ? align_value - 1 : size_type{0};

    auto al = coalescing_arena_allocator::allocate(size + mask, manager, ouly::alignment<>{}, dedicated);
    if (al.get_allocation_id() == allocation_id())
    {
      return al;
    }
    auto const id = al.get_allocation_id().get();
    if (alignments_.size() <= id)
    {
      alignments_.resize(static_cast<size_t>(id) + 1, 0);
    }
    // dedicated allocations are pinned: defragment never relocates them out of their arena
    bool const is_dedicated = Dedicated::value || size + mask >= get_arena_size();
    alignments_[id]         = static_cast<uint8_t>(std::popcount(mask) | (is_dedicated ? dedicated_bit : 0));
    al.offset_              = (al.offset_ + mask) & ~mask;
    return al;
  }

  /**
   * @brief Compact allocations towards the front of earlier arenas and drop emptied arenas.
   *
   * Blocks are processed in (arena, offset) order and repacked to the lowest raw offset they fit;
   * because of this ordering, same-arena moves always shift data towards lower offsets and the emitted
   * `move_memory` sequence never overwrites data that has not been moved yet (individual calls can
   * still self-overlap and must behave like memmove). Allocation ids remain valid; the manager is told
   * about relocations through `rebind_alloc` with the new aligned offset.
   *
   * @param max_bytes_to_move Optional budget; once moving another allocation would exceed it, all
   * remaining allocations stay in place and `completed_` is false in the result.
   */
  template <typename M>
    requires(CoalescingDefragMemoryManager<M, best_fit_defrag_allocator>)
  auto defragment(M& manager, size_type max_bytes_to_move = std::numeric_limits<size_type>::max())
   -> coalescing_defrag_result
  {
    manager.begin_defragment(*this);
    coalescing_defrag_result result;

    // Snapshot arena order; lists are rebuilt below
    std::vector<uint16_t> order;
    for (auto arena = arena_list().front(); arena != 0; arena = arena_list().next(arena_entries(), arena))
    {
      order.push_back(static_cast<uint16_t>(arena));
    }

    std::vector<placement> plan;
    plan.reserve(block_entries().offsets_.size());
    std::vector<defrag_move> moves;
    std::vector<uint32_t>    rebinds;
    std::vector<uint32_t>    old_free_blocks;
    std::vector<size_type>   cursor(arena_entries().entries_.size(), 0);
    bool                     budget_left = true;

    for (uint32_t pos = 0; pos < static_cast<uint32_t>(order.size()); ++pos)
    {
      auto const arena_idx = order[pos];
      auto const blocks    = ouly::detail::vector_access(arena_entries().entries_, arena_idx).blocks_;
      for (auto block = blocks.front(); block != 0; block = blocks.next(block_entries(), block))
      {
        if (ouly::detail::vector_access(block_entries().free_marker_, block))
        {
          old_free_blocks.push_back(block);
          continue;
        }
        auto const raw_size = ouly::detail::vector_access(block_entries().sizes_, block);
        auto const from_raw = ouly::detail::vector_access(block_entries().offsets_, block);
        bool const movable  = budget_left && !is_dedicated(block);
        auto [dst, to] = movable ? find_placement(order, pos, cursor, raw_size) : std::make_pair(arena_idx, from_raw);
        bool const relocates = dst != arena_idx || to != from_raw;
        auto const mask      = mask_of(block);

        if (relocates && result.bytes_moved_ + (raw_size - mask) > max_bytes_to_move)
        {
          result.completed_ = false;
          budget_left       = false;
          dst               = arena_idx;
          to                = from_raw;
        }
        else if (relocates)
        {
          result.bytes_moved_ += raw_size - mask;
          result.allocations_moved_++;
          push_move(moves, {.from_ = (from_raw + mask) & ~mask,
                            .to_   = (to + mask) & ~mask,
                            .size_ = raw_size - mask,
                            .src_  = arena_idx,
                            .dst_  = dst});
          rebinds.push_back(block);
          ouly::detail::vector_access(block_entries().arenas_, block) = dst;
        }
        cursor[dst]                                                  = to + raw_size;
        ouly::detail::vector_access(block_entries().offsets_, block) = to;
        plan.push_back({.to_ = to, .size_ = raw_size, .block_ = block, .dst_ = dst});
      }
    }
    result.moves_ = static_cast<uint32_t>(moves.size());

    // Recycle the old free block entries; gap and tail blocks below will reuse the slots
    for (auto block : old_free_blocks)
    {
      ouly::detail::vector_access(block_entries().offsets_, block) = block_entries().free_idx_;
      block_entries().free_idx_                                    = block;
    }

    rebuild_lists(order, plan);
    auto removed           = drop_empty_arenas(order);
    result.arenas_removed_ = static_cast<uint32_t>(removed.size());

    for (auto const& m : moves)
    {
      manager.move_memory(arena_id{m.src_}, arena_id{m.dst_}, m.from_, m.to_, m.size_);
    }
    for (auto block : rebinds)
    {
      auto const mask = mask_of(block);
      manager.rebind_alloc(allocation_id{block}, arena_id{ouly::detail::vector_access(block_entries().arenas_, block)},
                           (ouly::detail::vector_access(block_entries().offsets_, block) + mask) & ~mask);
    }
    for (auto arena_idx : removed)
    {
      manager.remove(arena_id{arena_idx});
    }

    manager.end_defragment(*this);
    return result;
  }

private:
  struct placement
  {
    size_type to_;
    size_type size_;
    uint32_t  block_;
    uint16_t  dst_;
  };

  struct defrag_move
  {
    size_type from_;
    size_type to_;
    size_type size_;
    uint16_t  src_;
    uint16_t  dst_;
  };

  /** Earliest already-processed arena with room at the pack cursor, falling back to the own arena. */
  [[nodiscard]] OULY_API auto find_placement(std::vector<uint16_t> const& order, uint32_t pos,
                                             std::vector<size_type> const& cursor, size_type raw_size) const
   -> std::pair<uint16_t, size_type>;

  /** Rebuild per-arena block lists and the size-sorted free list from the placement plan. */
  OULY_API void rebuild_lists(std::vector<uint16_t> const& order, std::vector<placement>& plan);

  /** Unlink arenas that no longer host any block and recycle their entries. */
  OULY_API auto drop_empty_arenas(std::vector<uint16_t> const& order) -> std::vector<uint16_t>;

  static OULY_API void push_move(std::vector<defrag_move>& moves, defrag_move value);

  // high bit of an alignments_ entry marks a pinned (dedicated) allocation
  static constexpr uint8_t dedicated_bit = 0x80;

  [[nodiscard]] OULY_API auto mask_of(uint32_t block) const noexcept -> size_type;

  [[nodiscard]] OULY_API auto is_dedicated(uint32_t block) const noexcept -> bool;

  // log2(alignment) per allocated block id (high bit: pinned); free-list split blocks are never read
  std::vector<uint8_t> alignments_;
};

} // namespace ouly
