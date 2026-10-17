// SPDX-License-Identifier: MIT

#include "ouly/allocators/coalescing_arena_allocator.hpp"

#include <cstddef>
#include <utility>

namespace ouly
{

coalescing_arena_allocator::coalescing_arena_allocator() = default;

coalescing_arena_allocator::coalescing_arena_allocator(size_type arena_size) : arena_size_(arena_size) {}

void coalescing_arena_allocator::set_arena_size(size_type size) noexcept
{
  arena_size_ = std::max(arena_size_, size);
}

auto coalescing_arena_allocator::get_arena_size() const noexcept -> size_type
{
  return arena_size_;
}

auto coalescing_arena_allocator::get_offset(allocation_id id) const noexcept -> size_type
{
  OULY_ASSERT(ouly::detail::vector_access(blocks_, id.get()).kind_ == block_kind::allocation);
  return ouly::detail::vector_access(blocks_, id.get()).offset_;
}

auto coalescing_arena_allocator::get_size(allocation_id id) const noexcept -> size_type
{
  OULY_ASSERT(ouly::detail::vector_access(blocks_, id.get()).kind_ == block_kind::allocation);
  return ouly::detail::vector_access(blocks_, id.get()).size_;
}

auto coalescing_arena_allocator::get_alignment(allocation_id id) const noexcept -> size_type
{
  OULY_ASSERT(ouly::detail::vector_access(blocks_, id.get()).kind_ == block_kind::allocation);
  return size_type{1} << ouly::detail::vector_access(blocks_, id.get()).alignment_;
}

auto coalescing_arena_allocator::get_arena(allocation_id id) const noexcept -> arena_id
{
  OULY_ASSERT(ouly::detail::vector_access(blocks_, id.get()).kind_ == block_kind::allocation);
  return arena_id{ouly::detail::vector_access(blocks_, id.get()).arena_};
}

auto coalescing_arena_allocator::get_memory_class(allocation_id id) const noexcept -> uint32_t
{
  OULY_ASSERT(ouly::detail::vector_access(blocks_, id.get()).kind_ == block_kind::allocation);
  return ouly::detail::vector_access(arena_pool_, ouly::detail::vector_access(blocks_, id.get()).arena_).memory_class_;
}

auto coalescing_arena_allocator::get_resource_class(allocation_id id) const noexcept -> uint32_t
{
  OULY_ASSERT(ouly::detail::vector_access(blocks_, id.get()).kind_ == block_kind::allocation);
  return ouly::detail::vector_access(blocks_, id.get()).resource_class_;
}

auto coalescing_arena_allocator::is_movable(allocation_id id) const noexcept -> bool
{
  OULY_ASSERT(ouly::detail::vector_access(blocks_, id.get()).kind_ == block_kind::allocation);
  return ouly::detail::vector_access(blocks_, id.get()).movable_;
}

auto coalescing_arena_allocator::is_relocating(allocation_id id) const noexcept -> bool
{
  OULY_ASSERT(ouly::detail::vector_access(blocks_, id.get()).kind_ == block_kind::allocation);
  return ouly::detail::vector_access(blocks_, id.get()).move_ != null_index;
}

void coalescing_arena_allocator::set_movable(allocation_id id, bool movable)
{
  auto& block = ouly::detail::vector_access(blocks_, id.get());
  auto& state = ouly::detail::vector_access(arena_pool_, block.arena_);
  OULY_ASSERT(block.kind_ == block_kind::allocation);
  OULY_ASSERT(!is_relocating(id));
  OULY_ASSERT(!state.dedicated_ || !movable);
  if (block.movable_ != movable)
  {
    block.movable_ = movable;
    state.pinned_  = movable ? state.pinned_ - 1 : state.pinned_ + 1;
  }
}

auto coalescing_arena_allocator::try_allocate(size_type size, ca_allocation_options const& options) -> ca_allocation
{
  auto const bins = find_bins(options.memory_class_);
  if (bins == null_index)
  {
    return {};
  }
  auto const fit = ouly::detail::find_gap(blocks_, ouly::detail::vector_access(bins_, bins), size, options.alignment_);
  if (fit.block_ == null_index)
  {
    return {};
  }
  return place_allocation(fit.block_, static_cast<size_type>(fit.offset_), size, options);
}

auto coalescing_arena_allocator::allocate_in_new_arena(uint16_t arena, size_type size,
                                                       ca_allocation_options const& options) -> ca_allocation
{
  // arenas start at offset zero, which satisfies every alignment
  return place_allocation(ouly::detail::vector_access(arena_pool_, arena).head_, 0, size, options);
}

auto coalescing_arena_allocator::create_arena(size_type capacity, uint32_t memory_class, bool dedicated) -> arena_id
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

