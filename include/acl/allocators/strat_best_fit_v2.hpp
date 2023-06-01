#pragma once
#include "best_fit_options.hpp"
#include <acl/detail/arena.hpp>
#include <acl/utility/type_traits.hpp>
#include <optional>

namespace acl::strat
{
#define ACL_BINARY_SEARCH_STEP                                                                                         \
  do                                                                                                                   \
  {                                                                                                                    \
    const size_type* const middle = it + (size >> 1);                                                                  \
    size                          = (size + 1) >> 1;                                                                   \
    it                            = *middle < key ? middle : it;                                                       \
  }                                                                                                                    \
  while (0)

/// @brief  Strategy class for arena_allocator that stores a
///         sorted list of free available slots.
///         Binary search is used to find the best slot that fits
///         the requested memory
template <typename Options = acl::options<>>
class best_fit_v2
{

public:
  using extension       = uint64_t;
  using size_type       = detail::choose_size_t<uint32_t, Options>;
  using arena_bank      = detail::arena_bank<size_type, extension>;
  using block_bank      = detail::block_bank<size_type, extension>;
  using block           = detail::block<size_type, extension>;
  using bank_data       = detail::bank_data<size_type, extension>;
  using block_link      = typename block_bank::link;
  using size_list       = acl::vector<size_type>;
  using optional_addr   = size_type*;
  using allocate_result = optional_addr;

  static constexpr int bsearch_algo =
    std::conditional_t<acl::detail::has_bsearch_algo<Options>, Options, acl::opt::bsearch_min0>::bsearch_algo;

  static constexpr size_type min_granularity = 4;

  best_fit_v2() noexcept              = default;
  best_fit_v2(best_fit_v2 const&)     = default;
  best_fit_v2(best_fit_v2&&) noexcept = default;

  best_fit_v2& operator=(best_fit_v2 const&)     = default;
  best_fit_v2& operator=(best_fit_v2&&) noexcept = default;

  inline optional_addr try_allocate(bank_data& bank, size_type size) noexcept
  {
    if (sizes.empty() || sizes.back() < size)
      return nullptr;
    return find_free(size);
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, auto found) noexcept
  {
    auto          free_idx  = std::distance(sizes.data(), found);
    std::uint32_t free_node = free_ordering[free_idx];
    auto&         blk       = bank.blocks[block_link(free_node)];
    // Marker
    size_type     offset    = blk.offset;
    std::uint32_t arena_num = blk.arena;

    blk.is_free = false;

    auto remaining = *found - size;
    blk.size       = size;
    if (remaining > 0)
    {
      auto& list   = bank.arenas[blk.arena].block_order;
      auto  arena  = blk.arena;
      auto  newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, detail::k_null_sz<uhandle>, true);
      list.insert_after(bank.blocks, free_node, (uint32_t)newblk);
      // reinsert the left-over size in free list
      reinsert_left(bank.blocks, free_idx, remaining, (uint32_t)newblk);
    }
    else
    {
      // delete the existing found index from free list
      sizes.erase(sizes.begin() + free_idx);
      free_ordering.erase(free_ordering.begin() + free_idx);
    }

    return free_node;
  }

  inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block) noexcept
  {
    sizes.push_back(blocks[block_link(block)].size);
    free_ordering.push_back(block);
  }

  inline void add_free(block_bank& blocks, std::uint32_t block) noexcept
  {
    add_free_after_begin(blocks, block);
  }

  inline void grow_free_node(block_bank& blocks, std::uint32_t block, size_type newsize) noexcept
  {
    auto& blk = blocks[block_link(block)];

    auto it = find_free_it(sizes.data(), sizes.size(), blk.size);
    for (uint32_t end = static_cast<uint32_t>(free_ordering.size()); it != end && free_ordering[it] != block; ++it)
      ;

    ACL_ASSERT(it != static_cast<uint32_t>(free_ordering.size()));
    blk.size = newsize;
    reinsert_right(blocks, it, newsize, block);
  }

  inline void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block,
                               size_type new_size) noexcept
  {
    size_type size                     = blocks[block_link(block)].size;
    blocks[block_link(new_block)].size = new_size;

    auto it = find_free_it(sizes.data(), sizes.size(), size);
    for (uint32_t end = static_cast<uint32_t>(free_ordering.size()); it != end && free_ordering[it] != block; ++it)
      ;

    ACL_ASSERT(it != static_cast<uint32_t>(free_ordering.size()));
    reinsert_right(blocks, it, new_size, new_block);
  }

  inline void erase(block_bank& blocks, std::uint32_t block) noexcept
  {
    auto it = find_free_it(sizes.data(), sizes.size(), blocks[block_link(block)].size);
    for (uint32_t end = static_cast<uint32_t>(free_ordering.size()); it != end && free_ordering[it] != block; ++it)
      ;
    ACL_ASSERT(it != static_cast<uint32_t>(free_ordering.size()));
    free_ordering.erase(it + free_ordering.begin());
    sizes.erase(it + sizes.begin());
  }

  inline std::uint32_t total_free_nodes(block_bank const& blocks) const noexcept
  {
    return static_cast<std::uint32_t>(free_ordering.size());
  }

  inline size_type total_free_size(block_bank const& blocks) const noexcept
  {
    size_type sz = 0;
    for (auto fn : sizes)
      sz += fn;

    return sz;
  }

  void validate_integrity(block_bank const& blocks) const noexcept
  {
    size_type sz = 0;
    ACL_ASSERT(free_ordering.size() == sizes.size());
    for (size_t i = 1; i < sizes.size(); ++i)
    {
      ACL_ASSERT(sizes[i - 1] <= sizes[i]);
    }
    for (size_t i = 0; i < free_ordering.size(); ++i)
    {
      auto fn = free_ordering[i];
      ACL_ASSERT(sz <= blocks[block_link(fn)].size);
      ACL_ASSERT(blocks[block_link(fn)].size == sizes[i]);
      sz = blocks[block_link(fn)].size;
    }
  }

  template <typename Owner>
  inline void init(Owner const& owner)
  {}

