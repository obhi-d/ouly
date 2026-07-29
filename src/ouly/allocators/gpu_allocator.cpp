// SPDX-License-Identifier: MIT

#include "ouly/allocators/gpu_allocator.hpp"

#include <cstddef>

namespace ouly
{

namespace
{

using ouly::detail::align_offset;

/**
 * @brief Assert that the occupied and free ranges of an arena tile it exactly once
 *
 * @param ranges Ranges held by live allocations, reservations and retiring sources; taken by value
 * because the free ranges of the arena are merged into it
 * @param state Arena the ranges belong to
 */
template <typename Ranges, typename State>
void validate_arena_layout(Ranges ranges, State const& state)
{
  for (auto const& free_range : state.free_ranges_)
  {
    ranges.push_back({.offset_ = free_range.offset_, .size_ = free_range.size_});
  }
  std::ranges::sort(ranges,
                    [](auto const& lhs, auto const& rhs) -> bool
                    {
                      return lhs.offset_ < rhs.offset_;
                    });

  [[maybe_unused]] allocation_size_type cursor = 0;
  for (auto const& range : ranges)
  {
    OULY_ASSERT(range.size_ != 0);
    OULY_ASSERT(range.offset_ == cursor);
    cursor += range.size_;
  }

  [[maybe_unused]] allocation_size_type free = 0;
  for (auto const& free_range : state.free_ranges_)
  {
    free += free_range.size_;
  }

  OULY_ASSERT(cursor == state.capacity_);
  OULY_ASSERT(free == state.free_);
}

} // namespace

gpu_allocator::gpu_allocator() = default;

gpu_allocator::gpu_allocator(size_type arena_size) : arena_size_(arena_size) {}

void gpu_allocator::set_arena_size(size_type size) noexcept
{
  arena_size_ = std::max(arena_size_, size);
}

auto gpu_allocator::get_arena_size() const noexcept -> size_type
{
  return arena_size_;
}

auto gpu_allocator::get_offset(allocation_id id) const noexcept -> size_type
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  return ouly::detail::vector_access(entry_offsets_, id.get());
}

auto gpu_allocator::get_size(allocation_id id) const noexcept -> size_type
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  return ouly::detail::vector_access(entry_sizes_, id.get());
}

auto gpu_allocator::get_alignment(allocation_id id) const noexcept -> size_type
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  return size_type{1} << ouly::detail::vector_access(entry_alignments_, id.get());
}

auto gpu_allocator::get_arena(allocation_id id) const noexcept -> arena_id
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  return arena_id{ouly::detail::vector_access(entry_arenas_, id.get())};
}

auto gpu_allocator::get_memory_class(allocation_id id) const noexcept -> uint32_t
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  return ouly::detail::vector_access(entry_memory_classes_, id.get());
}

auto gpu_allocator::get_resource_class(allocation_id id) const noexcept -> uint32_t
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  return ouly::detail::vector_access(entry_resource_classes_, id.get());
}

auto gpu_allocator::is_movable(allocation_id id) const noexcept -> bool
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  return ouly::detail::vector_access(entry_movable_, id.get());
}

auto gpu_allocator::is_relocating(allocation_id id) const noexcept -> bool
{
  if (!target_)
  {
    return false;
  }
  return std::ranges::any_of(target_->moves_,
                             [id](planned_move const& move) -> bool
                             {
                               return move.allocation_ == id && move.state_ != move_state::source_retired &&
                                      move.state_ != move_state::cancelled;
                             });
}

void gpu_allocator::set_movable(allocation_id id, bool movable)
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  OULY_ASSERT(!is_relocating(id));
  [[maybe_unused]] auto const arena = ouly::detail::vector_access(entry_arenas_, id.get());
  OULY_ASSERT(!ouly::detail::vector_access(arena_pool_, arena).dedicated_ || !movable);
  ouly::detail::vector_access(entry_movable_, id.get()) = movable;
}

