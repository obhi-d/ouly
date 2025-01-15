#pragma once
#include <acl/containers/detail/vlist.hpp>
#include <acl/containers/sparse_table.hpp>
#include <utility>

namespace acl::detail
{

template <typename UsizeType, typename Extension>
struct block
{

  using size_type       = UsizeType;
  size_type     offset_ = std::numeric_limits<size_type>::max();
  size_type     size_   = 0;
  std::uint32_t arena_  = 0;
  std::uint32_t self_   = {};
  using uint32_pair     = std::pair<uint32_t, uint32_t>;
  union
  {
    std::uint32_t data_;
    uint32_t      reserved32_;
    list_node     list_;
    uint32_pair   rtup_;
    uint64_t      reserved64_;
    Extension     ext_ = {};
  };

  acl::detail::list_node arena_order_ = acl::detail::list_node();
  bool                   is_slotted_  = false;
  bool                   is_flagged_  = false;
  bool                   is_free_     = false;
  std::uint8_t           alignment_   = 0;

  struct table_traits
  {
    using size_type                                  = std::uint32_t;
    static constexpr std::uint32_t pool_size_v       = 4096;
    static constexpr std::uint32_t index_pool_size_v = 4096;
    using offset                                     = cfg::member<&block<size_type, Extension>::self_>;
  };

  block() noexcept                       = default;
  block(const block&)                    = default;
  block(block&&)                         = default;
  auto operator=(const block&) -> block& = default;
  auto operator=(block&&) -> block&      = default;
  block(size_type ioffset, size_type isize, std::uint32_t iarena) noexcept
      : offset_(ioffset), size_(isize), arena_(iarena)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, std::uint32_t idata) noexcept
      : offset_(ioffset), size_(isize), arena_(iarena), rtup_(idata, 0)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, std::uint32_t idata, bool ifree) noexcept
      : offset_(ioffset), size_(isize), arena_(iarena), rtup_(idata, 0), is_free_(ifree)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, uint32_pair idata, bool ifree) noexcept
      : offset_(ioffset), size_(isize), arena_(iarena), rtup_(std::move(idata)), is_free_(ifree)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, list_node idata, bool ifree) noexcept
      : offset_(ioffset), size_(isize), arena_(iarena), list_(idata), is_free_(ifree)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, Extension idata, bool ifree) noexcept
      : offset_(ioffset), size_(isize), arena_(iarena), ext_(idata), is_free_(ifree)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, std::uint32_t idata, bool ifree,
        bool islotted) noexcept
    requires(!std::convertible_to<std::uint32_t, Extension>)
      : offset_(ioffset), size_(isize), arena_(iarena), rtup_(idata, 0), is_free_(ifree), is_slotted_(islotted)
  {}
  block(size_type ioffset, size_type isize, std::uint32_t iarena, Extension idata, bool ifree, bool islotted) noexcept
      : offset_(ioffset), size_(isize), arena_(iarena), ext_(idata), is_free_(ifree), is_slotted_(islotted)
  {}

  ~block() noexcept = default;

  [[nodiscard]] auto adjusted_block() const -> std::pair<size_type, size_type>
  {
    size_type alignment_mask = ((size_type)1U << (size_type)alignment_) - (size_type)1U;
    return std::make_pair<size_type>((offset_ + alignment_mask) & ~alignment_mask, size_ - alignment_mask);
  }

  [[nodiscard]] auto adjusted_size() const -> size_type
  {
    size_type alignment_mask = ((size_type)1U << (size_type)alignment_) - (size_type)1U;
    return size_ - alignment_mask;
  }

  [[nodiscard]] auto adjusted_offset() const -> size_type
  {
    size_type alignment_mask = ((size_type)1U << (size_type)alignment_) - (size_type)1U;
    return (offset_ + alignment_mask) & ~alignment_mask;
  }

  auto size() const noexcept -> size_type
  {
    return size_;
  }
};

template <typename SizeType, typename Extension>
using block_bank = acl::sparse_table<block<SizeType, Extension>, typename block<SizeType, Extension>::table_traits>;

template <typename UsizeType, typename Uextension>
struct block_accessor
{
  using value_type = block<UsizeType, Uextension>;
  using bank_type  = block_bank<UsizeType, Uextension>;
  using size_type  = UsizeType;
  using container  = bank_type;
  using block_link = typename bank_type::link;

  static void erase(bank_type& bank, std::uint32_t node)
  {
    bank.erase(block_link(node));
  }

  static auto node(bank_type& bank, std::uint32_t node) -> acl::detail::list_node&
  {
    return bank[block_link(node)].arena_order_;
  }

  static auto node(bank_type const& bank, std::uint32_t node) -> acl::detail::list_node const&
  {
    return bank[block_link(node)].arena_order_;
  }

  static auto get(bank_type const& bank, std::uint32_t node) -> value_type const&
  {
    return bank[block_link(node)];
  }

  static auto get(bank_type& bank, std::uint32_t node) -> value_type&
  {
    return bank[block_link(node)];
  }
};

template <typename SizeType, typename Extension>
using block_list = acl::detail::vlist<block_accessor<SizeType, Extension>>;

} // namespace acl::detail