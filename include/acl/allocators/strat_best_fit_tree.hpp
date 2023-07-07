#pragma once
#include <acl/allocators/arena.hpp>
#include <acl/containers/rbtree.hpp>
#include <acl/utils/type_traits.hpp>

namespace acl::strat
{
/// @brief  Strategy class using RBTree for arena_allocator
/// @tparam Options accepted options are only opt::basic_size_type
/// @remarks Class uses best fit using rbtree implementation to find the best match for requested size
template <typename Options = acl::options<>>
class best_fit_tree
{
  using optional_addr = detail::optional_val<acl::detail::k_null_0>;

public:
  best_fit_tree() noexcept                = default;
  best_fit_tree(best_fit_tree const&)     = default;
  best_fit_tree(best_fit_tree&&) noexcept = default;

  best_fit_tree& operator=(best_fit_tree const&)     = default;
  best_fit_tree& operator=(best_fit_tree&&) noexcept = default;

  using extension  = acl::detail::tree_node<1>;
  using size_type  = detail::choose_size_t<uint32_t, Options>;
  using arena_bank = detail::arena_bank<size_type, extension>;
  using block_bank = detail::block_bank<size_type, extension>;
  using block      = detail::block<size_type, extension>;
  using bank_data  = detail::bank_data<size_type, extension>;
  using block_link = typename block_bank::link;

  static constexpr size_type min_granularity = 4;

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

  using tree_type       = detail::rbtree<blk_tree_node_accessor, 1>;
  using allocate_result = optional_addr;

  inline auto try_allocate(bank_data& bank, size_type size)
  {
    auto blk = tree.lower_bound(bank.blocks, size);
    return optional_addr((bank.blocks[block_link(blk)].size < size) ? 0 : blk);
  }

  inline std::uint32_t commit(bank_data& bank, size_type size, optional_addr found)
  {
    auto& blk = bank.blocks[block_link(found.value)];
    // Marker
    size_type     offset    = blk.offset;
    std::uint32_t arena_num = blk.arena;

    blk.is_free = false;

    auto remaining = blk.size - size;
    tree.erase(bank.blocks, found.value);
    blk.size = size;
    if (remaining > 0)
    {
      auto& list   = bank.arenas[blk.arena].block_order;
      auto  arena  = blk.arena;
      auto  newblk = bank.blocks.emplace(blk.offset + size, remaining, arena, extension(), true);
      list.insert_after(bank.blocks, found.value, (uint32_t)newblk);
      tree.insert(bank.blocks, (uint32_t)newblk);
    }

    return found.value;
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

  inline void replace_and_grow(block_bank& blocks, std::uint32_t block, std::uint32_t new_block, size_type new_size)
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
  {}

private:
  tree_type tree;
};
} // namespace acl::strat