auto gpu_allocator::try_allocate(size_type size, gpu_allocation_options const& options) -> gpu_allocation
{
  for (auto arena : arena_order_)
  {
    auto const& state = ouly::detail::vector_access(arena_pool_, arena);
    if (state.role_ != arena_role::normal || state.dedicated_ || state.memory_class_ != options.memory_class_ ||
        state.free_ < size)
    {
      continue;
    }
    if (auto allocation = try_allocate_in_arena(arena, size, options); allocation.id_ != allocation_id())
    {
      return allocation;
    }
  }
  return {};
}

auto gpu_allocator::try_allocate_in_arena(uint16_t arena, size_type size, gpu_allocation_options const& options)
 -> gpu_allocation
{
  auto& state = ouly::detail::vector_access(arena_pool_, arena);
  for (size_t index = 0; index < state.free_ranges_.size(); ++index)
  {
    auto const range   = ouly::detail::vector_access(state.free_ranges_, index);
    auto const aligned = align_offset(range.offset_, options.alignment_);
    if (!aligned || *aligned < range.offset_ || *aligned - range.offset_ > range.size_ ||
        size > range.size_ - (*aligned - range.offset_))
    {
      continue;
    }

    [[maybe_unused]] bool const reserved = reserve_range(arena, *aligned, size);
    OULY_ASSERT(reserved);
    --state.reservations_;
    ++state.allocations_;
    auto const id = push_allocation(arena, *aligned, size, options);
    return gpu_allocation{.offset_ = *aligned, .id_ = id, .arena_ = arena_id{arena}};
  }
  return {};
}

auto gpu_allocator::create_arena(size_type capacity, uint32_t memory_class, bool dedicated) -> arena_id
{
  uint16_t arena = 0;
  if (!free_arenas_.empty())
  {
    arena = free_arenas_.back();
    free_arenas_.pop_back();
  }
  else
  {
    OULY_ASSERT(arena_pool_.size() < std::numeric_limits<uint16_t>::max());
    arena = static_cast<uint16_t>(arena_pool_.size());
    arena_pool_.emplace_back();
  }

  auto& state         = ouly::detail::vector_access(arena_pool_, arena);
  state.capacity_     = capacity;
  state.free_         = capacity;
  state.memory_class_ = memory_class;
  state.active_       = true;
  state.dedicated_    = dedicated;
  state.free_ranges_  = {
   {.offset_ = 0, .size_ = capacity}
  };
  arena_order_.push_back(arena);
  return arena_id{arena};
}

auto gpu_allocator::push_allocation(uint16_t arena, size_type offset, size_type size,
                                    gpu_allocation_options const& options) -> allocation_id
{
  uint32_t id = 0;
  if (free_entry_ != 0)
  {
    id          = free_entry_;
    free_entry_ = static_cast<uint32_t>(ouly::detail::vector_access(entry_offsets_, id));
  }
  else
  {
    OULY_ASSERT(entry_offsets_.size() < std::numeric_limits<uint32_t>::max());
    id = static_cast<uint32_t>(entry_offsets_.size());
    entry_offsets_.emplace_back();
    entry_sizes_.emplace_back();
    entry_arenas_.emplace_back();
    entry_alignments_.emplace_back();
    entry_memory_classes_.emplace_back();
    entry_resource_classes_.emplace_back();
    entry_live_.emplace_back();
    entry_movable_.emplace_back();
  }

  ouly::detail::vector_access(entry_offsets_, id)          = offset;
  ouly::detail::vector_access(entry_sizes_, id)            = size;
  ouly::detail::vector_access(entry_arenas_, id)           = arena;
  ouly::detail::vector_access(entry_alignments_, id)       = static_cast<uint8_t>(std::countr_zero(options.alignment_));
  ouly::detail::vector_access(entry_memory_classes_, id)   = options.memory_class_;
  ouly::detail::vector_access(entry_resource_classes_, id) = options.resource_class_;
  ouly::detail::vector_access(entry_live_, id)             = true;
  ouly::detail::vector_access(entry_movable_, id)          = options.movable_;
  return allocation_id{id};
}

