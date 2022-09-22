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
template <typename traits>
struct arena
{
  using size_type = typename traits::size_type;

  block_list<traits> block_order;
  detail::list_node  order;
  size_type          size = 0;
  size_type          free = 0;
  uhandle            data = detail::k_null_sz<uhandle>;
};

template <typename traits>
using arena_bank = detail::table<detail::arena<traits>, true>;

template <typename traits>
struct arena_accessor
{
  using value_type = arena<traits>;
  using bank_type  = arena_bank<traits>;
  using size_type  = typename traits::size_type;
  using container  = bank_type;

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

template <typename traits>
using arena_list = detail::vlist<arena_accessor<traits>>;

using free_list = acl::vector<std::uint32_t>;

template <alloc_strategy strategy, typename traits>
class alloc_strategy_impl;

template <alloc_strategy strategy>
struct block_ext
{
  struct type
  {};
};

template <typename traits>
using alloc_strategy_type = alloc_strategy_impl<traits::strategy, traits>;

// ██████╗--█████╗-███╗---██╗██╗--██╗--------██████╗--█████╗-████████╗-█████╗-
// ██╔══██╗██╔══██╗████╗--██║██║-██╔╝--------██╔══██╗██╔══██╗╚══██╔══╝██╔══██╗
// ██████╔╝███████║██╔██╗-██║█████╔╝---------██║--██║███████║---██║---███████║
// ██╔══██╗██╔══██║██║╚██╗██║██╔═██╗---------██║--██║██╔══██║---██║---██╔══██║
// ██████╔╝██║--██║██║-╚████║██║--██╗███████╗██████╔╝██║--██║---██║---██║--██║
// ╚═════╝-╚═╝--╚═╝╚═╝--╚═══╝╚═╝--╚═╝╚══════╝╚═════╝-╚═╝--╚═╝---╚═╝---╚═╝--╚═╝
// ---------------------------------------------------------------------------
template <typename traits>
struct bank_data
{
  block_bank<traits>                  blocks;
  arena_bank<traits>                  arenas;
  arena_list<traits>                  arena_order;
  detail::alloc_strategy_type<traits> strat;
  typename traits::size_type          free_size = 0;

  bank_data()
  {
    // blocks 0 is sentinel
    blocks.emplace();
  }
};
} // namespace acl::detail