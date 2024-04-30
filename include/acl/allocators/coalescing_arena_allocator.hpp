
#pragma once

#include <acl/allocators/allocator.hpp>
#include <acl/allocators/memory_stats.hpp>
#include <acl/containers/vlist.hpp>
#include <acl/utils/config.hpp>
#include <compare>
#include <cstdint>
#include <limits>
#include <vector>

namespace acl
{
#define ACL_BINARY_SEARCH_STEP                                                                                         \
  do                                                                                                                   \
  {                                                                                                                    \
    const size_type* const middle = it + (size >> 1);                                                                  \
    size                          = (size + 1) >> 1;                                                                   \
    it                            = *middle < key ? middle : it;                                                       \
  }                                                                                                                    \
  while (0)

/**
 * @brief This allocator grows the buffer size, and merges free sizes.
 */
struct allocation_id
{
  uint32_t id = std::numeric_limits<uint32_t>::max();

  auto operator<=>(allocation_id const&) const noexcept = default;
};

struct arena_id
{
  uint16_t id = std::numeric_limits<uint16_t>::max();

  auto operator<=>(arena_id const&) const noexcept = default;
};

using allocation_size_type = std::conditional_t<detail::coalescing_allocator_large_size, uint64_t, uint32_t>;

template <typename T>
concept CoalescingMemoryManager = requires(T m) {
  /**
   * Drop Arena
   */
  {
    m.remove(arena_id())
  } -> std::same_as<void>;
  /**
   * Add an arena
   */
  {
    m.add(arena_id(), allocation_size_type())
  } -> std::same_as<void>;
};

namespace detail
{

template <typename T>
struct ca_bank
{
  uint32_t       free_idx_ = 0;
  std::vector<T> entries_  = {T()};

  uint32_t push(T const& data)
  {
    if (free_idx_)
    {
      auto entry      = free_idx_;
      free_idx_       = entries_[free_idx_].order.next;
      entries_[entry] = data;
      return entry;
    }
    else
    {
      auto id = static_cast<uint32_t>(entries_.size());
      entries_.emplace_back(data);
      return id;
    }
  }
};

template <typename T>
struct ca_accessor
{
  using value_type = T;
  using bank_type  = ca_bank<T>;
  using size_type  = allocation_size_type;
  using container  = bank_type;

  inline static void erase(bank_type& bank, std::uint32_t node)
  {
    bank.entries_[node].order.next = bank.free_idx_;
    bank.free_idx_                 = node;
  }

  inline static detail::list_node& node(bank_type& bank, std::uint32_t node)
  {
    return bank.entries_[node].order;
  }

  inline static detail::list_node const& node(bank_type const& bank, std::uint32_t node)
  {
    return bank.entries_[node].order;
  }

  inline static value_type const& get(bank_type const& bank, std::uint32_t node)
  {
    return bank.entries_[node];
  }

  inline static value_type& get(bank_type& bank, std::uint32_t node)
  {
    return bank.entries_[node];
  }
};

template <typename T>
using ca_list = detail::vlist<ca_accessor<T>>;

struct ca_block
{
  allocation_size_type offset    = 0;
  allocation_size_type size      = 0;
  detail::list_node    order     = detail::list_node();
  std::uint16_t        arena     = 0;
  bool                 is_free   = false;
  std::uint8_t         alignment = 0;
};

using ca_block_entries = ca_bank<ca_block>;
using ca_block_list    = ca_list<ca_block>;

struct ca_arena
{
  using size_type = allocation_size_type;

  ca_block_list     blocks;
  detail::list_node order;
  size_type         size      = 0;
  size_type         free_size = 0;
};

using ca_arena_entries = ca_bank<ca_arena>;
using ca_arena_list    = ca_list<ca_arena>;

struct ca_allocator_tag
{};

} // namespace detail

struct ca_allocation
{
  allocation_size_type offset = 0;
  allocation_id        id;
  arena_id             arena;
};

#ifdef ACL_REC_STATS
using coalescing_arena_allocator_base = detail::statistics<detail::ca_allocator_tag, acl::options<opt::compute_stats>>;
#else
using coalescing_arena_allocator_base = detail::statistics<detail::ca_allocator_tag, acl::options<>>;
#endif

/** @brief Arena's and Allocation IDs are consequetive integers, and can be used as indexes. */
class coalescing_arena_allocator : coalescing_arena_allocator_base
{
public:
  using size_type = allocation_size_type;

  coalescing_arena_allocator() noexcept                                  = default;
  coalescing_arena_allocator(coalescing_arena_allocator const&) noexcept = default;
  coalescing_arena_allocator(coalescing_arena_allocator&&) noexcept      = default;
  coalescing_arena_allocator(size_type arena_sz) noexcept : arena_size(arena_sz) {}

