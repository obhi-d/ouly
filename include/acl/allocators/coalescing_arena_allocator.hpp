
#pragma once

#include <acl/allocators/allocation_id.hpp>
#include <acl/allocators/allocator.hpp>
#include <acl/allocators/detail/ca_structs.hpp>
#include <acl/allocators/detail/memory_stats.hpp>
#include <acl/containers/detail/vlist.hpp>
#include <acl/utility/config.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace acl
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

#ifdef ACL_DEBUG
using coalescing_arena_allocator_base =
 acl::detail::statistics<acl::detail::ca_allocator_tag, acl::config<cfg::compute_stats>>;
#else
using coalescing_arena_allocator_base = acl::detail::statistics<acl::detail::ca_allocator_tag, acl::config<>>;
#endif

/**
 * @brief A coalescing arena allocator that manages memory blocks within arenas
 *
 * This allocator maintains a collection of memory arenas and manages allocations within them.
 * It supports coalescing of free blocks to reduce fragmentation. The allocator tracks block
 * sizes, offsets and arena assignments.
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
  coalescing_arena_allocator(size_type arena_sz) noexcept : arena_size_(arena_sz) {}
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
    return block_entries_.sizes_[id.get()];
  }

  /** @brief Given an allocation_id return the offset in the arena it belongs to */
  [[nodiscard]] auto get_offset(allocation_id id) const noexcept -> size_type
  {
    return block_entries_.offsets_[id.get()];
  }

  /** @brief Given an allocation_id return the arena it belongs to */
  [[nodiscard]] auto get_arena(allocation_id id) const noexcept -> arena_id
  {
    return arena_id{block_entries_.arenas_[id.get()]};
  }
  /** @brief The method `allocate` returns an allocation desc, with extra information about the allocation offset and
   * the arena the allocation belongs to. The information need not be stored, as the alllocation_id can be used to fetch
   * this information */
  template <CoalescingMemoryManager M, typename Alignment = acl::alignment<>, typename Dedicated = std::false_type>
  auto allocate(size_type size, M& manager, Alignment alignment = {}, Dedicated /*unused*/ = {}) -> ca_allocation
  {
    auto measure = statistics::report_allocate(size);
    auto vsize   = alignment ? size + static_cast<size_type>(alignment) : size;

    if (Dedicated::value || vsize >= arena_size_)
    {
      auto [arena, block] = add_arena_filled(vsize, manager);
      return ca_allocation{.offset_ = 0, .id_ = block, .arena_ = arena};
    }

    ca_allocation al = try_allocate(size);

    if (al.get_allocation_id() == allocation_id())
    {
      add_arena(vsize, manager);
      al = try_allocate(size);
    }

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

private:
  auto add_arena(size_type size, bool empty) -> std::pair<arena_id, allocation_id>;

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
    sizes_.push_back(block_entries_.sizes_[block]);
    free_ordering_.push_back(block);
  }

  void grow_free_node(uint32_t /*block*/, size_type size);
  void replace_and_grow(uint32_t right, uint32_t node, size_type new_size);
  void add_free(uint32_t node);
  void erase(uint32_t node);
  auto deallocate(allocation_id id) -> arena_id;

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

  auto try_allocate(size_type size) noexcept -> ca_allocation
  {
    if (sizes_.empty() || sizes_.back() < size)
    {
      return {};
    }
    const auto* it = find_free(size);
    auto        id = commit(size, it);
    return ca_allocation{
     .offset_ = block_entries_.offsets_[id], .id_ = {.id_ = id}, .arena_ = {.id_ = block_entries_.arenas_[id]}};
  }

  void reinsert_left(size_t of, size_type size, std::uint32_t node) noexcept;
  void reinsert_right(size_t of, size_type size, std::uint32_t node);
  auto commit(size_type size, size_type const* found) noexcept -> uint32_t;

  // Free blocks
  acl::detail::ca_arena_entries arena_entries_;
  acl::detail::ca_block_entries block_entries_;
  acl::detail::ca_arena_list    arenas_;

  std::vector<size_type> sizes_;
  std::vector<uint32_t>  free_ordering_;

  size_type arena_size_ = 0;
};

} // namespace acl