auto gpu_allocator::release_allocation(allocation_id id) -> uint16_t
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  auto const arena  = ouly::detail::vector_access(entry_arenas_, id.get());
  auto const offset = ouly::detail::vector_access(entry_offsets_, id.get());
  auto const size   = ouly::detail::vector_access(entry_sizes_, id.get());
  auto&      state  = ouly::detail::vector_access(arena_pool_, arena);

  OULY_ASSERT(state.allocations_ != 0);
  --state.allocations_;
  free_range_in_arena(arena, offset, size);

  ouly::detail::vector_access(entry_live_, id.get())    = false;
  ouly::detail::vector_access(entry_offsets_, id.get()) = free_entry_;
  free_entry_                                           = id.get();
  return arena;
}

auto gpu_allocator::prepare_deallocate(allocation_id id) -> bool
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  if (!target_)
  {
    return true;
  }

  auto move = std::ranges::find_if(target_->moves_,
                                   [id](planned_move const& candidate) -> bool
                                   {
                                     return candidate.allocation_ == id;
                                   });
  if (move == target_->moves_.end())
  {
    return true;
  }
  if (move->state_ == move_state::copy_scheduled)
  {
    return false;
  }
  if (move->state_ == move_state::reserved)
  {
    cancel_move(*move);
  }
  return true;
}

auto gpu_allocator::measure_arena(uint16_t arena) const -> arena_usage
{
  arena_usage usage;
  for (uint32_t id = 1; id < static_cast<uint32_t>(entry_live_.size()); ++id)
  {
    if (ouly::detail::vector_access(entry_live_, id) && ouly::detail::vector_access(entry_arenas_, id) == arena)
    {
      usage.live_ += ouly::detail::vector_access(entry_sizes_, id);
      ++usage.count_;
      usage.pinned_ = usage.pinned_ || !ouly::detail::vector_access(entry_movable_, id);
    }
  }
  return usage;
}

auto gpu_allocator::can_evacuate(uint16_t arena) const -> bool
{
  auto const& state = ouly::detail::vector_access(arena_pool_, arena);
  return state.role_ == arena_role::normal && !state.dedicated_ && state.allocations_ != 0 && state.free_ != 0 &&
         state.reservations_ == 0 && state.retiring_ == 0;
}

auto gpu_allocator::recovers_more(evacuation_candidate const& lhs, evacuation_candidate const& rhs) -> bool
{
  auto const lhs_recovery = lhs.capacity_ - lhs.live_;
  auto const rhs_recovery = rhs.capacity_ - rhs.live_;
  if (lhs_recovery != rhs_recovery)
  {
    return lhs_recovery > rhs_recovery;
  }
  if (lhs.live_ != rhs.live_)
  {
    return lhs.live_ < rhs.live_;
  }
  if (lhs.count_ != rhs.count_)
  {
    return lhs.count_ < rhs.count_;
  }
  return lhs.arena_ < rhs.arena_;
}

auto gpu_allocator::reserve_plan(evacuation_target& plan) -> bool
{
  size_t reserved_count = 0;
  for (auto& move : plan.moves_)
  {
    if (!reserve_range(move.destination_arena_, move.destination_offset_, move.size_))
    {
      break;
    }
    move.move_ = allocate_move_id();
    ++reserved_count;
  }

  if (reserved_count == plan.moves_.size())
  {
    return true;
  }

  for (size_t index = 0; index < reserved_count; ++index)
  {
    auto const& move = ouly::detail::vector_access(plan.moves_, index);
    release_reserved_range(move.destination_arena_, move.destination_offset_, move.size_);
  }
  return false;
}

