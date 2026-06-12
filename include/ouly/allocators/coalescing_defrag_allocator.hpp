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
 * @brief Arena based allocator using offset-ordered free lists (the `coalescing_allocator` scheme),
 * with external memory management and defragmentation support.
 *
 * Memory is organized in arenas obtained from a `CoalescingMemoryManager`. Each arena keeps its free
 * blocks in two parallel vectors sorted by offset, so deallocation coalesces neighbors with a binary
 * search and allocation is a first-fit scan. Allocations are tracked by `allocation_id` so they can be
 * relocated during defragmentation.
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
class coalescing_defrag_allocator
{
public:
  using size_type = allocation_size_type;

  coalescing_defrag_allocator() noexcept = default;
  explicit coalescing_defrag_allocator(size_type arena_sz) noexcept : arena_size_(arena_sz) {}

  /** @brief Arena size can be changed any time with this method, but it can only increase in size. */
  void set_arena_size(size_type s) noexcept
  {
    arena_size_ = std::max(arena_size_, s);
  }

  [[nodiscard]] auto get_arena_size() const noexcept -> size_type
  {
    return arena_size_;
  }

  /** @brief Given an allocation_id return the offset within its arena (already aligned) */
  [[nodiscard]] auto get_offset(allocation_id id) const noexcept -> size_type
  {
    return ouly::detail::vector_access(entry_offsets_, id.get());
  }

  /** @brief Given an allocation_id return the allocation size */
  [[nodiscard]] auto get_size(allocation_id id) const noexcept -> size_type
  {
    return ouly::detail::vector_access(entry_sizes_, id.get());
  }

  /** @brief Given an allocation_id return the arena it belongs to */
  [[nodiscard]] auto get_arena(allocation_id id) const noexcept -> arena_id
  {
    return arena_id{ouly::detail::vector_access(entry_arenas_, id.get())};
  }

  /** @brief Given an allocation_id return its alignment in bytes */
  [[nodiscard]] auto get_alignment(allocation_id id) const noexcept -> size_type
  {
    return size_type{1} << ouly::detail::vector_access(entry_alignments_, id.get());
  }

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
    requires(CoalescingDefragMemoryManager<M, coalescing_defrag_allocator>)
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

  void validate_integrity() const
  {
    std::vector<std::vector<uint32_t>> per_arena(arena_pool_.size());
    for (uint32_t id = 1; id < static_cast<uint32_t>(entry_offsets_.size()); ++id)
    {
      if (ouly::detail::vector_access(entry_live_, id))
      {
        per_arena[ouly::detail::vector_access(entry_arenas_, id)].push_back(id);
      }
    }

    std::vector<bool> active(arena_pool_.size(), false);
    for (auto arena : arena_order_)
    {
      active[arena] = true;
    }
    for (uint32_t arena = 0; arena < static_cast<uint32_t>(arena_pool_.size()); ++arena)
    {
      OULY_ASSERT(active[arena] || per_arena[arena].empty());
    }

    for (auto arena : arena_order_)
    {
      validate_arena(ouly::detail::vector_access(arena_pool_, arena), per_arena[arena]);
    }
  }

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
  [[nodiscard]] auto snapshot_allocations() const -> std::vector<defrag_item>
  {
    std::vector<uint32_t> arena_pos(arena_pool_.size(), 0);
    for (uint32_t i = 0; i < static_cast<uint32_t>(arena_order_.size()); ++i)
    {
      arena_pos[ouly::detail::vector_access(arena_order_, i)] = i;
    }
    std::vector<defrag_item> items;
    items.reserve(entry_offsets_.size());
    for (uint32_t id = 1; id < static_cast<uint32_t>(entry_offsets_.size()); ++id)
    {
      if (ouly::detail::vector_access(entry_live_, id))
      {
        items.push_back({.pos_    = arena_pos[ouly::detail::vector_access(entry_arenas_, id)],
                         .offset_ = ouly::detail::vector_access(entry_offsets_, id),
                         .id_     = id});
      }
    }
    std::ranges::sort(items,
                      [](defrag_item const& a, defrag_item const& b) -> bool
                      {
                        return a.pos_ != b.pos_ ? a.pos_ < b.pos_ : a.offset_ < b.offset_;
                      });
    return items;
  }

