// SPDX-License-Identifier: MIT

#include "ouly/utility/user_config.hpp"

#include "ouly/allocators/best_fit_defrag_allocator.hpp"

namespace ouly
{

auto best_fit_defrag_allocator::get_adjusted_offset(allocation_id id) const noexcept -> size_type
{
  auto const mask = mask_of(id.get());
  return (get_offset(id) + mask) & ~mask;
}

auto best_fit_defrag_allocator::get_adjusted_size(allocation_id id) const noexcept -> size_type
{
  return get_size(id) - mask_of(id.get());
}

auto best_fit_defrag_allocator::get_alignment(allocation_id id) const noexcept -> size_type
{
  return mask_of(id.get()) + 1;
}

auto best_fit_defrag_allocator::find_placement(std::vector<uint16_t> const& order, uint32_t pos,
                                               std::vector<size_type> const& cursor, size_type raw_size) const
 -> std::pair<uint16_t, size_type>
{
  for (uint32_t k = 0; k < pos; ++k)
  {
    auto const cand = order[k];
    if (cursor[cand] + raw_size <= ouly::detail::vector_access(arena_entries().entries_, cand).size_)
    {
      return {cand, cursor[cand]};
    }
  }
  // the source arena always has room at its own cursor
  return {order[pos], cursor[order[pos]]};
}

void best_fit_defrag_allocator::rebuild_lists(std::vector<uint16_t> const& order, std::vector<placement>& plan)
{
  std::ranges::sort(plan,
                    [](placement const& a, placement const& b) -> bool
                    {
                      return a.dst_ != b.dst_ ? a.dst_ < b.dst_ : a.to_ < b.to_;
                    });
  for (auto arena_idx : order)
  {
    auto& arena      = ouly::detail::vector_access(arena_entries().entries_, arena_idx);
    arena.blocks_    = {};
    arena.free_size_ = arena.size_;
  }

  std::vector<std::pair<size_type, uint32_t>> new_free;
  for (size_t i = 0; i < plan.size();)
  {
    auto const dst   = plan[i].dst_;
    auto&      arena = ouly::detail::vector_access(arena_entries().entries_, dst);
    size_type  pos   = 0;
    for (; i < plan.size() && plan[i].dst_ == dst; ++i)
    {
      if (plan[i].to_ > pos)
      {
        auto gap = block_entries().push(pos, plan[i].to_ - pos, dst, true);
        arena.blocks_.push_back(block_entries(), gap);
        new_free.emplace_back(plan[i].to_ - pos, gap);
      }
      auto const block                                              = plan[i].block_;
      ouly::detail::vector_access(block_entries().ordering_, block) = {};
      arena.blocks_.push_back(block_entries(), block);
      arena.free_size_ -= plan[i].size_;
      pos = plan[i].to_ + plan[i].size_;
    }
    if (pos < arena.size_)
    {
      auto tail = block_entries().push(pos, arena.size_ - pos, dst, true);
      arena.blocks_.push_back(block_entries(), tail);
      new_free.emplace_back(arena.size_ - pos, tail);
    }
  }

  std::ranges::sort(new_free);
  free_sizes().clear();
  free_ordering().clear();
  free_sizes().reserve(new_free.size());
  free_ordering().reserve(new_free.size());
  for (auto const& [size, block] : new_free)
  {
    free_sizes().push_back(size);
    free_ordering().push_back(block);
  }
}

auto best_fit_defrag_allocator::drop_empty_arenas(std::vector<uint16_t> const& order) -> std::vector<uint16_t>
{
  std::vector<uint16_t> removed;
  for (auto arena_idx : order)
  {
    auto& arena = ouly::detail::vector_access(arena_entries().entries_, arena_idx);
    if (arena.blocks_.front() == 0)
    {
      arena.size_      = 0;
      arena.free_size_ = 0;
      arena_list().erase(arena_entries(), arena_idx);
      removed.push_back(arena_idx);
    }
  }
  return removed;
}

void best_fit_defrag_allocator::push_move(std::vector<defrag_move>& moves, defrag_move value)
{
  if (!moves.empty())
  {
    auto& back = moves.back();
    if (back.src_ == value.src_ && back.dst_ == value.dst_ && back.from_ + back.size_ == value.from_ &&
        back.to_ + back.size_ == value.to_)
    {
      back.size_ += value.size_;
      return;
    }
  }
  moves.push_back(value);
}

auto best_fit_defrag_allocator::mask_of(uint32_t block) const noexcept -> size_type
{
  auto const pow2 = block < alignments_.size() ? alignments_[block] & ~dedicated_bit : 0;
  return (size_type{1} << pow2) - 1;
}

auto best_fit_defrag_allocator::is_dedicated(uint32_t block) const noexcept -> bool
{
  return block < alignments_.size() && (alignments_[block] & dedicated_bit) != 0;
}

} // namespace ouly
