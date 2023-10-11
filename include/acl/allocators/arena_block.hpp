#pragma once
#include <acl/containers/sparse_table.hpp>
#include <acl/containers/vlist.hpp>

namespace acl::detail
{

template <typename usize_type, typename extension>
struct block
{

  using size_type      = usize_type;
  size_type     offset = detail::k_null_sz<size_type>;
  size_type     size   = 0;
  std::uint32_t arena  = 0;
  std::uint32_t self   = {};
  using uint32_pair    = std::pair<uint32_t, uint32_t>;
  union
  {
    uhandle     data;
    uint32_t    reserved32_;
    list_node   list_;
    uint32_pair rtup_;
    uint64_t    reserved64_;
    extension   ext = {};
  };

  detail::list_node arena_order = detail::list_node();
  bool              is_slotted  = false;
  bool              is_flagged  = false;
  bool              is_free     = false;
  std::uint8_t      alignment   = 0;

  struct table_traits
  {
    using size_type                                  = std::uint32_t;
    static constexpr std::uint32_t pool_size_v       = 4096;
    static constexpr std::uint32_t index_pool_size_v = 4096;
    using offset                                     = opt::member<&block<size_type, extension>::self>;
  };

  block() noexcept {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena) noexcept : offset(ioffset), size(isize), arena(iarena)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, uhandle idata) noexcept
      : offset(ioffset), size(isize), arena(iarena), rtup_(idata, 0)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, uhandle idata, bool ifree) noexcept
      : offset(ioffset), size(isize), arena(iarena), rtup_(idata, 0), is_free(ifree)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, uint32_pair idata, bool ifree) noexcept
      : offset(ioffset), size(isize), arena(iarena), rtup_(idata), is_free(ifree)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, list_node idata, bool ifree) noexcept
      : offset(ioffset), size(isize), arena(iarena), list_(idata), is_free(ifree)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, extension idata, bool ifree) noexcept
      : offset(ioffset), size(isize), arena(iarena), ext(idata), is_free(ifree)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, uhandle idata, bool ifree, bool islotted) noexcept
    requires(!std::convertible_to<uhandle, extension>)
      : offset(ioffset), size(isize), arena(iarena), rtup_(idata, 0), is_free(ifree), is_slotted(islotted)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, extension idata, bool ifree, bool islotted) noexcept
      : offset(ioffset), size(isize), arena(iarena), ext(idata), is_free(ifree), is_slotted(islotted)
  {}

  ~block() {}

  inline std::pair<size_type, size_type> adjusted_block() const
  {
    size_type alignment_mask = ((size_type)1u << (size_type)alignment) - (size_type)1u;
    return std::make_pair<size_type>((offset + alignment_mask) & ~alignment_mask, size - alignment_mask);
  }

  inline size_type adjusted_size() const
  {
    size_type alignment_mask = ((size_type)1u << (size_type)alignment) - (size_type)1u;
    return size - alignment_mask;
  }

  inline size_type adjusted_offset() const
  {
    size_type alignment_mask = ((size_type)1u << (size_type)alignment) - (size_type)1u;
    return (offset + alignment_mask) & ~alignment_mask;
  }
};

template <typename size_type, typename extension>
using block_bank = acl::sparse_table<block<size_type, extension>, typename block<size_type, extension>::table_traits>;

template <typename usize_type, typename uextension>
struct block_accessor
{
  using value_type = block<usize_type, uextension>;
  using bank_type  = block_bank<usize_type, uextension>;
  using size_type  = usize_type;
  using container  = bank_type;
  using block_link = typename bank_type::link;

  inline static void erase(bank_type& bank, std::uint32_t node)
  {
    bank.erase(block_link(node));
  }

  inline static detail::list_node& node(bank_type& bank, std::uint32_t node)
  {
    return bank[block_link(node)].arena_order;
  }

  inline static detail::list_node const& node(bank_type const& bank, std::uint32_t node)
  {
    return bank[block_link(node)].arena_order;
  }

  inline static value_type const& get(bank_type const& bank, std::uint32_t node)
  {
    return bank[block_link(node)];
  }

  inline static value_type& get(bank_type& bank, std::uint32_t node)
  {
    return bank[block_link(node)];
  }
};

template <typename size_type, typename extension>
using block_list = detail::vlist<block_accessor<size_type, extension>>;

} // namespace acl::detail