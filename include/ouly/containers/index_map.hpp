// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/containers/small_vector.hpp"
#include <cstdint>
#include <limits>
#include <vector>

namespace ouly
{
/**
 * @brief This class provided a map from one index value to another, with a mechanism to have a smaller memory footprint
 * for a limited number of index size, by using a minimum offset automatically as the base offset for the `key` index
 * value. Consider a range of indices `0...N`, if you insert `M...N` indices in the map, `M` will be used as the base
 * offset until the OffsetLimit number of indices occupy the list, upon which the list will be fully grown to support
 * `0...N` elements.
 */
constexpr uint32_t default_offset_limit = 16;
template <typename T = uint32_t, T OffsetLimit = default_offset_limit>
class index_map
{
  static constexpr T min_offset = std::max<T>(OffsetLimit, 1);

  struct with_offset_limit
  {
    ouly::small_vector<T, min_offset> indices_;
    T                                 min_offset_ = null;
  };

  struct without_offset_limit
  {
    std::vector<T> indices_;
  };

  using index_list = std::conditional_t<OffsetLimit == 0, without_offset_limit, with_offset_limit>;

public:
  using size_type          = T;
  static constexpr T limit = OffsetLimit;
  static constexpr T null  = std::numeric_limits<size_type>::max();

  auto operator[](T idx) noexcept -> T&
  {
    if constexpr (OffsetLimit > 0)
    {
      if (data_.min_offset_ > idx)
      {
        if (data_.indices_.empty())
        {
          data_.min_offset_ = idx;
        }
        else
        {
          T to_min_offset = 0;
          if (data_.indices_.size() < limit)
          {
            to_min_offset = idx;
          }
          data_.min_offset_ = shift(to_min_offset);
        }
      }
      idx = idx - data_.min_offset_;
    }
    if (idx >= data_.indices_.size())
    {
      data_.indices_.resize(idx + 1, null);
    }
    return data_.indices_[idx];
  }

  auto contains(T idx) const noexcept -> bool
  {
    if constexpr (OffsetLimit > 0)
    {
      return (static_cast<size_t>(idx - data_.min_offset_) < data_.indices_.size());
    }
    else
    {
      return idx < data_.indices_.size();
    }
  }

  auto find(T idx) const noexcept -> T
  {
    if constexpr (OffsetLimit > 0)
    {
      idx = idx - data_.min_offset_;
    }
    return idx < data_.indices_.size() ? data_.indices_[idx] : null;
  }

  auto operator[](T idx) const noexcept -> T
  {
    idx = idx - data_.min_offset_;
    return data_.indices_[idx];
  }

  auto get_if(T idx) const noexcept -> T
  {
    idx = idx - data_.min_offset_;
    return idx < data_.indices_.size() ? data_.indices_[idx] : null;
  }

  void clear()
  {
    if constexpr (OffsetLimit > 0)
    {
      data_.min_offset_ = null;
    }
    data_.indices_.clear();
  }

  /** @brief This value must be substracted from the index value while doing a query */
  auto base_offset() const noexcept -> T
  {
    if constexpr (OffsetLimit > 0)
    {
      return data_.min_offset_;
    }
    else
    {
      return 0;
    }
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return data_.indices_.empty();
  }

  auto size() const noexcept
  {
    return data_.indices_.size();
  }

  auto begin() noexcept
  {
    return data_.indices_.begin();
  }

  auto end() noexcept
  {
    return data_.indices_.end();
  }

  auto begin() const noexcept
  {
    return data_.indices_.begin();
  }

  auto end() const noexcept
  {
    return data_.indices_.end();
  }

  auto rbegin() noexcept
  {
    return data_.indices_.rbegin();
  }

  auto rend() noexcept
  {
    return data_.indices_.rend();
  }

  auto rbegin() const noexcept
  {
    return data_.indices_.rbegin();
  }

  auto rend() const noexcept
  {
    return data_.indices_.rend();
  }

private:
  auto shift(T offset) -> T
  {
    if constexpr (OffsetLimit > 0)
    {
      T    amount   = data_.min_offset_ - offset;
      auto cur_size = data_.indices_.size();
      data_.indices_.resize(data_.indices_.size() + amount, null);
      if (cur_size)
      {
        for (auto i = static_cast<int64_t>(cur_size - 1); i >= 0; --i)
        {
          data_.indices_[amount + static_cast<T>(i)] = data_.indices_[static_cast<T>(i)];
          data_.indices_[static_cast<T>(i)]          = null;
        }
      }
    }
    return offset;
  }

  index_list data_;
};
} // namespace ouly
