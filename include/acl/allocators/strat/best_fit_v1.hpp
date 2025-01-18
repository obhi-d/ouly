﻿#pragma once

#include "acl/allocators/config.hpp"
#include "acl/allocators/detail/arena.hpp"
#include "acl/utility/type_traits.hpp"
#include <optional>

namespace acl::strat
{
#define ACL_BINARY_SEARCH_STEP                                                                                         \
  {                                                                                                                    \
    const block_link* const middle = it + (size >> 1);                                                                 \
    size                           = (size + 1) >> 1;                                                                  \
    it                             = blocks[*middle].size_ < key ? middle : it;                                        \
  }

/**
 * @brief  Strategy class for arena_allocator that stores a
 *         sorted list of free available slots.
 *         Binary search is used to find the best slot that fits
 *         the requested memory
 */
template <typename Config = acl::config<>>
class best_fit_v1
{

public:
  using extension       = uint64_t;
  using size_type       = acl::detail::choose_size_t<uint32_t, Config>;
  using arena_bank      = acl::detail::arena_bank<size_type, extension>;
  using block_bank      = acl::detail::block_bank<size_type, extension>;
  using block           = acl::detail::block<size_type, extension>;
  using bank_data       = acl::detail::bank_data<size_type, extension>;
  using block_link      = typename block_bank::link;
  using size_list       = acl::vector<size_type>;
  using optional_addr   = uint32_t*;
  using allocate_result = optional_addr;

  static constexpr int bsearch_algo =
   std::conditional_t<acl::detail::HasBsearchAlgo<Config>, Config, acl::cfg::bsearch_min0>::bsearch_algo;

  static constexpr size_type min_granularity = 4;

  best_fit_v1() noexcept              = default;
  best_fit_v1(best_fit_v1 const&)     = default;
  best_fit_v1(best_fit_v1&&) noexcept = default;
  ~best_fit_v1() noexcept             = default;

  auto operator=(best_fit_v1 const&) -> best_fit_v1&     = default;
  auto operator=(best_fit_v1&&) noexcept -> best_fit_v1& = default;

  [[nodiscard]] auto try_allocate(bank_data& bank, size_type size) noexcept -> optional_addr
  {
    if (free_ordering_.empty() || bank.blocks_[block_link(free_ordering_.back())].size_ < size)
    {
      return nullptr;
    }
    return find_free(bank.blocks_, size);
  }

  auto commit(bank_data& bank, size_type size, auto found) noexcept -> std::uint32_t
  {
    auto          free_idx  = std::distance(free_ordering_.data(), found);
    std::uint32_t free_node = *found;
    auto&         blk       = bank.blocks_[block_link(free_node)];
    // Marker
    size_type     offset    = blk.offset_;
    std::uint32_t arena_num = blk.arena_;

    blk.is_free_ = false;

    auto remaining = blk.size_ - size;
    blk.size_      = size;
    if (remaining > 0)
    {
      auto& list  = bank.arenas_[blk.arena_].block_order();
      auto  arena = blk.arena_;
      auto  newblk =
       bank.blocks_.emplace(blk.offset_ + size, remaining, arena, std::numeric_limits<uint32_t>::max(), true);
      list.insert_after(bank.blocks_, free_node, (uint32_t)newblk);
      // reinsert the left-over size in free list
      reinsert_left(bank.blocks_, free_idx, remaining, (uint32_t)newblk);
    }
    else
    {
      // delete the existing found index from free list
      free_ordering_.erase(free_ordering_.begin() + free_idx);
    }

    return free_node;
  }

  void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block) noexcept
  {
    free_ordering_.push_back(block);
  }

  void add_free(block_bank& blocks, std::uint32_t block) noexcept
  {
    add_free_after_begin(blocks, block);
  }

  void grow_free_node(block_bank& blocks, std::uint32_t block, size_type newsize) noexcept
  {
    auto& blk = blocks[block_link(block)];

    auto it = find_free_it(blocks, free_ordering_.data(), free_ordering_.size(), blk.size_);
    for (auto end = static_cast<uint32_t>(free_ordering_.size()); it != end && free_ordering_[it] != block; ++it)
    {
      ;
    }

    assert(it != static_cast<uint32_t>(free_ordering_.size()));
    blk.size_ = newsize;
    reinsert_right(blocks, it, newsize, block);
  }

