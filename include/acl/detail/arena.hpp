#pragma once
#include "../default_allocator.hpp"
#include "arena_block.hpp"

namespace acl::detail
{

//  -█████╗-██████╗-███████╗███╗---██╗-█████╗-
//  ██╔══██╗██╔══██╗██╔════╝████╗--██║██╔══██╗
//  ███████║██████╔╝█████╗--██╔██╗-██║███████║
//  ██╔══██║██╔══██╗██╔══╝--██║╚██╗██║██╔══██║
//  ██║--██║██║--██║███████╗██║-╚████║██║--██║
//  ╚═╝--╚═╝╚═╝--╚═╝╚══════╝╚═╝--╚═══╝╚═╝--╚═╝
//  ------------------------------------------
template <typename usize_type, typename uextension>
struct arena
{
  using size_type = usize_type;
  using list      = block_list<usize_type, uextension>;

  list              block_order;
  detail::list_node order;
  size_type         size = 0;
  size_type         free = 0;
  uhandle           data = detail::k_null_sz<uhandle>;
};

template <typename usize_type, typename uextension>
using arena_bank = detail::table<detail::arena<usize_type, uextension>, true>;

template <typename usize_type, typename uextension>
struct arena_accessor
{
  using value_type = arena<usize_type, uextension>;
  using bank_type  = arena_bank<usize_type, uextension>;
  using size_type  = usize_type;
  using container  = bank_type;

  inline static void erase(bank_type& bank,
    std::uint32_t node)
  {
    bank.erase(node);
  }

  inline static detail::list_node& node(bank_type& bank, std::uint32_t node)
  {
    return bank[node].order;
  }

  inline static detail::list_node const& node(bank_type const& bank, std::uint32_t node)
  {
    return bank[node].order;
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

template <typename usize_type, typename uextension>
using arena_list = detail::vlist<arena_accessor<usize_type, uextension>>;

using free_list = acl::vector<std::uint32_t>;

// ██████╗--█████╗-███╗---██╗██╗--██╗--------██████╗--█████╗-████████╗-█████╗-
// ██╔══██╗██╔══██╗████╗--██║██║-██╔╝--------██╔══██╗██╔══██╗╚══██╔══╝██╔══██╗
// ██████╔╝███████║██╔██╗-██║█████╔╝---------██║--██║███████║---██║---███████║
// ██╔══██╗██╔══██║██║╚██╗██║██╔═██╗---------██║--██║██╔══██║---██║---██╔══██║
// ██████╔╝██║--██║██║-╚████║██║--██╗███████╗██████╔╝██║--██║---██║---██║--██║
// ╚═════╝-╚═╝--╚═╝╚═╝--╚═══╝╚═╝--╚═╝╚══════╝╚═════╝-╚═╝--╚═╝---╚═╝---╚═╝--╚═╝
// ---------------------------------------------------------------------------
template <typename usize_type, typename extension>
struct bank_data
{
  using link = typename block_bank<usize_type, extension>::link;
  block_bank<usize_type, extension> blocks;
  arena_bank<usize_type, extension> arenas;
  arena_list<usize_type, extension> arena_order;
  usize_type                        free_size = 0;
  link                              root_blk;

  bank_data()
  {
    // blocks 0 is sentinel
    root_blk = blocks.emplace();
    // arena 0 is sentinel
    arenas.emplace();
  }
};
} // namespace acl::detail