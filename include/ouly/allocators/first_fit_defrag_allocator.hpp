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
 * @brief First-fit arena allocator with external memory management and defragmentation support.
 *
 * Memory is organized in arenas obtained from a `CoalescingMemoryManager`. Each arena keeps its free
 * blocks in two parallel vectors sorted by offset (the `coalescing_allocator` scheme), so
 * deallocation coalesces neighbors with a binary search and allocation is a first-fit scan.
 * Allocations are tracked by `allocation_id` so they can be relocated during defragmentation.
 *
 * Compared to `best_fit_defrag_allocator`: allocation is first-fit over offset-ordered free lists
 * instead of best-fit by size, and alignment is exact-fit — no bytes are over-allocated, so
 * `get_offset` directly returns the aligned offset.
 *
 * @note This class is meant for virtual allocations, for example GPU memory management. No actual
 * memory is touched by the allocator; the manager owns the backing arenas.
 *
 * Key features:
 * - Exact-fit alignment support: alignment padding stays in the free list instead of being
 *   permanently attached to the allocation.
 * - Coalesces adjacent free blocks on deallocation.
 * - Arenas that become empty are returned to the manager.
 * - `defragment` compacts allocations towards the front of earlier arenas, drops emptied arenas and
 *   reports moves/rebinds through the manager. Allocation ids remain stable across defragmentation.
 * - An optional byte budget makes defragmentation incremental, which is useful to bound GPU copy
 *   bandwidth per frame.
 */
class first_fit_defrag_allocator
{
public:
  using size_type = allocation_size_type;

  OULY_API first_fit_defrag_allocator() noexcept;
  OULY_API explicit first_fit_defrag_allocator(size_type arena_sz) noexcept;

  /** @brief Arena size can be changed any time with this method, but it can only increase in size. */
  OULY_API void set_arena_size(size_type s) noexcept;

  [[nodiscard]] OULY_API auto get_arena_size() const noexcept -> size_type;

  /** @brief Given an allocation_id return the offset within its arena (already aligned) */
  [[nodiscard]] OULY_API auto get_offset(allocation_id id) const noexcept -> size_type;

  /** @brief Given an allocation_id return the allocation size */
  [[nodiscard]] OULY_API auto get_size(allocation_id id) const noexcept -> size_type;

  /** @brief Given an allocation_id return the arena it belongs to */
  [[nodiscard]] OULY_API auto get_arena(allocation_id id) const noexcept -> arena_id;

  /** @brief Given an allocation_id return its alignment in bytes */
  [[nodiscard]] OULY_API auto get_alignment(allocation_id id) const noexcept -> size_type;

  /**
   * @brief Allocate `size` bytes with optional alignment. A new arena is requested from the manager
   * when no existing arena can satisfy the request. Dedicated allocations (or those larger than the
   * arena size) get an arena of their own.
   */
  template <CoalescingMemoryManager M, typename Alignment = ouly::alignment<>, typename Dedicated = std::false_type>
  auto allocate(size_type size, M& manager, Alignment alignment = {}, Dedicated /*unused*/ = {}) -> ca_allocation
  {
    OULY_ASSERT(size != 0);
    auto const align_value = static_cast<size_type>(alignment);
    auto const mask        = align_value > 1 ? align_value - 1 : size_type{0};

    if (Dedicated::value || size + mask >= arena_size_)
    {
      auto  arena = create_arena(size, manager, false);
      auto& ar    = ouly::detail::vector_access(arena_pool_, arena);
      ar.allocs_  = 1;
      auto id     = push_entry(0, size, arena, static_cast<uint8_t>(std::popcount(mask)));
      // dedicated allocations are pinned: defragment never relocates them out of their arena
      ouly::detail::vector_access(entry_dedicated_, id) = true;
      return ca_allocation{.offset_ = 0, .id_ = {.id_ = id}, .arena_ = {.id_ = arena}};
    }

    for (auto arena : arena_order_)
    {
      auto& ar = ouly::detail::vector_access(arena_pool_, arena);
      if (ar.free_ < size)
      {
        continue;
      }
      if (auto al = try_allocate(arena, ar, size, mask); al.get_allocation_id() != allocation_id())
      {
        return al;
      }
    }

    auto  arena = create_arena(arena_size_, manager, true);
    auto& ar    = ouly::detail::vector_access(arena_pool_, arena);
    auto  al    = try_allocate(arena, ar, size, mask);
    OULY_ASSERT(al.get_allocation_id() != allocation_id());
    return al;
  }

