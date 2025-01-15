
#pragma once

#include <acl/allocators/allocation_id.hpp>
#include <acl/allocators/detail/arena.hpp>
#include <acl/containers/detail/vlist.hpp>
#include <vector>

namespace acl::detail
{

template <typename T>
struct ca_bank
{
  uint32_t       free_idx_ = 0;
  std::vector<T> entries_  = {T()};

  auto push(T const& data) -> uint32_t
  {
    if (free_idx_ != 0U)
    {
      auto entry      = free_idx_;
      free_idx_       = entries_[free_idx_].order_.next_;
      entries_[entry] = data;
      return entry;
    }

    auto id = static_cast<uint32_t>(entries_.size());
    entries_.emplace_back(data);
    return id;
  }
};

template <typename T>
struct ca_accessor
{
  using value_type = T;
  using bank_type  = ca_bank<T>;
  using size_type  = allocation_size_type;
  using container  = bank_type;

  static void erase(bank_type& bank, std::uint32_t node)
  {
    bank.entries_[node].order_.next_ = bank.free_idx_;
    bank.free_idx_                   = node;
  }

  static auto node(bank_type& bank, std::uint32_t node) -> acl::detail::list_node&
  {
    return bank.entries_[node].order_;
  }

  static auto node(bank_type const& bank, std::uint32_t node) -> acl::detail::list_node const&
  {
    return bank.entries_[node].order_;
  }

  static auto get(bank_type const& bank, std::uint32_t node) -> value_type const&
  {
    return bank.entries_[node];
  }

  static auto get(bank_type& bank, std::uint32_t node) -> value_type&
  {
    return bank.entries_[node];
  }
};

template <typename T>
using ca_list = acl::detail::vlist<ca_accessor<T>>;

struct ca_block_entries
{
  uint32_t                            free_idx_    = 0;
  std::vector<acl::detail::list_node> ordering_    = {acl::detail::list_node()};
  std::vector<allocation_size_type>   offsets_     = {0};
  std::vector<allocation_size_type>   sizes_       = {0};
  std::vector<uint16_t>               arenas_      = {0};
  std::vector<bool>                   free_marker_ = {false};

  auto push() -> uint32_t
  {
    if (free_idx_ != 0U)
    {
      auto entry = free_idx_;
      free_idx_  = offsets_[free_idx_];
      return entry;
    }

    auto id = static_cast<uint32_t>(ordering_.size());
    ordering_.emplace_back();
    offsets_.emplace_back();
    sizes_.emplace_back();
    arenas_.emplace_back();
    free_marker_.emplace_back();
    return id;
  }

  auto push(allocation_size_type offset, allocation_size_type size, uint16_t arena, bool is_free) -> uint32_t
  {
    if (free_idx_ != 0U)
    {
      auto entry          = free_idx_;
      free_idx_           = offsets_[free_idx_];
      ordering_[entry]    = {};
      offsets_[entry]     = offset;
      sizes_[entry]       = size;
      arenas_[entry]      = arena;
      free_marker_[entry] = is_free;
      return entry;
    }

    auto id = static_cast<uint32_t>(offsets_.size());
    ordering_.emplace_back();
    offsets_.emplace_back(offset);
    sizes_.emplace_back(size);
    arenas_.emplace_back(arena);
    free_marker_.emplace_back(is_free);
    return id;
  }
};

struct ca_block_accessor
{
  using container  = ca_block_entries;
  using value_type = std::uint32_t;

  static void erase(ca_block_entries& bank, std::uint32_t node)
  {
    bank.offsets_[node] = bank.free_idx_;
    bank.free_idx_      = node;
  }

  static auto node(ca_block_entries& bank, std::uint32_t node_id) -> acl::detail::list_node&
  {
    return bank.ordering_[node_id];
  }

  static auto node(ca_block_entries const& bank, std::uint32_t node_id) -> acl::detail::list_node const&
  {
    return bank.ordering_[node_id];
  }

  static auto get(ca_block_entries const& bank, std::uint32_t node) -> value_type
  {
    return node;
  }

  static auto get(ca_block_entries& bank, std::uint32_t& node) -> value_type&
  {
    return node;
  }
};

using ca_block_list = acl::detail::vlist<ca_block_accessor>;

struct ca_arena
{
  using size_type = allocation_size_type;

  ca_block_list          blocks_;
  acl::detail::list_node order_;
  size_type              size_      = 0;
  size_type              free_size_ = 0;
};

using ca_arena_entries = ca_bank<ca_arena>;
using ca_arena_list    = ca_list<ca_arena>;

struct ca_allocator_tag
{};
} // namespace acl::detail