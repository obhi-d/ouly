#pragma once

#include "linear_allocator.hpp"

namespace acl
{

struct linear_arena_allocator_tag
{};

template <typename underlying_allocator = acl::default_allocator<>, bool k_compute_stats = false>
class linear_arena_allocator
    : public detail::statistics<linear_arena_allocator_tag, k_compute_stats, underlying_allocator>
{
public:
  using tag        = linear_arena_allocator_tag;
  using statistics = detail::statistics<linear_arena_allocator_tag, k_compute_stats, underlying_allocator>;
  using size_type  = typename underlying_allocator::size_type;
  using address    = typename underlying_allocator::address;

  enum : size_type
  {
    k_minimum_size = static_cast<size_type>(64)
  };

  template <typename... Args>
  explicit linear_arena_allocator(size_type i_arena_size, Args&&... i_args)
      : k_arena_size(i_arena_size), statistics(std::forward<Args>(i_args)...), current_arena(0)
  {
    // Initializing the cursor is important for the
    // allocate loop to work.
  }
  ~linear_arena_allocator()
  {
    for (auto& arena : arenas)
    {
      underlying_allocator::deallocate(arena.buffer, arena.arena_size);
    }
  }

  inline constexpr static address null()
  {
    return underlying_allocator::null();
  }

  address allocate(size_type i_size, size_type i_alignment = 0)
  {

    auto measure = statistics::report_allocate(i_size);
    // assert
    auto fixup = i_alignment - 1;
    // make sure you allocate enough space
    // but keep alignment distance so that next allocations
    // do not suffer from lost alignment
    if (i_alignment)
      i_size += i_alignment;

    address ret_value = null();

    size_type index = current_arena;
    for (auto end = static_cast<size_type>(arenas.size()); index < end; ++index)
    {

      if (arenas[index].left_over >= i_size)
      {
        ret_value = allocate_from(index, i_size);
        break;
      }
      else
      {
        if (arenas[index].left_over < k_minimum_size && index != current_arena)
          std::swap(arenas[index], arenas[current_arena++]);
      }
    }

    if (ret_value == null())
    {
      size_type max_arena_size = std::max<size_type>(i_size, k_arena_size);
      ret_value                = allocate_from(index = allocate_new_arena(max_arena_size), i_size);
    }

    if (i_alignment)
    {
      auto pointer = reinterpret_cast<std::uintptr_t>(ret_value);
      // already aligned
      if ((pointer & fixup) == 0)
      {
        arenas[index].left_over += i_alignment;
        return ret_value;
      }
      else
      {
        auto ret = (pointer + static_cast<std::uintptr_t>(fixup)) & ~static_cast<std::uintptr_t>(fixup);
        return reinterpret_cast<address>(ret);
      }
    }
    else
      return ret_value;
  }

  void deallocate(address i_data, size_type i_size, size_type i_alignment = 0)
  {
    auto measure = statistics::report_deallocate(i_size);

    for (size_type id = static_cast<size_type>(arenas.size()) - 1; id >= current_arena; --id)
    {
      if (in_range(arenas[id], i_data))
      {
        // merge back?

        size_type new_left_over = arenas[id].left_over + i_size;
        size_type offset        = (arenas[id].arena_size - new_left_over);
        if (reinterpret_cast<std::uint8_t*>(arenas[id].buffer) + offset == reinterpret_cast<std::uint8_t*>(i_data))
        {
          arenas[id].left_over = new_left_over;
        }
        else if (i_alignment)
        {
          i_size += i_alignment;

          new_left_over = arenas[id].left_over + i_size;
          offset        = (arenas[id].arena_size - new_left_over);

          // This memory fixed up by alignment and is within range of alignment
          if ((reinterpret_cast<std::uintptr_t>(i_data) -
               (reinterpret_cast<std::uintptr_t>(arenas[id].buffer) + offset)) < i_alignment)
            arenas[id].left_over = new_left_over;
        }

        break;
      }
    }
  }

  void smart_rewind()
  {
    // delete remaining arenas
    for (size_type index = current_arena + 1, end = static_cast<size_type>(arenas.size()); index < end; ++index)
    {
      underlying_allocator::deallocate(arenas[index].buffer, arenas[index].arena_size);
    }
    arenas.resize(current_arena + 1);
    current_arena = 0;
    for (auto& ar : arenas)
      ar.reset();
  }

  void rewind()
  {
    current_arena = 0;
    for (auto& ar : arenas)
      ar.reset();
  }

  std::uint32_t get_arena_count() const
  {
    return static_cast<std::uint32_t>(arenas.size());
  }

private:
  struct arena
  {
    address   buffer;
    size_type left_over;
    size_type arena_size;
    arena() = default;
    arena(address i_buffer, size_type i_left_over, size_type i_arena_size)
        : buffer(i_buffer), left_over(i_left_over), arena_size(i_arena_size)
    {}

    void reset()
    {
      left_over = arena_size;
    }
  };

  inline bool in_range(const arena& i_arena, address i_data)
  {
    return (i_arena.buffer <= i_data && i_data < (static_cast<std::uint8_t*>(i_arena.buffer) + i_arena.arena_size)) !=
           0;
  }

  inline size_type allocate_new_arena(size_type size)
  {
    statistics::report_new_arena();

    size_type index = static_cast<size_type>(arenas.size());
    arenas.emplace_back(underlying_allocator::allocate(size), size, size);
    return index;
  }

  inline address allocate_from(size_type id, size_type size)
  {
    size_type offset = arenas[id].arena_size - arenas[id].left_over;
    arenas[id].left_over -= size;
    return static_cast<std::uint8_t*>(arenas[id].buffer) + offset;
  }

  acl::vector<arena> arenas;
  size_type          current_arena;

  const size_type k_arena_size;

public:
};

} // namespace acl