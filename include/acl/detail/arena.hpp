#pragma once
#include "../default_allocator.hpp"
#include "arena_block.hpp"

namespace acl::opt
{
template <size_t value>
struct granularity
{
  static constexpr size_t granularity_v = value;
};

template <size_t value>
struct max_bucket
{
  static constexpr size_t max_bucket_v = value;
};

template <size_t value>
struct search_window
{
  static constexpr size_t search_window_v = value;
};

template <typename T>
struct fallback_start
{
  using fallback_strat_t = T;
};

template <size_t value>
struct fixed_max_per_slot
{
  static constexpr size_t fixed_max_per_slot_v = value;
};

} // namespace acl::opt

namespace acl::detail
{
// Concepts
// clang-format off
template <typename T>
concept HasGranularity = requires { {T::granularity_v} -> std::convertible_to<std::size_t>; };
template <typename T>
concept HasMaxBucket = requires { {T::max_bucket_v} -> std::convertible_to<std::size_t>; };
template <typename T>
concept HasSearchWindow = requires { {T::search_window_v} -> std::convertible_to<std::size_t>; };
template <typename T>
concept HasFixedMaxPerSlot = requires { {T::fixed_max_per_slot_v} -> std::convertible_to<std::size_t>; };
template <typename T>
concept HasFallbackStrat = requires { typename T::fallback_strat_t; };

template <typename T>
constexpr auto granularity_v = std::conditional_t<HasGranularity<T>, T, acl::opt::granularity<256>>::granularity_v;
template <typename T>
constexpr auto max_bucket_v = std::conditional_t<HasMaxBucket<T>, T, acl::opt::max_bucket<255>>::max_bucket_v;
template <typename T>
constexpr auto search_window_v = std::conditional_t<HasSearchWindow<T>, T, acl::opt::search_window<4>>::search_window_v;
template <typename T>
constexpr auto fixed_max_per_slot_v = std::conditional_t<HasFixedMaxPerSlot<T>, T, acl::opt::fixed_max_per_slot<8>>::fixed_max_per_slot_v;
template <typename T, typename D>
using fallback_strat_t = typename std::conditional_t<HasFallbackStrat<T>, T, acl::opt::fallback_start<D>>::fallback_strat_t;

// clang-format on

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

  inline static void erase(bank_type& bank, std::uint32_t node)
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