void gpu_allocator::begin_evacuation(uint32_t max_blocks_scanned, scratch_allocator_ref scratch)
{
  ouly::scratch_vector<evacuation_candidate> candidates{scratch, arena_order_.size()};
  for (auto arena : arena_order_)
  {
    if (!can_evacuate(arena))
    {
      continue;
    }

    auto const usage = measure_arena(arena);
    if (!usage.pinned_)
    {
      candidates.push_back({.arena_    = arena,
                            .capacity_ = ouly::detail::vector_access(arena_pool_, arena).capacity_,
                            .live_     = usage.live_,
                            .count_    = usage.count_});
    }
  }

  std::ranges::sort(candidates, &gpu_allocator::recovers_more);

  uint32_t scanned = 0;
  for (auto const& candidate : candidates)
  {
    if (scanned >= max_blocks_scanned)
    {
      break;
    }
    ++scanned;

    auto plan = simulate_evacuation(candidate.arena_, scratch);
    if (!plan || !reserve_plan(*plan))
    {
      continue;
    }

    ouly::detail::vector_access(arena_pool_, candidate.arena_).role_ = arena_role::evacuation_source;
    target_                                                          = std::move(*plan);
    return;
  }
}

auto gpu_allocator::simulate_evacuation(uint16_t source, scratch_allocator_ref scratch) const
 -> std::optional<evacuation_target>
{
  evacuation_target plan{.source_arena_ = source, .moves_ = {}, .aborted_ = false};
  for (uint32_t id = 1; id < static_cast<uint32_t>(entry_live_.size()); ++id)
  {
    if (ouly::detail::vector_access(entry_live_, id) && ouly::detail::vector_access(entry_arenas_, id) == source)
    {
      plan.moves_.push_back({.move_               = gpu_move_id{},
                             .allocation_         = allocation_id{id},
                             .source_arena_       = source,
                             .destination_arena_  = 0,
                             .source_offset_      = ouly::detail::vector_access(entry_offsets_, id),
                             .destination_offset_ = 0,
                             .size_               = ouly::detail::vector_access(entry_sizes_, id)});
    }
  }

  std::ranges::sort(plan.moves_,
                    [this](planned_move const& lhs, planned_move const& rhs) -> bool
                    {
                      auto const lhs_alignment = ouly::detail::vector_access(entry_alignments_, lhs.allocation_.get());
                      auto const rhs_alignment = ouly::detail::vector_access(entry_alignments_, rhs.allocation_.get());
                      return lhs_alignment != rhs_alignment ? lhs_alignment > rhs_alignment : lhs.size_ > rhs.size_;
                    });

  auto destinations = collect_destinations(source, plan.moves_.size(), scratch);
  for (auto& move : plan.moves_)
  {
    auto const alignment = size_type{1} << ouly::detail::vector_access(entry_alignments_, move.allocation_.get());
    auto const fit       = find_best_fit(destinations, move.size_, alignment);
    if (fit.arena_ == nullptr)
    {
      return std::nullopt;
    }

    occupy(*fit.arena_, fit.range_, fit.offset_, move.size_);
    move.destination_arena_  = fit.arena_->arena_;
    move.destination_offset_ = fit.offset_;
  }

  return plan;
}

auto gpu_allocator::collect_destinations(uint16_t source, size_t move_count, scratch_allocator_ref scratch) const
 -> ouly::scratch_vector<gpu_allocator::simulated_arena>
{
  ouly::scratch_vector<simulated_arena> destinations{scratch, arena_order_.size()};
  auto const                            memory_class = ouly::detail::vector_access(arena_pool_, source).memory_class_;
  for (auto arena : arena_order_)
  {
    auto const& state = ouly::detail::vector_access(arena_pool_, arena);
    if (arena == source || state.role_ != arena_role::normal || state.dedicated_ || state.memory_class_ != memory_class)
    {
      continue;
    }

    simulated_arena destination{
     .arena_ = arena, .free_ = state.free_, .ranges_ = ouly::scratch_vector<free_range>{scratch}};
    // Placing a move splits one free range into at most two, so a slot per move is all the
    // simulated range list can ever need on top of the ranges the arena already has.
    destination.ranges_.reserve(state.free_ranges_.size() + move_count);
    for (auto const& range : state.free_ranges_)
    {
      destination.ranges_.push_back(range);
    }
    destinations.push_back(destination);
  }
  return destinations;
}

