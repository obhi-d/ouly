#pragma once

#include "alloc_desc.hpp"
#include <acl/detail/arena.hpp>

#include <bit>
#include <functional>

#include "strat_best_fit_tree.hpp"
#include "strat_best_fit_v0.hpp"
#include "strat_best_fit_v1.hpp"
#include "strat_best_fit_v2.hpp"
#include "strat_greedy_v0.hpp"
#include "strat_greedy_v1.hpp"
#include "strat_slotted_v0.hpp"
#include "strat_slotted_v1.hpp"
#include "strat_slotted_v2.hpp"

namespace acl::opt
{
template <typename T>
struct extension
{
  using extension_t = T;
};
template <typename T>
struct manager
{
  using manager_t = T;
};
template <typename T>
struct strategy
{
  using strategy_t = T;
};

} // namespace acl::opt

namespace acl::detail
{

struct defrag_stats
{
  std::uint32_t total_mem_move_merge = 0;
  std::uint32_t total_arenas_removed = 0;

  void report_defrag_mem_move_merge()
  {
    total_mem_move_merge++;
  }

  void report_defrag_arenas_removed()
  {
    total_arenas_removed++;
  }

  std::string print() const
  {
    std::stringstream ss;
    ss << "Defrag memory move merges: " << total_mem_move_merge << "\n"
       << "Defrag arenas removed: " << total_arenas_removed;
    return ss.str();
  }
};

} // namespace acl::detail

