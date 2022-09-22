#pragma once
#include "arena.hpp"
#include "best_fit_strat.hpp"
#include "best_fit_tree_strat.hpp"
#include <bit>

namespace acl::detail
{
template <alloc_strategy>
struct arena_allocator_tag
{};

template <typename manager_t, typename usize_t = std::size_t, alloc_strategy strategy_v = alloc_strategy::best_fit_tree,
          bool k_compute_stats_v = false>
struct arena_allocator_traits
{
  static inline constexpr bool           k_compute_stats = k_compute_stats_v;
  static inline constexpr alloc_strategy strategy        = strategy_v;
  using manager                                          = manager_t;
  using size_type                                        = usize_t;
  using extension                                        = typename block_ext<strategy_v>::type;
};

template <bool allow = true>
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

template <>
struct defrag_stats<false>
{

  static void report_defrag_mem_move_merge() {}

  static void report_defrag_arenas_removed() {}

  static std::string print()
  {
    return std::string();
  }
};

template <typename traits>
class arena_allocator_impl : public detail::statistics<detail::arena_allocator_tag<traits::strategy>,
                                                       traits::k_compute_stats, defrag_stats<traits::k_compute_stats>>
{
  using size_type      = typename traits::size_type;
  using block          = detail::block<traits>;
  using block_bank     = detail::block_bank<traits>;
  using block_accessor = detail::block_accessor<traits>;
  using block_list     = detail::block_list<traits>;
  using arena_bank     = detail::arena_bank<traits>;
  using arena_list     = detail::arena_list<traits>;
  using statistics     = detail::statistics<detail::arena_allocator_tag<traits::strategy>, traits::k_compute_stats,
                                        defrag_stats<traits::k_compute_stats>>;
  using strategy       = detail::alloc_strategy_type<traits>;
  using memory_move    = detail::memory_move<traits>;
  using alloc_desc     = acl::alloc_desc<size_type>;
  using arena_manager  = typename traits::manager;
  using bank_data      = detail::bank_data<traits>;

public:
  using tag          = detail::arena_allocator_tag<traits::strategy>;
  using alloc_info   = acl::alloc_info<size_type>;
  using option_flags = std::uint32_t;

  template <typename... Args>
  inline arena_allocator_impl(size_type i_arena_size, arena_manager& i_manager, Args&&... args);
  //! Allocate
  alloc_info allocate(alloc_desc const& desc);
  //! Deallocate, size is optional
  void deallocate(ihandle i_address);

  // set default arena size
  inline void set_arena_size(size_type isz)
  {
    arena_size = isz;
  }

  // null
  inline static constexpr ihandle null()
  {
    return detail::k_null_32;
  }

  // validate
  void validate_integrity();

private:
  inline std::pair<ihandle, ihandle>        add_arena(uhandle ihandle, size_type iarena_size, bool empty);
  inline static std::pair<ihandle, ihandle> add_arena(bank_data& ibank, uhandle ihandle, size_type iarena_size,
                                                      bool empty);

  void               defragment();
  inline size_type   finalize_commit(block& blk, uhandle huser, size_type alignment);
  inline static void copy(block const& src, block& dst);
  inline void        push_memmove(acl::vector<memory_move>& dst, memory_move value);

  bank_data      bank;
  arena_manager& manager;
  size_type      arena_size;
};

template <typename traits>
template <typename... Args>
inline arena_allocator_impl<traits>::arena_allocator_impl(size_type i_arena_size, arena_manager& i_manager,
                                                          Args&&... i_args)
    : statistics(std::forward<Args>(i_args)...), manager(i_manager), arena_size(i_arena_size)
{}

template <typename traits>
inline typename arena_allocator_impl<traits>::alloc_info arena_allocator_impl<traits>::allocate(alloc_desc const& desc)
{
  auto measure = this->statistics::report_allocate(desc.size());
  auto size    = desc.adjusted_size();

  assert(desc.huser() != detail::k_null_uh);
  if (desc.flags() & f_dedicated_arena || size >= arena_size)
  {
    auto ret = add_arena(desc.huser(), size, false);
    return alloc_info(bank.arenas[ret.first].data, 0, ret.second);
  }

  std::uint32_t id = bank.strat.commit(bank, size, bank.strat.try_allocate(bank, size));
  if (id == null())
  {
    if (desc.flags() & f_defrag)
    {
      defragment();
      id = bank.strat.commit(bank, size, bank.strat.try_allocate(bank, size));
    }

    if (id == null())
    {
      add_arena(detail::k_null_sz<uhandle>, arena_size, true);
      id = bank.strat.commit(bank, size, bank.strat.try_allocate(bank, size));
    }
  }

  if (id == null())
    return alloc_info();
  auto& blk = bank.blocks[id];
  return alloc_info(bank.arenas[blk.arena].data, finalize_commit(blk, desc.huser(), desc.alignment_mask()), id);
}

template <typename traits>
inline void arena_allocator_impl<traits>::deallocate(ihandle node)
{
  auto& blk     = bank.blocks[node];
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

  auto& arena     = bank.arenas[blk.arena];
  auto& node_list = arena.block_order;

  // last index is not used
  bank.free_size += blk.size;
  arena.free += blk.size;
  auto size = blk.size;

  std::uint32_t left = detail::k_null_32, right = detail::k_null_32;
  std::uint32_t merges = 0;

  if (node != node_list.front() && bank.blocks[blk.arena_order.prev].is_free)
  {
    left = blk.arena_order.prev;
    merges |= f_left;
  }

  if (node != node_list.back() && bank.blocks[blk.arena_order.next].is_free)
  {
    right = blk.arena_order.next;
    merges |= f_right;
  }

  if (arena.free == arena.size && manager.drop_arena(arena.data))
  {
    // drop arena?
    if (left != k_null_32)
      bank.strat.erase(bank.blocks, left);
    if (right != k_null_32)
      bank.strat.erase(bank.blocks, right);

    std::uint32_t arena_id = blk.arena;
    bank.free_size -= arena.size;
    arena.size = 0;
    arena.block_order.clear(bank.blocks);
    bank.arena_order.erase(bank.arenas, arena_id);
    return;
  }

  switch (merges)
  {
  case merge_type::e_none:
    bank.strat.add_free(bank.blocks, node);
    blk.is_free = true;
    break;
  case merge_type::e_left:
    bank.strat.replace(bank.blocks, left, left, bank.blocks[left].size + size);
    node_list.erase(bank.blocks, node);
    break;
  case merge_type::e_right:
    bank.strat.replace(bank.blocks, right, node, bank.blocks[right].size + size);
    node_list.erase(bank.blocks, right);
    blk.is_free = true;
    break;
  case merge_type::e_left_and_right:
    bank.strat.erase(bank.blocks, right);
    bank.strat.replace(bank.blocks, left, left, bank.blocks[left].size + bank.blocks[right].size + size);
    node_list.erase2(bank.blocks, node);
  }
}

template <typename traits>
inline void arena_allocator_impl<traits>::validate_integrity()
{
  std::uint32_t total_free_nodes = 0;
  for (auto arena_it = bank.arena_order.begin(bank.arenas), arena_end_it = bank.arena_order.end(bank.arenas);
       arena_it != arena_end_it; ++arena_it)
  {
    auto& arena           = *arena_it;
    bool  arena_allocated = false;

    for (auto blk_it = arena.block_order.begin(bank.blocks), blk_end_it = arena.block_order.end(bank.blocks);
         blk_it != blk_end_it; ++blk_it)
    {
      auto& blk = *blk_it;
      if ((blk.is_free))
        total_free_nodes++;
    }
  }

  assert(total_free_nodes == bank.strat.total_free_nodes(bank.blocks));
  assert(bank.strat.total_free_size(bank.blocks) == bank.free_size);

  for (auto arena_it = bank.arena_order.begin(bank.arenas), arena_end_it = bank.arena_order.end(bank.arenas);
       arena_it != arena_end_it; ++arena_it)
  {
    auto&     arena           = *arena_it;
    bool      arena_allocated = false;
    size_type expected_offset = 0;

    for (auto blk_it = arena.block_order.begin(bank.blocks), blk_end_it = arena.block_order.end(bank.blocks);
         blk_it != blk_end_it; ++blk_it)
    {
      auto& blk = *blk_it;
      assert(blk.offset == expected_offset);
      expected_offset += blk.size;
    }
  }

  bank.strat.validate_integrity(bank.blocks);
}

template <typename traits>
inline std::pair<ihandle, ihandle> arena_allocator_impl<traits>::add_arena(uhandle ihandle, size_type iarena_size,
                                                                           bool empty)
{
  this->statistics::report_new_arena();
  auto ret                    = add_arena(bank, ihandle, iarena_size, empty);
  bank.arenas[ret.first].data = manager.add_arena(ret.first, iarena_size);
  return ret;
}

template <typename traits>
inline std::pair<ihandle, ihandle> arena_allocator_impl<traits>::add_arena(bank_data& ibank, uhandle ihandle,
                                                                           size_type iarena_size, bool iempty)
{

  std::uint32_t arena_id  = ibank.arenas.emplace();
  auto&         arena_ref = ibank.arenas[arena_id];
  arena_ref.size          = iarena_size;
  std::uint32_t block_id  = ibank.blocks.emplace();
  block&        block_ref = ibank.blocks[block_id];
  block_ref.offset        = 0;
  block_ref.arena         = arena_id;
  block_ref.data          = ihandle;
  block_ref.size          = iarena_size;
  if (iempty)
  {
    block_ref.is_free = true;
    arena_ref.free    = iarena_size;
    ibank.strat.add_free_arena(ibank.blocks, block_id);
    ibank.free_size += iarena_size;
  }
  else
  {
    arena_ref.free = 0;
  }
  arena_ref.block_order.push_back(ibank.blocks, block_id);
  ibank.arena_order.push_back(ibank.arenas, arena_id);
  return std::make_pair(arena_id, block_id);
}

template <typename traits>
inline void arena_allocator_impl<traits>::defragment()
{
  manager.begin_defragment(*this);
  std::uint32_t arena_id = bank.arena_order.first;
  // refresh all banks
  bank_data refresh;

  acl::vector<std::uint32_t> rebinds;
  rebinds.reserve(bank.blocks.size());

  acl::vector<memory_move>   moves;
  decltype(bank.arena_order) deleted_arenas;
  for (auto arena_it = bank.arena_order.front(); arena_it != k_null_32;)
  {
    auto& arena           = bank.arenas[arena_it];
    bool  arena_allocated = false;

    for (auto blk_it = arena.block_order.begin(bank.blocks); blk_it; blk_it = arena.block_order.erase(blk_it))
    {
      auto& blk = *blk_it;
      if (!blk.is_free)
      {
        auto id = refresh.strat.try_allocate(refresh, blk.size);
        if (!refresh.strat.is_valid(id) && !arena_allocated)
        {
          auto p                       = add_arena(refresh, detail::k_null_uh, std::max(arena.size, blk.size), true);
          refresh.arenas[p.first].data = arena.data;
          id                           = refresh.strat.try_allocate(refresh, blk.size);
          arena_allocated              = true;
        }
        assert(refresh.strat.is_valid(id));

        auto  new_blk_id = refresh.strat.commit(refresh, blk.size, id);
        auto& new_blk    = refresh.blocks[new_blk_id];
        refresh.arenas[new_blk.arena].free -= blk.size;
        refresh.free_size -= blk.size;

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
      arena_it       = bank.arena_order.unlink(bank.arenas, to_delete);
      arena.free     = arena.size;
      deleted_arenas.push_back(bank.arenas, to_delete);
    }
    else
      arena_it = bank.arena_order.next(bank.arenas, arena_it);
  }

  for (auto& m : moves)
    // follow the copy sequence to ensure there is no overwrite
    manager.move_memory(bank.arenas[m.arena_src].data, refresh.arenas[m.arena_dst].data, m.from, m.to, m.size);

  for (auto rb : rebinds)
  {
    auto& dst_blk = refresh.blocks[rb];
    manager.rebind_alloc(dst_blk.data, alloc_info(refresh.arenas[dst_blk.arena].data, dst_blk.adjusted_offset(), rb));
  }

  for (auto arena_it = deleted_arenas.begin(bank.arenas); arena_it; arena_it = deleted_arenas.erase(arena_it))
  {
    auto& arena = *arena_it;
    manager.remove_arena(arena.data);
    statistics::report_defrag_arenas_removed();
  }

  bank = std::move(refresh);
  manager.end_defragment(*this);
}

template <typename traits>
inline typename arena_allocator_impl<traits>::size_type arena_allocator_impl<traits>::finalize_commit(
  block& blk, uhandle huser, size_type alignment)
{
  blk.data      = huser;
  blk.alignment = static_cast<std::uint8_t>(ACL_POPCOUNT(static_cast<std::uint32_t>(alignment)));
  bank.arenas[blk.arena].free -= blk.size;
  bank.free_size -= blk.size;
  return ((blk.offset + alignment) & ~alignment);
}

template <typename traits>
inline void arena_allocator_impl<traits>::copy(block const& src, block& dst)
{
  dst.data      = src.data;
  dst.alignment = src.alignment;
}

template <typename traits>
inline void arena_allocator_impl<traits>::push_memmove(acl::vector<memory_move>& dst, memory_move value)
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
    statistics::report_defrag_mem_move_merge();
  }
}
} // namespace acl::detail
