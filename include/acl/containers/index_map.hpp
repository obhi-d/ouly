#pragma once
#include <cstdint>
#include <limits>
#include <acl/containers/small_vector.hpp>

namespace acl
{
/**
 * @brief This class provided a map from one index value to another, with a mechanism to have a smaller memory footprint
 * for a limited number of index size, by using a minimum offset automatically as the base offset for the `key` index
 * value. Consider a range of indices `0...N`, if you insert `M...N` indices in the map, `M` will be used as the base
 * offset until the OffsetLimit number of indices occupy the list, upon which the list will be fully grown to support
 * `0...N` elements.
 */
template <typename T = uint32_t, T OffsetLimit = 16>
class index_map
{
public:
  using size_type          = T;
  static constexpr T limit = OffsetLimit;
  static constexpr T null  = std::numeric_limits<size_type>::max();

  T& operator[](T idx) noexcept
  {
    if (min_offset_ > idx)
    {
      if (indices_.empty())
        min_offset_ = idx;
      else
      {
        T to_min_offset = 0;
        if (indices_.size() < limit)
          to_min_offset = idx;
        min_offset_ = shift(to_min_offset);
      }
    }

    idx = idx - min_offset_;
    if (idx >= indices_.size())
      indices_.resize(idx + 1, null);
    return indices_[idx];
  }

  bool contains(T idx) const noexcept
  {
    return ((idx - min_offset_) < indices_.size());
  }

  T find(T idx) const noexcept
  {
    idx = idx - min_offset_;
    return idx < indices_.size() ? indices_[idx] : null;
  }

  T operator[](T idx) const noexcept
  {
    idx = idx - min_offset_;
    return indices_[idx];
  }

  void clear()
  {
    min_offset_ = null;
    indices_.clear();
  }

  /** @brief This value must be substracted from the index value while doing a query */
  auto base_offset() const noexcept
  {
    return min_offset_;
  }

  bool empty() const noexcept
  {
    return indices_.empty();
  }

  auto size() const noexcept
  {
    return indices_.size();
  }

  auto begin() noexcept
  {
    return indices_.begin();
  }

  auto end() noexcept
  {
    return indices_.end();
  }

  auto begin() const noexcept
  {
    return indices_.begin();
  }

  auto end() const noexcept
  {
    return indices_.end();
  }

  auto rbegin() noexcept
  {
    return indices_.rbegin();
  }

  auto rend() noexcept
  {
    return indices_.rend();
  }

  auto rbegin() const noexcept
  {
    return indices_.rbegin();
  }

  auto rend() const noexcept
  {
    return indices_.rend();
  }

private:
  inline T shift(T offset)
  {
    T    amount   = min_offset_ - offset;
    auto cur_size = indices_.size();
    indices_.resize(indices_.size() + amount, null);
    if (cur_size)
    {
      for (int64_t i = (int64_t)(cur_size - 1); i >= 0; --i)
      {
        indices_[amount + (T)i] = indices_[(T)i];
        indices_[(T)i]          = null;
      }
    }
    return offset;
  }

  acl::small_vector<T, OffsetLimit> indices_;
  T                                 min_offset_ = null;
};
} // namespace acl