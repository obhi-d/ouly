// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/utility/user_config.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <vector>

namespace ouly::detail
{

/**
 * @brief Two-level segregated size bins (TLSF) over intrusive lists of node indices.
 *
 * A size maps to a bin through a tiny floating point encoding: the exponent selects one of 64 groups
 * and the three bits under the leading one select one of 8 bins inside the group. The relative error
 * of a bin is therefore at most 1/8. A bitmap per group and a bitmap of non-empty groups find the
 * first non-empty bin at or above a given bin with two count-trailing-zero instructions, so `find`,
 * `push` and `erase` are all O(1).
 *
 * Nodes live in a caller owned vector and are addressed by index. `Node` must provide `bin_`
 * (`uint16_t`), `bin_prev_` and `bin_next_` (`uint32_t`) members; `bin_ == unbinned` marks a node
 * that is not in any list.
 */
template <typename Node>
class tlsf_bins
{
public:
  static constexpr uint32_t null_node     = std::numeric_limits<uint32_t>::max();
  static constexpr uint16_t unbinned      = std::numeric_limits<uint16_t>::max();
  static constexpr uint32_t mantissa_bits = 3;
  static constexpr uint32_t bins_in_group = 1U << mantissa_bits;
  static constexpr uint32_t group_count   = 64;
  static constexpr uint32_t bin_count     = group_count * bins_in_group;

  tlsf_bins() noexcept
  {
    heads_.fill(null_node);
  }

  /** @brief Bin whose sizes are all <= `size`; free blocks are filed under this bin */
  static auto bin_down(uint64_t size) noexcept -> uint32_t
  {
    if (size < bins_in_group)
    {
      return static_cast<uint32_t>(size);
    }
    auto const shift = static_cast<uint32_t>(std::bit_width(size)) - 1 - mantissa_bits;
    return ((shift + 1) << mantissa_bits) | static_cast<uint32_t>((size >> shift) & (bins_in_group - 1));
  }

  /** @brief First bin whose sizes are all >= `size`; any block found from here on fits */
  static auto bin_up(uint64_t size) noexcept -> uint32_t
  {
    if (size < bins_in_group)
    {
      return static_cast<uint32_t>(size);
    }
    auto const shift = static_cast<uint32_t>(std::bit_width(size)) - 1 - mantissa_bits;
    auto const bin   = ((shift + 1) << mantissa_bits) | static_cast<uint32_t>((size >> shift) & (bins_in_group - 1));
    return bin + static_cast<uint32_t>((size & ((uint64_t{1} << shift) - 1)) != 0);
  }

  /** @brief First non-empty bin at or above `bin`, or `bin_count` when there is none */
  [[nodiscard]] auto find(uint32_t bin) const noexcept -> uint32_t
  {
    auto group = bin / bins_in_group;
    if (group >= group_count)
    {
      return bin_count;
    }
    uint32_t const in_group =
     ouly::detail::vector_access(groups_, group) & (0xFFU << (bin & (bins_in_group - 1))) & 0xFFU;
    if (in_group != 0)
    {
      return (group * bins_in_group) | static_cast<uint32_t>(std::countr_zero(in_group));
    }
    uint64_t const above = group + 1 == group_count ? 0 : top_ & (~uint64_t{0} << (group + 1));
    if (above == 0)
    {
      return bin_count;
    }
    group = static_cast<uint32_t>(std::countr_zero(above));
    return (group * bins_in_group) |
           static_cast<uint32_t>(std::countr_zero(static_cast<uint32_t>(ouly::detail::vector_access(groups_, group))));
  }

  [[nodiscard]] auto head(uint32_t bin) const noexcept -> uint32_t
  {
    return ouly::detail::vector_access(heads_, bin);
  }

  void push(std::vector<Node>& nodes, uint32_t index, uint64_t size) noexcept
  {
    auto const bin  = bin_down(size);
    auto&      node = ouly::detail::vector_access(nodes, index);
    auto&      head = ouly::detail::vector_access(heads_, bin);
    node.bin_       = static_cast<uint16_t>(bin);
    node.bin_prev_  = null_node;
    node.bin_next_  = head;
    if (head != null_node)
    {
      ouly::detail::vector_access(nodes, head).bin_prev_ = index;
    }
    else
    {
      ouly::detail::vector_access(groups_, bin / bins_in_group) |= static_cast<uint8_t>(1U << (bin % bins_in_group));
      top_ |= uint64_t{1} << (bin / bins_in_group);
    }
    head = index;
  }

