//
// Created by obhi on 11/17/20.
//
#pragma once
#include "linear_allocator.hpp"
#include "podvector.hpp"
#include <limits>

namespace acl
{

struct linear_stack_allocator_tag
{};

template <typename underlying_allocator = acl::default_allocator<>, bool k_compute_stats = false>
class linear_stack_allocator
    : public detail::statistics<linear_stack_allocator_tag, k_compute_stats, underlying_allocator>
{
public:
  using tag        = linear_stack_allocator_tag;
  using statistics = detail::statistics<linear_stack_allocator_tag, k_compute_stats, underlying_allocator>;
  using size_type  = typename underlying_allocator::size_type;
  using address    = typename underlying_allocator::address;

  enum : size_type
  {
    k_minimum_size = static_cast<size_type>(64)
  };

  struct rewind_point
  {
    size_type arena;
    size_type left_over;
  };

  struct scoped_rewind
  {
    scoped_rewind(scoped_rewind const&) = delete;
    scoped_rewind(scoped_rewind&& mv) : marker(mv.marker), ref(mv.ref)
    {
      mv.marker.arena = std::numeric_limits<size_type>::max();
    }
    scoped_rewind(linear_stack_allocator<underlying_allocator, k_compute_stats>& r)
        : ref(r), marker(r.get_rewind_point())
    {}
    ~scoped_rewind()
    {
      if (marker.arena != std::numeric_limits<size_type>::max())
        ref.rewind(marker);
    }

    rewind_point                                                   marker;
    linear_stack_allocator<underlying_allocator, k_compute_stats>& ref;
  };

  template <typename... Args>
  explicit linear_stack_allocator(size_type i_arena_size, Args&&... i_args)
      : k_arena_size(i_arena_size), statistics(std::forward<Args>(i_args)...), current_arena(0)
  {
    // Initializing the cursor is important for the
    // allocate loop to work.
  }
  ~linear_stack_allocator()
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

  scoped_rewind get_auto_rewind_point()
  {
    return scoped_rewind(*this);
  }

  rewind_point get_rewind_point() const
  {
    rewind_point m;
    m.arena = current_arena;
    if (current_arena < arenas.size())
      m.left_over = arenas[current_arena].left_over;
    else
      m.left_over = 0xffffffff;
    return m;
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
        current_arena++;
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

  address zero_allocate(size_type i_size, size_type i_alignment = 0)
  {
    auto z = allocate(i_size, i_alignment);
    std::memset(z, 0, i_size);
    return z;
  }

  void deallocate(address i_data, size_type i_size, size_type i_alignment = 0)
  {
    // does not support deallocate, only rewinds are supported
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

  inline void rewind(rewind_point marker)
  {
    current_arena = marker.arena;
    if (current_arena < arenas.size())
      arenas[current_arena].left_over = std::min(marker.left_over, arenas[current_arena].arena_size);
    size_type end = static_cast<size_type>(arenas.size());
    for (size_type i = marker.arena + 1; i < end; ++i)
      arenas[i].reset();
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

  podvector<arena, underlying_allocator> arenas;
  size_type                              current_arena = 0;

  const size_type k_arena_size;

public:
};

} // namespace acl