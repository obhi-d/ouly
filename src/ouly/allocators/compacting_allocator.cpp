// SPDX-License-Identifier: MIT

#include "ouly/utility/user_config.hpp"

#include "ouly/allocators/compacting_allocator.hpp"

namespace ouly
{

auto compacting_allocator::allocate(size_type size) -> allocation_id
{
  OULY_ASSERT(size != 0);
  for (uint32_t i = 0, end = static_cast<uint32_t>(free_offsets_.size()); i < end; ++i)
  {
    if (free_sizes_[i] < size)
    {
      continue;
    }
    auto const offset = free_offsets_[i];
    if (free_sizes_[i] == size)
    {
      free_offsets_.erase(free_offsets_.begin() + i);
      free_sizes_.erase(free_sizes_.begin() + i);
    }
    else
    {
      free_offsets_[i] += size;
      free_sizes_[i] -= size;
    }
    return allocation_id{push_entry(offset, size)};
  }
  return {};
}

void compacting_allocator::deallocate(allocation_id id)
{
  OULY_ASSERT(ouly::detail::vector_access(entry_live_, id.get()));
  auto const offset = ouly::detail::vector_access(entry_offsets_, id.get());
  auto const size   = ouly::detail::vector_access(entry_sizes_, id.get());
  free_entry(id.get());

  auto&      offsets = free_offsets_;
  auto&      sizes   = free_sizes_;
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
}

auto compacting_allocator::get_offset(allocation_id id) const noexcept -> size_type
{
  return ouly::detail::vector_access(entry_offsets_, id.get());
}

auto compacting_allocator::get_size(allocation_id id) const noexcept -> size_type
{
  return ouly::detail::vector_access(entry_sizes_, id.get());
}

void compacting_allocator::validate_integrity() const
{
  std::vector<uint32_t> allocs;
  for (uint32_t id = 1; id < static_cast<uint32_t>(entry_offsets_.size()); ++id)
  {
    if (ouly::detail::vector_access(entry_live_, id))
    {
      allocs.push_back(id);
    }
  }
  std::ranges::sort(allocs,
                    [this](uint32_t a, uint32_t b) -> bool
                    {
                      return ouly::detail::vector_access(entry_offsets_, a) <
                             ouly::detail::vector_access(entry_offsets_, b);
                    });

  // free blocks and allocations must exactly tile [0, max)
  constexpr auto space_end = std::numeric_limits<size_type>::max();
  size_type      pos       = 0;
  size_t         fi        = 0;
  size_t         ai        = 0;
  while (pos < space_end)
  {
    if (fi < free_offsets_.size() && free_offsets_[fi] == pos)
    {
      OULY_ASSERT(free_sizes_[fi] > 0);
      pos += free_sizes_[fi];
      ++fi;
      // adjacent free blocks must have been coalesced
      OULY_ASSERT(fi == free_offsets_.size() || free_offsets_[fi] > pos);
    }
    else if (ai < allocs.size() && ouly::detail::vector_access(entry_offsets_, allocs[ai]) == pos)
    {
      pos += ouly::detail::vector_access(entry_sizes_, allocs[ai]);
      ++ai;
    }
    else
    {
      OULY_ASSERT(false && "layout has a hole or an overlap");
      return;
    }
  }
  OULY_ASSERT(pos == space_end);
  OULY_ASSERT(fi == free_offsets_.size());
  OULY_ASSERT(ai == allocs.size());
}

auto compacting_allocator::push_entry(size_type offset, size_type size) -> uint32_t
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
    entry_live_.emplace_back();
  }
  ouly::detail::vector_access(entry_offsets_, id) = offset;
  ouly::detail::vector_access(entry_sizes_, id)   = size;
  ouly::detail::vector_access(entry_live_, id)    = true;
  return id;
}

void compacting_allocator::free_entry(uint32_t id)
{
  ouly::detail::vector_access(entry_live_, id)    = false;
  ouly::detail::vector_access(entry_offsets_, id) = free_entry_;
  free_entry_                                     = id;
}

} // namespace ouly