auto gpu_allocator::find_best_fit(ouly::scratch_vector<simulated_arena>& destinations, size_type size,
                                  size_type alignment) -> gpu_allocator::simulated_fit
{
  simulated_fit fit;
  size_type     best_waste = std::numeric_limits<size_type>::max();

  for (auto& destination : destinations)
  {
    if (destination.free_ < size)
    {
      continue;
    }
    for (size_t index = 0; index < destination.ranges_.size(); ++index)
    {
      auto const range   = ouly::detail::vector_access(destination.ranges_, index);
      auto const aligned = align_offset(range.offset_, alignment);
      if (!aligned || *aligned < range.offset_ || *aligned - range.offset_ > range.size_ ||
          size > range.size_ - (*aligned - range.offset_))
      {
        continue;
      }

      auto const waste = range.size_ - size;
      if (fit.arena_ == nullptr || waste < best_waste || (waste == best_waste && destination.free_ < fit.arena_->free_))
      {
        fit        = {.arena_ = &destination, .range_ = index, .offset_ = *aligned};
        best_waste = waste;
      }
    }
  }
  return fit;
}

void gpu_allocator::occupy(simulated_arena& destination, size_t range_index, size_type offset, size_type size)
{
  auto const range = ouly::detail::vector_access(destination.ranges_, range_index);
  destination.ranges_.erase(range_index);
  if (offset > range.offset_)
  {
    destination.ranges_.insert(range_index, {.offset_ = range.offset_, .size_ = offset - range.offset_});
    ++range_index;
  }

  auto const end = offset + size;
  if (end < range.offset_ + range.size_)
  {
    destination.ranges_.insert(range_index, {.offset_ = end, .size_ = range.offset_ + range.size_ - end});
  }
  destination.free_ -= size;
}

auto gpu_allocator::reserve_range(uint16_t arena, size_type offset, size_type size) -> bool
{
  auto& state = ouly::detail::vector_access(arena_pool_, arena);
  for (size_t index = 0; index < state.free_ranges_.size(); ++index)
  {
    auto const range = ouly::detail::vector_access(state.free_ranges_, index);
    if (offset < range.offset_ || offset - range.offset_ > range.size_ || size > range.size_ - (offset - range.offset_))
    {
      continue;
    }

    state.free_ranges_.erase(state.free_ranges_.begin() + static_cast<std::ptrdiff_t>(index));
    if (offset > range.offset_)
    {
      state.free_ranges_.insert(state.free_ranges_.begin() + static_cast<std::ptrdiff_t>(index),
                                {.offset_ = range.offset_, .size_ = offset - range.offset_});
      ++index;
    }
    auto const end = offset + size;
    if (end < range.offset_ + range.size_)
    {
      state.free_ranges_.insert(state.free_ranges_.begin() + static_cast<std::ptrdiff_t>(index),
                                {.offset_ = end, .size_ = range.offset_ + range.size_ - end});
    }
    state.free_ -= size;
    ++state.reservations_;
    return true;
  }
  return false;
}

void gpu_allocator::release_reserved_range(uint16_t arena, size_type offset, size_type size)
{
  auto& state = ouly::detail::vector_access(arena_pool_, arena);
  OULY_ASSERT(state.reservations_ != 0);
  --state.reservations_;
  free_range_in_arena(arena, offset, size);
}

void gpu_allocator::free_range_in_arena(uint16_t arena, size_type offset, size_type size)
{
  auto& state = ouly::detail::vector_access(arena_pool_, arena);
  auto  it    = std::ranges::lower_bound(state.free_ranges_, offset, {}, &free_range::offset_);
  auto  index = static_cast<size_t>(std::distance(state.free_ranges_.begin(), it));

  bool const merge_left =
   index != 0 && state.free_ranges_[index - 1].offset_ + state.free_ranges_[index - 1].size_ == offset;
  bool const merge_right = index != state.free_ranges_.size() && offset + size == state.free_ranges_[index].offset_;

  if (merge_left && merge_right)
  {
    state.free_ranges_[index - 1].size_ += size + state.free_ranges_[index].size_;
    state.free_ranges_.erase(state.free_ranges_.begin() + static_cast<std::ptrdiff_t>(index));
  }
  else if (merge_left)
  {
    state.free_ranges_[index - 1].size_ += size;
  }
  else if (merge_right)
  {
    state.free_ranges_[index].offset_ = offset;
    state.free_ranges_[index].size_ += size;
  }
  else
  {
    state.free_ranges_.insert(state.free_ranges_.begin() + static_cast<std::ptrdiff_t>(index),
                              {.offset_ = offset, .size_ = size});
  }
  state.free_ += size;
}

