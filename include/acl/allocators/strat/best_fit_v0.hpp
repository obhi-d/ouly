#pragma once

#include <acl/allocators/config.hpp>
#include <acl/allocators/detail/arena.hpp>
#include <acl/utility/type_traits.hpp>
#include <optional>

namespace acl::strat
{

/**
 * @brief  Strategy class for arena_allocator that stores a
 *         sorted list of free available slots.
 *         Binary search is used to find the best slot that fits
 *         the requested memory
 * @todo   optimize, branchless binary
 */
template <typename Config = acl::config<>>
class best_fit_v0
{
  using optional_addr = std::optional<acl::detail::free_list::iterator>;

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

  best_fit_v0() noexcept              = default;
  best_fit_v0(best_fit_v0 const&)     = default;
  best_fit_v0(best_fit_v0&&) noexcept = default;
  ~best_fit_v0() noexcept             = default;

  auto operator=(best_fit_v0 const&) -> best_fit_v0&     = default;
  auto operator=(best_fit_v0&&) noexcept -> best_fit_v0& = default;

  [[nodiscard]] auto try_allocate(bank_data& bank, size_type size) -> optional_addr
  {
    if (free_ordering_.empty() || bank.blocks_[block_link(free_ordering_.back())].size_ < size)
    {
      return {};
    }
    return find_free(bank.blocks_, free_ordering_.begin(), free_ordering_.end(), size);
  }

  auto commit(bank_data& bank, size_type size, optional_addr const& found_) -> std::uint32_t
  {
    auto          found     = *found_;
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
      reinsert_left(bank.blocks_, found, (uint32_t)newblk);
    }
    else
    {
      // delete the existing found index from free list
      free_ordering_.erase(found);
    }

    return free_node;
  }

  void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    free_ordering_.push_back(block);
  }

  void add_free(block_bank& blocks, std::uint32_t block)
  {
    add_free_after_begin(blocks, block);
  }

  void grow_free_node(block_bank& blocks, std::uint32_t block, size_type newsize)
  {
    auto& blk = blocks[block_link(block)];

    auto end = free_ordering_.end();
    auto it  = find_free_it(blocks, free_ordering_.begin(), end, blk.size_);
    if constexpr (acl::debug)
    {
      while (it != end && *it != block)
      {
        it++;
      }
      assert(it < end);
    }
    else
    {
      while (*it != block)
      {
        it++;
      }
    }
    blk.size_ = newsize;
    reinsert_right(blocks, it, block);
  }

  void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
  {
    size_type size = blocks[block_link(block)].size_;

    auto end = free_ordering_.end();
    auto it  = find_free_it(blocks, free_ordering_.begin(), end, size);
    if constexpr (acl::debug)
    {
      while (it != end && *it != block)
      {
        it++;
      }
      assert(it < end);
    }
    else
    {
      while (*it != block)
      {
        it++;
      }
    }

    blocks[block_link(new_block)].size_ = new_size;
    reinsert_right(blocks, it, new_block);
  }

  void erase(block_bank& blocks, std::uint32_t block)
  {
    auto end = free_ordering_.end();
    auto it  = find_free_it(blocks, free_ordering_.begin(), end, blocks[block_link(block)].size_);
    if constexpr (acl::debug)
    {
      while (it != end && *it != block)
      {
        it++;
      }
      assert(it < end);
    }
    else
    {
      while (*it != block)
      {
        it++;
      }
    }
    free_ordering_.erase(it);
  }

  auto total_free_nodes(block_bank const& blocks) const -> std::uint32_t
  {
    return static_cast<std::uint32_t>(free_ordering_.size());
  }

  auto total_free_size(block_bank const& blocks) const -> size_type
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
  void add_free_after_begin(block_bank& blocks, std::uint32_t block)
  {
    auto blkid             = block_link(block);
    blocks[blkid].is_free_ = true;
    auto it                = find_free_it(blocks, free_ordering_.begin(), free_ordering_.end(), blocks[blkid].size_);
    free_ordering_.emplace(it, block);
  }

  template <typename It>
  static auto find_free_it(block_bank& blocks, It b, It e, size_type i_size)
  {
    return std::lower_bound(b, e, i_size,
                            [&blocks](std::uint32_t block, size_type i_size) -> bool
                            {
                              return blocks[block_link(block)].size_ < i_size;
                            });
  }

  auto find_free(block_bank& blocks, auto b, auto e, size_type i_size) const -> optional_addr
  {
    auto it = find_free_it(blocks, b, e, i_size);
    return (it != e) ? optional_addr(it) : optional_addr();
  }

  void reinsert_left(block_bank& blocks, auto of, std::uint32_t node)
  {
    auto begin_it = free_ordering_.begin();
    if (begin_it == of)
    {
      *of = node;
    }
    else
    {
      auto it = find_free_it(blocks, begin_it, of, blocks[block_link(node)].size_);
      if (it != of)
      {
        std::uint32_t* src   = &*it;
        std::uint32_t* dest  = src + 1;
        std::size_t    count = std::distance(it, of);
        std::memmove(dest, src, count * sizeof(std::uint32_t));
        *it = node;
      }
      else
      {
        *of = node;
      }
    }
  }

  void reinsert_right(block_bank& blocks, auto of, std::uint32_t node)
  {

    auto end_it = free_ordering_.end();
    auto next   = std::next(of);
    if (next == end_it)
    {
      *of = node;
    }
    else
    {
      auto it = find_free_it(blocks, next, end_it, blocks[block_link(node)].size_);
      if (it != next)
      {
        std::uint32_t* dest  = &(*of);
        std::uint32_t* src   = dest + 1;
        std::size_t    count = std::distance(next, it);
        std::memmove(dest, src, count * sizeof(std::uint32_t));
        auto* ptr = (dest + count);
        *ptr      = node;
      }
      else
      {
        *of = node;
      }
    }
  }

private:
  acl::detail::free_list free_ordering_;
};

/**
 * alloc_strategy::best_fit Impl
 */

} // namespace acl::strat
