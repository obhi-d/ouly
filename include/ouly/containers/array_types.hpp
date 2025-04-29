// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/default_allocator.hpp"
#include <algorithm>
#include "ouly/utility/user_config.hpp"
#include <cstdint>
#include <memory>
#include <type_traits>

namespace ouly
{
/**
 * @remarks Defines a dynamic non-growable vector like container
 */
template <typename T, typename Allocator = ouly::default_allocator<>>
class dynamic_array : public Allocator
{

public:
  using value_type     = T;
  using allocator_type = Allocator;

  dynamic_array() = default;

  dynamic_array(dynamic_array&& other) noexcept : data_(other.data_), count_(other.count_)
  {
    other.data_  = nullptr;
    other.count_ = 0;
  }

  auto operator=(dynamic_array&& other) noexcept -> dynamic_array&
  {
    if (this == &other)
    {
      return *this;
    }
    clear();
    data_        = other.data_;
    count_       = other.count_;
    other.data_  = nullptr;
    other.count_ = 0;
    return *this;
  }

  dynamic_array(dynamic_array const& other) noexcept : dynamic_array(other.begin(), other.end()) {}

  auto operator=(dynamic_array const& other) noexcept -> dynamic_array&
  {
    if (this == &other)
    {
      return *this;
    }

    clear();
    count_ = other.count_;
    if (count_ != 0U)
    {
      data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
      std::uninitialized_copy_n(other.begin(), count_, data_);
    }
    return *this;
  }

  template <typename It>
  dynamic_array(It first, It last) noexcept : count_(static_cast<uint32_t>(std::distance(first, last)))
  {
    if (count_)
    {
      data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
      std::uninitialized_copy_n(first, count_, data_);
    }
  }

  template <typename OT>
  dynamic_array(std::initializer_list<OT> data) noexcept : dynamic_array(data.begin(), data.end())
  {}

  dynamic_array(uint32_t n, T const& fill = T()) noexcept : count_(n)
  {
    if (count_ != 0U)
    {
      data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
      std::uninitialized_fill_n(data_, count_, fill);
    }
  }

  ~dynamic_array() noexcept
  {
    clear();
  }

  void clear() noexcept
  {
    if (data_)
    {
      if constexpr (!std::is_trivially_destructible_v<T>)
      {
        std::destroy_n(data_, count_);
      }
      Allocator::deallocate(data_, sizeof(T) * count_, alignarg<T>);
      data_  = nullptr;
      count_ = 0;
    }
  }

  auto operator[](std::size_t i) noexcept -> T&
  {
    OULY_ASSERT(i < count_);
    return data_[i];
  }

  auto operator[](std::size_t i) const noexcept -> T const&
  {
    OULY_ASSERT(i < count_);
    return data_[i];
  }

  auto begin() noexcept -> T*
  {
    return data_;
  }

  auto end() noexcept -> T*
  {
    return data_ + count_;
  }

  auto begin() const noexcept -> T const*
  {
    return data_;
  }

  auto end() const noexcept -> T const*
  {
    return data_ + count_;
  }

  auto data() noexcept -> T*
  {
    return data_;
  }

  auto data() const noexcept -> T const*
  {
    return data_;
  }

  [[nodiscard]] auto size() const noexcept -> uint32_t
  {
    return count_;
  }

  void resize(uint32_t n, T const& fill = T()) noexcept
  {
    if (n != count_)
    {
      auto data = n > 0 ? (T*)Allocator::allocate(sizeof(T) * n, alignarg<T>) : nullptr;
      std::uninitialized_move_n(begin(), std::min(count_, n), data);
      if (n > count_)
      {
        std::uninitialized_fill_n(data + count_, n - count_, fill);
      }
      clear();
      data_  = data;
      count_ = n;
    }
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return count_ == 0;
  }

  auto operator==(dynamic_array const& other) const noexcept -> bool
  {
    return count_ == other.size() && std::ranges::equal(*this, other);
  }

