#pragma once

#include "default_allocator.hpp"
#include "strat_best_fit_v0.hpp"
#include <acl/utils/common.hpp>

namespace acl::strat
{

/**
 * @brief This strategy employes an immediate cache of memory of dynamic size
 *   Blocks are allocated from cache, if free blocks are available in cache
 *   Cache size grows as new fixed granulairty memory blocks are freed.
 *   Cache is not linear in memory, rather a linked list of free cache blocks per slot
 *   is maitained (which has some additional overhead)
 *   The memory is tightly packed as compared to slotted_v0.
 *   The bucket granularity is fixed, and the expectation is allocation is based
 *   on a multiple of this unit size.
 *   The certain number of bucket slots are made avialable based on max_bucket
 */
template <typename Options = acl::options<>>
class slotted_v1
{
public:
  static constexpr auto granularity   = detail::granularity_v<Options>;
  static constexpr auto max_bucket    = detail::max_bucket_v<Options>;
  static constexpr auto search_window = detail::search_window_v<Options>;
  using fallback_allocator            = detail::fallback_strat_t<Options, strat::best_fit_v0<Options>>;

  using extension                = typename fallback_allocator::extension;
  using fallback_allocate_result = typename fallback_allocator::allocate_result;

  using size_type  = detail::choose_size_t<uint32_t, Options>;
  using arena_bank = detail::arena_bank<size_type, extension>;
  using block_bank = detail::block_bank<size_type, extension>;
  using block      = detail::block<size_type, extension>;
  using bank_data  = detail::bank_data<size_type, extension>;
  using block_link = typename block_bank::link;

  static constexpr size_type min_granularity = static_cast<size_type>(granularity);

  static constexpr size_type max_bucket_   = max_bucket + 1;
  static constexpr size_type max_size_     = static_cast<size_type>(granularity * max_bucket);
  static constexpr size_type sz_div        = static_cast<size_type>(detail::log2(granularity));
  static constexpr size_type sz_mask       = static_cast<size_type>(granularity) - 1;
  static constexpr uint32_t  bucket_mask   = 0x80000000;
  static constexpr uint32_t  bucket_unmask = 0x7fffffff;

  struct bucket_idx
  {
    uint32_t value = 0;
  };

  using allocate_result_v = detail::variant_result<std::variant<std::monostate, fallback_allocate_result, bucket_idx>>;
  using allocate_result   = allocate_result_v;

  [[nodiscard]] inline allocate_result try_allocate(bank_data& bank, size_type size)
  {
    if (size < max_size_)
    {
      size_type id = (size >> sz_div);
      size_type ac = std::min<size_type>(search_window + id, max_bucket_);
      for (; id < ac; ++id)
      {
        if ((uint32_t)buckets[id].block)
        {
          return allocate_result_v(bucket_idx{id});
        }
      }
    }

    if (auto fta = fallback.try_allocate(bank, size))
      return allocate_result_v(*fta);
    return allocate_result_v();
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, allocate_result_v const& r)
  {
    auto const& vdx = *r;
    if (std::holds_alternative<fallback_allocate_result>(vdx))
    {
      // Commit will result in a split, but we do not ask for the
      // splitted block if its size is less than max_size.
      // We instead wait for the release of the current block,
      // or other blocks, when they are freed we reclaim and put
      // them inside bucket.
      return fallback.commit(bank, size, std::get<fallback_allocate_result>(vdx));
    }

    auto  udx   = std::get<bucket_idx>(vdx).value;
    auto  block = buckets[udx].block;
    auto& blk   = bank.blocks[block_link(block)];

    remove_free_top(bank.blocks, udx);

    auto s = ((granularity * udx) - size);

    size_type     offset    = blk.offset;
    std::uint32_t arena_num = blk.arena;

    blk.is_free    = false;
    blk.is_slotted = false;

    auto remaining = blk.size - size;
    blk.size       = size;
    if (remaining > 0)
    {
      auto& list  = bank.arenas[blk.arena].block_order;
      auto  arena = blk.arena;
      // Create a new free block, since its smaller than the original block, insert it in the bucket
      auto newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, extension(), true, true);
      list.insert_after(bank.blocks, (uint32_t)block, (uint32_t)newblk);
      // append to free list
      append_free_top(bank.blocks, (uint32_t)(remaining >> sz_div), newblk);
    }

    return (uint32_t)block;
  }

  inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    add_free(blocks, block);
  }

  inline void add_free(block_bank& blocks, std::uint32_t block)
  {
    auto& b = blocks[block_link(block)];
    if (b.size < max_size_)
    {
      b.is_slotted = true;
      append_free_top(blocks, (uint32_t)(b.size >> sz_div), block_link(block));
    }
    else
    {
      ACL_ASSERT(b.is_slotted == false);
      fallback.add_free(blocks, block);
    }
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

  inline void erase(block_bank& blocks, std::uint32_t block)
  {
    auto& b = blocks[block_link(block)];
    if (b.is_slotted)
    {
      b.is_slotted = false;
      // are we removing top?
      if (b.rtup_.second < max_bucket_)
        remove_free_top(blocks, b.rtup_.second);
      else
      {
        // Remember, granularity >> log2(granularity) = 1,
        // so first slot is still empty
        auto& buck = buckets[b.rtup_.second];
        if (b.rtup_.first)
        {
          buckets[b.rtup_.first].next = buck.next;
        }
        if (buck.next)
          blocks[buckets[buck.next].block].rtup_.first = b.rtup_.first;
        buck.block   = block_link(0);
        buck.next    = free_entries;
        free_entries = b.rtup_.second;
      }
    }
    else
    {
      fallback.erase(blocks, block);
    }
  }

  inline std::uint32_t total_free_nodes(block_bank const& blocks) const
  {
    std::uint32_t count = 0;
    for (std::size_t i = 0; i < buckets.size(); ++i)
    {
      auto const& b = buckets[i];
      if ((uint32_t)b.block)
      {
        ACL_ASSERT(blocks[b.block].is_free);
        count++;
      }
    }
    return count + fallback.total_free_nodes(blocks);
  }

  inline size_type total_free_size(block_bank const& blocks) const
  {
    size_type size = 0;
    for (auto const& b : buckets)
    {
      if ((uint32_t)b.block)
        size += blocks[b.block].size;
    }

    return size + fallback.total_free_size(blocks);
  }

  void validate_integrity(block_bank const& blocks) const
  {
    uint32_t    f              = free_entries;
    std::size_t nb_free_slots  = 0;
    std::size_t nb_free_nodes  = 0;
    std::size_t nb_empty_slots = 0;
    while (f)
    {
      ACL_ASSERT(buckets[f].block == block_link(0));
      f = buckets[f].next;
      nb_free_slots++;
    }

    for (uint32_t i = 0; i < max_bucket_; ++i)
    {
      auto const& b = buckets[i];
      if ((uint32_t)b.block)
      {
        uint32_t prev = 0;
        uint32_t curr = i;
        while (curr)
        {
          auto const& ib  = buckets[curr];
          auto const& blk = blocks[ib.block];
          ACL_ASSERT(blk.rtup_.first == prev);
          ACL_ASSERT(blk.rtup_.second == curr);
          prev = curr;
          curr = ib.next;
          nb_free_nodes++;
        }
      }
      else if (!b.next)
      {
        nb_empty_slots++;
      }
    }

    ACL_ASSERT(buckets.size() - (nb_free_slots + nb_empty_slots) == nb_free_nodes);
    fallback.validate_integrity(blocks);
  }

  template <typename Owner>
  inline void init(Owner const& owner)
  {}

  slotted_v1() noexcept                                   = default;
  slotted_v1(slotted_v1 const& other) noexcept            = delete;
  slotted_v1& operator=(slotted_v1 const& other) noexcept = delete;
  slotted_v1(slotted_v1&& other) noexcept : buckets(std::move(other.buckets)), fallback(std::move(other.fallback)) {}
  inline slotted_v1& operator=(slotted_v1&& other) noexcept
  {
    buckets            = std::move(other.buckets);
    fallback           = std::move(other.fallback);
    free_entries       = other.free_entries;
    other.free_entries = 0;
    return *this;
  }

private:
  // first = prev
  // second = current

  inline void remove_free_top(block_bank& bank, uint32_t udx)
  {
    auto& bucket_node = buckets[udx];
    ACL_ASSERT(bucket_node.block);
    if (bucket_node.next)
    {
      auto  next   = bucket_node.next;
      auto& nblk   = buckets[next];
      bucket_node  = nblk;
      nblk.block   = block_link(0);
      nblk.next    = free_entries;
      free_entries = next;
      // copy
      auto& blk        = bank[bucket_node.block];
      blk.rtup_.first  = 0;
      blk.rtup_.second = udx;
      if (bucket_node.next)
        bank[buckets[bucket_node.next].block].rtup_.first = udx;
    }
    else
    {
      bucket_node.block = block_link(0);
      bucket_node.next  = 0;
    }
  }

  inline void append_free_top(block_bank& bank, uint32_t idx, block_link block)
  {
    auto bucket_node = buckets[idx];
    if ((uint32_t)bucket_node.block)
    {
      auto new_slot = free_entries;
      if (free_entries)
      {
        free_entries      = buckets[free_entries].next;
        buckets[new_slot] = bucket_node;
      }
      else
      {
        new_slot = static_cast<uint32_t>(buckets.size());
        // the push back causes a resize
        // and thus changes the container pointer
        buckets.push_back(bucket_node);
      }

      if (bucket_node.next)
        bank[buckets[bucket_node.next].block].rtup_.first = new_slot;
      auto& blk         = bank[bucket_node.block];
      blk.rtup_.first   = idx;
      blk.rtup_.second  = new_slot;
      buckets[idx].next = new_slot;
    }

    buckets[idx].block = block;
    auto& blk          = bank[block];
    blk.rtup_.first    = 0;
    blk.rtup_.second   = idx;
  }
  struct chase
  {
    block_link block{0};
    uint32_t   next = 0;
  };

  std::vector<chase> buckets{max_bucket_, chase()};
  fallback_allocator fallback;
  uint32_t           free_entries = 0;
};

} // namespace acl::strat