  auto const head  = make_block();
  auto&      block = ouly::detail::vector_access(blocks_, head);
  block.gap_       = capacity;
  block.arena_     = arena;
  block.kind_      = block_kind::head;

  auto& state         = ouly::detail::vector_access(arena_pool_, arena);
  state.capacity_     = capacity;
  state.free_         = capacity;
  state.head_         = head;
  state.memory_class_ = memory_class;
  state.bins_         = bins_for_class(memory_class);
  state.active_       = true;
  state.dedicated_    = dedicated;
  if (auto* bins = arena_bins(arena))
  {
    bins->push(blocks_, head, capacity);
  }
  arena_order_.push_back(arena);
  return arena_id{arena};
}

auto coalescing_arena_allocator::place_allocation(uint32_t pred, size_type offset, size_type size,
                                                  ca_allocation_options const& options) -> ca_allocation
{
  auto const index = make_block();
  auto const arena = ouly::detail::vector_access(blocks_, pred).arena_;
  ouly::detail::split_gap(blocks_, arena_bins(arena), pred, index, offset, size);

  auto& block           = ouly::detail::vector_access(blocks_, index);
  block.arena_          = arena;
  block.kind_           = block_kind::allocation;
  block.alignment_      = static_cast<uint8_t>(std::countr_zero(options.alignment_));
  block.resource_class_ = options.resource_class_;
  block.movable_        = options.movable_;

  auto& state = ouly::detail::vector_access(arena_pool_, arena);
  state.free_ -= size;
  state.live_ += size;
  ++state.allocations_;
  state.pinned_ += options.movable_ ? 0U : 1U;
  return ca_allocation{.offset_ = offset, .id_ = allocation_id{index}, .arena_ = arena_id{arena}};
}

auto coalescing_arena_allocator::release_allocation(allocation_id id) -> uint16_t
{
  auto const& block = ouly::detail::vector_access(blocks_, id.get());
  OULY_ASSERT(block.kind_ == block_kind::allocation);
  auto const arena = block.arena_;
  auto&      state = ouly::detail::vector_access(arena_pool_, arena);

  OULY_ASSERT(state.allocations_ != 0);
  --state.allocations_;
  state.free_ += block.size_;
  state.live_ -= block.size_;
  state.pinned_ -= block.movable_ ? 0U : 1U;

  ouly::detail::merge_into_prev(blocks_, arena_bins(arena), id.get());
  free_block(id.get());
  return arena;
}

auto coalescing_arena_allocator::prepare_deallocate(allocation_id id) -> bool
{
  auto& block = ouly::detail::vector_access(blocks_, id.get());
  OULY_ASSERT(block.kind_ == block_kind::allocation);
  if (!target_ || block.move_ == null_index)
  {
    return true;
  }

  auto& move = ouly::detail::vector_access(target_->moves_, block.move_);
  if (move.state_ == move_state::copy_scheduled)
  {
    return false;
  }
  if (move.state_ == move_state::reserved)
  {
    cancel_move(move);
  }
  else
  {
    // references are switched; the move only retires the old source from here on, which is a
    // separate block, so the allocation can go
    block.move_ = null_index;
  }
  return true;
}