namespace acl
{

template <typename Options>
class arena_allocator : public detail::statistics<typename Options::strategy_t,
                                                  acl::options<Options, opt::base_stats<detail::defrag_stats>>>
{
protected:
  using strategy  = typename Options::strategy_t;
  using extension = typename strategy::extension;
  using manager   = typename Options::manager_t;
  using size_type = detail::choose_size_t<uint32_t, Options>;
  using options   = Options;

  using block          = detail::block<size_type, extension>;
  using block_bank     = detail::block_bank<size_type, extension>;
  using block_accessor = detail::block_accessor<size_type, extension>;
  using block_list     = detail::block_list<size_type, extension>;
  using arena_bank     = detail::arena_bank<size_type, extension>;
  using arena_list     = detail::arena_list<size_type, extension>;
  using statistics =
    detail::statistics<typename Options::strategy_t, acl::options<Options, opt::base_stats<detail::defrag_stats>>>;
  using block_link = typename block_bank::link;

  using memory_move   = detail::memory_move<size_type>;
  using arena_manager = manager;
  using bank_data     = detail::bank_data<size_type, extension>;

  struct remap_data
  {
    bank_data bank;
    strategy  strat;
  };

public:
  using alloc_offset = acl::alloc_offset<size_type>;
  using alloc_info   = acl::alloc_info<size_type>;
  using option_flags = std::uint32_t;

  template <typename... Args>
  inline arena_allocator(size_type i_arena_size, arena_manager& i_manager, Args&&... iargs)
      : statistics(std::forward<Args>(iargs)...), mgr(i_manager), arena_size(i_arena_size)
  {
    ibank.strat.init(*this);
  }

  arena_allocator(arena_allocator const&)     = default;
  arena_allocator(arena_allocator&&) noexcept = default;

  arena_allocator& operator=(arena_allocator const&)     = default;
  arena_allocator& operator=(arena_allocator&&) noexcept = default;

  inline auto get_root_block() const
  {
    return ibank.bank.root_blk;
  }

  //! get allocation info
  inline alloc_offset get_alloc_offset(ihandle i_address) const
  {
    auto const& blk = ibank.bank.blocks[block_link(i_address)];
    return alloc_offset(blk.arena, blk.offset);
  }

  //! Allocate
  template <typename alloc_desc_t>
  alloc_info allocate(alloc_desc_t const& desc)
  {
    auto measure = this->statistics::report_allocate(desc.size());
    auto size    = desc.adjusted_size();

    ACL_ASSERT(desc.huser() != detail::k_null_uh);
    if (desc.flags() & f_dedicated_arena || size >= arena_size)
    {
      auto ret = add_arena(desc.huser(), size, false);
      return alloc_info(ibank.bank.arenas[ret.first].data, 0, ret.second);
    }

    std::uint32_t id = null();
    if (auto ta = ibank.strat.try_allocate(ibank.bank, size))
    {
      id = ibank.strat.commit(ibank.bank, size, ta);
    }
    else
    {
      if (desc.flags() & f_defrag)
      {
        defragment();

        if (ta = ibank.strat.try_allocate(ibank.bank, size))
          id = ibank.strat.commit(ibank.bank, size, ta);
      }

      if (id == null())
      {
        add_arena(detail::k_null_sz<uhandle>, arena_size, true);
        if (ta = ibank.strat.try_allocate(ibank.bank, size))
          id = ibank.strat.commit(ibank.bank, size, ta);
      }
    }

    if (id == null())
      return alloc_info();
    auto& blk = ibank.bank.blocks[block_link(id)];
    return alloc_info(ibank.bank.arenas[blk.arena].data, finalize_commit(blk, desc.huser(), desc), id);
  }

  //! Deallocate, size is optional
  void deallocate(ihandle node)
  {
    auto& blk     = ibank.bank.blocks[block_link(node)];
    auto  measure = this->statistics::report_deallocate(blk.size);

    enum
    {
      f_left  = 1 << 0,
      f_right = 1 << 1,
    };

    enum merge_type
    {
      e_none,
      e_left,
      e_right,
      e_left_and_right
    };

    auto& arena     = ibank.bank.arenas[blk.arena];
    auto& node_list = arena.block_order;

    // last index is not used
    ibank.bank.free_size += blk.size;
    arena.free += blk.size;
    auto size = blk.size;

    std::uint32_t left = 0, right = 0;
    std::uint32_t merges = 0;

    if (node != node_list.front() && ibank.bank.blocks[block_link(blk.arena_order.prev)].is_free)
    {
      left = blk.arena_order.prev;
      merges |= f_left;
    }

    if (node != node_list.back() && ibank.bank.blocks[block_link(blk.arena_order.next)].is_free)
    {
      right = blk.arena_order.next;
      merges |= f_right;
    }

    if (arena.free == arena.size && mgr.get().drop_arena(arena.data))
    {
      // drop arena?
      if (left)
      {
        ibank.strat.erase(ibank.bank.blocks, left);
      }
      if (right)
      {
        ibank.strat.erase(ibank.bank.blocks, right);
      }

      std::uint32_t arena_id = blk.arena;
      ibank.bank.free_size -= arena.size;
      arena.size = 0;
      arena.block_order.clear(ibank.bank.blocks);
      ibank.bank.arena_order.erase(ibank.bank.arenas, arena_id);
      return;
    }

    switch (merges)
    {
    case merge_type::e_none:
      ibank.strat.add_free(ibank.bank.blocks, node);
      blk.is_free = true;
      break;
    case merge_type::e_left:
    {
      auto left_size = ibank.bank.blocks[block_link(left)].size;
      ibank.strat.grow_free_node(ibank.bank.blocks, left, left_size + size);
      node_list.erase(ibank.bank.blocks, node);
    }
    break;
    case merge_type::e_right:
    {
      auto right_size = ibank.bank.blocks[block_link(right)].size;
      ibank.strat.replace_and_grow(ibank.bank.blocks, right, node, right_size + size);
      node_list.erase(ibank.bank.blocks, right);
      blk.is_free = true;
    }
    break;
    case merge_type::e_left_and_right:
    {
      auto left_size  = ibank.bank.blocks[block_link(left)].size;
      auto right_size = ibank.bank.blocks[block_link(right)].size;
      ibank.strat.erase(ibank.bank.blocks, right);
      ibank.strat.grow_free_node(ibank.bank.blocks, left, left_size + right_size + size);
      node_list.erase2(ibank.bank.blocks, node);
    }
    break;
    }
  }

  // set default arena size
  inline void set_arena_size(size_type isz)
  {
    arena_size = isz;
  }

  // null
  inline static constexpr ihandle null()
  {
    return 0;
  }

  // validate
  void validate_integrity() const
  {
    std::uint32_t total_free_nodes = 0;
    for (auto arena_it     = ibank.bank.arena_order.begin(ibank.bank.arenas),
              arena_end_it = ibank.bank.arena_order.end(ibank.bank.arenas);
         arena_it != arena_end_it; ++arena_it)
    {
      auto& arena           = *arena_it;
      bool  arena_allocated = false;

      for (auto blk_it     = arena.block_order.begin(ibank.bank.blocks),
                blk_end_it = arena.block_order.end(ibank.bank.blocks);
           blk_it != blk_end_it; ++blk_it)
      {
        auto& blk = *blk_it;
        if ((blk.is_free))
          total_free_nodes++;
      }
    }

    ACL_ASSERT(total_free_nodes == ibank.strat.total_free_nodes(ibank.bank.blocks));
    auto total = ibank.strat.total_free_size(ibank.bank.blocks);
    ACL_ASSERT(total == ibank.bank.free_size);

    for (auto arena_it     = ibank.bank.arena_order.begin(ibank.bank.arenas),
              arena_end_it = ibank.bank.arena_order.end(ibank.bank.arenas);
         arena_it != arena_end_it; ++arena_it)
    {
      auto&     arena           = *arena_it;
      bool      arena_allocated = false;
      size_type expected_offset = 0;

      for (auto blk_it     = arena.block_order.begin(ibank.bank.blocks),
                blk_end_it = arena.block_order.end(ibank.bank.blocks);
           blk_it != blk_end_it; ++blk_it)
      {
        auto& blk = *blk_it;
        ACL_ASSERT(blk.offset == expected_offset);
        expected_offset += blk.size;
      }
    }

    ibank.strat.validate_integrity(ibank.bank.blocks);
  }

private:
  inline std::pair<ihandle, ihandle> add_arena(uhandle ihandle, size_type iarena_size, bool empty)
  {
    this->statistics::report_new_arena();
    auto ret                          = add_arena(ibank, ihandle, iarena_size, empty);
    ibank.bank.arenas[ret.first].data = mgr.get().add_arena(ret.first, iarena_size);
    return ret;
  }

  inline static std::pair<ihandle, ihandle> add_arena(remap_data& ibank, uhandle ihandle, size_type iarena_size,
                                                      bool iempty)
  {

    std::uint32_t arena_id  = ibank.bank.arenas.emplace();
    auto&         arena_ref = ibank.bank.arenas[arena_id];
    arena_ref.size          = iarena_size;
    auto   block_id         = ibank.bank.blocks.emplace();
    block& block_ref        = ibank.bank.blocks[block_id];
    block_ref.offset        = 0;
    block_ref.arena         = arena_id;
    block_ref.data          = ihandle;
    block_ref.size          = iarena_size;
    if (iempty)
    {
      block_ref.is_free = true;
      arena_ref.free    = iarena_size;
      ibank.strat.add_free_arena(ibank.bank.blocks, (uint32_t)block_id);
      ibank.bank.free_size += iarena_size;
    }
    else
    {
      arena_ref.free = 0;
    }
    arena_ref.block_order.push_back(ibank.bank.blocks, (uint32_t)block_id);
    ibank.bank.arena_order.push_back(ibank.bank.arenas, arena_id);
    return std::make_pair(arena_id, (uint32_t)block_id);
  }

  void defragment()
  {
    mgr.get().begin_defragment(*this);
    std::uint32_t arena_id = ibank.bank.arena_order.first;
    // refresh all banks
    remap_data refresh;
    refresh.strat.init(*this);

    acl::vector<std::uint32_t> rebinds;
    rebinds.reserve(ibank.bank.blocks.size());

    acl::vector<memory_move>         moves;
    decltype(ibank.bank.arena_order) deleted_arenas;
    for (auto arena_it = ibank.bank.arena_order.front(); arena_it != 0;)
    {
      auto& arena           = ibank.bank.arenas[arena_it];
      bool  arena_allocated = false;

      for (auto blk_it = arena.block_order.begin(ibank.bank.blocks); blk_it; blk_it = arena.block_order.erase(blk_it))
      {
        auto& blk = *blk_it;
        if (!blk.is_free)
        {
          auto ta = refresh.strat.try_allocate(refresh.bank, blk.size);
          if (!ta && !arena_allocated)
          {
            auto p = add_arena(refresh, detail::k_null_uh, std::max(arena.size, blk.size), true);
            refresh.bank.arenas[p.first].data = arena.data;
            ta                                = refresh.strat.try_allocate(refresh.bank, blk.size);
            arena_allocated                   = true;
          }
          ACL_ASSERT(ta);

          auto  new_blk_id = refresh.strat.commit(refresh.bank, blk.size, ta);
          auto& new_blk    = refresh.bank.blocks[block_link(new_blk_id)];
          refresh.bank.arenas[new_blk.arena].free -= blk.size;
          refresh.bank.free_size -= blk.size;

          copy(blk, new_blk);
          rebinds.emplace_back(new_blk_id);
          auto blk_adj = blk.adjusted_block();
          push_memmove(moves,
                       memory_move(blk_adj.first, new_blk.adjusted_offset(), blk_adj.second, blk.arena, new_blk.arena));
        }
      }

      if (!arena_allocated)
      {
        auto to_delete = arena_it;
        arena_it       = ibank.bank.arena_order.unlink(ibank.bank.arenas, to_delete);
        arena.free     = arena.size;
        deleted_arenas.push_back(ibank.bank.arenas, to_delete);
      }
      else
        arena_it = ibank.bank.arena_order.next(ibank.bank.arenas, arena_it);
    }

    for (auto& m : moves)
      // follow the copy sequence to ensure there is no overwrite
      mgr.get().move_memory(ibank.bank.arenas[m.arena_src].data, refresh.bank.arenas[m.arena_dst].data, m.from, m.to,
                            m.size);

    for (auto rb : rebinds)
    {
      auto& dst_blk = refresh.bank.blocks[block_link(rb)];
      mgr.get().rebind_alloc(dst_blk.data,
                             alloc_info(refresh.bank.arenas[dst_blk.arena].data, dst_blk.adjusted_offset(), rb));
    }

    for (auto arena_it = deleted_arenas.begin(ibank.bank.arenas); arena_it; arena_it = deleted_arenas.erase(arena_it))
    {
      auto& arena = *arena_it;
      mgr.get().remove_arena(arena.data);
      if constexpr (detail::HasComputeStats<Options>)
        statistics::report_defrag_arenas_removed();
    }

    ibank.bank  = std::move(refresh.bank);
    ibank.strat = std::move(refresh.strat);
    mgr.get().end_defragment(*this);
  }

  template <typename alloc_desc_t>
  inline size_type finalize_commit(block& blk, uhandle huser, alloc_desc_t const& desc)
  {
    blk.data      = huser;
    blk.alignment = desc.alignment_bits();
    ibank.bank.arenas[blk.arena].free -= blk.size;
    ibank.bank.free_size -= blk.size;
    size_type alignment = desc.alignment_mask();
    return ((blk.offset + alignment) & ~alignment);
  }

  inline static void copy(block const& src, block& dst)
  {
    dst.data      = src.data;
    dst.alignment = src.alignment;
  }

  inline void push_memmove(acl::vector<memory_move>& dst, memory_move value)
  {
    if (!value.is_moved())
      return;
    auto can_merge = [](memory_move const& m1, memory_move const& m2) -> bool
    {
      return ((m1.arena_dst == m2.arena_dst && m1.arena_src == m2.arena_src) &&
              (m1.from + m1.size == m2.from && m1.to + m1.size == m2.to));
    };
    if (dst.empty() || !can_merge(dst.back(), value))
      dst.emplace_back(value);
    else
    {
      dst.back().size += value.size;
      if constexpr (detail::HasComputeStats<Options>)
        statistics::report_defrag_mem_move_merge();
    }
  }

  remap_data                            ibank;
  std::reference_wrapper<arena_manager> mgr;
  size_type                             arena_size;
};

} // namespace acl