  /** @brief Deallocate an allocation. The manager is notified when an arena becomes empty. */
  template <CoalescingMemoryManager M>
  void deallocate(allocation_id id, M& manager)
  {
    OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
    auto const arena  = ouly::detail::vector_access(entry_arenas_, id.get());
    auto const offset = ouly::detail::vector_access(entry_offsets_, id.get());
    auto const size   = ouly::detail::vector_access(entry_sizes_, id.get());
    auto&      ar     = ouly::detail::vector_access(arena_pool_, arena);

    free_entry(id.get());
    if (--ar.allocs_ == 0)
    {
      drop_arena(arena);
      manager.remove(arena_id{arena});
      return;
    }
    arena_free(ar, offset, size);
  }

  /**
   * @brief Compact allocations towards the front of earlier arenas and drop emptied arenas.
   *
   * Allocations are processed in (arena, offset) order and moved to the lowest position they fit;
   * because of this ordering, same-arena moves always shift data towards lower offsets and the emitted
   * `move_memory` sequence never overwrites data that has not been moved yet (individual calls can
   * still self-overlap and must behave like memmove). Allocation ids remain valid; the manager is told
   * about relocations through `rebind_alloc`.
   *
   * @param max_bytes_to_move Optional budget; once moving another allocation would exceed it, all
   * remaining allocations stay in place and `completed_` is false in the result.
   */
  template <typename M>
    requires(CoalescingDefragMemoryManager<M, first_fit_defrag_allocator>)
  auto defragment(M& manager, size_type max_bytes_to_move = std::numeric_limits<size_type>::max())
   -> coalescing_defrag_result
  {
    manager.begin_defragment(*this);
    coalescing_defrag_result result;

    auto items = snapshot_allocations();

    std::vector<placement> plan;
    plan.reserve(items.size());
    std::vector<defrag_move> moves;
    std::vector<uint32_t>    rebinds;
    std::vector<size_type>   cursor(arena_pool_.size(), 0);
    bool                     budget_left = true;

    for (auto const& it : items)
    {
      auto const id        = it.id_;
      auto const src       = ouly::detail::vector_access(entry_arenas_, id);
      auto const size      = ouly::detail::vector_access(entry_sizes_, id);
      auto const offset    = ouly::detail::vector_access(entry_offsets_, id);
      bool const movable   = budget_left && !ouly::detail::vector_access(entry_dedicated_, id);
      auto [dst, to]       = movable ? find_placement(it, cursor) : std::make_pair(src, offset);
      bool const relocates = dst != src || to != offset;

      if (relocates && result.bytes_moved_ + size > max_bytes_to_move)
      {
        result.completed_ = false;
        budget_left       = false;
        dst               = src;
        to                = offset;
      }
      else if (relocates)
      {
        result.bytes_moved_ += size;
        result.allocations_moved_++;
        push_move(moves, {.from_ = offset, .to_ = to, .size_ = size, .src_ = src, .dst_ = dst});
        rebinds.push_back(id);
        ouly::detail::vector_access(entry_arenas_, id)  = dst;
        ouly::detail::vector_access(entry_offsets_, id) = to;
      }
      cursor[dst] = to + size;
      plan.push_back({.to_ = to, .size_ = size, .dst_ = dst});
    }
    result.moves_ = static_cast<uint32_t>(moves.size());

    apply_plan(plan);
    auto removed           = drop_empty_arenas();
    result.arenas_removed_ = static_cast<uint32_t>(removed.size());

    for (auto const& m : moves)
    {
      manager.move_memory(arena_id{m.src_}, arena_id{m.dst_}, m.from_, m.to_, m.size_);
    }
    for (auto id : rebinds)
    {
      manager.rebind_alloc(allocation_id{id}, arena_id{ouly::detail::vector_access(entry_arenas_, id)},
                           ouly::detail::vector_access(entry_offsets_, id));
    }
    for (auto arena : removed)
    {
      manager.remove(arena_id{arena});
    }

    manager.end_defragment(*this);
    return result;
  }