auto coalescing_arena_allocator::find_bins(uint32_t memory_class) const -> uint32_t
{
  for (uint32_t index = 0; index < static_cast<uint32_t>(bin_classes_.size()); ++index)
  {
    if (ouly::detail::vector_access(bin_classes_, index) == memory_class)
    {
      return index;
    }
  }
  return null_index;
}

auto coalescing_arena_allocator::bins_for_class(uint32_t memory_class) -> uint32_t
{
  auto index = find_bins(memory_class);
  if (index == null_index)
  {
    index = static_cast<uint32_t>(bins_.size());
    bins_.emplace_back();
    bin_classes_.push_back(memory_class);
  }
  return index;
}

auto coalescing_arena_allocator::arena_bins(uint16_t arena) -> bins_type*
{
  auto const& state = ouly::detail::vector_access(arena_pool_, arena);
  if (state.role_ != arena_role::normal || state.dedicated_)
  {
    return nullptr;
  }
  return &ouly::detail::vector_access(bins_, state.bins_);
}

void coalescing_arena_allocator::open_arena(uint16_t arena)
{
  ouly::detail::vector_access(arena_pool_, arena).role_ = arena_role::normal;
  auto* bins                                            = arena_bins(arena);
  if (bins == nullptr)
  {
    return;
  }
  for (auto index = ouly::detail::vector_access(arena_pool_, arena).head_; index != null_index;
       index      = ouly::detail::vector_access(blocks_, index).next_)
  {
    auto const& block = ouly::detail::vector_access(blocks_, index);
    if (block.gap_ != 0 && block.bin_ == bins_type::unbinned)
    {
      bins->push(blocks_, index, block.gap_);
    }
  }
}

void coalescing_arena_allocator::close_arena(uint16_t arena)
{
  if (auto* bins = arena_bins(arena))
  {
    for (auto index = ouly::detail::vector_access(arena_pool_, arena).head_; index != null_index;
         index      = ouly::detail::vector_access(blocks_, index).next_)
    {
      if (ouly::detail::vector_access(blocks_, index).bin_ != bins_type::unbinned)
      {
        bins->erase(blocks_, index);
      }
    }
  }
  ouly::detail::vector_access(arena_pool_, arena).role_ = arena_role::evacuation_source;
}

auto coalescing_arena_allocator::make_block() -> uint32_t
{
  if (free_block_ != null_index)
  {
    auto const index = free_block_;
    auto&      block = ouly::detail::vector_access(blocks_, index);
    free_block_      = block.next_;
    block            = {};
    return index;
  }
  OULY_ASSERT(blocks_.size() < null_index);
  blocks_.emplace_back();
  return static_cast<uint32_t>(blocks_.size() - 1);
}

void coalescing_arena_allocator::free_block(uint32_t index)
{
  auto& block = ouly::detail::vector_access(blocks_, index);
  block       = {};
  block.next_ = free_block_;
  free_block_ = index;
}

void coalescing_arena_allocator::swap_positions(uint32_t lhs, uint32_t rhs)
{
  auto* lhs_bins = arena_bins(ouly::detail::vector_access(blocks_, lhs).arena_);
  auto* rhs_bins = arena_bins(ouly::detail::vector_access(blocks_, rhs).arena_);
  OULY_ASSERT(ouly::detail::vector_access(blocks_, lhs).arena_ != ouly::detail::vector_access(blocks_, rhs).arena_);

  if (ouly::detail::vector_access(blocks_, lhs).bin_ != bins_type::unbinned)
  {
    lhs_bins->erase(blocks_, lhs);
  }
  if (ouly::detail::vector_access(blocks_, rhs).bin_ != bins_type::unbinned)
  {
    rhs_bins->erase(blocks_, rhs);
  }

  auto& left  = ouly::detail::vector_access(blocks_, lhs);
  auto& right = ouly::detail::vector_access(blocks_, rhs);
  std::swap(left.offset_, right.offset_);
  std::swap(left.size_, right.size_);
  std::swap(left.gap_, right.gap_);
  std::swap(left.prev_, right.prev_);
  std::swap(left.next_, right.next_);
  std::swap(left.arena_, right.arena_);

  // the blocks live in different arenas, so neither is a neighbour of the other
  for (auto const index : {lhs, rhs})
  {
    auto const& block                                       = ouly::detail::vector_access(blocks_, index);
    ouly::detail::vector_access(blocks_, block.prev_).next_ = index;
    if (block.next_ != null_index)
    {
      ouly::detail::vector_access(blocks_, block.next_).prev_ = index;
    }
  }

  if (rhs_bins != nullptr && left.gap_ != 0)
  {
    rhs_bins->push(blocks_, lhs, left.gap_);
  }
  if (lhs_bins != nullptr && right.gap_ != 0)
  {
    lhs_bins->push(blocks_, rhs, right.gap_);
  }
}