  void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size) noexcept
  {
    size_type size = blocks[block_link(block)].size_;

    auto it = find_free_it(blocks, free_ordering_.data(), free_ordering_.size(), size);
    for (auto end = static_cast<uint32_t>(free_ordering_.size()); it != end && free_ordering_[it] != block; ++it)
    {
      ;
    }

    assert(it != static_cast<uint32_t>(free_ordering_.size()));
    blocks[block_link(new_block)].size_ = new_size;
    reinsert_right(blocks, it, new_size, new_block);
  }

  void erase(block_bank& blocks, std::uint32_t block) noexcept
  {
    auto it = find_free_it(blocks, free_ordering_.data(), free_ordering_.size(), blocks[block_link(block)].size_);
    for (auto end = static_cast<uint32_t>(free_ordering_.size()); it != end && free_ordering_[it] != block; ++it)
    {
      ;
    }
    assert(it != static_cast<uint32_t>(free_ordering_.size()));
    free_ordering_.erase(it + free_ordering_.begin());
  }

  auto total_free_nodes(block_bank const& blocks) const noexcept -> std::uint32_t
  {
    return static_cast<std::uint32_t>(free_ordering_.size());
  }

  auto total_free_size(block_bank const& blocks) const noexcept -> size_type
  {
    size_type sz = 0;
    for (auto fn : free_ordering_)
    {
      assert(blocks[block_link(fn)].is_free_);
      sz += blocks[block_link(fn)].size_;
    }

    return sz;
  }

  void validate_integrity(block_bank const& blocks) const
  {
    size_type sz = 0;
    for (auto fn : free_ordering_)
    {
      assert(sz <= blocks[block_link(fn)].size_);
      sz = blocks[block_link(fn)].size_;
    }
  }

  template <typename Owner>
  void init(Owner const& owner)
  {}

protected:
  // Private
  void add_free_after_begin(block_bank& blocks, std::uint32_t block) noexcept
  {
    auto blkid             = block_link(block);
    blocks[blkid].is_free_ = true;
    auto size              = blocks[blkid].size_;
    auto it                = find_free_it(blocks, free_ordering_.data(), free_ordering_.size(), size);
    free_ordering_.emplace(free_ordering_.begin() + it, block);
  }

  static auto mini0(block_bank const& blocks, block_link const* it, size_t size, size_type key) noexcept
  {
    while (size > 2)
      ACL_BINARY_SEARCH_STEP;
    it += size > 1 && (blocks[*it].size_ < key);
    it += size > 0 && (blocks[*it].size_ < key);
    return it;
  }

  static auto mini1(block_bank const& blocks, block_link const* it, size_t size, size_type key) noexcept
  {
    while (true)
    {
      ACL_BINARY_SEARCH_STEP;
      if (size <= 2)
      {
        break;
      }
    }

    it += size > 1 && (blocks[*it].size_ < key);
    it += size > 0 && (blocks[*it].size_ < key);
    return it;
  }

  static auto mini2(block_bank const& blocks, block_link const* it, size_t size, size_type key) noexcept
  {
    while (true)
    {
      ACL_BINARY_SEARCH_STEP;
      ACL_BINARY_SEARCH_STEP;
      if (size <= 2)
      {
        break;
      }
    }
    it += size > 1 && (blocks[*it].size_ < key);
    it += size > 0 && (blocks[*it].size_ < key);
    return it;
  }

  static auto bsearch(block_bank const& blocks, uint32_t const* it, size_t s, size_type key) noexcept
  {
    if constexpr (bsearch_algo == 0)
    {
      return (uint32_t const*)mini0(blocks, (block_link const*)it, s, key);
    }
    else if constexpr (bsearch_algo == 1)
    {
      return (uint32_t const*)mini1(blocks, (block_link const*)it, s, key);
    }
    else if constexpr (bsearch_algo == 2)
    {
      return (uint32_t const*)mini2(blocks, (block_link const*)it, s, key);
    }
  }

  static auto find_free_it(block_bank const& blocks, size_type const* it, size_t s, size_type key) noexcept
  {
    return std::distance(it, bsearch(blocks, it, s, key));
  }

  auto find_free(block_bank const& blocks, size_type size) const noexcept -> optional_addr
  {
    auto it = bsearch(blocks, free_ordering_.data(), free_ordering_.size(), size);
    return (it < (free_ordering_.data() + free_ordering_.size())) ? optional_addr(it) : optional_addr(nullptr);
  }

  void reinsert_left(block_bank const& blocks, size_t of, size_type size, std::uint32_t node) noexcept
  {
    if (of == 0U)
    {
      free_ordering_[of] = node;
    }
    else
    {
      auto it = find_free_it(blocks, free_ordering_.data(), of, size);
      if (it != of)
      {
        std::size_t count = of - it;

        {
          auto src  = free_ordering_.data() + it;
          auto dest = src + 1;
          std::memmove(dest, src, count * sizeof(std::uint32_t));
        }

        free_ordering_[it] = node;
      }
      else
      {
        free_ordering_[of] = node;
      }
    }
  }

  void reinsert_right(block_bank const& blocks, size_t of, size_type size, std::uint32_t node)
  {
    auto next = of + 1;
    if (next == free_ordering_.size())
    {
      free_ordering_[of] = node;
    }
    else
    {
      auto it = find_free_it(blocks, free_ordering_.data() + next, free_ordering_.size() - next, size);
      if (it)
      {
        std::size_t count = it;

        {
          auto* dest = free_ordering_.data() + of;
          auto* src  = dest + 1;
          std::memmove(dest, src, count * sizeof(uint32_t));
          auto* ptr = (dest + count);
          *ptr      = node;
        }
      }
      else
      {
        free_ordering_[of] = node;
      }
    }
  }

private:
  acl::detail::free_list free_ordering_;
};

#undef ACL_BINARY_SEARCH_STEP
/**
 * alloc_strategy::best_fit Impl
 */

} // namespace acl::strat
