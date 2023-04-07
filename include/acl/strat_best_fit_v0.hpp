#pragma once
#include "detail/arena.hpp"

namespace acl::strat
{

/// @brief  Strategy class for arena_allocator that stores a
///   sorted list of free available slots.
///   Binary search is used to find the best slot that fits
///   the requested memory
template <typename usize_type>
class best_fit_v0
{
  using optional_addr = detail::voptional<detail::k_null_32>;

public:
  static constexpr usize_type min_granularity = 4;

  using extension       = uint64_t;
  using size_type       = usize_type;
  using arena_bank      = detail::arena_bank<size_type, extension>;
  using block_bank      = detail::block_bank<size_type, extension>;
  using block           = detail::block<size_type, extension>;
  using bank_data       = detail::bank_data<size_type, extension>;
  using block_link      = typename block_bank::link;
  using allocate_result = uint32_t;

  inline optional_addr try_allocate(bank_data& bank, size_type size)
  {
    if (free_ordering.size() == 0 || bank.blocks[block_link(free_ordering.back())].size < size)
      return optional_addr();
    return optional_addr(find_free(bank.blocks, 0, static_cast<uint32_t>(free_ordering.size()), size));
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, uint32_t found)
  {
    assert(found < static_cast<uint32_t>(free_ordering.size()));

    std::uint32_t free_node = free_ordering[found];
    auto&         blk       = bank.blocks[block_link(free_node)];
    // Marker
    size_type     offset    = blk.offset;
    std::uint32_t arena_num = blk.arena;

    blk.is_free = false;

    auto remaining = blk.size - size;
    blk.size       = size;
    if (remaining > 0)
    {
      auto& list   = bank.arenas[blk.arena].block_order;
      auto  arena  = blk.arena;
      auto  newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, detail::k_null_sz<uhandle>, true);
      list.insert_after(bank.blocks, free_node, (uint32_t)newblk);
      // reinsert the left-over size in free list
      reinsert_left(bank.blocks, found, (uint32_t)newblk);
    }
    else
    {
      // delete the existing found index from free list
      free_ordering.erase(free_ordering.begin() + found);
    }

    return free_node;
  }

  inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    free_ordering.push_back(block);
  }

  inline void add_free(block_bank& blocks, std::uint32_t block)
  {
    add_free_after(blocks, 0, block);
  }

  inline void replace(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
  {
    size_type size = blocks[block_link(block)].size;
    if (size == new_size && block == new_block)
      return;

    auto nbfree = static_cast<uint32_t>(free_ordering.size());
    auto it     = find_free(blocks, 0, nbfree, size);
    while (it != nbfree && free_ordering[it] != block)
      it++;
    assert(it != nbfree);

    blocks[block_link(new_block)].size = new_size;
    if (size < new_size)
      reinsert_right(blocks, it, new_block);
    else if (size > new_size)
      reinsert_left(blocks, it, new_block);
    else
      free_ordering[it] = new_block;
  }

  inline void erase(block_bank& blocks, std::uint32_t node)
  {
    auto count = static_cast<uint32_t>(free_ordering.size());
    auto it    = find_free(blocks, 0, count, blocks[block_link(node)].size);
    while (it != count && free_ordering[it] != node)
      it++;
    assert(it != count);
    free_ordering.erase(free_ordering.begin() + it);
  }

  inline std::uint32_t total_free_nodes(block_bank const& blocks) const
  {
    return static_cast<std::uint32_t>(free_ordering.size());
  }

  inline usize_type total_free_size(block_bank const& blocks) const
  {
    size_type sz = 0;
    for (auto fn : free_ordering)
    {
      assert(blocks[block_link(fn)].is_free);
      sz += blocks[block_link(fn)].size;
    }

    return sz;
  }

  void validate_integrity(block_bank const& blocks) const
  {
    size_type sz = 0;
    for (auto fn : free_ordering)
    {
      assert(sz <= blocks[block_link(fn)].size);
      sz = blocks[block_link(fn)].size;
    }
  }

protected:
  // Private
  inline void add_free_after(block_bank& blocks, uint32_t loc, std::uint32_t block)
  {
    auto blkid            = block_link(block);
    blocks[blkid].is_free = true;
    auto it = find_free_it(blocks, free_ordering.begin() + loc, free_ordering.end(),
                           blocks[blkid].size);
    free_ordering.emplace(it, block);
  }

  template <typename It>
  static inline auto find_free_it(block_bank& blocks, It b, It e,
                           size_type i_size) 
  {
    return std::lower_bound(b, e, i_size,
                            [&blocks](std::uint32_t block, size_type i_size) -> bool
                            {
                              return blocks[block_link(block)].size < i_size;
                            });
  }

  inline uint32_t find_free(block_bank& blocks, uint32_t b, uint32_t e, size_type i_size) const
  {
    auto end = free_ordering.begin() + e;
    auto it  = find_free_it(blocks, free_ordering.begin() + b, end, i_size);
    return (it != end) ? (uint32_t)std::distance(free_ordering.begin(), it) : detail::k_null_32;
  }

  inline void reinsert_left(block_bank& blocks, uint32_t iof, std::uint32_t node)
  {
    if (!iof)
    {
      free_ordering[iof] = node;
      return;
    }
    auto of = free_ordering.begin() + iof;
    auto it = find_free_it(blocks, free_ordering.begin(), of, blocks[block_link(node)].size);
    if (it != of)
    {
      std::uint32_t* src   = &(*it);
      std::uint32_t* dest  = src + 1;
      size_t         count = std::distance(it, of);
      std::memmove(dest, src, count * sizeof(std::uint32_t));
      *it = node;
    }
    else
    {
      *it = node;
    }
  }

  inline void reinsert_right(block_bank& blocks, uint32_t iof, std::uint32_t node)
  {
    auto of     = free_ordering.begin() + iof;
    auto end_it = free_ordering.end();
    auto next   = std::next(of);
    if (next == end_it)
    {
      *of = node;
      return;
    }
    auto it = find_free_it(blocks, next, end_it, blocks[block_link(node)].size);
    if (it != next)
    {
      std::uint32_t* dest  = &(*of);
      std::uint32_t* src   = dest + 1;
      size_t         count = std::distance(next, it);
      std::memmove(dest, src, count * sizeof(std::uint32_t));
      auto ptr = (dest + count);
      *ptr     = node;
    }
    else
    {
      *of = node;
    }

  }

  detail::free_list free_ordering;
};

/// alloc_strategy::best_fit Impl

} // namespace acl::strat