auto coalescing_arena_allocator::can_evacuate(uint16_t arena) const -> bool
{
  auto const& state = ouly::detail::vector_access(arena_pool_, arena);
  return state.role_ == arena_role::normal && !state.dedicated_ && state.allocations_ != 0 && state.free_ != 0 &&
         state.reservations_ == 0 && state.retiring_ == 0 && state.pinned_ == 0;
}

auto coalescing_arena_allocator::recovers_more(evacuation_candidate const& lhs, evacuation_candidate const& rhs) -> bool
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

void coalescing_arena_allocator::begin_evacuation(uint32_t max_blocks_scanned, scratch_allocator_ref scratch)
{
  ouly::scratch_vector<evacuation_candidate> candidates{scratch, arena_order_.size()};
  for (auto arena : arena_order_)
  {
    if (can_evacuate(arena))
    {
      auto const& state = ouly::detail::vector_access(arena_pool_, arena);
      candidates.push_back(
       {.arena_ = arena, .capacity_ = state.capacity_, .live_ = state.live_, .count_ = state.allocations_});
    }
  }

  std::ranges::sort(candidates, &coalescing_arena_allocator::recovers_more);

  uint32_t scanned = 0;
  for (auto const& candidate : candidates)
  {
    if (scanned >= max_blocks_scanned)
    {
      break;
    }
    ++scanned;
    if (plan_evacuation(candidate.arena_))
    {
      return;
    }
  }
}

