
#include <acl/allocators/coalescing_arena_allocator.hpp>

namespace acl
{

auto coalescing_arena_allocator::add_arena(size_type size, bool empty) -> std::pair<arena_id, allocation_id>
{
  uint16_t arena_idx = static_cast<uint16_t>(arena_entries_.push(acl::detail::ca_arena()));
  auto&    arena_ref = arena_entries_.entries_[arena_idx];
  arena_ref.size_    = size;
  auto block_id      = block_entries_.push(0, size, arena_idx, empty);
  if (empty)
  {
    arena_ref.free_size_ = size;
    add_free_arena(block_id);
  }
  else
  {
    arena_ref.free_size_ = 0;
  }
  arena_ref.blocks_.push_back(block_entries_, block_id);
  arenas_.push_back(arena_entries_, arena_idx);
  return std::make_pair(arena_id{arena_idx}, allocation_id{block_id});
}

auto coalescing_arena_allocator::commit(size_type size, size_type const* found) noexcept -> std::uint32_t
{
  auto          free_idx  = std::distance((size_type const*)sizes_.data(), found);
  std::uint32_t free_node = free_ordering_[free_idx];

  // Marker
  block_entries_.free_marker_[free_node] = false;

  auto  arena_id                   = block_entries_.arenas_[free_node];
  auto& arena                      = arena_entries_.entries_[arena_id];
  auto  remaining                  = *found - size;
  block_entries_.sizes_[free_node] = size;
  arena.free_size_ -= size;
  if (remaining > 0)
  {
    auto& list     = arena.blocks_;
    auto  new_node = block_entries_.push(block_entries_.offsets_[free_node] + size, remaining, arena_id, true);

    list.insert_after(block_entries_, free_node, new_node);
    // reinsert the left-over size in free list
    reinsert_left(free_idx, remaining, new_node);
  }
  else
  {
    // delete the existing found index from free list
    sizes_.erase(sizes_.begin() + free_idx);
    free_ordering_.erase(free_ordering_.begin() + free_idx);
  }

  return free_node;
}

void coalescing_arena_allocator::reinsert_left(size_t of, size_type size, std::uint32_t node) noexcept
{
  if (of == 0U)
  {
    free_ordering_[of] = node;
    sizes_[of]         = size;
  }
  else
  {
    auto it = mini2_it(sizes_.data(), of, size);
    if (it != of)
    {
      std::size_t count = of - it;
      {
        auto* src  = sizes_.data() + it;
        auto* dest = src + 1;
        std::memmove(dest, src, count * sizeof(size_type));
      }
      {
        auto* src  = free_ordering_.data() + it;
        auto* dest = src + 1;
        std::memmove(dest, src, count * sizeof(std::uint32_t));
      }

      free_ordering_[it] = node;
      sizes_[it]         = size;
    }
    else
    {
      free_ordering_[of] = node;
      sizes_[of]         = size;
    }
  }
}

auto coalescing_arena_allocator::deallocate(allocation_id id) -> arena_id
{
  auto const node = id.id_;
  auto const size = block_entries_.sizes_[node];
  // NOLINTNEXTLINE
  [[maybe_unused]] auto measure = statistics::report_deallocate(size);

  enum : std::uint8_t
  {
    f_left  = 1 << 0,
    f_right = 1 << 1,
  };

  enum merge_type : std::uint8_t
  {
    e_none,
    e_left,
    e_right,
    e_left_and_right
  };

  auto& arena     = arena_entries_.entries_[block_entries_.arenas_[node]];
  auto& node_list = arena.blocks_;

  // last index is not used
  arena.free_size_ += size;
  assert(arena.free_size_ <= arena.size_);
  std::uint32_t left   = 0;
  std::uint32_t right  = 0;
  std::uint32_t merges = 0;
  auto const    order  = block_entries_.ordering_[node];
  if (node != node_list.front() && block_entries_.free_marker_[order.prev_])
  {
    left = order.prev_;
    merges |= f_left;
  }

  if (node != node_list.back() && block_entries_.free_marker_[order.next_])
  {
    right = order.next_;
    merges |= f_right;
  }

  if (arena.free_size_ == arena.size_)
  {
    // drop arena?
    if (left != 0U)
    {
      erase(left);
    }
    if (right != 0U)
    {
      erase(right);
    }

    std::uint16_t arena_idx = block_entries_.arenas_[node];
    arena.size_             = 0;
    arena.blocks_.clear(block_entries_);
    arenas_.erase(arena_entries_, arena_idx);
    return arena_id{.id_ = arena_idx};
  }

  switch (merges)
  {
  case merge_type::e_none:
    add_free(node);
    block_entries_.free_marker_[node] = true;
    break;
  case merge_type::e_left:
  {
    auto left_size = block_entries_.sizes_[left];
    grow_free_node(left, left_size + size);
    node_list.erase(block_entries_, node);
  }
  break;
  case merge_type::e_right:
  {
    auto right_size = block_entries_.sizes_[right];
    replace_and_grow(right, node, right_size + size);
    node_list.erase(block_entries_, right);
    block_entries_.free_marker_[node] = true;
  }
  break;
  case merge_type::e_left_and_right:
  {
    auto left_size  = block_entries_.sizes_[left];
    auto right_size = block_entries_.sizes_[right];
    erase(right);
    grow_free_node(left, left_size + right_size + size);
    node_list.erase2(block_entries_, node);
  }
  break;
  default:
    break;
  }

  return {};
}

void coalescing_arena_allocator::add_free(std::uint32_t node)
{
  block_entries_.free_marker_[node] = true;
  auto size                         = block_entries_.sizes_[node];
  auto it                           = mini2_it(sizes_.data(), sizes_.size(), size);
  free_ordering_.emplace(free_ordering_.begin() + it, node);
  sizes_.emplace(sizes_.begin() + it, size);
}

void coalescing_arena_allocator::grow_free_node(std::uint32_t block, size_type newsize)
{

  auto it = mini2_it(sizes_.data(), sizes_.size(), block_entries_.sizes_[block]);
  for (auto end = static_cast<uint32_t>(free_ordering_.size()); it != end && free_ordering_[it] != block; ++it)
  {
    ;
  }

  assert(it != static_cast<uint32_t>(free_ordering_.size()));
  block_entries_.sizes_[block] = newsize;
  reinsert_right(it, newsize, block);
}

void coalescing_arena_allocator::replace_and_grow(std::uint32_t right, std::uint32_t node, size_type new_size)
{
  size_type size              = block_entries_.sizes_[right];
  block_entries_.sizes_[node] = new_size;

  auto it = mini2_it(sizes_.data(), sizes_.size(), size);
  for (auto end = static_cast<uint32_t>(free_ordering_.size()); it != end && free_ordering_[it] != right; ++it)
  {
    ;
  }

  assert(it != static_cast<uint32_t>(free_ordering_.size()));
  reinsert_right(it, new_size, node);
}

void coalescing_arena_allocator::erase(std::uint32_t node)
{
  auto it = mini2_it(sizes_.data(), sizes_.size(), block_entries_.sizes_[node]);
  for (auto end = static_cast<uint32_t>(free_ordering_.size()); it != end && free_ordering_[it] != node; ++it)
  {
    ;
  }
  assert(it != static_cast<uint32_t>(free_ordering_.size()));
  free_ordering_.erase(it + free_ordering_.begin());
  sizes_.erase(it + sizes_.begin());
}

void coalescing_arena_allocator::reinsert_right(size_t of, size_type size, std::uint32_t node)
{
  auto next = of + 1;
  if (next == sizes_.size())
  {
    free_ordering_[of] = node;
    sizes_[of]         = size;
  }
  else
  {
    auto it = mini2_it(sizes_.data() + next, sizes_.size() - next, size);
    if (it != 0)
    {
      std::size_t count = it;
      {
        auto* dest = sizes_.data() + of;
        auto* src  = dest + 1;
        std::memmove(dest, src, count * sizeof(size_type));
        auto* ptr = (dest + count);
        *ptr      = size;
      }

      {
        auto* dest = free_ordering_.data() + of;
        auto* src  = dest + 1;
        std::memmove(dest, src, count * sizeof(uint32_t));
        auto* ptr = (dest + count);
        *ptr      = node;
      }
    }
    else
    {
      free_ordering_[of] = node;
      sizes_[of]         = size;
    }
  }
}

void coalescing_arena_allocator::validate_integrity() const
{
  uint32_t counted_free_nodes = 0;
  for (auto arena_it = arenas_.begin(arena_entries_), arena_end_it = arenas_.end(arena_entries_);
       arena_it != arena_end_it; ++arena_it)
  {
    const auto& arena           = *arena_it;
    bool        arena_allocated = false;

    for (auto blk_it = arena.blocks_.begin(block_entries_), blk_end_it = arena.blocks_.end(block_entries_);
         blk_it != blk_end_it; ++blk_it)
    {
      auto blk = *blk_it;
      if (block_entries_.free_marker_[blk])
      {
        counted_free_nodes++;
      }
    }
  }

  assert(counted_free_nodes == total_free_nodes());

  for (auto arena_it = arenas_.begin(arena_entries_), arena_end_it = arenas_.end(arena_entries_);
       arena_it != arena_end_it; ++arena_it)
  {
    const auto& arena           = *arena_it;
    bool        arena_allocated = false;
    size_type   expected_offset = 0;

    for (auto blk_it = arena.blocks_.begin(block_entries_), blk_end_it = arena.blocks_.end(block_entries_);
         blk_it != blk_end_it; ++blk_it)
    {
      auto blk = *blk_it;
      assert(block_entries_.offsets_[blk] == expected_offset);
      expected_offset += block_entries_.sizes_[blk];
    }
  }

  assert(free_ordering_.size() == sizes_.size());
  for (size_t i = 1; i < sizes_.size(); ++i)
  {
    assert(sizes_[i - 1] <= sizes_[i]);
  }

  size_type sz = 0;
  // NOLINTNEXTLINE
  for (size_t free_idx = 0; free_idx < free_ordering_.size(); ++free_idx)
  {
    auto fn = free_ordering_[free_idx];
    assert(sz <= block_entries_.sizes_[fn]);
    assert(block_entries_.sizes_[fn] == sizes_[free_idx]);
    // NOLINTNEXTLINE
    sz = block_entries_.sizes_[fn];
  }
}
} // namespace acl