  /** Earliest arena (up to and including the source) where the allocation fits at the pack cursor. */
  [[nodiscard]] auto find_placement(defrag_item const& it, std::vector<size_type> const& cursor) const
   -> std::pair<uint16_t, size_type>
  {
    auto const src  = ouly::detail::vector_access(entry_arenas_, it.id_);
    auto const size = ouly::detail::vector_access(entry_sizes_, it.id_);
    auto const mask = (size_type{1} << ouly::detail::vector_access(entry_alignments_, it.id_)) - 1;
    for (uint32_t k = 0; k <= it.pos_; ++k)
    {
      auto const cand    = ouly::detail::vector_access(arena_order_, k);
      auto const aligned = (cursor[cand] + mask) & ~mask;
      // the source arena always has room at its own cursor
      if (cand == src || aligned + size <= ouly::detail::vector_access(arena_pool_, cand).size_)
      {
        return {cand, aligned};
      }
    }
    OULY_ASSERT(false && "unreachable: the source arena always fits");
    return {src, ouly::detail::vector_access(entry_offsets_, it.id_)};
  }

  /** Rebuild every arena's free list and allocation count from the placement plan. */
  void apply_plan(std::vector<placement>& plan)
  {
    std::ranges::sort(plan,
                      [](placement const& a, placement const& b) -> bool
                      {
                        return a.dst_ != b.dst_ ? a.dst_ < b.dst_ : a.to_ < b.to_;
                      });
    for (auto arena : arena_order_)
    {
      auto& ar = ouly::detail::vector_access(arena_pool_, arena);
      ar.free_offsets_.clear();
      ar.free_sizes_.clear();
      ar.free_   = ar.size_;
      ar.allocs_ = 0;
    }
    for (size_t i = 0; i < plan.size();)
    {
      auto const dst = plan[i].dst_;
      auto&      ar  = ouly::detail::vector_access(arena_pool_, dst);
      size_type  pos = 0;
      for (; i < plan.size() && plan[i].dst_ == dst; ++i)
      {
        if (plan[i].to_ > pos)
        {
          ar.free_offsets_.push_back(pos);
          ar.free_sizes_.push_back(plan[i].to_ - pos);
        }
        ar.free_ -= plan[i].size_;
        ar.allocs_++;
        pos = plan[i].to_ + plan[i].size_;
      }
      if (pos < ar.size_)
      {
        ar.free_offsets_.push_back(pos);
        ar.free_sizes_.push_back(ar.size_ - pos);
      }
    }
  }

  /** Unlink arenas that no longer host any allocation and recycle their slots. */
  auto drop_empty_arenas() -> std::vector<uint16_t>
  {
    std::vector<uint16_t> removed;
    std::erase_if(arena_order_,
                  [&](uint16_t arena) -> bool
                  {
                    if (ouly::detail::vector_access(arena_pool_, arena).allocs_ != 0)
                    {
                      return false;
                    }
                    removed.push_back(arena);
                    return true;
                  });
    for (auto arena : removed)
    {
      ouly::detail::vector_access(arena_pool_, arena) = {};
      free_arenas_.push_back(arena);
    }
    return removed;
  }

  void validate_arena(arena_state const& ar, std::vector<uint32_t>& allocs) const
  {
    std::ranges::sort(allocs,
                      [this](uint32_t a, uint32_t b) -> bool
                      {
                        return ouly::detail::vector_access(entry_offsets_, a) <
                               ouly::detail::vector_access(entry_offsets_, b);
                      });
    OULY_ASSERT(ar.allocs_ == allocs.size());

    // free blocks and allocations must exactly tile [0, size)
    [[maybe_unused]] size_type free_total = 0;
    size_type                  pos        = 0;
    size_t                     fi         = 0;
    size_t                     ai         = 0;
    while (pos < ar.size_)
    {
      if (fi < ar.free_offsets_.size() && ar.free_offsets_[fi] == pos)
      {
        OULY_ASSERT(ar.free_sizes_[fi] > 0);
        free_total += ar.free_sizes_[fi];
        pos += ar.free_sizes_[fi];
        ++fi;
        // adjacent free blocks must have been coalesced
        OULY_ASSERT(fi == ar.free_offsets_.size() || ar.free_offsets_[fi] > pos);
      }
      else if (ai < allocs.size() && ouly::detail::vector_access(entry_offsets_, allocs[ai]) == pos)
      {
        pos += ouly::detail::vector_access(entry_sizes_, allocs[ai]);
        ++ai;
      }
      else
      {
        OULY_ASSERT(false && "arena layout has a hole or an overlap");
        return;
      }
    }
    OULY_ASSERT(pos == ar.size_);
    OULY_ASSERT(fi == ar.free_offsets_.size());
    OULY_ASSERT(ai == allocs.size());
    OULY_ASSERT(free_total == ar.free_);
  }

  static void push_move(std::vector<defrag_move>& moves, defrag_move value)
  {
    if (!moves.empty())
    {
      auto& back = moves.back();
      if (back.src_ == value.src_ && back.dst_ == value.dst_ && back.from_ + back.size_ == value.from_ &&
          back.to_ + back.size_ == value.to_)
      {
        back.size_ += value.size_;
        return;
      }
    }
    moves.push_back(value);
  }

