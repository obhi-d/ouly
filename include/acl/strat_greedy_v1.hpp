#pragma once
#include "detail/arena.hpp"

namespace acl::strat
{

/// \remarks
/// This class provides a mechanism to allocate blocks of addresses
/// by linearly searching through a list of available free sizes and
/// returning the first chunk that can fit the requested memory size.
template <typename usize_type>
class greedy_v1
{
  using optional_addr = detail::voptional<detail::k_null_0>;

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
    uint32_t i = free_list;
    while (i)
    {
      auto const& blk = bank.blocks[block_link(i)];
      if (blk.size >= size)
        return optional_addr(i);
      i = blk.reserved32_;
    }
    return optional_addr();
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, uint32_t found)
  {
    auto& blk = bank.blocks[block_link(found)];
    // Marker
    size_type     offset    = blk.offset;
    std::uint32_t arena_num = blk.arena;

    blk.is_free = false;

    auto remaining = blk.size - size;
    blk.size       = size;
    if (remaining > 0)
    {
      auto& list  = bank.arenas[blk.arena].block_order;
      auto  arena = blk.arena;

      auto newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, free_list, true);
      list.insert_after(bank.blocks, found, (uint32_t)newblk);
      // reinsert the left-over size in free list
      free_list = (uint32_t)newblk;
    }

    return found;
  }

  inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    add_free(blocks, block);
  }

  inline void add_free(block_bank& blocks, std::uint32_t block)
  {
    auto  hblock    = block_link(block);
    auto& blk       = blocks[hblock];
    blk.reserved32_ = free_list;
    free_list       = block;
  }

  inline void replace(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
  {
    erase(blocks, block);
    erase(blocks, new_block);
    blocks[block_link(new_block)].size = new_size;
    add_free(blocks, new_block);
  }

  inline void erase(block_bank& blocks, std::uint32_t node) {}

  inline std::uint32_t total_free_nodes(block_bank const& blocks) const
  {
    uint32_t count = 0;
    uint32_t i     = free_list;
    while (i)
    {
      auto const& blk = blocks[block_link(i)];
      assert(blk.size);
      count++;
      i = blk.reserved32_;
    }
    return count;
  }

  inline usize_type total_free_size(block_bank const& blocks) const
  {
    size_type sz = 0;
    uint32_t  i  = free_list;
    while (i)
    {
      auto const& blk = blocks[block_link(i)];
      sz += blk.size;
      i = blk.reserved32_;
    }
    return sz;
  }

  void validate_integrity(block_bank const& blocks) const {}

protected:
  // Private

  uint32_t free_list = 0;
};

/// alloc_strategy::best_fit Impl

} // namespace acl::strat