auto coalescing_arena_allocator::plan_evacuation(uint16_t source) -> bool
{
  auto const& state = ouly::detail::vector_access(arena_pool_, source);
  auto const  bins  = state.bins_;

  evacuation_target plan{.source_arena_ = source, .moves_ = {}};
  plan.moves_.reserve(state.allocations_);
  for (auto index = ouly::detail::vector_access(blocks_, state.head_).next_; index != null_index;
       index      = ouly::detail::vector_access(blocks_, index).next_)
  {
    auto const& block = ouly::detail::vector_access(blocks_, index);
    OULY_ASSERT(block.kind_ == block_kind::allocation);
    plan.moves_.push_back({.move_          = {},
                           .allocation_    = allocation_id{index},
                           .source_arena_  = source,
                           .source_offset_ = block.offset_,
                           .size_          = block.size_});
  }

  std::ranges::sort(plan.moves_,
                    [this](planned_move const& lhs, planned_move const& rhs) -> bool
                    {
                      auto const lhs_alignment = ouly::detail::vector_access(blocks_, lhs.allocation_.get()).alignment_;
                      auto const rhs_alignment = ouly::detail::vector_access(blocks_, rhs.allocation_.get()).alignment_;
                      return lhs_alignment != rhs_alignment ? lhs_alignment > rhs_alignment : lhs.size_ > rhs.size_;
                    });

  // the source's own gaps must not receive its allocations
  close_arena(source);

  size_t reserved = 0;
  for (auto& move : plan.moves_)
  {
    auto const alignment  = size_type{1} << ouly::detail::vector_access(blocks_, move.allocation_.get()).alignment_;
    auto&      class_bins = ouly::detail::vector_access(bins_, bins);
    auto const fit        = ouly::detail::find_gap(blocks_, class_bins, move.size_, alignment);
    if (fit.block_ == null_index)
    {
      break;
    }

    auto const reservation = make_block();
    auto const destination = ouly::detail::vector_access(blocks_, fit.block_).arena_;
    ouly::detail::split_gap(blocks_, &class_bins, fit.block_, reservation, fit.offset_, move.size_);
    auto& block  = ouly::detail::vector_access(blocks_, reservation);
    block.arena_ = destination;
    block.kind_  = block_kind::reservation;

    auto& destination_state = ouly::detail::vector_access(arena_pool_, destination);
    destination_state.free_ -= move.size_;
    ++destination_state.reservations_;

    move.reservation_        = reservation;
    move.destination_arena_  = destination;
    move.destination_offset_ = static_cast<size_type>(fit.offset_);
    ++reserved;
  }

  if (reserved != plan.moves_.size())
  {
    for (size_t index = 0; index < reserved; ++index)
    {
      release_reservation(ouly::detail::vector_access(plan.moves_, index));
    }
    open_arena(source);
    return false;
  }

  for (uint32_t index = 0; index < static_cast<uint32_t>(plan.moves_.size()); ++index)
  {
    auto& move = ouly::detail::vector_access(plan.moves_, index);
    move.move_ = allocate_move_id();
    ouly::detail::vector_access(blocks_, move.allocation_.get()).move_ = index;
  }
  plan.open_moves_     = static_cast<uint32_t>(plan.moves_.size());
  plan.reserved_moves_ = plan.open_moves_;
  target_              = std::move(plan);
  return true;
}

void coalescing_arena_allocator::release_reservation(planned_move const& move)
{
  auto& state = ouly::detail::vector_access(arena_pool_, move.destination_arena_);
  OULY_ASSERT(state.reservations_ != 0);
  --state.reservations_;
  state.free_ += move.size_;
  ouly::detail::merge_into_prev(blocks_, arena_bins(move.destination_arena_), move.reservation_);
  free_block(move.reservation_);
  if (can_release_arena(move.destination_arena_))
  {
    emptied_.push_back(move.destination_arena_);
  }
}

void coalescing_arena_allocator::close_move(planned_move& move, move_state state)
{
  OULY_ASSERT(target_ && target_->open_moves_ != 0);
  move.state_ = state;
  --target_->open_moves_;

  // the allocation may be gone, and its id reused, if it was released after its references switched
  auto const index = static_cast<uint32_t>(&move - target_->moves_.data());
  auto&      block = ouly::detail::vector_access(blocks_, move.allocation_.get());
  if (block.kind_ == block_kind::allocation && block.move_ == index)
  {
    block.move_ = null_index;
  }
}

void coalescing_arena_allocator::cancel_move(planned_move& move)
{
  if (move.state_ == move_state::reserved)
  {
    --target_->reserved_moves_;
  }
  if (move.state_ == move_state::reserved || move.state_ == move_state::copy_scheduled)
  {
    release_reservation(move);
  }
  close_move(move, move_state::cancelled);
}

void coalescing_arena_allocator::abort_evacuation()
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

auto coalescing_arena_allocator::find_schedulable_move(size_type remaining_bytes) -> planned_move*
{
  if (!target_ || target_->reserved_moves_ == 0)
  {
    return nullptr;
  }
  auto& moves = target_->moves_;
  auto& next  = target_->next_reserved_;
  while (next < moves.size() && ouly::detail::vector_access(moves, next).state_ != move_state::reserved)
  {
    ++next;
  }
  for (auto index = next; index < moves.size(); ++index)
  {
    auto& move = ouly::detail::vector_access(moves, index);
    if (move.state_ == move_state::reserved && move.size_ <= remaining_bytes)
    {
      return &move;
    }
  }
  return nullptr;
}