  void erase(std::vector<Node>& nodes, uint32_t index) noexcept
  {
    auto& node = ouly::detail::vector_access(nodes, index);
    if (node.bin_prev_ != null_node)
    {
      ouly::detail::vector_access(nodes, node.bin_prev_).bin_next_ = node.bin_next_;
    }
    else
    {
      uint32_t const bin                       = node.bin_;
      ouly::detail::vector_access(heads_, bin) = node.bin_next_;
      if (node.bin_next_ == null_node)
      {
        auto& group = ouly::detail::vector_access(groups_, bin / bins_in_group);
        group       = static_cast<uint8_t>(group & ~(1U << (bin % bins_in_group)));
        if (group == 0)
        {
          top_ &= ~(uint64_t{1} << (bin / bins_in_group));
        }
      }
    }
    if (node.bin_next_ != null_node)
    {
      ouly::detail::vector_access(nodes, node.bin_next_).bin_prev_ = node.bin_prev_;
    }
    node.bin_ = unbinned;
  }

private:
  uint64_t                         top_ = 0;
  std::array<uint8_t, group_count> groups_{};
  std::array<uint32_t, bin_count>  heads_{};
};

/**
 * @brief Free-space bookkeeping without free blocks.
 *
 * Only occupied blocks are stored. Every block records the free gap that follows it and is linked to
 * its neighbours in address order through stable indices; a zero sized head block at the start of a
 * range owns the leading gap. Freeing a block is then a single addition into its predecessor's gap,
 * so coalescing costs nothing, and the gaps are filed in `tlsf_bins` to make the search for space
 * O(1). `Block` needs `offset_`, `size_`, `gap_` (the size type), `prev_`, `next_` (`uint32_t`) and
 * the members `tlsf_bins` requires.
 *
 * A null `bins` pointer means the range is not open for allocation (its gaps are kept out of the
 * bins); the helpers then maintain the layout alone.
 */
struct gap_fit
{
  uint32_t block_  = tlsf_bins<int>::null_node;
  uint64_t offset_ = 0;
};

/** @brief Number of blocks in the bin just below the request that are tried before rounding up */
inline constexpr uint32_t gap_probe_count = 4;

template <typename Block>
auto fit_in_gap(Block const& block, uint64_t size, uint64_t alignment, gap_fit& fit) noexcept -> bool
{
  uint64_t const start   = uint64_t{block.offset_} + block.size_;
  uint64_t const aligned = (start + alignment - 1) & ~(alignment - 1);
  if (aligned - start + size > block.gap_)
  {
    return false;
  }
  fit.offset_ = aligned;
  return true;
}

/**
 * @brief Find a block whose trailing gap can hold `size` bytes at `alignment`.
 *
 * The bin holding sizes just below `size` is probed first because it contains the tightest fits and
 * rounding up would skip it. Otherwise any block in the first non-empty bin at or above `size` fits,
 * unless alignment padding gets in the way, in which case the search is repeated for
 * `size + alignment - 1`, which always fits.
 */
template <typename Block>
auto find_gap(std::vector<Block> const& blocks, tlsf_bins<Block> const& bins, uint64_t size,
              uint64_t alignment) noexcept -> gap_fit
{
  using bins_t = tlsf_bins<Block>;
  gap_fit fit;

  auto candidate = bins.head(bins_t::bin_down(size));
  for (uint32_t probe = 0; probe < gap_probe_count && candidate != bins_t::null_node; ++probe)
  {
    auto const& block = ouly::detail::vector_access(blocks, candidate);
    if (fit_in_gap(block, size, alignment, fit))
    {
      fit.block_ = candidate;
      return fit;
    }
    candidate = block.bin_next_;
  }

  for (auto const request : {size, size + alignment - 1})
  {
    auto const bin = bins.find(bins_t::bin_up(request));
    if (bin != bins_t::bin_count)
    {
      candidate = bins.head(bin);
      if (fit_in_gap(ouly::detail::vector_access(blocks, candidate), size, alignment, fit))
      {
        fit.block_ = candidate;
        return fit;
      }
    }
    if (alignment == 1)
    {
      break;
    }
  }
  return {};
}

/**
 * @brief Carve `[offset, offset + size)` out of the gap after `pred` and give it to `block`.
 *
 * `block` must be a fresh index that is not linked anywhere. What remains of the gap is split between
 * `pred` (alignment padding) and `block` (the tail).
 */
template <typename Block>
void split_gap(std::vector<Block>& blocks, tlsf_bins<Block>* bins, uint32_t pred, uint32_t block, uint64_t offset,
               uint64_t size) noexcept
{
  using bins_t = tlsf_bins<Block>;
  auto& prev   = ouly::detail::vector_access(blocks, pred);
  auto& next   = ouly::detail::vector_access(blocks, block);
  if (prev.bin_ != bins_t::unbinned)
  {
    bins->erase(blocks, pred);
  }
  auto const start   = uint64_t{prev.offset_} + prev.size_;
  auto const gap_end = start + prev.gap_;
  OULY_ASSERT(offset >= start && offset + size <= gap_end);

  using size_type = decltype(prev.gap_);
  next.offset_    = static_cast<size_type>(offset);
  next.size_      = static_cast<size_type>(size);
  next.gap_       = static_cast<size_type>(gap_end - offset - size);
  next.prev_      = pred;
  next.next_      = prev.next_;
  next.bin_       = bins_t::unbinned;
  prev.gap_       = static_cast<size_type>(offset - start);
  if (prev.next_ != bins_t::null_node)
  {
    ouly::detail::vector_access(blocks, prev.next_).prev_ = block;
  }
  prev.next_ = block;

  if (bins != nullptr)
  {
    if (prev.gap_ != 0)
    {
      bins->push(blocks, pred, prev.gap_);
    }
    if (next.gap_ != 0)
    {
      bins->push(blocks, block, next.gap_);
    }
  }
}

/**
 * @brief Unlink `block` and return its bytes and trailing gap to its predecessor's gap.
 *
 * @return The predecessor, whose gap now covers the released range.
 */
template <typename Block>
auto merge_into_prev(std::vector<Block>& blocks, tlsf_bins<Block>* bins, uint32_t block) noexcept -> uint32_t
{
  using bins_t    = tlsf_bins<Block>;
  auto&      node = ouly::detail::vector_access(blocks, block);
  auto const pred = node.prev_;
  auto&      prev = ouly::detail::vector_access(blocks, pred);
  if (prev.bin_ != bins_t::unbinned)
  {
    bins->erase(blocks, pred);
  }
  if (node.bin_ != bins_t::unbinned)
  {
    bins->erase(blocks, block);
  }
  prev.gap_ += node.size_ + node.gap_;
  prev.next_ = node.next_;
  if (node.next_ != bins_t::null_node)
  {
    ouly::detail::vector_access(blocks, node.next_).prev_ = pred;
  }
  if (bins != nullptr)
  {
    bins->push(blocks, pred, prev.gap_);
  }
  return pred;
}

} // namespace ouly::detail