  OULY_API void validate_integrity() const;

private:
  struct arena_state
  {
    // free blocks sorted by offset
    std::vector<size_type> free_offsets_;
    std::vector<size_type> free_sizes_;
    size_type              size_   = 0;
    size_type              free_   = 0;
    uint32_t               allocs_ = 0;
  };

  struct defrag_item
  {
    uint32_t  pos_;
    size_type offset_;
    uint32_t  id_;
  };

  struct placement
  {
    size_type to_;
    size_type size_;
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

  /** Live allocations ordered by (arena position, offset) — the defragmentation processing order. */
  [[nodiscard]] OULY_API auto snapshot_allocations() const -> std::vector<defrag_item>;

  /** Earliest arena (up to and including the source) where the allocation fits at the pack cursor. */
  [[nodiscard]] OULY_API auto find_placement(defrag_item const& it, std::vector<size_type> const& cursor) const
   -> std::pair<uint16_t, size_type>;

  /** Rebuild every arena's free list and allocation count from the placement plan. */
  OULY_API void apply_plan(std::vector<placement>& plan);

  /** Unlink arenas that no longer host any allocation and recycle their slots. */
  OULY_API auto drop_empty_arenas() -> std::vector<uint16_t>;

  OULY_API void validate_arena(arena_state const& ar, std::vector<uint32_t>& allocs) const;

  static OULY_API void push_move(std::vector<defrag_move>& moves, defrag_move value);

  OULY_API auto try_allocate(uint16_t arena, arena_state& ar, size_type size, size_type mask) -> ca_allocation;

  static OULY_API void arena_free(arena_state& ar, size_type offset, size_type size);

  template <CoalescingMemoryManager M>
  auto create_arena(size_type size, M& manager, bool empty) -> uint16_t
  {
    uint16_t arena = 0;
    if (!free_arenas_.empty())
    {
      arena = free_arenas_.back();
      free_arenas_.pop_back();
    }
    else
    {
      OULY_ASSERT(arena_pool_.size() < std::numeric_limits<uint16_t>::max());
      arena = static_cast<uint16_t>(arena_pool_.size());
      arena_pool_.emplace_back();
    }
    auto& ar = ouly::detail::vector_access(arena_pool_, arena);
    ar.size_ = size;
    if (empty)
    {
      ar.free_ = size;
      ar.free_offsets_.push_back(0);
      ar.free_sizes_.push_back(size);
    }
    arena_order_.push_back(arena);
    manager.add(arena_id{arena}, size);
    return arena;
  }

  OULY_API void drop_arena(uint16_t arena);

  OULY_API auto push_entry(size_type offset, size_type size, uint16_t arena, uint8_t align_pow2) -> uint32_t;

  OULY_API void free_entry(uint32_t id);

  // allocation table; slot 0 is a sentinel so id 0 can terminate the free chain
  std::vector<size_type> entry_offsets_    = {0};
  std::vector<size_type> entry_sizes_      = {0};
  std::vector<uint16_t>  entry_arenas_     = {0};
  std::vector<uint8_t>   entry_alignments_ = {0};
  std::vector<bool>      entry_live_       = {false};
  std::vector<bool>      entry_dedicated_  = {false};
  uint32_t               free_entry_       = 0;

  // arena pool; slot 0 is a sentinel so arena ids match the manager-facing ids of other allocators
  std::vector<arena_state> arena_pool_ = {arena_state{}};
  std::vector<uint16_t>    arena_order_;
  std::vector<uint16_t>    free_arenas_;

  size_type arena_size_ = 0;
};

} // namespace ouly