auto coalescing_arena_allocator::has_reserved_moves() const -> bool
{
  return target_ && target_->reserved_moves_ != 0;
}

auto coalescing_arena_allocator::target_finished() const -> bool
{
  return target_ && target_->open_moves_ == 0;
}

auto coalescing_arena_allocator::make_relocation(planned_move const& move) const -> ca_relocation
{
  return {.move_               = move.move_,
          .allocation_         = move.allocation_,
          .source_arena_       = arena_id{move.source_arena_},
          .destination_arena_  = arena_id{move.destination_arena_},
          .source_offset_      = move.source_offset_,
          .destination_offset_ = move.destination_offset_,
          .size_               = move.size_,
          .memory_class_       = ouly::detail::vector_access(arena_pool_, move.source_arena_).memory_class_,
          .resource_class_     = ouly::detail::vector_access(blocks_, move.allocation_.get()).resource_class_};
}

void coalescing_arena_allocator::switch_allocation(planned_move& move)
{
  auto& source      = ouly::detail::vector_access(arena_pool_, move.source_arena_);
  auto& destination = ouly::detail::vector_access(arena_pool_, move.destination_arena_);
  OULY_ASSERT(source.allocations_ != 0);
  OULY_ASSERT(destination.reservations_ != 0);
  --source.allocations_;
  ++source.retiring_;
  source.live_ -= move.size_;
  --destination.reservations_;
  ++destination.allocations_;
  destination.live_ += move.size_;

  // the allocation id takes the reserved place, and the reservation's block stays behind to hold
  // the source range until the old users are done with it
  swap_positions(move.allocation_.get(), move.reservation_);
  ouly::detail::vector_access(blocks_, move.reservation_).kind_ = block_kind::retiring;
}

void coalescing_arena_allocator::retire_source(planned_move& move)
{
  auto& source = ouly::detail::vector_access(arena_pool_, move.source_arena_);
  OULY_ASSERT(source.retiring_ != 0);
  --source.retiring_;
  source.free_ += move.size_;
  ouly::detail::merge_into_prev(blocks_, arena_bins(move.source_arena_), move.reservation_);
  free_block(move.reservation_);
  close_move(move, move_state::source_retired);
}

auto coalescing_arena_allocator::can_release_arena(uint16_t arena) const -> bool
{
  auto const& state = ouly::detail::vector_access(arena_pool_, arena);
  return state.active_ && state.role_ == arena_role::normal && state.allocations_ == 0 && state.reservations_ == 0 &&
         state.retiring_ == 0 && state.free_ == state.capacity_;
}

auto coalescing_arena_allocator::drop_arena(uint16_t arena) -> size_type
{
  OULY_ASSERT(can_release_arena(arena));
  auto const head = ouly::detail::vector_access(arena_pool_, arena).head_;
  if (ouly::detail::vector_access(blocks_, head).bin_ != bins_type::unbinned)
  {
    arena_bins(arena)->erase(blocks_, head);
  }
  free_block(head);

  auto& state    = ouly::detail::vector_access(arena_pool_, arena);
  auto  capacity = state.capacity_;
  auto  position = std::ranges::find(arena_order_, arena);
  OULY_ASSERT(position != arena_order_.end());
  arena_order_.erase(position);
  state = {};
  free_arenas_.push_back(arena);
  return capacity;
}

void coalescing_arena_allocator::set_result_state(ca_defrag_result& result) const
{
  result.evacuation_active_ = target_.has_value();
  result.completed_         = !target_;
}

auto coalescing_arena_allocator::allocate_move_id() -> ca_move_id
{
  OULY_ASSERT(next_move_id_ != std::numeric_limits<uint32_t>::max());
  return ca_move_id{next_move_id_++};
}

