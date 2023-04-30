#pragma once
#include "detail/arena.hpp"
#include "detail/rbtree.hpp"

namespace acl::strat
{
/// @brief  Strategy class using RBTree for arena_allocator
/// @tparam usize_type size type
/// @remarks
///   Class uses best fit using rbtree implementation to find the best match
///   for requested size
template <typename usize_type>
class best_fit_tree
{
  using optional_addr = detail::voptional<acl::detail::k_null_0>;

public:

  best_fit_tree() noexcept = default;
  best_fit_tree(best_fit_tree const&) = default;
  best_fit_tree(best_fit_tree &&) noexcept = default;

  best_fit_tree& operator=(best_fit_tree const&) = default;
  best_fit_tree& operator=(best_fit_tree&&) noexcept = default;

  static constexpr usize_type min_granularity = 4;

  using extension  = acl::detail::tree_node<1>;
  using size_type  = usize_type;
  using arena_bank = detail::arena_bank<size_type, extension>;
  using block_bank = detail::block_bank<size_type, extension>;
  using block      = detail::block<size_type, extension>;
  using bank_data  = detail::bank_data<size_type, extension>;
  using block_link = typename block_bank::link;
  

  struct blk_tree_node_accessor
  {
    using value_type = size_type;
    using node_type  = block;
    using container  = block_bank;
    using tree_node  = detail::tree_node<1>;
    using block_link = typename block_bank::link;

    inline static void erase(container& icont, std::uint32_t node)
    {
      icont.erase(block_link(node));
    }

    inline static node_type const& node(container const& icont, std::uint32_t id)
    {
      return icont[block_link(id)];
    }
    inline static node_type& node(container& icont, std::uint32_t id)
    {
      return icont[block_link(id)];
    }
    inline static tree_node const& links(node_type const& inode)
    {
      return inode.ext;
    }
    inline static tree_node& links(node_type& inode)
    {
      return inode.ext;
    }
    inline static size_type const& value(node_type const& inode)
    {
      return inode.size;
    }
    inline static bool is_set(node_type const& inode)
    {
      return inode.is_flagged;
    }
    inline static void set_flag(node_type& inode)
    {
      inode.is_flagged = true;
    }
    inline static void set_flag(node_type& inode, bool v)
    {
      inode.is_flagged = v;
    }
    inline static void unset_flag(node_type& inode)
    {
      inode.is_flagged = false;
    }
  };

  using tree_type = detail::rbtree<blk_tree_node_accessor, 1>;
  using allocate_result = uint32_t;

  inline auto try_allocate(bank_data& bank, size_type size)
  {
    auto blk = tree.lower_bound(bank.blocks, size);
    return optional_addr((bank.blocks[block_link(blk)].size < size) ? 0 : blk);
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, std::uint32_t found)
  {    
    auto& blk = bank.blocks[block_link(found)];
    // Marker
    size_type     offset    = blk.offset;
    std::uint32_t arena_num = blk.arena;

    blk.is_free = false;

    auto remaining = blk.size - size;
    tree.erase(bank.blocks, found);
    blk.size = size;
    if (remaining > 0)
    {
      auto& list   = bank.arenas[blk.arena].block_order;
      auto  arena  = blk.arena;
      auto  newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, extension(), true);
      list.insert_after(bank.blocks, found, (uint32_t)newblk);
      tree.insert(bank.blocks, (uint32_t)newblk);
    }

    return found;
  }

  inline void add_free_arena([[maybe_unused]] block_bank& blocks, std::uint32_t block)
  {
    tree.insert(blocks, block);
  }

  inline void add_free(block_bank& blocks, std::uint32_t block)
  {
    tree.insert(blocks, block);
  }

  inline void grow_free_node(block_bank& blocks, std::uint32_t block, size_type new_size)
  {
    tree.erase(blocks, block);
    blocks[block_link(block)].size = new_size;
    tree.insert(blocks, block);
  }

  inline void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block,
                               size_type new_size)
  {
    tree.erase(blocks, block);
    blocks[block_link(new_block)].size = new_size;
    tree.insert(blocks, new_block);
  }

  inline void erase(block_bank& blocks, std::uint32_t node)
  {
    tree.erase(blocks, node);
  }

  inline std::uint32_t total_free_nodes(block_bank const& blocks) const
  {
    return tree.node_count(blocks);
  }

  inline size_type total_free_size(block_bank const& blocks) const
  {
    size_type sz = 0;
    tree.in_order_traversal(blocks,
                            [&sz](block const& n)
                            {
                              sz += n.size;
                            });
    return sz;
  }

  void validate_integrity(block_bank const& blocks) const
  {
    tree.validate_integrity(blocks);
  }

  template <typename Owner>
  inline void init(Owner const& owner)
  {
  }

private:
  tree_type tree;
};
} // namespace acl::detail