
#include <acl/allocators/coalescing_arena_allocator.hpp>

namespace acl
{

std::pair<arena_id, allocation_id> coalescing_arena_allocator::add_arena(size_type size, bool empty)
{
  uint16_t arena_idx = static_cast<uint16_t>(arena_entries.push(detail::ca_arena()));
  auto&    arena_ref = arena_entries.entries_[arena_idx];
  arena_ref.size     = size;
  auto  block_id     = block_entries.push(detail::ca_block());
  auto& block_ref    = block_entries.entries_[block_id];
  block_ref.offset   = 0;
  block_ref.arena    = arena_idx;
  block_ref.size     = size;
  if (empty)
  {
    block_ref.is_free   = true;
    arena_ref.free_size = size;
    add_free_arena(block_id);
  }
  else
  {
    arena_ref.free_size = 0;
  }
  arena_ref.blocks.push_back(block_entries, block_id);
  arenas.push_back(arena_entries, arena_idx);
  return std::make_pair(arena_id{arena_idx}, allocation_id{block_id});
}

std::uint32_t coalescing_arena_allocator::commit(size_type size, size_type const* found) noexcept
{
  auto          free_idx  = std::distance((size_type const*)sizes.data(), found);
  std::uint32_t free_node = free_ordering[free_idx];
  auto&         blk       = block_entries.entries_[free_node];
  // Marker
  size_type     offset    = blk.offset;
  std::uint32_t arena_num = blk.arena;

  blk.is_free = false;

  auto remaining = *found - size;
  blk.size       = size;
  if (remaining > 0)
  {
    auto& list   = arena_entries.entries_[blk.arena].blocks;
    auto  arena  = blk.arena;
    auto  newblk = block_entries.push(
      detail::ca_block{.offset = blk.offset + size, .size = remaining, .order = {}, .arena = arena, .is_free = true});
    list.insert_after(block_entries, free_node, (uint32_t)newblk);
    // reinsert the left-over size in free list
    reinsert_left(free_idx, remaining, (uint32_t)newblk);
  }
  else
  {
    // delete the existing found index from free list
    sizes.erase(sizes.begin() + free_idx);
    free_ordering.erase(free_ordering.begin() + free_idx);
  }

  return free_node;
}

void coalescing_arena_allocator::reinsert_left(size_t of, size_type size, std::uint32_t node) noexcept
{
  if (!of)
  {
    free_ordering[of] = node;
    sizes[of]         = size;
  }
  else
  {
    auto it = mini2_it(sizes.data(), of, size);
    if (it != of)
    {
      std::size_t count = of - it;
      {
        auto src  = sizes.data() + it;
        auto dest = src + 1;
        std::memmove(dest, src, count * sizeof(size_type));
      }
      {
        auto src  = free_ordering.data() + it;
        auto dest = src + 1;
        std::memmove(dest, src, count * sizeof(std::uint32_t));
      }

      free_ordering[it] = node;
      sizes[it]         = size;
    }
    else
    {
      free_ordering[of] = node;
      sizes[of]         = size;
    }
  }
}

arena_id coalescing_arena_allocator::deallocate(allocation_id id)
{
  auto  node    = id.id;
  auto& blk     = block_entries.entries_[node];
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

  auto& arena     = arena_entries.entries_[blk.arena];
  auto& node_list = arena.blocks;

  // last index is not used
  arena.free_size += blk.size;
  auto size = blk.size;

  std::uint32_t left = 0, right = 0;
  std::uint32_t merges = 0;

  if (node != node_list.front() && block_entries.entries_[blk.order.prev].is_free)
  {
    left = blk.order.prev;
    merges |= f_left;
  }

  if (node != node_list.back() && block_entries.entries_[blk.order.next].is_free)
  {
    right = blk.order.next;
    merges |= f_right;
  }

  if (arena.free_size == arena.size)
  {
    // drop arena?
    if (left)
    {
      erase(left);
    }
    if (right)
    {
      erase(right);
    }

    std::uint16_t arena_idx = blk.arena;
    arena.size              = 0;
    arena.blocks.clear(block_entries);
    arenas.erase(arena_entries, arena_idx);
    return arena_id{.id = arena_idx};
  }

  switch (merges)
  {
  case merge_type::e_none:
    add_free(node);
    blk.is_free = true;
    break;
  case merge_type::e_left:
  {
    auto left_size = block_entries.entries_[left].size;
    grow_free_node(left, left_size + size);
    node_list.erase(block_entries, node);
  }
  break;
  case merge_type::e_right:
  {
    auto right_size = block_entries.entries_[right].size;
    replace_and_grow(right, node, right_size + size);
    node_list.erase(block_entries, right);
    blk.is_free = true;
  }
  break;
  case merge_type::e_left_and_right:
  {
    auto left_size  = block_entries.entries_[left].size;
    auto right_size = block_entries.entries_[right].size;
    erase(right);
    grow_free_node(left, left_size + right_size + size);
    node_list.erase2(block_entries, node);
  }
  break;
  }

  return arena_id();
}

void coalescing_arena_allocator::add_free(std::uint32_t blkid)
{
  block_entries.entries_[blkid].is_free = true;
  auto size                             = block_entries.entries_[blkid].size;
  auto it                               = mini2_it(sizes.data(), sizes.size(), size);
  free_ordering.emplace(free_ordering.begin() + it, blkid);
  sizes.emplace(sizes.begin() + it, size);
}

void coalescing_arena_allocator::grow_free_node(std::uint32_t block, size_type newsize)
{
  auto& blk = block_entries.entries_[block];

  auto it = mini2_it(sizes.data(), sizes.size(), blk.size);
  for (uint32_t end = static_cast<uint32_t>(free_ordering.size()); it != end && free_ordering[it] != block; ++it)
    ;

  ACL_ASSERT(it != static_cast<uint32_t>(free_ordering.size()));
  blk.size = newsize;
  reinsert_right(it, newsize, block);
}

void coalescing_arena_allocator::replace_and_grow(std::uint32_t block, std::uint32_t new_block, size_type new_size)
{
  size_type size                         = block_entries.entries_[block].size;
  block_entries.entries_[new_block].size = new_size;

  auto it = mini2_it(sizes.data(), sizes.size(), size);
  for (uint32_t end = static_cast<uint32_t>(free_ordering.size()); it != end && free_ordering[it] != block; ++it)
    ;

  ACL_ASSERT(it != static_cast<uint32_t>(free_ordering.size()));
  reinsert_right(it, new_size, new_block);
}

void coalescing_arena_allocator::erase(std::uint32_t block)
{
  auto it = mini2_it(sizes.data(), sizes.size(), block_entries.entries_[block].size);
  for (uint32_t end = static_cast<uint32_t>(free_ordering.size()); it != end && free_ordering[it] != block; ++it)
    ;
  ACL_ASSERT(it != static_cast<uint32_t>(free_ordering.size()));
  free_ordering.erase(it + free_ordering.begin());
  sizes.erase(it + sizes.begin());
}

void coalescing_arena_allocator::reinsert_right(size_t of, size_type size, std::uint32_t node)
{
  auto next = of + 1;
  if (next == sizes.size())
  {
    free_ordering[of] = node;
    sizes[of]         = size;
  }
  else
  {
    auto it = mini2_it(sizes.data() + next, sizes.size() - next, size);
    if (it)
    {
      std::size_t count = it;
      {
        auto dest = sizes.data() + of;
        auto src  = dest + 1;
        std::memmove(dest, src, count * sizeof(size_type));
        auto ptr = (dest + count);
        *ptr     = size;
      }

      {
        auto dest = free_ordering.data() + of;
        auto src  = dest + 1;
        std::memmove(dest, src, count * sizeof(uint32_t));
        auto ptr = (dest + count);
        *ptr     = node;
      }
    }
    else
    {
      free_ordering[of] = node;
      sizes[of]         = size;
    }
  }
}

} // namespace acl
