#pragma once
#include "arena.hpp"

namespace acl::detail
{

//  ██████╗-███████╗███████╗████████╗-----███████╗██╗████████╗
//  ██╔══██╗██╔════╝██╔════╝╚══██╔══╝-----██╔════╝██║╚══██╔══╝
//  ██████╔╝█████╗--███████╗---██║--------█████╗--██║---██║---
//  ██╔══██╗██╔══╝--╚════██║---██║--------██╔══╝--██║---██║---
//  ██████╔╝███████╗███████║---██║███████╗██║-----██║---██║---
//  ╚═════╝-╚══════╝╚══════╝---╚═╝╚══════╝╚═╝-----╚═╝---╚═╝---
//  ----------------------------------------------------------
template <typename traits>
class alloc_strategy_impl<alloc_strategy::best_fit, traits>
{
public:
  using size_type  = typename traits::size_type;
  using arena_bank = detail::arena_bank<traits>;
  using block_bank = detail::block_bank<traits>;
  using block      = detail::block<traits>;
  using alloc_desc = acl::alloc_desc<size_type>;
  using bank_data  = detail::bank_data<traits>;

  inline free_list::iterator try_allocate(bank_data& bank, size_type size);
  inline free_list::iterator try_allocate(bank_data& bank, size_type size, free_list::iterator from);
  inline std::uint32_t       commit(bank_data& bank, size_type size, free_list::iterator);

  inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block);
  inline void add_free(block_bank& blocks, std::uint32_t block);
  inline void replace(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size);

  inline std::uint32_t node(free_list::iterator it);
  inline bool          is_valid(free_list::iterator it);

  inline void erase(block_bank& blocks, std::uint32_t node);

  inline std::uint32_t total_free_nodes(block_bank& blocks) const;
  inline size_type     total_free_size(block_bank& blocks) const;

  void validate_integrity(block_bank& blocks);

private:
  // Private
  inline void                add_free_after(block_bank& blocks, free_list::iterator loc, std::uint32_t block);
  inline free_list::iterator find_free(block_bank& blocks, free_list::iterator b, free_list::iterator e,
                                       size_type i_size);
  inline free_list::iterator reinsert_left(block_bank& blocks, free_list::iterator of, std::uint32_t node);
  inline free_list::iterator reinsert_right(block_bank& blocks, free_list::iterator of, std::uint32_t node);

  inline static constexpr std::uint32_t null()
  {
    return detail::k_null_32;
  }

  free_list free_ordering;
};

