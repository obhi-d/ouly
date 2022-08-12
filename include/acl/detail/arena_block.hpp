#pragma once
#include "memory_move.hpp"

namespace acl::detail
{

//  ██████╗-██╗------██████╗--██████╗██╗--██╗
//  ██╔══██╗██║-----██╔═══██╗██╔════╝██║-██╔╝
//  ██████╔╝██║-----██║---██║██║-----█████╔╝-
//  ██╔══██╗██║-----██║---██║██║-----██╔═██╗-
//  ██████╔╝███████╗╚██████╔╝╚██████╗██║--██╗
//  ╚═════╝-╚══════╝-╚═════╝--╚═════╝╚═╝--╚═╝
//  -----------------------------------------
template <typename traits>
struct block
{
  using size_type = typename traits::size_type;
  using extension = typename traits::extension;

  size_type         offset      = detail::k_null_sz<size_type>;
  size_type         size        = 0;
  uhandle           data        = detail::k_null_sz<uhandle>;
  std::uint32_t     arena       = detail::k_null_32;
  detail::list_node arena_order = detail::list_node();
  bool              is_free     = false;
  std::uint8_t      alignment   = 0;
  bool              is_flagged  = false;
  extension         ext;

  block() {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena) : offset(ioffset), size(isize), arena(iarena) {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, uhandle idata)
      : offset(ioffset), size(isize), arena(iarena), data(idata)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, uhandle idata, bool ifree)
      : offset(ioffset), size(isize), arena(iarena), data(idata), is_free(ifree)
  {}

  inline std::pair<size_type, size_type> adjusted_block() const
  {
    size_type alignment_mask = (1u << alignment) - 1u;
    return std::make_pair<size_type>((offset + alignment_mask) & ~alignment_mask, size - alignment_mask);
  }

  inline size_type adjusted_size() const
  {
    size_type alignment_mask = (1u << alignment) - 1u;
    return size - alignment_mask;
  }

  inline size_type adjusted_offset() const
  {
    size_type alignment_mask = (1u << alignment) - 1u;
    return (offset + alignment_mask) & ~alignment_mask;
  }
};

template <typename traits>
using block_bank = detail::table<block<traits>, true>;

template <typename traits>
struct block_accessor
{
  using value_type = block<traits>;
  using bank_type  = block_bank<traits>;
  using size_type  = typename traits::size_type;
  using container  = bank_type;

  inline static detail::list_node& node(bank_type& bank, std::uint32_t node)
  {
    return bank[node].arena_order;
  }

  inline static detail::list_node const& node(bank_type const& bank, std::uint32_t node)
  {
    return bank[node].arena_order;
  }

  inline static value_type const& get(bank_type const& bank, std::uint32_t node)
  {
    return bank[node];
  }

  inline static value_type& get(bank_type& bank, std::uint32_t node)
  {
    return bank[node];
  }
};

template <typename traits>
using block_list = detail::vlist<block_accessor<traits>>;

} // namespace acl::detail