void gpu_allocator::cancel_move(planned_move& move)
{
  if (move.state_ == move_state::reserved || move.state_ == move_state::copy_scheduled)
  {
    release_reserved_range(move.destination_arena_, move.destination_offset_, move.size_);
  }
  move.state_ = move_state::cancelled;
}

void gpu_allocator::abort_evacuation()
{
  OULY_ASSERT(target_);
  if (target_)
  {
    target_->aborted_ = true;

    for (auto& move : target_->moves_)
    {
      if (move.state_ == move_state::reserved)
      {
        cancel_move(move);
      }
    }
  }
}

auto gpu_allocator::find_schedulable_move(size_type remaining_bytes) -> planned_move*
{
  if (!target_)
  {
    return nullptr;
  }
  auto move =
   std::ranges::find_if(target_->moves_,
                        [remaining_bytes](planned_move const& candidate) -> bool
                        {
                          return candidate.state_ == move_state::reserved && candidate.size_ <= remaining_bytes;
                        });
  return move == target_->moves_.end() ? nullptr : &*move;
}

auto gpu_allocator::has_reserved_moves() const -> bool
{
  return target_ && std::ranges::any_of(target_->moves_,
                                        [](planned_move const& move) -> bool
                                        {
                                          return move.state_ == move_state::reserved;
                                        });
}

auto gpu_allocator::target_finished() const -> bool
{
  return target_ && std::ranges::all_of(target_->moves_,
                                        [](planned_move const& move) -> bool
                                        {
                                          return move.state_ == move_state::source_retired ||
                                                 move.state_ == move_state::cancelled;
                                        });
}

auto gpu_allocator::make_relocation(planned_move const& move) const -> gpu_relocation
{
  auto const id = move.allocation_.get();
  return {.move_               = move.move_,
          .allocation_         = move.allocation_,
          .source_arena_       = arena_id{move.source_arena_},
          .destination_arena_  = arena_id{move.destination_arena_},
          .source_offset_      = move.source_offset_,
          .destination_offset_ = move.destination_offset_,
          .size_               = move.size_,
          .memory_class_       = ouly::detail::vector_access(entry_memory_classes_, id),
          .resource_class_     = ouly::detail::vector_access(entry_resource_classes_, id)};
}

void gpu_allocator::switch_allocation(planned_move const& move)
{
  auto& source      = ouly::detail::vector_access(arena_pool_, move.source_arena_);
  auto& destination = ouly::detail::vector_access(arena_pool_, move.destination_arena_);
  OULY_ASSERT(source.allocations_ != 0);
  OULY_ASSERT(destination.reservations_ != 0);
  --source.allocations_;
  ++source.retiring_;
  --destination.reservations_;
  ++destination.allocations_;

  auto const id                                   = move.allocation_.get();
  ouly::detail::vector_access(entry_arenas_, id)  = move.destination_arena_;
  ouly::detail::vector_access(entry_offsets_, id) = move.destination_offset_;
}

void gpu_allocator::retire_source(planned_move const& move)
{
  auto& source = ouly::detail::vector_access(arena_pool_, move.source_arena_);
  OULY_ASSERT(source.retiring_ != 0);
  --source.retiring_;
  free_range_in_arena(move.source_arena_, move.source_offset_, move.size_);
}

