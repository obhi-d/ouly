#pragma once
#include <acl/allocators/default_allocator.hpp>
#include <acl/allocators/detail/arena_block.hpp>
#include <acl/containers/table.hpp>
#include <concepts>
#include <cstdint>

namespace acl::detail
{
// Concepts

template <typename T>
concept HasGranularity = requires {
  { T::granularity_v } -> std::convertible_to<std::size_t>;
};
template <typename T>
concept HasMaxBucket = requires {
  { T::max_bucket_v } -> std::convertible_to<std::size_t>;
};
template <typename T>
concept HasSearchWindow = requires {
  { T::search_window_v } -> std::convertible_to<std::size_t>;
};
template <typename T>
concept HasFixedMaxPerSlot = requires {
  { T::fixed_max_per_slot_v } -> std::convertible_to<std::size_t>;
};
template <typename T>
concept HasFallbackStrat = requires { typename T::fallback_strat_t; };

template <typename T>
constexpr auto granularity_v = std::conditional_t<HasGranularity<T>, T, acl::cfg::granularity<256>>::granularity_v;
template <typename T>
constexpr auto max_bucket_v = std::conditional_t<HasMaxBucket<T>, T, acl::cfg::max_bucket<255>>::max_bucket_v;
template <typename T>
constexpr auto search_window_v = std::conditional_t<HasSearchWindow<T>, T, acl::cfg::search_window<4>>::search_window_v;
template <typename T>
constexpr auto fixed_max_per_slot_v =
 std::conditional_t<HasFixedMaxPerSlot<T>, T, acl::cfg::fixed_max_per_slot<8>>::fixed_max_per_slot_v;
template <typename T, typename D>
using fallback_strat_t =
 typename std::conditional_t<HasFallbackStrat<T>, T, acl::cfg::fallback_start<D>>::fallback_strat_t;

template <typename UsizeType, typename Uextension>
struct arena
{
  using size_type = UsizeType;
  using list      = block_list<UsizeType, Uextension>;

  list                   block_order_;
  acl::detail::list_node order_;
  size_type              size_ = 0;
  size_type              free_ = 0;
  std::uint32_t          data_ = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] auto block_count() const noexcept -> std::uint32_t
  {
    return block_order_.size();
  }

  [[nodiscard]] auto size() const noexcept -> std::uint32_t
  {
    return size_;
  }

  auto block_order() noexcept -> list&
  {
    return block_order_;
  }

  auto block_order() const noexcept -> list const&
  {
    return block_order_;
  }
};

template <typename UsizeType, typename Uextension>
using arena_bank = table<acl::detail::arena<UsizeType, Uextension>, true>;

template <typename UsizeType, typename Uextension>
struct arena_accessor
{
  using value_type = arena<UsizeType, Uextension>;
  using bank_type  = arena_bank<UsizeType, Uextension>;
  using size_type  = UsizeType;
  using container  = bank_type;

  static void erase(bank_type& bank, std::uint32_t node)
  {
    bank.erase(node);
  }

  static auto node(bank_type& bank, std::uint32_t node) -> acl::detail::list_node&
  {
    return bank[node].order_;
  }

  static auto node(bank_type const& bank, std::uint32_t node) -> acl::detail::list_node const&
  {
    return bank[node].order_;
  }

  static auto get(bank_type const& bank, std::uint32_t node) -> value_type const&
  {
    return bank[node];
  }

  static auto get(bank_type& bank, std::uint32_t node) -> value_type&
  {
    return bank[node];
  }
};

template <typename UsizeType, typename Uextension>
using arena_list = acl::detail::vlist<arena_accessor<UsizeType, Uextension>>;

using free_list = acl::vector<std::uint32_t>;

template <typename UsizeType, typename Extension>
struct bank_data
{
  using link = typename block_bank<UsizeType, Extension>::link;
  block_bank<UsizeType, Extension> blocks_;
  arena_bank<UsizeType, Extension> arenas_;
  arena_list<UsizeType, Extension> arena_order_;
  UsizeType                        free_size_ = 0;
  link                             root_blk_;

  bank_data() : root_blk_(blocks_.emplace())
  {
    // blocks 0 is sentinel

    // arena 0 is sentinel
    arenas_.emplace();
  }

  auto blocks() -> block_bank<UsizeType, Extension>&
  {
    return blocks_;
  }

  auto blocks() const -> block_bank<UsizeType, Extension> const&
  {
    return blocks_;
  }

  auto arenas() -> arena_bank<UsizeType, Extension>&
  {
    return arenas_;
  }

  auto arenas() const -> arena_bank<UsizeType, Extension> const&
  {
    return arenas_;
  }

  auto free_size() -> UsizeType&
  {
    return free_size_;
  }

  auto free_size() const -> UsizeType
  {
    return free_size_;
  }
};

} // namespace acl::detail
