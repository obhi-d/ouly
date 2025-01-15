#pragma once

#include <acl/allocators/config.hpp>
#include <acl/allocators/detail/arena.hpp>
#include <acl/containers/detail/rbtree.hpp>
#include <acl/utility/optional_val.hpp>
#include <acl/utility/type_traits.hpp>

namespace acl::strat
{
/**
 * @brief  Strategy class using RBTree for arena_allocator
 * @tparam Config accepted config are only cfg::basic_size_type
 * @remarks Class uses best fit using rbtree implementation to find the best match for requested size
 */
template <typename Config = acl::config<>>
class best_fit_tree
{
  static constexpr uint32_t k_null_0 = 0;
  using optional_addr                = acl::optional_val<k_null_0>;

public:
  best_fit_tree() noexcept                = default;
  best_fit_tree(best_fit_tree const&)     = default;
  best_fit_tree(best_fit_tree&&) noexcept = default;

  auto operator=(best_fit_tree const&) -> best_fit_tree&     = default;
  auto operator=(best_fit_tree&&) noexcept -> best_fit_tree& = default;
  ~best_fit_tree() noexcept                                  = default;

  using extension  = acl::detail::tree_node<1>;
  using size_type  = acl::detail::choose_size_t<uint32_t, Config>;
  using arena_bank = acl::detail::arena_bank<size_type, extension>;
  using block_bank = acl::detail::block_bank<size_type, extension>;
  using block      = acl::detail::block<size_type, extension>;
  using bank_data  = acl::detail::bank_data<size_type, extension>;
  using block_link = typename block_bank::link;

  static constexpr size_type min_granularity = 4;

  struct blk_tree_node_accessor
  {
    using value_type = size_type;
    using node_type  = block;
    using container  = block_bank;
    using tree_node  = acl::detail::tree_node<1>;
    using block_link = typename block_bank::link;

    static void erase(container& icont, std::uint32_t node)
    {
      icont.erase(block_link(node));
    }

    static auto node(container const& icont, std::uint32_t id) -> node_type const&
    {
      return icont[block_link(id)];
    }
    static auto node(container& icont, std::uint32_t id) -> node_type&
    {
      return icont[block_link(id)];
    }
    static auto links(node_type const& inode) -> tree_node const&
    {
      return inode.ext_;
    }
    static auto links(node_type& inode) -> tree_node&
    {
      return inode.ext_;
    }
    static auto value(node_type const& inode) -> size_type const&
    {
      return inode.size_;
    }
    static auto is_set(node_type const& inode) -> bool
    {
      return inode.is_flagged_;
    }
    static void set_flag(node_type& inode)
    {
      inode.is_flagged_ = true;
    }
    static void set_flag(node_type& inode, bool v)
    {
      inode.is_flagged_ = v;
    }
    static void unset_flag(node_type& inode)
    {
      inode.is_flagged_ = false;
    }
  };

  using tree_type       = acl::detail::rbtree<blk_tree_node_accessor, 1>;
  using allocate_result = optional_addr;

  [[nodiscard]] auto try_allocate(bank_data& bank, size_type size)
  {
    auto blk = tree_.lower_bound(bank.blocks_, size);
    return optional_addr((bank.blocks_[block_link(blk)].size_ < size) ? 0 : blk);
  }

  auto commit(bank_data& bank, size_type size, optional_addr found) -> std::uint32_t
  {
    auto& blk = bank.blocks_[block_link(found.value_)];
    // Marker
    size_type     offset    = blk.offset_;
    std::uint32_t arena_num = blk.arena_;

    blk.is_free_ = false;

    auto remaining = blk.size_ - size;
    tree_.erase(bank.blocks_, found.value_);
    blk.size_ = size;
    if (remaining > 0)
    {
      auto& list   = bank.arenas_[blk.arena_].block_order();
      auto  arena  = blk.arena_;
      auto  newblk = bank.blocks_.emplace(blk.offset_ + size, remaining, arena, extension(), true);
      list.insert_after(bank.blocks_, found.value_, (uint32_t)newblk);
      tree_.insert(bank.blocks_, (uint32_t)newblk);
    }

    return found.value_;
  }

  void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    tree_.insert(blocks, block);
  }

  void add_free(block_bank& blocks, std::uint32_t block)
  {
    tree_.insert(blocks, block);
  }

  void grow_free_node(block_bank& blocks, std::uint32_t block, size_type new_size)
  {
    tree_.erase(blocks, block);
    blocks[block_link(block)].size_ = new_size;
    tree_.insert(blocks, block);
  }

  void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
  {
    tree_.erase(blocks, block);
    blocks[block_link(new_block)].size_ = new_size;
    tree_.insert(blocks, new_block);
  }

  void erase(block_bank& blocks, std::uint32_t node)
  {
    tree_.erase(blocks, node);
  }

  auto total_free_nodes(block_bank const& blocks) const -> std::uint32_t
  {
    return tree_.node_count(blocks);
  }

  auto total_free_size(block_bank const& blocks) const -> size_type
  {
    size_type sz = 0;
    tree_.in_order_traversal(blocks,
                             [&sz](block const& n)
                             {
                               sz += n.size_;
                             });
    return sz;
  }

  void validate_integrity(block_bank const& blocks) const
  {
    tree_.validate_integrity(blocks);
  }

  template <typename Owner>
  void init(Owner const& owner)
  {}

private:
  tree_type tree_;
};
} // namespace acl::strat