  /** @brief Arena size can be changed any time with this method, but it can only increase in size. */
  void set_arena_size(size_type s) noexcept
  {
    arena_size = std::max(arena_size, s);
  }

  /** @return Arena size currently in use. */
  size_type get_arena_size() const noexcept
  {
    return arena_size;
  }

  /** @brief Given an allocation_id return the offset in the arena it belongs to */
  size_type get_offset(allocation_id id) const noexcept
  {
    return block_entries.entries_[id.id].offset;
  }

  /** @brief Given an allocation_id return the arena it belongs to */
  arena_id get_arena(allocation_id id) const noexcept
  {
    return arena_id{block_entries.entries_[id.id].arena};
  }
  /** @brief The method `allocate` returns an allocation desc, with extra information about the allocation offset and
   * the arena the allocation belongs to. The information need not be stored, as the alllocation_id can be used to fetch
   * this information */
  template <CoalescingMemoryManager M, typename Alignment = acl::alignment<>, typename Dedicated = std::false_type>
  ca_allocation allocate(size_type size, M& manager, Alignment alignment = {}, Dedicated = {})
  {
    auto measure = this->statistics::report_allocate(size);
    auto vsize   = alignment ? size + static_cast<size_type>(alignment) : size;

    if (Dedicated::value || vsize >= arena_size)
    {
      auto [arena, block] = add_arena_filled(vsize, manager);
      return ca_allocation{.offset = 0, .id = block, .arena = arena};
    }

    ca_allocation al = try_allocate(size);

    if (al.id == allocation_id())
    {
      add_arena(vsize, manager);
      al = try_allocate(size);
    }

    return al;
  }

  /** @brief Dellocate an allocation. The manager must be provided for removal of arenas. */
  template <CoalescingMemoryManager M>
  void deallocate(allocation_id id, M& manager)
  {
    auto aa = deallocate(id);
    if (aa != arena_id())
      manager.remove(aa);
  }

  void validate_integrity() const;

private:
  std::pair<arena_id, allocation_id> add_arena(size_type size, bool empty);

  template <CoalescingMemoryManager M>
  std::pair<arena_id, allocation_id> add_arena_filled(size_type size, M& manager)
  {
    this->statistics::report_new_arena();
    auto ret = add_arena(size, false);
    manager.add(ret.first, size);
    return ret;
  }

  template <CoalescingMemoryManager M>
  void add_arena(size_type size, M& manager)
  {
    auto [arena, _] = add_arena(size, true);
    manager.add(arena, size);
  }

  inline void add_free_arena(uint32_t block)
  {
    sizes.push_back(block_entries.entries_[block].size);
    free_ordering.push_back(block);
  }

  void     grow_free_node(uint32_t, size_type size);
  void     replace_and_grow(uint32_t right, uint32_t node, size_type size);
  void     add_free(uint32_t node);
  void     erase(uint32_t node);
  arena_id deallocate(allocation_id id);

  static inline auto mini2(size_type const* it, size_t size, size_type key) noexcept
  {
    do
    {
      ACL_BINARY_SEARCH_STEP;
      ACL_BINARY_SEARCH_STEP;
    }
    while (size > 2);
    it += size > 1 && (*it < key);
    it += size > 0 && (*it < key);
    return it;
  }

  static inline auto mini2_it(size_type const* it, size_t s, size_type key) noexcept
  {
    return std::distance(it, mini2(it, s, key));
  }

  inline auto find_free(size_type size) const noexcept
  {
    return mini2(sizes.data(), sizes.size(), size);
  }

  inline uint32_t total_free_nodes() const noexcept
  {
    return static_cast<std::uint32_t>(free_ordering.size());
  }

  inline size_type total_free_size() const noexcept
  {
    size_type sz = 0;
    for (auto fn : sizes)
      sz += fn;

    return sz;
  }

  inline ca_allocation try_allocate(size_type size) noexcept
  {
    if (sizes.empty() || sizes.back() < size)
      return ca_allocation();
    auto        it   = find_free(size);
    auto        id   = commit(size, it);
    auto const& node = block_entries.entries_[id];
    return ca_allocation{.offset = node.offset, .id = {.id = id}, .arena = {.id = node.arena}};
  }

  void     reinsert_left(size_t of, size_type size, std::uint32_t node) noexcept;
  void     reinsert_right(size_t of, size_type size, std::uint32_t node);
  uint32_t commit(size_type size, size_type const* found) noexcept;

  // Free blocks
  detail::ca_arena_entries arena_entries;
  detail::ca_block_entries block_entries;
  detail::ca_arena_list    arenas;

  std::vector<size_type> sizes;
  std::vector<uint32_t>  free_ordering;

  size_type arena_size = 0;
};
#undef ACL_BINARY_SEARCH_STEP
} // namespace acl
