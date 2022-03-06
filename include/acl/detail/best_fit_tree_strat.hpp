#pragma once
#include "arena.hpp"
#include "rbtree.hpp"

namespace acl::detail
{
template <>
struct block_ext<alloc_strategy::best_fit_tree>
{
  using type = acl::detail::tree_node;
};

// ██████╗-███████╗███████╗████████╗-----███████╗██╗████████╗--████████╗██████╗-███████╗███████╗
// ██╔══██╗██╔════╝██╔════╝╚══██╔══╝-----██╔════╝██║╚══██╔══╝--╚══██╔══╝██╔══██╗██╔════╝██╔════╝
// ██████╔╝█████╗--███████╗---██║--------█████╗--██║---██║--------██║---██████╔╝█████╗--█████╗--
// ██╔══██╗██╔══╝--╚════██║---██║--------██╔══╝--██║---██║--------██║---██╔══██╗██╔══╝--██╔══╝--
// ██████╔╝███████╗███████║---██║███████╗██║-----██║---██║███████╗██║---██║--██║███████╗███████╗
// ╚═════╝-╚══════╝╚══════╝---╚═╝╚══════╝╚═╝-----╚═╝---╚═╝╚══════╝╚═╝---╚═╝--╚═╝╚══════╝╚══════╝
// ---------------------------------------------------------------------------------------------
template <typename traits>
class alloc_strategy_impl<alloc_strategy::best_fit_tree, traits>
{
public:
  using size_type  = typename traits::size_type;
  using arena_bank = detail::arena_bank<traits>;
  using block_bank = detail::block_bank<traits>;
  using block      = detail::block<traits>;
  using alloc_desc = acl::alloc_desc<size_type>;
  using bank_data  = detail::bank_data<traits>;

  struct blk_tree_node_accessor
  {
    using value_type = size_type;
    using node_type  = block;
    using container  = block_bank;

    static inline node_type const& node(container const& icont, std::uint32_t id)
    {
      return icont[id];
    }
    static inline node_type& node(container& icont, std::uint32_t id)
    {
      return icont[id];
    }
    static inline tree_node const& links(node_type const& inode)
    {
      return inode.ext;
    }
    static inline tree_node& links(node_type& inode)
    {
      return inode.ext;
    }
    static inline size_type const& value(node_type const& inode)
    {
      return inode.size;
    }
    static inline bool is_set(node_type const& inode)
    {
      return inode.is_flagged;
    }
    static inline void set_flag(node_type& inode)
    {
      inode.is_flagged = true;
    }
    static inline void set_flag(node_type& inode, bool v)
    {
      inode.is_flagged = v;
    }
    static inline void unset_flag(node_type& inode)
    {
      inode.is_flagged = false;
    }
  };

  using tree_type = rbtree<blk_tree_node_accessor>;

  inline std::uint32_t try_allocate(bank_data& bank, size_type size)
  {
    auto blk = tree.lower_bound(bank.blocks, size);
    return ((blk == 0) || (bank.blocks[blk].size < size)) ? k_null_32 : blk;
  }

  inline std::uint32_t try_allocate(bank_data& bank, size_type size, std::uint32_t from)
  {
    auto blk = tree.lower_bound(bank.blocks, tree.next_more(bank.blocks, from), size);
    return blk == 0 || bank.blocks[blk].size < size ? k_null_32 : blk;
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, std::uint32_t found)
  {
    if (!found || found == k_null_32)
    {
      return k_null_32;
    }

    auto& blk = bank.blocks[found];
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
      auto  newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, detail::k_null_sz<uhandle>, true);
      list.insert_after(bank.blocks, found, newblk);
      tree.insert(bank.blocks, newblk);
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
  inline void replace(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
  {
    blocks[new_block].size = new_size;
    tree.insert_hint(blocks, block, new_block);
    tree.erase(blocks, block);
  }
  inline std::uint32_t node(std::uint32_t it)
  {
    return it;
  }
  inline bool is_valid(std::uint32_t it)
  {
    return it != k_null_32;
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

  void validate_integrity(block_bank const& blocks)
  {
    tree.validate_integrity(blocks);
  }

private:
  tree_type tree;
};
} // namespace acl::detail