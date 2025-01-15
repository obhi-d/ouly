#pragma once

#include <acl/allocators/config.hpp>
#include <acl/allocators/detail/arena.hpp>
#include <acl/utility/optional_val.hpp>
#include <acl/utility/type_traits.hpp>

namespace acl::strat
{

/**
 * @brief This class provides a mechanism to allocate blocks of addresses
 *        by linearly searching through a list of available free sizes and
 *        returning the first chunk that can fit the requested memory size.
 */
template <typename Config = acl::config<>>
class greedy_v0
{
  using optional_addr = acl::optional_val<std::numeric_limits<uint32_t>::max()>;

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

  greedy_v0() noexcept            = default;
  greedy_v0(greedy_v0 const&)     = default;
  greedy_v0(greedy_v0&&) noexcept = default;
  ~greedy_v0() noexcept           = default;

  auto operator=(greedy_v0 const&) -> greedy_v0&     = default;
  auto operator=(greedy_v0&&) noexcept -> greedy_v0& = default;

  [[nodiscard]] auto try_allocate(bank_data& bank, size_type size) -> optional_addr
  {
    for (uint32_t i = 0, en = static_cast<uint32_t>(free_list_.size()); i < en; ++i)
    {
      auto& f = free_list_[i];
      if (f.first >= size)
      {
        return {i};
      }
    }
    return {};
  }

  auto commit(bank_data& bank, size_type size, optional_addr found) -> std::uint32_t
  {
    assert(found.value_ < static_cast<uint32_t>(free_list_.size()));

    auto& free_node = free_list_[found.value_];
    auto  block     = free_node.second;
    auto& blk       = bank.blocks_[block];
    // Marker
    size_type     offset    = blk.offset_;
    std::uint32_t arena_num = blk.arena_;

    blk.is_free_ = false;

    auto remaining = blk.size() - size;
    blk.size_      = size;
    if (remaining > 0)
    {
      auto& list  = bank.arenas_[blk.arena_].block_order();
      auto  arena = blk.arena_;

      auto newblk = bank.blocks_.emplace(blk.offset_ + size, remaining, arena, found.value_, true);
      list.insert_after(bank.blocks_, (uint32_t)free_node.second, (uint32_t)newblk);
      // reinsert the left-over size in free list
      free_node.first  = remaining;
      free_node.second = newblk;
    }
    else
    {
      free_node.first  = 0;
      free_node.second = block_link(free_slot_);
      free_slot_       = found.value_;
    }

    return (uint32_t)block;
  }

  void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    add_free(blocks, block);
  }

  void add_free(block_bank& blocks, std::uint32_t block)
  {
    auto  hblock            = block_link(block);
    auto  slot              = ensure_free_slot();
    auto& blk               = blocks[hblock];
    blk.reserved32_         = slot;
    free_list_[slot].first  = blk.size();
    free_list_[slot].second = hblock;
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
    auto  hblock     = block_link(node);
    auto  idx        = blocks[hblock].reserved32_;
    auto& free_node  = free_list_[idx];
    free_node.first  = 0;
    free_node.second = block_link(free_slot_);
    free_slot_       = idx;
  }

  auto total_free_nodes(block_bank const& blocks) const -> std::uint32_t
  {
    uint32_t count = 0;
    for (auto fn : free_list_)
    {
      if (fn.first)
      {
        count++;
      }
    }
    return count;
  }

  auto total_free_size(block_bank const& blocks) const -> size_type
  {
    size_type sz = 0;
    for (auto fn : free_list_)
    {
      sz += fn.first;
    }
    return sz;
  }

  void validate_integrity(block_bank const& blocks) const
  {
    size_type sz = 0;
    for (uint32_t i = 0, en = (uint32_t)free_list_.size(); i < en; ++i)
    {
      auto fn = free_list_[i];
      if (fn.first)
      {
        auto& blk = blocks[fn.second];
        assert(blk.size() == fn.first);
        assert(blk.reserved32_ == i);
      }
    }
  }

  template <typename Owner>
  void init(Owner const& owner)
  {}

protected:
  // Private

  auto ensure_free_slot() -> uint32_t
  {
    uint32_t r = free_slot_;
    if (free_slot_ != 0U)
    {
      free_slot_ = free_list_[free_slot_].first;
    }
    else
    {
      r = (uint32_t)free_list_.size();
      free_list_.emplace_back();
    }
    return r;
  }

private:
  std::vector<std::pair<size_type, block_link>> free_list_;
  uint32_t                                      free_slot_ = 0;
};

/**
 * alloc_strategy::best_fit Impl
 */

} // namespace acl::strat