auto gpu_allocator::can_release_arena(uint16_t arena) const -> bool
{
  auto const& state = ouly::detail::vector_access(arena_pool_, arena);
  return state.active_ && state.role_ == arena_role::normal && state.allocations_ == 0 && state.reservations_ == 0 &&
         state.retiring_ == 0 && state.free_ == state.capacity_;
}

auto gpu_allocator::drop_arena(uint16_t arena) -> size_type
{
  auto& state = ouly::detail::vector_access(arena_pool_, arena);
  OULY_ASSERT(can_release_arena(arena));
  auto const capacity = state.capacity_;
  auto       position = std::ranges::find(arena_order_, arena);
  OULY_ASSERT(position != arena_order_.end());
  arena_order_.erase(position);
  state = {};
  free_arenas_.push_back(arena);
  return capacity;
}

void gpu_allocator::set_result_state(gpu_defrag_result& result) const
{
  result.evacuation_active_ = target_.has_value();
  result.completed_         = !target_;
}

auto gpu_allocator::allocate_move_id() -> gpu_move_id
{
  OULY_ASSERT(next_move_id_ != std::numeric_limits<uint32_t>::max());
  return gpu_move_id{next_move_id_++};
}

auto gpu_allocator::statistics() const noexcept -> gpu_defrag_statistics
{
  auto result = statistics_;
  if (target_)
  {
    result.moves_pending_ =
     static_cast<uint32_t>(std::ranges::count_if(target_->moves_,
                                                 [](planned_move const& move) -> bool
                                                 {
                                                   return move.state_ == move_state::copy_scheduled ||
                                                          move.state_ == move_state::references_switched;
                                                 }));
    result.active_evacuation_targets_ = 1;
  }
  return result;
}

// NOLINTNEXTLINE
void gpu_allocator::validate_integrity() const
{
  struct occupied_range
  {
    size_type offset_ = 0;
    size_type size_   = 0;
  };

  std::vector<std::vector<occupied_range>> occupied(arena_pool_.size());
  std::vector<uint32_t>                    allocations(arena_pool_.size(), 0);
  std::vector<uint32_t>                    reservations(arena_pool_.size(), 0);
  std::vector<uint32_t>                    retiring(arena_pool_.size(), 0);

  for (uint32_t id = 1; id < static_cast<uint32_t>(entry_live_.size()); ++id)
  {
    if (ouly::detail::vector_access(entry_live_, id))
    {
      auto const arena = ouly::detail::vector_access(entry_arenas_, id);
      occupied[arena].push_back({.offset_ = ouly::detail::vector_access(entry_offsets_, id),
                                 .size_   = ouly::detail::vector_access(entry_sizes_, id)});
      ++allocations[arena];
    }
  }

  if (target_)
  {
    for (auto const& move : target_->moves_)
    {
      if (move.state_ == move_state::reserved || move.state_ == move_state::copy_scheduled)
      {
        occupied[move.destination_arena_].push_back({.offset_ = move.destination_offset_, .size_ = move.size_});
        ++reservations[move.destination_arena_];
      }
      else if (move.state_ == move_state::references_switched)
      {
        occupied[move.source_arena_].push_back({.offset_ = move.source_offset_, .size_ = move.size_});
        ++retiring[move.source_arena_];
      }
    }
  }

  std::vector<bool> active(arena_pool_.size(), false);
  for (auto arena : arena_order_)
  {
    active[arena]                      = true;
    [[maybe_unused]] auto const& state = ouly::detail::vector_access(arena_pool_, arena);
    OULY_ASSERT(state.active_);
    OULY_ASSERT(state.allocations_ == allocations[arena]);
    OULY_ASSERT(state.reservations_ == reservations[arena]);
    OULY_ASSERT(state.retiring_ == retiring[arena]);

    validate_arena_layout(occupied[arena], state);
  }

  for (size_t arena = 1; arena < arena_pool_.size(); ++arena)
  {
    OULY_ASSERT(active[arena] == ouly::detail::vector_access(arena_pool_, arena).active_);
  }
}

} // namespace ouly