/// alloc_strategy::best_fit Impl

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::try_allocate(bank_data& bank,
                                                                                               size_type  size)
{
  if (free_ordering.size() == 0 || bank.blocks[free_ordering.back()].size < size)
    return free_ordering.end();
  return find_free(bank.blocks, free_ordering.begin(), free_ordering.end(), size);
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::try_allocate(bank_data&          bank,
                                                                                               size_type           size,
                                                                                               free_list::iterator prev)
{
  return find_free(bank.blocks, std::next(prev), free_ordering.end(), size);
}

template <typename traits>
inline std::uint32_t alloc_strategy_impl<alloc_strategy::best_fit, traits>::commit(bank_data& bank, size_type size,
                                                                                   free_list::iterator found)
{
  if (found == free_ordering.end())
  {
    return null();
  }
  std::uint32_t free_node = *found;
  auto&         blk       = bank.blocks[free_node];
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
    list.insert_after(bank.blocks, free_node, newblk);
    // reinsert the left-over size in free list
    reinsert_left(bank.blocks, found, newblk);
  }
  else
  {
    // delete the existing found index from free list
    free_ordering.erase(found);
  }

  return free_node;
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::add_free_arena([[maybe_unused]] block_bank& blocks,
                                                                                  std::uint32_t                block)
{
  free_ordering.push_back(block);
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::replace(block_bank& blocks, std::uint32_t block,
                                                                           std::uint32_t new_block, size_type new_size)
{
  size_type size = blocks[block].size;
  if (size == new_size && block == new_block)
    return;

  auto it = find_free(blocks, free_ordering.begin(), free_ordering.end(), size);
  while (it != free_ordering.end() && *it != block)
    it++;
  assert(it != free_ordering.end());

  blocks[new_block].size = new_size;
  if (size < new_size)
    reinsert_right(blocks, it, new_block);
  else if (size > new_size)
    reinsert_left(blocks, it, new_block);
  else
    *it = new_block;
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::add_free(block_bank& blocks, std::uint32_t block)
{
  add_free_after(blocks, free_ordering.begin(), block);
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::add_free_after(block_bank&         blocks,
                                                                                  free_list::iterator loc,
                                                                                  std::uint32_t       block)
{
  blocks[block].is_free = true;
  auto it               = find_free(blocks, loc, free_ordering.end(), blocks[block].size);
  free_ordering.emplace(it, block);
}

template <typename traits>
inline std::uint32_t alloc_strategy_impl<alloc_strategy::best_fit, traits>::node(free_list::iterator it)
{
  return *it;
}

template <typename traits>
inline bool alloc_strategy_impl<alloc_strategy::best_fit, traits>::is_valid(free_list::iterator it)
{
  return it != free_ordering.end();
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::erase(block_bank& blocks, std::uint32_t node)
{
  auto it = find_free(blocks, free_ordering.begin(), free_ordering.end(), blocks[node].size);
  while (it != free_ordering.end() && *it != node)
    it++;
  assert(it != free_ordering.end());
  free_ordering.erase(it);
}

template <typename traits>
inline std::uint32_t alloc_strategy_impl<alloc_strategy::best_fit, traits>::total_free_nodes(block_bank& blocks) const
{
  return static_cast<std::uint32_t>(free_ordering.size());
}

template <typename traits>
inline typename alloc_strategy_impl<alloc_strategy::best_fit, traits>::size_type alloc_strategy_impl<
  alloc_strategy::best_fit, traits>::total_free_size(block_bank& blocks) const
{
  size_type sz = 0;
  for (auto fn : free_ordering)
  {
    assert(blocks[fn].is_free);
    sz += blocks[fn].size;
  }

  return sz;
}

template <typename traits>
inline void alloc_strategy_impl<alloc_strategy::best_fit, traits>::validate_integrity(block_bank& blocks)
{
  size_type sz = 0;
  for (auto fn : free_ordering)
  {
    assert(sz <= blocks[fn].size);
    sz = blocks[fn].size;
  }
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::find_free(block_bank&         blocks,
                                                                                            free_list::iterator b,
                                                                                            free_list::iterator e,
                                                                                            size_type           i_size)
{
  return std::lower_bound(b, e, i_size,
                          [&blocks](std::uint32_t block, size_type i_size) -> bool
                          {
                            return blocks[block].size < i_size;
                          });
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::reinsert_left(block_bank& blocks,
                                                                                                free_list::iterator of,
                                                                                                std::uint32_t node)
{
  auto begin_it = free_ordering.begin();
  if (begin_it == of)
  {
    *of = node;
    return of;
  }
  auto it = find_free(blocks, begin_it, of, blocks[node].size);
  if (it != of)
  {
    std::uint32_t* src   = &*it;
    std::uint32_t* dest  = src + 1;
    size_t         count = std::distance(it, of);
    std::memmove(dest, src, count * sizeof(std::uint32_t));
    *it = node;
    return it;
  }
  else
  {
    *of = node;
    return it;
  }
}

template <typename traits>
inline free_list::iterator alloc_strategy_impl<alloc_strategy::best_fit, traits>::reinsert_right(block_bank& blocks,
                                                                                                 free_list::iterator of,
                                                                                                 std::uint32_t node)
{

  auto end_it = free_ordering.end();
  auto next   = std::next(of);
  if (next == end_it)
  {
    *of = node;
    return of;
  }
  auto it = find_free(blocks, next, end_it, blocks[node].size);
  if (it != next)
  {
    std::uint32_t* dest  = &(*of);
    std::uint32_t* src   = dest + 1;
    size_t         count = std::distance(next, it);
    std::memmove(dest, src, count * sizeof(std::uint32_t));
    auto ptr = (dest + count);
    *ptr     = node;
    return free_ordering.begin() + std::distance(free_ordering.data(), ptr);
  }
  else
  {
    *of = node;
    return it;
  }
}
} // namespace acl::detail