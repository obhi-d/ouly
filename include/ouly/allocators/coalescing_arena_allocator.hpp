// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/allocation_id.hpp"
#include "ouly/allocators/allocator.hpp"
#include "ouly/allocators/detail/ca_structs.hpp"
#include "ouly/allocators/detail/memory_stats.hpp"
#include "ouly/containers/detail/vlist.hpp"
#include "ouly/utility/config.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace ouly
{

template <typename T>
concept CoalescingMemoryManager = requires(T m) {
  /**
   * Drop Arena
   */
  { m.remove(arena_id()) } -> std::same_as<void>;
  /**
   * Add an arena
   */
  { m.add(arena_id(), allocation_size_type()) } -> std::same_as<void>;
};

/**
 * @brief Memory manager concept for coalescing allocators that additionally supports defragmentation.
 *
 * On top of the arena add/remove interface, the manager must implement:
 * - begin_defragment/end_defragment: bracket a defragmentation pass
 * - move_memory: copy `size` bytes from (src_arena, from) to (dst_arena, to). Calls are emitted in a
 *   safe sequence (a destination region is always either free or already vacated by a previous call),
 *   but a single call's source and destination ranges may overlap when src and dst arena are the same;
 *   the implementation must behave like memmove (stage through a temporary if the backend cannot copy
 *   overlapping ranges directly, e.g. GPU buffer-to-buffer copies).
 * - rebind_alloc: an allocation kept its allocation_id but now lives at a new arena/offset.
 */
template <typename T, typename A>
concept CoalescingDefragMemoryManager = CoalescingMemoryManager<T> && requires(T m, A& allocator) {
  m.begin_defragment(allocator);
  m.end_defragment(allocator);
  m.move_memory(arena_id(), arena_id(), allocation_size_type(), allocation_size_type(), allocation_size_type());
  m.rebind_alloc(allocation_id(), arena_id(), allocation_size_type());
};

/** @brief Outcome of a defragmentation pass. */
struct coalescing_defrag_result
{
  allocation_size_type bytes_moved_       = 0;
  uint32_t             moves_             = 0;
  uint32_t             allocations_moved_ = 0;
  uint32_t             arenas_removed_    = 0;
  /** false when the pass stopped early because the move budget was exhausted */
  bool completed_ = true;
};

struct ca_allocation
{
  allocation_size_type offset_ = 0;
  allocation_id        id_;
  arena_id             arena_;

  auto operator<=>(ca_allocation const&) const noexcept = default;

  [[nodiscard]] auto get_offset() const noexcept -> allocation_size_type
  {
    return offset_;
  }

  [[nodiscard]] auto get_allocation_id() const noexcept -> allocation_id
  {
    return id_;
  }

  [[nodiscard]] auto get_arena_id() const noexcept -> arena_id
  {
    return arena_;
  }
};

#ifdef OULY_DEBUG
using coalescing_arena_allocator_base =
 ouly::detail::statistics<ouly::detail::ca_allocator_tag, ouly::config<cfg::compute_stats>>;
#else
using coalescing_arena_allocator_base = ouly::detail::statistics<ouly::detail::ca_allocator_tag, ouly::config<>>;
#endif

/**
 * @brief A coalescing arena allocator that manages memory blocks within arenas
 *
 * This allocator maintains a collection of memory arenas and manages allocations within them.
 * It supports coalescing of free blocks to reduce fragmentation. The allocator tracks block
 * sizes, offsets and arena assignments.
 *
 * @note This class is meant for doing virtual allocations, for example when doing GPU memory
 * management. It is not meant to be used as a general purpose allocator.
 *
 * Key features:
 * - Supports variable sized allocations
 * - Coalesces adjacent free blocks
 * - Tracks arena assignments for blocks
 * - Allows arena size adjustments
 * - Supports dedicated allocations
 * - Maintains allocation metadata
 *
 * The allocator uses an arena-based approach where memory is allocated in chunks (arenas)
 * and then sub-allocated into smaller blocks. When blocks are freed, adjacent free blocks
 * are merged to reduce fragmentation.
 *
 * @note This allocator inherits from coalescing_arena_allocator_base
 * @note Arena size can only be increased, not decreased
 * @note Allocation information can be retrieved using allocation IDs
 * @note Dedicated allocations bypass block coalescing
 * @note Arena and allocation IDs are consequetive integers, and can be used as indexes.
 */
class coalescing_arena_allocator : coalescing_arena_allocator_base
{
public:
  using size_type = allocation_size_type;

  coalescing_arena_allocator() noexcept                                            = default;
  auto operator=(const coalescing_arena_allocator&) -> coalescing_arena_allocator& = delete;
  auto operator=(coalescing_arena_allocator&&) -> coalescing_arena_allocator&      = delete;
  coalescing_arena_allocator(coalescing_arena_allocator const&) noexcept           = delete;
  coalescing_arena_allocator(coalescing_arena_allocator&&) noexcept                = delete;
  coalescing_arena_allocator(size_type arena_sz) : arena_size_(arena_sz) {}
  ~coalescing_arena_allocator() noexcept = default;

  /** @brief Arena size can be changed any time with this method, but it can only increase in size. */
  void set_arena_size(size_type s) noexcept
  {
    arena_size_ = std::max(arena_size_, s);
  }

  /** @return Arena size currently in use. */
  [[nodiscard]] auto get_arena_size() const noexcept -> size_type
  {
    return arena_size_;
  }

  /** @brief Given an allocation_id return the offset in the arena it belongs to */
  [[nodiscard]] auto get_size(allocation_id id) const noexcept -> size_type
  {
    return ouly::detail::vector_access(block_entries_.sizes_, id.get());
  }

  /** @brief Given an allocation_id return the offset in the arena it belongs to */
  [[nodiscard]] auto get_offset(allocation_id id) const noexcept -> size_type
  {
    return ouly::detail::vector_access(block_entries_.offsets_, id.get());
  }

  /** @brief Given an allocation_id return the arena it belongs to */
  [[nodiscard]] auto get_arena(allocation_id id) const noexcept -> arena_id
  {
    return arena_id{ouly::detail::vector_access(block_entries_.arenas_, id.get())};
  }
  /** @brief The method `allocate` returns an allocation desc, with extra information about the allocation offset and
   * the arena the allocation belongs to. The information need not be stored, as the alllocation_id can be used to fetch
   * this information.
   * @note When an alignment is requested, the block is over-allocated by `alignment - 1` bytes and the
   * returned offset is rounded up to the alignment; `get_offset`/`get_size` report the raw block. */
  template <CoalescingMemoryManager M, typename Alignment = ouly::alignment<>, typename Dedicated = std::false_type>
  auto allocate(size_type size, M& manager, Alignment alignment = {}, Dedicated /*unused*/ = {}) -> ca_allocation
  {
    [[maybe_unused]] auto measure = statistics::report_allocate(size);

    auto const align_value  = static_cast<size_type>(alignment);
    auto const mask         = align_value > 1 ? align_value - 1 : size_type{0};
    auto const vsize        = size + mask;
    bool const is_dedicated = Dedicated::value || vsize >= arena_size_;

    if (is_dedicated)
    {
      // a dedicated allocation starts at offset 0, which is aligned for any power of two
      auto [arena, block] = add_arena_filled(size, manager);
      return ca_allocation{.offset_ = 0, .id_ = block, .arena_ = arena};
    }

    ca_allocation al = try_allocate(vsize);

    if (al.get_allocation_id() == allocation_id())
    {
      add_arena(vsize, manager);
      al = try_allocate(vsize);
    }

    al.offset_ = (al.offset_ + mask) & ~mask;
    return al;
  }

  /** @brief Dellocate an allocation. The manager must be provided for removal of arenas_. */
  template <CoalescingMemoryManager M>
  void deallocate(allocation_id id, M& manager)
  {
    auto aa = deallocate(id);
    if (aa != arena_id())
    {
      manager.remove(aa);
    }
  }

  void validate_integrity() const;

  [[nodiscard]] auto get_offsets() const noexcept -> std::span<allocation_size_type const>
  {
    return block_entries_.offsets_;
  }

  [[nodiscard]] auto get_sizes() const noexcept -> std::span<allocation_size_type const>
  {
    return block_entries_.sizes_;
  }

  [[nodiscard]] auto get_arena_indices() const noexcept -> std::span<uint16_t const>
  {
    return block_entries_.arenas_;
  }

protected:
  OULY_API auto add_arena(size_type size, bool empty) -> std::pair<arena_id, allocation_id>;

  template <CoalescingMemoryManager M>
  auto add_arena_filled(size_type size, M& manager) -> std::pair<arena_id, allocation_id>
  {
    statistics::report_new_arena();
    auto ret = add_arena(size, false);
    manager.add(ret.first, size);
    return ret;
  }

  template <CoalescingMemoryManager M>
  void add_arena(size_type size, M& manager)
  {
    size            = std::max(size, arena_size_);
    auto [arena, _] = add_arena(size, true);
    manager.add(arena, size);
  }

  void add_free_arena(uint32_t block)
  {
    sizes_.push_back(ouly::detail::vector_access(block_entries_.sizes_, block));
    free_ordering_.push_back(block);
  }

  OULY_API void grow_free_node(uint32_t /*block*/, size_type size);
  OULY_API void replace_and_grow(uint32_t right, uint32_t node, size_type new_size);
  OULY_API void add_free(uint32_t node);
  OULY_API void erase(uint32_t node);
  OULY_API auto deallocate(allocation_id id) -> arena_id;

  static auto mini2(size_type const* it, size_t size, size_type key) noexcept
  {
    while (true)
    {
      {
        const size_type* const middle = it + (size >> 1);
        size                          = (size + 1) >> 1;
        it                            = *middle < key ? middle : it;
      };
      {
        const size_type* const middle = it + (size >> 1);
        size                          = (size + 1) >> 1;
        it                            = *middle < key ? middle : it;
      };
      if (size <= 2)
      {
        break;
      }
    }

    it += static_cast<size_type>(size > 1 && (*it < key));
    it += static_cast<size_type>(size > 0 && (*it < key));
    return it;
  }

  static auto mini2_it(size_type const* it, size_t s, size_type key) noexcept
  {
    return std::distance(it, mini2(it, s, key));
  }

  [[nodiscard]] auto find_free(size_type size) const noexcept
  {
    return mini2(sizes_.data(), sizes_.size(), size);
  }

  [[nodiscard]] auto total_free_nodes() const noexcept -> uint32_t
  {
    return static_cast<std::uint32_t>(free_ordering_.size());
  }

  [[nodiscard]] auto total_free_size() const noexcept -> size_type
  {
    size_type sz = 0;
    for (auto fn : sizes_)
    {
      sz += fn;
    }

    return sz;
  }

  auto try_allocate(size_type size) -> ca_allocation
  {
    if (sizes_.empty() || sizes_.back() < size)
    {
      return {};
    }
    const auto* it = find_free(size);
    auto        id = commit(size, it);
    return ca_allocation{.offset_ = ouly::detail::vector_access(block_entries_.offsets_, id),
                         .id_     = {.id_ = id},
                         .arena_  = {.id_ = ouly::detail::vector_access(block_entries_.arenas_, id)}};
  }

  OULY_API void reinsert_left(size_t of, size_type size, std::uint32_t node);
  OULY_API void reinsert_right(size_t of, size_type size, std::uint32_t node);
  OULY_API auto commit(size_type size, size_type const* found) -> uint32_t;

  // Accessors for derived allocators (e.g. defragmentation support)
  [[nodiscard]] auto arena_entries() noexcept -> ouly::detail::ca_arena_entries&
  {
    return arena_entries_;
  }

  [[nodiscard]] auto arena_entries() const noexcept -> ouly::detail::ca_arena_entries const&
  {
    return arena_entries_;
  }

  [[nodiscard]] auto block_entries() noexcept -> ouly::detail::ca_block_entries&
  {
    return block_entries_;
  }

  [[nodiscard]] auto block_entries() const noexcept -> ouly::detail::ca_block_entries const&
  {
    return block_entries_;
  }

  [[nodiscard]] auto arena_list() noexcept -> ouly::detail::ca_arena_list&
  {
    return arenas_;
  }

  [[nodiscard]] auto arena_list() const noexcept -> ouly::detail::ca_arena_list const&
  {
    return arenas_;
  }

  /** Free block sizes, ascending; parallel to `free_ordering()` */
  [[nodiscard]] auto free_sizes() noexcept -> std::vector<size_type>&
  {
    return sizes_;
  }

  [[nodiscard]] auto free_sizes() const noexcept -> std::vector<size_type> const&
  {
    return sizes_;
  }

  /** Free block ids, ordered by block size; parallel to `free_sizes()` */
  [[nodiscard]] auto free_ordering() noexcept -> std::vector<uint32_t>&
  {
    return free_ordering_;
  }

  [[nodiscard]] auto free_ordering() const noexcept -> std::vector<uint32_t> const&
  {
    return free_ordering_;
  }

private:
  // Free blocks
  ouly::detail::ca_arena_entries arena_entries_{};
  ouly::detail::ca_block_entries block_entries_{};
  ouly::detail::ca_arena_list    arenas_{};

  std::vector<size_type> sizes_;
  std::vector<uint32_t>  free_ordering_;

  size_type arena_size_ = 0;
};

} // namespace ouly
