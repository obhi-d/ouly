#pragma once

#include <acl/allocators/config.hpp>
#include <acl/allocators/detail/arena.hpp>
#include <acl/utility/optional_val.hpp>
namespace acl::strat
{

/**
 * @brief This class provides a mechanism to allocate blocks of addresses
 *        by linearly searching through a list of available free sizes and
 *        returning the first chunk that can fit the requested memory size.
 */
template <typename Config = acl::config<>>
class greedy_v1
{
  static constexpr uint32_t k_null_0 = 0;
  using optional_addr                = acl::optional_val<k_null_0>;

public:
  using extension       = uint64_t;
  using size_type       = acl::detail::choose_size_t<uint32_t, Config>;
  using arena_bank      = acl::detail::arena_bank<size_type, extension>;
  using block_bank      = acl::detail::block_bank<size_type, extension>;
  using block           = acl::detail::block<size_type, extension>;
  using bank_data       = acl::detail::bank_data<size_type, extension>;
  using block_link      = typename block_bank::link;
  using allocate_result = optional_addr;

  static constexpr size_type min_granularity = 4;

  greedy_v1() noexcept            = default;
  greedy_v1(greedy_v1 const&)     = default;
  greedy_v1(greedy_v1&&) noexcept = default;
  ~greedy_v1() noexcept           = default;

  auto operator=(greedy_v1 const&) -> greedy_v1&     = default;
  auto operator=(greedy_v1&&) noexcept -> greedy_v1& = default;

  [[nodiscard]] auto try_allocate(bank_data& bank, size_type size) -> optional_addr
  {
    uint32_t i = head_;
    while (i != 0U)
    {
      auto const& blk = bank.blocks_[block_link(i)];
      if (blk.size_ >= size)
      {
        return {i};
      }
      i = blk.list_.next_;
    }
    return {};
  }

  auto commit(bank_data& bank, size_type size, optional_addr found) -> std::uint32_t
  {
    auto& blk = bank.blocks_[block_link(found.value_)];
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

      auto newblk = bank.blocks_.emplace(blk.offset_ + size, remaining, arena, blk.list_, true);
      list.insert_after(bank.blocks_, found.value_, (uint32_t)newblk);

      if (blk.list_.next_)
      {
        bank.blocks_[block_link(blk.list_.next_)].list_.prev_ = (uint32_t)newblk;
      }
      if (blk.list_.prev_)
      {
        bank.blocks_[block_link(blk.list_.prev_)].list_.next_ = (uint32_t)newblk;
      }
      else
      {
        head_ = (uint32_t)newblk;
      }
      blk.list_ = {};
    }
    else
    {
      erase(bank.blocks_, found.value_);
    }
    return found.value_;
  }

  void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    add_free(blocks, block);
  }

  void add_free(block_bank& blocks, std::uint32_t block)
  {
    auto  hblock = block_link(block);
    auto& blk    = blocks[hblock];
    assert(blk.list_.prev_ == 0);
    blk.list_.next_ = head_;
    if (head_ != 0U)
    {
      blocks[block_link(head_)].list_.prev_ = block;
    }
    head_ = block;
  }

  void grow_free_node(block_bank& blocks, std::uint32_t block, size_type newsize)
  {
    erase(blocks, block);
    blocks[block_link(block)].size_ = newsize;
    add_free(blocks, block);
  }

  void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
  {
    erase(blocks, block);
    blocks[block_link(new_block)].size_ = new_size;
    add_free(blocks, new_block);
  }

  void erase(block_bank& blocks, std::uint32_t node)
  {
    auto& blk = blocks[block_link(node)];
    if (blk.list_.next_)
    {
      blocks[block_link(blk.list_.next_)].list_.prev_ = blk.list_.prev_;
    }
    if (blk.list_.prev_)
    {
      blocks[block_link(blk.list_.prev_)].list_.next_ = blk.list_.next_;
    }
    else
    {
      head_ = blk.list_.next_;
    }
    blk.list_ = {};
  }

  auto total_free_nodes(block_bank const& blocks) const -> std::uint32_t
  {
    uint32_t count = 0;
    uint32_t i     = head_;
    while (i != 0U)
    {
      auto const& blk = blocks[block_link(i)];
      assert(blk.size_);
      count++;
      i = blk.list_.next_;
    }
    return count;
  }

  auto total_free_size(block_bank const& blocks) const -> size_type
  {
    size_type sz = 0;
    uint32_t  i  = head_;
    while (i != 0U)
    {
      auto const& blk = blocks[block_link(i)];
      sz += blk.size_;
      i = blk.list_.next_;
    }
    return sz;
  }

  void validate_integrity(block_bank const& blocks) const
  {
    uint32_t i = head_;
    uint32_t p = 0;
    while (i != 0U)
    {
      auto const& blk = blocks[block_link(i)];
      assert(blk.is_free_);
      assert(blk.list_.prev_ == p);
      p = i;
      i = blk.list_.next_;
    }
  }

  template <typename Owner>
  void init(Owner const& owner)
  {}

private:
  // Private

  uint32_t head_ = 0;
};

/**
 * alloc_strategy::best_fit Impl
 */

} // namespace acl::strat