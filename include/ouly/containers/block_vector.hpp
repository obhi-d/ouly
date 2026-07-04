// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/utility/config.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace ouly
{

/**
 * @brief Vector-like container with storage rounded to fixed-size blocks.
 *
 * block_vector keeps a logical element count while maintaining its underlying storage size
 * as a multiple of BlockSize. This is useful for SIMD-friendly processing where the tail
 * lanes of the final block must be materialized, while ordinary single-element indexing
 * remains available for packed ECS maps and swap-pop removal.
 */
template <typename Ty, std::size_t BlockSize, typename Vector = std::vector<Ty>>
class block_vector
{
  static_assert(BlockSize > 0, "BlockSize must be greater than zero");

public:
  using value_type             = Ty;
  using vector_type            = Vector;
  using size_type              = typename vector_type::size_type;
  using difference_type        = typename vector_type::difference_type;
  using reference              = typename vector_type::reference;
  using const_reference        = typename vector_type::const_reference;
  using pointer                = typename vector_type::pointer;
  using const_pointer          = typename vector_type::const_pointer;
  using iterator               = typename vector_type::iterator;
  using const_iterator         = typename vector_type::const_iterator;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr size_type block_size_v = static_cast<size_type>(BlockSize);

  block_vector() = default;

  explicit block_vector(size_type count)
  {
    resize(count);
  }

  block_vector(size_type count, value_type const& value)
  {
    resize(count, value);
  }

  block_vector(std::initializer_list<value_type> values)
  {
    resize(static_cast<size_type>(values.size()));
    std::copy(values.begin(), values.end(), begin());
  }

  [[nodiscard]] auto size() const noexcept -> size_type
  {
    return size_;
  }

  [[nodiscard]] auto storage_size() const noexcept -> size_type
  {
    return static_cast<size_type>(values_.size());
  }

  [[nodiscard]] auto capacity() const noexcept -> size_type
  {
    return storage_size();
  }

  [[nodiscard]] auto allocated_capacity() const noexcept -> size_type
  {
    return static_cast<size_type>(values_.capacity());
  }

  [[nodiscard]] auto block_count() const noexcept -> size_type
  {
    return storage_size() / block_size_v;
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return size_ == 0;
  }

  auto begin() noexcept -> iterator
  {
    return values_.begin();
  }

  auto begin() const noexcept -> const_iterator
  {
    return values_.begin();
  }

  auto cbegin() const noexcept -> const_iterator
  {
    return begin();
  }

  auto end() noexcept -> iterator
  {
    return values_.begin() + static_cast<difference_type>(size_);
  }

  auto end() const noexcept -> const_iterator
  {
    return values_.begin() + static_cast<difference_type>(size_);
  }

  auto cend() const noexcept -> const_iterator
  {
    return end();
  }

  auto rbegin() noexcept -> reverse_iterator
  {
    return reverse_iterator(end());
  }

  auto rbegin() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(end());
  }

  auto crbegin() const noexcept -> const_reverse_iterator
  {
    return rbegin();
  }

  auto rend() noexcept -> reverse_iterator
  {
    return reverse_iterator(begin());
  }

  auto rend() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(begin());
  }

  auto crend() const noexcept -> const_reverse_iterator
  {
    return rend();
  }

  auto operator[](size_type index) noexcept -> reference
  {
    return values_[index];
  }

  auto operator[](size_type index) const noexcept -> const_reference
  {
    return values_[index];
  }

  auto at(size_type index) noexcept -> reference
  {
    OULY_ASSERT(index < size_);
    return values_[index];
  }

  auto at(size_type index) const noexcept -> const_reference
  {
    OULY_ASSERT(index < size_);
    return values_[index];
  }

  auto front() noexcept -> reference
  {
    OULY_ASSERT(size_ > 0);
    return values_.front();
  }

  auto front() const noexcept -> const_reference
  {
    OULY_ASSERT(size_ > 0);
    return values_.front();
  }

  auto back() noexcept -> reference
  {
    OULY_ASSERT(size_ > 0);
    return values_[size_ - 1];
  }

  auto back() const noexcept -> const_reference
  {
    OULY_ASSERT(size_ > 0);
    return values_[size_ - 1];
  }

  auto data() noexcept -> pointer
    requires requires(vector_type& values) { values.data(); }
  {
    return values_.data();
  }

  auto data() const noexcept -> const_pointer
    requires requires(vector_type const& values) { values.data(); }
  {
    return values_.data();
  }

  template <std::size_t I>
  auto data() noexcept -> decltype(auto)
    requires detail::SoaVectorLike<vector_type>
  {
    return values_.template data<I>();
  }

  template <std::size_t I>
  auto data() const noexcept -> decltype(auto)
    requires detail::SoaVectorLike<vector_type>
  {
    return values_.template data<I>();
  }

  auto storage() noexcept -> vector_type&
  {
    return values_;
  }

  auto storage() const noexcept -> vector_type const&
  {
    return values_;
  }

  void reserve(size_type count)
  {
    ensure_storage(count);
  }

  void resize(size_type count)
  {
    resize(count, value_type{});
  }

  void resize(size_type count, value_type const& value)
  {
    auto old_size = size_;
    ensure_storage(count);

    if (count > old_size)
    {
      assign_range(old_size, count, value);
    }
    else if (count < old_size)
    {
      assign_range(count, old_size, value_type{});
      shrink_storage(count);
    }
    size_ = count;
  }

  template <typename... Args>
  auto emplace_back(Args&&... args) -> reference
  {
    ensure_storage(size_ + 1);
    auto index     = size_++;
    values_[index] = value_type{std::forward<Args>(args)...};
    return values_[index];
  }

  void push_back(value_type const& value)
  {
    ensure_storage(size_ + 1);
    values_[size_++] = value;
  }

  void push_back(value_type&& value)
  {
    ensure_storage(size_ + 1);
    values_[size_++] = std::move(value);
  }

  void pop_back()
  {
    OULY_ASSERT(size_ > 0);
    --size_;
    values_[size_] = value_type{};
    shrink_storage(size_);
  }

  void clear() noexcept
  {
    values_.clear();
    size_ = 0;
  }

  void shrink_to_fit()
  {
    shrink_storage(size_);
    values_.shrink_to_fit();
  }

  void swap(block_vector& other) noexcept
  {
    using std::swap;
    swap(values_, other.values_);
    swap(size_, other.size_);
  }

  friend void swap(block_vector& lhs, block_vector& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  [[nodiscard]] static constexpr auto rounded_size(size_type count) noexcept -> size_type
  {
    return count == 0 ? 0 : ((count + block_size_v - 1) / block_size_v) * block_size_v;
  }

private:
  void ensure_storage(size_type count)
  {
    auto required = rounded_size(count);
    if (values_.size() < required)
    {
      values_.resize(required);
    }
  }

  void shrink_storage(size_type count)
  {
    auto required = rounded_size(count);
    if (values_.size() != required)
    {
      values_.resize(required);
    }
  }

  void assign_range(size_type first, size_type last, value_type const& value)
  {
    for (; first < last; ++first)
    {
      values_[first] = value;
    }
  }

  vector_type values_;
  size_type   size_ = 0;
};

} // namespace ouly
