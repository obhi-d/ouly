#pragma once
#include <acl/detail/common.hpp>
#include <acl/detail/rbtree.hpp>
#include <acl/detail/table.hpp>
#include <acl/detail/vlist.hpp>
#include <bit>
#include <type_traits>
#include <utility>

namespace acl
{

enum alloc_option_bits : std::uint32_t
{
  f_defrag          = 1u << 0u,
  f_dedicated_arena = 1u << 1u,
};

using alloc_options = std::uint32_t;

template <typename size_type>
struct memory_manager_adapter
{
  bool drop_arena([[maybe_unused]] uhandle id)
  {
    return true;
  }

  uhandle add_arena([[maybe_unused]] ihandle id, [[maybe_unused]] size_type size)
  {
    return id;
  }

  void begin_defragment() {}
  void end_defragment() {}

  void remove_arena(acl::uhandle h) {}
  void move_memory([[maybe_unused]] uhandle src_arena, [[maybe_unused]] uhandle dst_arena,
                   [[maybe_unused]] size_type from, [[maybe_unused]] size_type to, size_type size)
  {}
  template <typename alloc_info>
  void rebind_alloc([[maybe_unused]] uhandle halloc, alloc_info info)
  {}
};

template <typename size_type>
class basic_alloc_desc
{
public:
  basic_alloc_desc(size_type isize) : size_(isize) {}
  basic_alloc_desc(size_type isize, uhandle huser) : size_(isize), huser_(huser) {}

  constexpr size_type alignment_mask() const
  {
    return ~(size_type)0;
  }

  constexpr size_type alignment() const
  {
    return 0;
  }

  constexpr size_type alignment_bits() const
  {
    return ~(size_type)0;
  }

  constexpr alloc_options flags() const
  {
    return {};
  }

  size_type size() const
  {
    return size_;
  }

  uhandle huser() const
  {
    return huser_;
  }

  size_type adjusted_size() const
  {
    // 1 extra byte for better merges
    return size();
  }

  size_type size_{};
  uhandle   huser_{};
};

template <typename size_type, uint32_t alignment_>
class fixed_alloc_desc
{
public:
  constexpr fixed_alloc_desc(size_type isize) : size_(isize) {}
  constexpr fixed_alloc_desc(size_type isize, uhandle huser) : size_(isize), huser_(huser) {}
  constexpr fixed_alloc_desc(size_type isize, uhandle ihuser, alloc_options iflags)
      : size_(isize), huser_(ihuser), flags_(iflags)
  {}

  constexpr size_type alignment() const
  {
    return alignment_;
  }

  constexpr size_type alignment_mask() const
  {
    if constexpr (alignment_ == 1)
      return ~(size_type)0;
    return (size_type)alignment_ - (size_type)1;
  }

  constexpr size_type alignment_bits() const
  {
    if constexpr (alignment_ == 1)
      return 0;
    return detail::log2(alignment_);
  }

  size_type size() const
  {
    return size_;
  }

  uhandle huser() const
  {
    return huser_;
  }

  alloc_options flags() const
  {
    return flags_;
  }

  size_type adjusted_size() const
  {
    // 1 extra byte for better merges
    if constexpr (alignment_ == 1)
      return size();
    else
      return size() + alignment_;
  }

  size_type     size_{};
  uhandle       huser_{};
  alloc_options flags_ = 0;
};

template <typename size_type>
class dynamic_alloc_desc
{
  dynamic_alloc_desc(size_type isize) : size_(isize) {}
  dynamic_alloc_desc(size_type isize, uhandle ihuser) : size_(isize), huser_(ihuser) {}
  dynamic_alloc_desc(size_type isize, size_type ialignment, uhandle ihuser, alloc_options iflags)
      : size_(isize), alignment_(ialignment), huser_(ihuser), flags_(iflags)
  {}

  size_type size() const
  {
    return size_;
  }

  uint8_t alignment_bits() const
  {
    return static_cast<std::uint8_t>(ACL_POPCOUNT(static_cast<std::uint32_t>(alignment_)));
  }

  size_type alignment_mask() const
  {
    return alignment_ - 1;
  }

  size_type alignment() const
  {
    return alignment_;
  }

  uhandle huser() const
  {
    return huser_;
  }

  alloc_options flags() const
  {
    return flags_;
  }

  size_type adjusted_size() const
  {
    // 1 extra byte for better merges
    return size() + alignment_mask() + 1;
  }

private:
  size_type     size_      = 0;
  size_type     alignment_ = 0;
  uhandle       huser_     = 0;
  alloc_options flags_     = 0;
};

// -█████╗-██╗-----██╗------██████╗--██████╗--------██╗███╗---██╗███████╗-██████╗-
// ██╔══██╗██║-----██║-----██╔═══██╗██╔════╝--------██║████╗--██║██╔════╝██╔═══██╗
// ███████║██║-----██║-----██║---██║██║-------------██║██╔██╗-██║█████╗--██║---██║
// ██╔══██║██║-----██║-----██║---██║██║-------------██║██║╚██╗██║██╔══╝--██║---██║
// ██║--██║███████╗███████╗╚██████╔╝╚██████╗███████╗██║██║-╚████║██║-----╚██████╔╝
// ╚═╝--╚═╝╚══════╝╚══════╝-╚═════╝--╚═════╝╚══════╝╚═╝╚═╝--╚═══╝╚═╝------╚═════╝-
// -------------------------------------------------------------------------------
template <typename size_type>
struct alloc_offset
{
  uhandle   harena = detail::k_null_sz<uhandle>;
  size_type offset = detail::k_null_sz<size_type>;
  alloc_offset()   = default;
  alloc_offset(uhandle iharena, size_type ioffset) : harena(iharena), offset(ioffset) {}
};

template <typename size_type>
struct alloc_info : alloc_offset<size_type>
{
  ihandle halloc = detail::k_null_32;
  alloc_info()   = default;
  alloc_info(uhandle iharena, size_type ioffset, ihandle ihalloc)
      : alloc_offset<size_type>(iharena, ioffset), halloc(ihalloc)
  {}
};

} // namespace acl