  auto try_allocate(uint16_t arena, arena_state& ar, size_type size, size_type mask) -> ca_allocation
  {
    for (uint32_t i = 0, end = static_cast<uint32_t>(ar.free_offsets_.size()); i < end; ++i)
    {
      auto const offset  = ar.free_offsets_[i];
      auto const fsize   = ar.free_sizes_[i];
      auto const aligned = (offset + mask) & ~mask;
      auto const pad     = aligned - offset;
      if (fsize < pad + size)
      {
        continue;
      }
      auto const remaining = fsize - pad - size;
      if (pad == 0)
      {
        if (remaining == 0)
        {
          ar.free_offsets_.erase(ar.free_offsets_.begin() + i);
          ar.free_sizes_.erase(ar.free_sizes_.begin() + i);
        }
        else
        {
          ar.free_offsets_[i] += size;
          ar.free_sizes_[i] = remaining;
        }
      }
      else
      {
        // alignment padding stays free
        ar.free_sizes_[i] = pad;
        if (remaining != 0)
        {
          ar.free_offsets_.insert(ar.free_offsets_.begin() + i + 1, aligned + size);
          ar.free_sizes_.insert(ar.free_sizes_.begin() + i + 1, remaining);
        }
      }
      ar.free_ -= size;
      ar.allocs_++;
      auto id = push_entry(aligned, size, arena, static_cast<uint8_t>(std::popcount(mask)));
      return ca_allocation{.offset_ = aligned, .id_ = {.id_ = id}, .arena_ = {.id_ = arena}};
    }
    return {};
  }

  static void arena_free(arena_state& ar, size_type offset, size_type size)
  {
    auto&      offsets = ar.free_offsets_;
    auto&      sizes   = ar.free_sizes_;
    auto const it      = std::ranges::lower_bound(offsets, offset);
    auto const idx     = static_cast<size_t>(std::distance(offsets.begin(), it));

    bool const merge_left  = idx > 0 && offsets[idx - 1] + sizes[idx - 1] == offset;
    bool const merge_right = idx < offsets.size() && offset + size == offsets[idx];

    if (merge_left && merge_right)
    {
      sizes[idx - 1] += size + sizes[idx];
      offsets.erase(offsets.begin() + static_cast<std::ptrdiff_t>(idx));
      sizes.erase(sizes.begin() + static_cast<std::ptrdiff_t>(idx));
    }
    else if (merge_left)
    {
      sizes[idx - 1] += size;
    }
    else if (merge_right)
    {
      offsets[idx] = offset;
      sizes[idx] += size;
    }
    else
    {
      offsets.insert(offsets.begin() + static_cast<std::ptrdiff_t>(idx), offset);
      sizes.insert(sizes.begin() + static_cast<std::ptrdiff_t>(idx), size);
    }
    ar.free_ += size;
  }

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

  void drop_arena(uint16_t arena)
  {
    auto pos = std::ranges::find(arena_order_, arena);
    OULY_ASSERT(pos != arena_order_.end());
    arena_order_.erase(pos);
    ouly::detail::vector_access(arena_pool_, arena) = {};
    free_arenas_.push_back(arena);
  }

  auto push_entry(size_type offset, size_type size, uint16_t arena, uint8_t align_pow2) -> uint32_t
  {
    uint32_t id = 0;
    if (free_entry_ != 0)
    {
      id          = free_entry_;
      free_entry_ = static_cast<uint32_t>(ouly::detail::vector_access(entry_offsets_, id));
    }
    else
    {
      id = static_cast<uint32_t>(entry_offsets_.size());
      entry_offsets_.emplace_back();
      entry_sizes_.emplace_back();
      entry_arenas_.emplace_back();
      entry_alignments_.emplace_back();
      entry_live_.emplace_back();
      entry_dedicated_.emplace_back();
    }
    ouly::detail::vector_access(entry_offsets_, id)    = offset;
    ouly::detail::vector_access(entry_sizes_, id)      = size;
    ouly::detail::vector_access(entry_arenas_, id)     = arena;
    ouly::detail::vector_access(entry_alignments_, id) = align_pow2;
    ouly::detail::vector_access(entry_live_, id)       = true;
    ouly::detail::vector_access(entry_dedicated_, id)  = false;
    return id;
  }

  void free_entry(uint32_t id)
  {
    ouly::detail::vector_access(entry_live_, id)    = false;
    ouly::detail::vector_access(entry_offsets_, id) = free_entry_;
    free_entry_                                     = id;
  }

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