protected:
  // Private
  inline void add_free_after_begin(block_bank& blocks, std::uint32_t block) noexcept
  {
    auto blkid            = block_link(block);
    blocks[blkid].is_free = true;
    auto size             = blocks[blkid].size;
    auto it               = find_free_it(sizes.data(), sizes.size(), size);
    free_ordering.emplace(free_ordering.begin() + it, block);
    sizes.emplace(sizes.begin() + it, size);
  }

  static inline auto mini0(size_type const* it, size_t size, size_type key) noexcept
  {
    while (size > 2)
      ACL_BINARY_SEARCH_STEP;
    it += size > 1 && (*it < key);
    it += size > 0 && (*it < key);
    return it;
  }

  static inline auto mini1(size_type const* it, size_t size, size_type key) noexcept
  {
    do
    {
      ACL_BINARY_SEARCH_STEP;
    }
    while (size > 2);
    it += size > 1 && (*it < key);
    it += size > 0 && (*it < key);
    return it;
  }

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

  static inline auto find_free_it_(size_type const* it, size_t s, size_type key) noexcept
  {
    if constexpr (bsearch_algo == 0)
      return mini0(it, s, key);
    else if constexpr (bsearch_algo == 1)
      return mini1(it, s, key);
    else if constexpr (bsearch_algo == 2)
      return mini2(it, s, key);
  }

  static inline auto find_free_it(size_type const* it, size_t s, size_type key) noexcept
  {
    return std::distance(it, find_free_it_(it, s, key));
  }

  inline optional_addr find_free(size_type size) const noexcept
  {
    auto it = find_free_it_(sizes.data(), sizes.size(), size);
    return (it < (sizes.data() + sizes.size())) ? optional_addr(it) : optional_addr(nullptr);
  }

  inline void reinsert_left(block_bank& blocks, size_t of, size_type size, std::uint32_t node) noexcept
  {
    if (!of)
    {
      free_ordering[of] = node;
      sizes[of]         = size;
    }
    else
    {
      auto it = find_free_it(sizes.data(), of, size);
      if (it != of)
      {
        std::size_t count = of - it;
        {
          auto src  = sizes.data() + it;
          auto dest = src + 1;
          std::memmove(dest, src, count * sizeof(size_type));
        }
        {
          auto src  = free_ordering.data() + it;
          auto dest = src + 1;
          std::memmove(dest, src, count * sizeof(std::uint32_t));
        }

        free_ordering[it] = node;
        sizes[it]         = size;
      }
      else
      {
        free_ordering[of] = node;
        sizes[of]         = size;
      }
    }
  }

  inline void reinsert_right(block_bank& blocks, size_t of, size_type size, std::uint32_t node)
  {
    auto next = of + 1;
    if (next == sizes.size())
    {
      free_ordering[of] = node;
      sizes[of]         = size;
    }
    else
    {
      auto it = find_free_it(sizes.data() + next, sizes.size() - next, size);
      if (it)
      {
        std::size_t count = it;
        {
          auto dest = sizes.data() + of;
          auto src  = dest + 1;
          std::memmove(dest, src, count * sizeof(size_type));
          auto ptr = (dest + count);
          *ptr     = size;
        }

        {
          auto dest = free_ordering.data() + of;
          auto src  = dest + 1;
          std::memmove(dest, src, count * sizeof(uint32_t));
          auto ptr = (dest + count);
          *ptr     = node;
        }
      }
      else
      {
        free_ordering[of] = node;
        sizes[of]         = size;
      }
    }
  }

  size_list         sizes;
  detail::free_list free_ordering;
};

/// alloc_strategy::best_fit Impl
#undef ACL_BINARY_SEARCH_STEP
} // namespace acl::strat