  auto operator!=(dynamic_array const& other) const noexcept -> bool
  {
    return !(*this == other);
  }

private:
  T*       data_  = nullptr;
  uint32_t count_ = 0;
};

/**
 * @brief This is a fixed sized array, but dynamically allocated
 * @tparam T Type of the object
 * @tparam Allocator Allocator allocator type
 * @tparam N size of the array
 */
template <typename T, uint32_t N, typename Allocator = ouly::default_allocator<>>
class fixed_array : public Allocator
{

public:
  using value_type     = T;
  using allocator_type = Allocator;

  static constexpr uint32_t count = N;
  static_assert(count > 0);

  fixed_array() = default;

  fixed_array(fixed_array&& other) noexcept : data_(other.data_)
  {
    other.data_ = nullptr;
  }

  auto operator=(fixed_array&& other) noexcept -> fixed_array&
  {
    clear();
    data_       = other.data_;
    other.data_ = nullptr;
    return *this;
  }

  fixed_array(fixed_array const& other) noexcept : fixed_array(other.begin(), other.end()) {}

  auto operator=(fixed_array const& other) noexcept -> fixed_array&
  {
    if (this == &other)
    {
      return *this;
    }
    clear();
    data_ = (T*)Allocator::allocate(sizeof(T) * count, alignarg<T>);
    std::uninitialized_copy_n(other.begin(), count, data_);
    return *this;
  }

  template <typename It>
  fixed_array(It first, It last) noexcept : data_((T*)Allocator::allocate(sizeof(T) * count, alignarg<T>))
  {
    auto assign_count = (static_cast<uint32_t>(std::distance(first, last)));

    std::uninitialized_copy_n(first, std::min(assign_count, count), data_);
    if (assign_count < count)
    {
      std::uninitialized_fill_n(data_ + count, count - assign_count, T());
    }
  }

  template <typename OT>
  fixed_array(std::initializer_list<OT> data) noexcept : fixed_array(data.begin(), data.end())
  {}

  fixed_array(uint32_t fill_count, T const& fill = T()) noexcept
      : data_((T*)Allocator::allocate(sizeof(T) * count, alignarg<T>))
  {

    std::uninitialized_fill_n(data_, std::min(fill_count, count), fill);
    if (fill_count < count)
    {
      std::uninitialized_fill_n(data_ + count, count - fill_count, T());
    }
  }

  ~fixed_array() noexcept
  {
    clear();
  }

  void clear() noexcept
  {
    if (data_)
    {
      if constexpr (!std::is_trivially_destructible_v<T>)
      {
        std::destroy_n(data_, count);
      }
      Allocator::deallocate(data_, sizeof(T) * count, alignarg<T>);
      data_ = nullptr;
    }
  }

  auto operator[](std::size_t i) noexcept -> T&
  {
    OULY_ASSERT(i < count);
    return data_[i];
  }

  auto operator[](std::size_t i) const noexcept -> T const&
  {
    OULY_ASSERT(i < count);
    return data_[i];
  }

  auto begin() noexcept -> T*
  {
    return data_;
  }

  auto end() noexcept -> T*
  {
    return data_ + count;
  }

  auto begin() const noexcept -> T const*
  {
    return data_;
  }

  auto end() const noexcept -> T const*
  {
    return data_ + count;
  }

  auto data() noexcept -> T*
  {
    return data_;
  }

  auto data() const noexcept -> T const*
  {
    return data_;
  }

  [[nodiscard]] auto size() const noexcept -> uint32_t
  {
    return count;
  }

  auto operator==(fixed_array const& other) const noexcept -> bool
  {
    return count == other.size() && std::ranges::equal(*this, other);
  }

  auto operator!=(fixed_array const& other) const noexcept -> bool
  {
    return !(*this == other);
  }

private:
  T* data_ = nullptr;
};
} // namespace ouly
