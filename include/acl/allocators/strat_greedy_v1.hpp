#pragma once
#include <acl/allocators/arena.hpp>

namespace acl::strat
{

/**
 * @brief This class provides a mechanism to allocate blocks of addresses
 *        by linearly searching through a list of available free sizes and
 *        returning the first chunk that can fit the requested memory size.
 */
template <typename Options = acl::options<>>
class greedy_v1
{
  using optional_addr = detail::optional_val<detail::k_null_0>;

public:
  using extension       = uint64_t;
  using size_type       = detail::choose_size_t<uint32_t, Options>;
  using arena_bank      = detail::arena_bank<size_type, extension>;
  using block_bank      = detail::block_bank<size_type, extension>;
  using block           = detail::block<size_type, extension>;
  using bank_data       = detail::bank_data<size_type, extension>;
  using block_link      = typename block_bank::link;
  using allocate_result = optional_addr;

  static constexpr size_type min_granularity = 4;

  greedy_v1() noexcept            = default;
  greedy_v1(greedy_v1 const&)     = default;
  greedy_v1(greedy_v1&&) noexcept = default;

  greedy_v1& operator=(greedy_v1 const&)     = default;
  greedy_v1& operator=(greedy_v1&&) noexcept = default;

  inline optional_addr try_allocate(bank_data& bank, size_type size)
  {
    uint32_t i = head;
    while (i)
    {
      auto const& blk = bank.blocks[block_link(i)];
      if (blk.size >= size)
        return optional_addr(i);
      i = blk.list_.next;
    }
    return optional_addr();
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, optional_addr found)
  {
    auto& blk = bank.blocks[block_link(found.value)];
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

      auto newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, blk.list_, true);
      list.insert_after(bank.blocks, found.value, (uint32_t)newblk);

      if (blk.list_.next)
        bank.blocks[block_link(blk.list_.next)].list_.prev = (uint32_t)newblk;
      if (blk.list_.prev)
        bank.blocks[block_link(blk.list_.prev)].list_.next = (uint32_t)newblk;
      else
        head = (uint32_t)newblk;
      blk.list_ = {};
    }
    else
    {
      erase(bank.blocks, found.value);
    }
    return found.value;
  }

  inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    add_free(blocks, block);
  }

  inline void add_free(block_bank& blocks, std::uint32_t block)
  {
    auto  hblock = block_link(block);
    auto& blk    = blocks[hblock];
    ACL_ASSERT(blk.list_.prev == 0);
    blk.list_.next = head;
    if (head)
      blocks[block_link(head)].list_.prev = block;
    head = block;
  }

  inline void grow_free_node(block_bank& blocks, std::uint32_t block, size_type newsize)
  {
    erase(blocks, block);
    blocks[block_link(block)].size = newsize;
    add_free(blocks, block);
  }

  inline void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
  {
    erase(blocks, block);
    blocks[block_link(new_block)].size = new_size;
    add_free(blocks, new_block);
  }

  inline void erase(block_bank& blocks, std::uint32_t node)
  {
    auto& blk = blocks[block_link(node)];
    if (blk.list_.next)
      blocks[block_link(blk.list_.next)].list_.prev = blk.list_.prev;
    if (blk.list_.prev)
      blocks[block_link(blk.list_.prev)].list_.next = blk.list_.next;
    else
      head = blk.list_.next;
    blk.list_ = {};
  }

  inline std::uint32_t total_free_nodes(block_bank const& blocks) const
  {
    uint32_t count = 0;
    uint32_t i     = head;
    while (i)
    {
      auto const& blk = blocks[block_link(i)];
      ACL_ASSERT(blk.size);
      count++;
      i = blk.list_.next;
    }
    return count;
  }

  inline size_type total_free_size(block_bank const& blocks) const
  {
    size_type sz = 0;
    uint32_t  i  = head;
    while (i)
    {
      auto const& blk = blocks[block_link(i)];
      sz += blk.size;
      i = blk.list_.next;
    }
    return sz;
  }

  void validate_integrity(block_bank const& blocks) const
  {
    uint32_t i = head;
    uint32_t p = 0;
    while (i)
    {
      auto const& blk = blocks[block_link(i)];
      ACL_ASSERT(blk.is_free);
      ACL_ASSERT(blk.list_.prev == p);
      p = i;
      i = blk.list_.next;
    }
  }

  template <typename Owner>
  inline void init(Owner const& owner)
  {}

protected:
  // Private

  uint32_t head = 0;
};

/**
 * alloc_strategy::best_fit Impl
 */

} // namespace acl::strat