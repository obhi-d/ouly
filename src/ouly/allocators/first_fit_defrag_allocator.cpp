// SPDX-License-Identifier: MIT

#include "ouly/utility/user_config.hpp"

#include "ouly/allocators/first_fit_defrag_allocator.hpp"

namespace ouly
{

first_fit_defrag_allocator::first_fit_defrag_allocator() noexcept = default;

first_fit_defrag_allocator::first_fit_defrag_allocator(size_type arena_sz) noexcept : arena_size_(arena_sz) {}

void first_fit_defrag_allocator::set_arena_size(size_type s) noexcept
{
  arena_size_ = std::max(arena_size_, s);
}

auto first_fit_defrag_allocator::get_arena_size() const noexcept -> size_type
{
  return arena_size_;
}

auto first_fit_defrag_allocator::get_offset(allocation_id id) const noexcept -> size_type
{
  return ouly::detail::vector_access(entry_offsets_, id.get());
}

auto first_fit_defrag_allocator::get_size(allocation_id id) const noexcept -> size_type
{
  return ouly::detail::vector_access(entry_sizes_, id.get());
}

auto first_fit_defrag_allocator::get_arena(allocation_id id) const noexcept -> arena_id
{
  return arena_id{ouly::detail::vector_access(entry_arenas_, id.get())};
}

auto first_fit_defrag_allocator::get_alignment(allocation_id id) const noexcept -> size_type
{
  return size_type{1} << ouly::detail::vector_access(entry_alignments_, id.get());
}

void first_fit_defrag_allocator::validate_integrity() const
{
  std::vector<std::vector<uint32_t>> per_arena(arena_pool_.size());
  for (uint32_t id = 1; id < static_cast<uint32_t>(entry_offsets_.size()); ++id)
  {
    if (ouly::detail::vector_access(entry_live_, id))
    {
      per_arena[ouly::detail::vector_access(entry_arenas_, id)].push_back(id);
    }
  }

  std::vector<bool> active(arena_pool_.size(), false);
  for (auto arena : arena_order_)
  {
    active[arena] = true;
  }
  for (uint32_t arena = 0; arena < static_cast<uint32_t>(arena_pool_.size()); ++arena)
  {
    OULY_ASSERT(active[arena] || per_arena[arena].empty());
  }

  for (auto arena : arena_order_)
  {
    validate_arena(ouly::detail::vector_access(arena_pool_, arena), per_arena[arena]);
  }
}

auto first_fit_defrag_allocator::snapshot_allocations() const -> std::vector<defrag_item>
{
  std::vector<uint32_t> arena_pos(arena_pool_.size(), 0);
  for (uint32_t i = 0; i < static_cast<uint32_t>(arena_order_.size()); ++i)
  {
    arena_pos[ouly::detail::vector_access(arena_order_, i)] = i;
  }
  std::vector<defrag_item> items;
  items.reserve(entry_offsets_.size());
  for (uint32_t id = 1; id < static_cast<uint32_t>(entry_offsets_.size()); ++id)
  {
    if (ouly::detail::vector_access(entry_live_, id))
    {
      items.push_back({.pos_    = arena_pos[ouly::detail::vector_access(entry_arenas_, id)],
                       .offset_ = ouly::detail::vector_access(entry_offsets_, id),
                       .id_     = id});
    }
  }
  std::ranges::sort(items,
                    [](defrag_item const& a, defrag_item const& b) -> bool
                    {
                      return a.pos_ != b.pos_ ? a.pos_ < b.pos_ : a.offset_ < b.offset_;
                    });
  return items;
}

auto first_fit_defrag_allocator::find_placement(defrag_item const& it, std::vector<size_type> const& cursor) const
 -> std::pair<uint16_t, size_type>
{
  auto const src  = ouly::detail::vector_access(entry_arenas_, it.id_);
  auto const size = ouly::detail::vector_access(entry_sizes_, it.id_);
  auto const mask = (size_type{1} << ouly::detail::vector_access(entry_alignments_, it.id_)) - 1;
  for (uint32_t k = 0; k <= it.pos_; ++k)
  {
    auto const cand    = ouly::detail::vector_access(arena_order_, k);
    auto const aligned = (cursor[cand] + mask) & ~mask;
    // the source arena always has room at its own cursor
    if (cand == src || aligned + size <= ouly::detail::vector_access(arena_pool_, cand).size_)
    {
      return {cand, aligned};
    }
  }
  OULY_ASSERT(false && "unreachable: the source arena always fits");
  return {src, ouly::detail::vector_access(entry_offsets_, it.id_)};
}

void first_fit_defrag_allocator::apply_plan(std::vector<placement>& plan)
{
  std::ranges::sort(plan,
                    [](placement const& a, placement const& b) -> bool
                    {
                      return a.dst_ != b.dst_ ? a.dst_ < b.dst_ : a.to_ < b.to_;
                    });
  for (auto arena : arena_order_)
  {
    auto& ar = ouly::detail::vector_access(arena_pool_, arena);
    ar.free_offsets_.clear();
    ar.free_sizes_.clear();
    ar.free_   = ar.size_;
    ar.allocs_ = 0;
  }
  for (size_t i = 0; i < plan.size();)
  {
    auto const dst = plan[i].dst_;
    auto&      ar  = ouly::detail::vector_access(arena_pool_, dst);
    size_type  pos = 0;
    for (; i < plan.size() && plan[i].dst_ == dst; ++i)
    {
      if (plan[i].to_ > pos)
      {
        ar.free_offsets_.push_back(pos);
        ar.free_sizes_.push_back(plan[i].to_ - pos);
      }
      ar.free_ -= plan[i].size_;
      ar.allocs_++;
      pos = plan[i].to_ + plan[i].size_;
    }
    if (pos < ar.size_)
    {
      ar.free_offsets_.push_back(pos);
      ar.free_sizes_.push_back(ar.size_ - pos);
    }
  }
}

auto first_fit_defrag_allocator::drop_empty_arenas() -> std::vector<uint16_t>
{
  std::vector<uint16_t> removed;
  std::erase_if(arena_order_,
                [&](uint16_t arena) -> bool
                {
                  if (ouly::detail::vector_access(arena_pool_, arena).allocs_ != 0)
                  {
                    return false;
                  }
                  removed.push_back(arena);
                  return true;
                });
  for (auto arena : removed)
  {
    ouly::detail::vector_access(arena_pool_, arena) = {};
    free_arenas_.push_back(arena);
  }
  return removed;
}

void first_fit_defrag_allocator::validate_arena(arena_state const& ar, std::vector<uint32_t>& allocs) const
{
  std::ranges::sort(allocs,
                    [this](uint32_t a, uint32_t b) -> bool
                    {
                      return ouly::detail::vector_access(entry_offsets_, a) <
                             ouly::detail::vector_access(entry_offsets_, b);
                    });
  OULY_ASSERT(ar.allocs_ == allocs.size());

  // free blocks and allocations must exactly tile [0, size)
  [[maybe_unused]] size_type free_total = 0;
  size_type                  pos        = 0;
  size_t                     fi         = 0;
  size_t                     ai         = 0;
  while (pos < ar.size_)
  {
    if (fi < ar.free_offsets_.size() && ar.free_offsets_[fi] == pos)
    {
      OULY_ASSERT(ar.free_sizes_[fi] > 0);
      free_total += ar.free_sizes_[fi];
      pos += ar.free_sizes_[fi];
      ++fi;
      // adjacent free blocks must have been coalesced
      OULY_ASSERT(fi == ar.free_offsets_.size() || ar.free_offsets_[fi] > pos);
    }
    else if (ai < allocs.size() && ouly::detail::vector_access(entry_offsets_, allocs[ai]) == pos)
    {
      pos += ouly::detail::vector_access(entry_sizes_, allocs[ai]);
      ++ai;
    }
    else
    {
      OULY_ASSERT(false && "arena layout has a hole or an overlap");
      return;
    }
  }
  OULY_ASSERT(pos == ar.size_);
  OULY_ASSERT(fi == ar.free_offsets_.size());
  OULY_ASSERT(ai == allocs.size());
  OULY_ASSERT(free_total == ar.free_);
}

void first_fit_defrag_allocator::push_move(std::vector<defrag_move>& moves, defrag_move value)
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

auto first_fit_defrag_allocator::try_allocate(uint16_t arena, arena_state& ar, size_type size, size_type mask)
 -> ca_allocation
{
  for (uint32_t i = 0, end = static_cast<uint32_t>(ar.free_offsets_.size()); i < end; ++i)
  {
    auto const offset  = ar.free_offsets_[i];
    auto const fsize   = ar.free_sizes_[i];
    auto const aligned = (offset + mask) & ~mask;
    auto const pad     = aligned - offset;
    if (fsize < pad + size)
    {
      continue;
    }
    auto const remaining = fsize - pad - size;
    if (pad == 0)
    {
      if (remaining == 0)
      {
        ar.free_offsets_.erase(ar.free_offsets_.begin() + i);
        ar.free_sizes_.erase(ar.free_sizes_.begin() + i);
      }
      else
      {
        ar.free_offsets_[i] += size;
        ar.free_sizes_[i] = remaining;
      }
    }
    else
    {
      // alignment padding stays free
      ar.free_sizes_[i] = pad;
      if (remaining != 0)
      {
        ar.free_offsets_.insert(ar.free_offsets_.begin() + i + 1, aligned + size);
        ar.free_sizes_.insert(ar.free_sizes_.begin() + i + 1, remaining);
      }
    }
    ar.free_ -= size;
    ar.allocs_++;
    auto id = push_entry(aligned, size, arena, static_cast<uint8_t>(std::popcount(mask)));
    return ca_allocation{.offset_ = aligned, .id_ = {.id_ = id}, .arena_ = {.id_ = arena}};
  }
  return {};
}

void first_fit_defrag_allocator::arena_free(arena_state& ar, size_type offset, size_type size)
{
  auto&      offsets = ar.free_offsets_;
  auto&      sizes   = ar.free_sizes_;
  auto const it      = std::ranges::lower_bound(offsets, offset);
  auto const idx     = static_cast<size_t>(std::distance(offsets.begin(), it));

  bool const merge_left  = idx > 0 && offsets[idx - 1] + sizes[idx - 1] == offset;
  bool const merge_right = idx < offsets.size() && offset + size == offsets[idx];

  if (merge_left && merge_right)
  {
    sizes[idx - 1] += size + sizes[idx];
    offsets.erase(offsets.begin() + static_cast<std::ptrdiff_t>(idx));
    sizes.erase(sizes.begin() + static_cast<std::ptrdiff_t>(idx));
  }
  else if (merge_left)
  {
    sizes[idx - 1] += size;
  }
  else if (merge_right)
  {
    offsets[idx] = offset;
    sizes[idx] += size;
  }
  else
  {
    offsets.insert(offsets.begin() + static_cast<std::ptrdiff_t>(idx), offset);
    sizes.insert(sizes.begin() + static_cast<std::ptrdiff_t>(idx), size);
  }
  ar.free_ += size;
}

void first_fit_defrag_allocator::drop_arena(uint16_t arena)
{
  auto pos = std::ranges::find(arena_order_, arena);
  OULY_ASSERT(pos != arena_order_.end());
  arena_order_.erase(pos);
  ouly::detail::vector_access(arena_pool_, arena) = {};
  free_arenas_.push_back(arena);
}

auto first_fit_defrag_allocator::push_entry(size_type offset, size_type size, uint16_t arena, uint8_t align_pow2)
 -> uint32_t
{
  uint32_t id = 0;
  if (free_entry_ != 0)
  {
    id          = free_entry_;
    free_entry_ = static_cast<uint32_t>(ouly::detail::vector_access(entry_offsets_, id));
  }
  else
  {
    id = static_cast<uint32_t>(entry_offsets_.size());
    entry_offsets_.emplace_back();
    entry_sizes_.emplace_back();
    entry_arenas_.emplace_back();
    entry_alignments_.emplace_back();
    entry_live_.emplace_back();
    entry_dedicated_.emplace_back();
  }
  ouly::detail::vector_access(entry_offsets_, id)    = offset;
  ouly::detail::vector_access(entry_sizes_, id)      = size;
  ouly::detail::vector_access(entry_arenas_, id)     = arena;
  ouly::detail::vector_access(entry_alignments_, id) = align_pow2;
  ouly::detail::vector_access(entry_live_, id)       = true;
  ouly::detail::vector_access(entry_dedicated_, id)  = false;
  return id;
}

void first_fit_defrag_allocator::free_entry(uint32_t id)
{
  ouly::detail::vector_access(entry_live_, id)    = false;
  ouly::detail::vector_access(entry_offsets_, id) = free_entry_;
  free_entry_                                     = id;
}

} // namespace ouly
