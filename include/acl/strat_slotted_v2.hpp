#pragma once

#include "default_allocator.hpp"
#include "detail/common.hpp"
#include "strat_best_fit_v0.hpp"

namespace acl::strat
{

/// \remarks
/// This strategy employes an immediate cache of memory of fixed size
///   Blocks are allocated from cache, if free blocks are available in cache
///   Cache size is controlled by fixed_max_per_slot size, which is allocated per bucket slot
///   The bucket granularity is fixed, and the expectation is allocation is based
///   on a multiple of this unit size.
///   The certain number of bucket slots are made avialable based on max_bucket
template <typename usize_t = std::size_t, std::size_t granularity = 256, std::size_t max_bucket = 255,
          std::size_t fixed_max_per_slot = 8, usize_t search_window = 4,
          typename fallback = strat::best_fit_v0<usize_t>>
class slotted_v2
{
public:
  using fallback_allocator = fallback;

  using extension                = fallback_allocator::extension;
  using fallback_allocate_result = typename fallback_allocator::allocate_result;

  static constexpr usize_t min_granularity = static_cast<usize_t>(granularity);

  using size_type  = usize_t;
  using arena_bank = detail::arena_bank<size_type, extension>;
  using block_bank = detail::block_bank<size_type, extension>;
  using block      = detail::block<size_type, extension>;
  using bank_data  = detail::bank_data<size_type, extension>;
  using block_link = typename block_bank::link;

  static constexpr size_type max_size_     = static_cast<size_type>(granularity * max_bucket);
  static constexpr size_type sz_div        = static_cast<size_type>(detail::log2(granularity));
  static constexpr size_type sz_mask       = static_cast<size_type>(granularity) - 1;
  static constexpr uint32_t  bucket_mask   = 0x80000000;
  static constexpr uint32_t  bucket_unmask = 0x7fffffff;

  struct bucket_idx
  {
    uint32_t value = 0;
  };

  using allocate_result_v = std::variant<std::monostate, fallback_allocate_result, bucket_idx>;
  using allocate_result   = detail::variant_result<allocate_result_v>;

  inline allocate_result try_allocate(bank_data& bank, size_type size)
  {
    if (size <= max_size_)
    {
      size_type id = (size >> sz_div);
      size_type nb = std::min<size_type>(search_window + id, static_cast<size_type>(buckets.size()));
      if (id < nb)
      {
        for (; id < nb; ++id)
        {
          if (!buckets[id].empty())
          {
            return allocate_result(bucket_idx{id});
          }
        }
      }
    }

    if (auto fta = fallback.try_allocate(bank, size))
      return allocate_result(*fta);
    return allocate_result();
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, allocate_result_v const& vdx)
  {
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
    auto  block = buckets[udx].pop_back();
    auto& blk   = bank.blocks[block];

    auto s = ((granularity << udx) - size);

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
      auto newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, 0, true, true);
      list.insert_after(bank.blocks, (uint32_t)block, (uint32_t)newblk);

      if (!buckets[remaining >> sz_div].try_emplace(newblk))
      {
        auto& blk      = bank.blocks[newblk];
        blk.is_slotted = false;
        blk.ext        = extension();
        fallback.add_free(bank.blocks, (uint32_t)newblk);
      }
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
    if (b.size <= max_size_)
    {
      size_type id = (b.size >> sz_div);
      if (id >= buckets.size())
        buckets.resize(id + 1);
      if (!(b.is_slotted = buckets[id].try_emplace(block_link(block))))
      {
        fallback.add_free(blocks, block);
      }
    }
    else
    {
      assert(!b.is_slotted);
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
      auto& vec    = buckets[(b.size >> sz_div)];
      vec.erase(block_link(block));
    }
    else
    {
      fallback.erase(blocks, block);
    }
  }

  inline std::uint32_t total_free_nodes(block_bank const& blocks) const
  {
    std::uint32_t node_count = 0;
    for (auto const& vv : buckets)
      node_count += static_cast<uint32_t>(vv.size);
    return node_count + fallback.total_free_nodes(blocks);
  }

  inline size_type total_free_size(block_bank const& blocks) const
  {
    size_type size = 0;
    for (uint32_t i = 1; i < static_cast<uint32_t>(buckets.size()); ++i)
      size += static_cast<size_type>(granularity * i) * static_cast<size_type>(buckets[i].size);
    return size + fallback.total_free_size(blocks);
  }

  void validate_integrity(block_bank const& blocks) const
  {
    if (!buckets.empty())
    {
      assert(buckets[0].empty());
      for (uint32_t i = 1; i < static_cast<uint32_t>(buckets.size()); ++i)
      {
        for (uint32_t v = 0; v < buckets[i].size; ++v)
        {
          auto const& b = blocks[block_link(buckets[i].slots[v])];
          assert(b.is_slotted);
          assert(b.size == static_cast<size_type>(granularity * i));
        }
      }
    }

    fallback.validate_integrity(blocks);
  }

  template <typename Owner>
  inline void init(Owner const& owner)
  {}

  slotted_v2() noexcept                                   = default;
  slotted_v2(slotted_v2 const& other) noexcept            = delete;
  slotted_v2& operator=(slotted_v2 const& other) noexcept = delete;
  slotted_v2(slotted_v2&& other) noexcept : buckets(std::move(other.buckets)), fallback(std::move(other.fallback)) {}
  inline slotted_v2& operator=(slotted_v2&& other) noexcept
  {
    buckets  = std::move(other.buckets);
    fallback = std::move(other.fallback);
    return *this;
  }

private:
  struct bucket_content
  {
    inline bool empty() const
    {
      return size == 0;
    }

    inline auto pop_back()
    {
      return slots[--size];
    }

    inline bool try_emplace(block_link b)
    {
      if (size < fixed_max_per_slot)
      {
        slots[size++] = b;
        return true;
      }
      return false;
    }

    inline void erase(block_link l)
    {
      for (uint32_t i = 0; i < size; ++i)
      {
        if (slots[i].value() == l.value())
        {
          if (size)
            slots[i] = slots[size - 1];
          size--;
          break;
        }
      }
    }

    std::array<block_link, fixed_max_per_slot> slots;
    uint32_t                                   size = 0;
  };

  using bucket_list = std::vector<bucket_content>;
  bucket_list        buckets;
  fallback_allocator fallback;
};

} // namespace acl::strat