auto coalescing_arena_allocator::statistics() const noexcept -> ca_defrag_statistics
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
void coalescing_arena_allocator::validate_integrity() const
{
  size_t reachable = 0;
  for (auto arena : arena_order_)
  {
    auto const& state = ouly::detail::vector_access(arena_pool_, arena);
    OULY_ASSERT(state.active_);
    // empty arenas go back to the manager as soon as they are empty
    OULY_ASSERT(!can_release_arena(arena));
    bool const open = state.role_ == arena_role::normal && !state.dedicated_;

    [[maybe_unused]] uint32_t  allocations  = 0;
    [[maybe_unused]] uint32_t  reservations = 0;
    [[maybe_unused]] uint32_t  retiring     = 0;
    [[maybe_unused]] uint32_t  pinned       = 0;
    [[maybe_unused]] size_type live         = 0;
    [[maybe_unused]] size_type free         = 0;
    size_type                  cursor       = 0;
    uint32_t                   prev         = null_index;

    for (auto index = state.head_; index != null_index; index = ouly::detail::vector_access(blocks_, index).next_)
    {
      auto const& block = ouly::detail::vector_access(blocks_, index);
      ++reachable;
      OULY_ASSERT(block.arena_ == arena);
      OULY_ASSERT(block.prev_ == prev);
      OULY_ASSERT(block.offset_ == cursor);
      OULY_ASSERT((index == state.head_) == (block.kind_ == block_kind::head));
      OULY_ASSERT(block.kind_ == block_kind::head || block.size_ != 0);
      OULY_ASSERT((block.bin_ != bins_type::unbinned) == (open && block.gap_ != 0));
      OULY_ASSERT(block.bin_ == bins_type::unbinned || block.bin_ == bins_type::bin_down(block.gap_));

      switch (block.kind_)
      {
      case block_kind::allocation:
        ++allocations;
        live += block.size_;
        pinned += block.movable_ ? 0U : 1U;
        OULY_ASSERT(block.move_ == null_index ||
                    (target_ && ouly::detail::vector_access(target_->moves_, block.move_).allocation_.get() == index));
        break;
      case block_kind::reservation:
        ++reservations;
        break;
      case block_kind::retiring:
        ++retiring;
        break;
      case block_kind::head:
      case block_kind::unused:
      default:
        break;
      }

      free += block.gap_;
      cursor = block.offset_ + block.size_ + block.gap_;
      prev   = index;
    }

    OULY_ASSERT(cursor == state.capacity_);
    OULY_ASSERT(free == state.free_);
    OULY_ASSERT(live == state.live_);
    OULY_ASSERT(allocations == state.allocations_);
    OULY_ASSERT(reservations == state.reservations_);
    OULY_ASSERT(retiring == state.retiring_);
    OULY_ASSERT(pinned == state.pinned_);
  }

  [[maybe_unused]] size_t used = 0;
  for (auto const& block : blocks_)
  {
    used += block.kind_ != block_kind::unused ? 1U : 0U;
  }
  OULY_ASSERT(used == reachable);

  if (target_)
  {
    for (auto const& move : target_->moves_)
    {
      [[maybe_unused]] auto const& block = ouly::detail::vector_access(blocks_, move.reservation_);
      if (move.state_ == move_state::reserved || move.state_ == move_state::copy_scheduled)
      {
        OULY_ASSERT(block.kind_ == block_kind::reservation && block.arena_ == move.destination_arena_ &&
                    block.offset_ == move.destination_offset_);
      }
      else if (move.state_ == move_state::references_switched)
      {
        OULY_ASSERT(block.kind_ == block_kind::retiring && block.arena_ == move.source_arena_ &&
                    block.offset_ == move.source_offset_);
      }
    }
  }

  for (size_t arena = 1; arena < arena_pool_.size(); ++arena)
  {
    OULY_ASSERT(ouly::detail::vector_access(arena_pool_, arena).active_ ==
                (std::ranges::find(arena_order_, static_cast<uint16_t>(arena)) != arena_order_.end()));
  }
}

} // namespace ouly
