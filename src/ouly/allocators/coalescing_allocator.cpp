// SPDX-License-Identifier: MIT

#include "ouly/allocators/coalescing_allocator.hpp"

#include <bit>

namespace ouly
{

coalescing_allocator::coalescing_allocator() : coalescing_allocator(std::numeric_limits<size_type>::max()) {}

coalescing_allocator::coalescing_allocator(size_type capacity) : capacity_(capacity)
{
  blocks_.push_back({.offset_ = 0, .size_ = 0, .gap_ = capacity});
  if (capacity != 0)
  {
    bins_.push(blocks_, head_index, capacity);
  }
}

auto coalescing_allocator::allocate(size_type size, size_type alignment) -> allocation_id
{
  OULY_ASSERT(size != 0);
  OULY_ASSERT(std::has_single_bit(alignment));
  auto const fit = ouly::detail::find_gap(blocks_, bins_, size, alignment);
  if (fit.block_ == null_index)
  {
    return {};
  }
  auto const index = make_block();
  ouly::detail::split_gap(blocks_, &bins_, fit.block_, index, fit.offset_, size);
  ouly::detail::vector_access(blocks_, index).live_ = true;
  return allocation_id{index};
}

void coalescing_allocator::deallocate(allocation_id id)
{
  OULY_ASSERT(id.get() != head_index && ouly::detail::vector_access(blocks_, id.get()).live_);
  ouly::detail::merge_into_prev(blocks_, &bins_, id.get());
  auto& block = ouly::detail::vector_access(blocks_, id.get());
  block       = {};
  block.next_ = free_block_;
  free_block_ = id.get();
}

auto coalescing_allocator::make_block() -> uint32_t
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

void coalescing_allocator::rebuild_gaps()
{
  bins_ = {};
  for (auto index = head_index; index != null_index; index = ouly::detail::vector_access(blocks_, index).next_)
  {
    auto&      block = ouly::detail::vector_access(blocks_, index);
    auto const end = block.next_ == null_index ? capacity_ : ouly::detail::vector_access(blocks_, block.next_).offset_;
    block.gap_     = end - block.offset_ - block.size_;
    block.bin_     = bins_type::unbinned;
    if (block.gap_ != 0)
    {
      bins_.push(blocks_, index, block.gap_);
    }
  }
}

// NOLINTNEXTLINE
void coalescing_allocator::validate_integrity() const
{
  [[maybe_unused]] size_type cursor    = 0;
  [[maybe_unused]] uint32_t  prev      = null_index;
  [[maybe_unused]] uint32_t  reachable = 0;
  for (auto index = head_index; index != null_index; index = ouly::detail::vector_access(blocks_, index).next_)
  {
    auto const& block = ouly::detail::vector_access(blocks_, index);
    OULY_ASSERT(block.prev_ == prev);
    OULY_ASSERT(block.offset_ == cursor);
    OULY_ASSERT(index == head_index ? !block.live_ && block.size_ == 0 : block.live_ && block.size_ != 0);
    OULY_ASSERT((block.bin_ != bins_type::unbinned) == (block.gap_ != 0));
    OULY_ASSERT(block.bin_ == bins_type::unbinned || block.bin_ == bins_type::bin_down(block.gap_));
    cursor = block.offset_ + block.size_ + block.gap_;
    prev   = index;
    reachable += block.live_ ? 1U : 0U;
  }
  OULY_ASSERT(cursor == capacity_);

  [[maybe_unused]] uint32_t live = 0;
  for (auto const& block : blocks_)
  {
    live += block.live_ ? 1U : 0U;
  }
  OULY_ASSERT(live == reachable);
}

} // namespace ouly
