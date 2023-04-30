#pragma once

#include "default_allocator.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace acl
{
/// @remarks Defines a dynamic non-growable vector like container
template <typename T, typename Allocator = default_allocator<>, uint32_t N = 0>
class dynamic_array;

template <typename T, typename Allocator>
class dynamic_array<T, Allocator, 0> : public Allocator
{

public:
  dynamic_array() = default;

  inline dynamic_array(dynamic_array&& other) noexcept : data_(other.data_), count_(other.count_)
  {
    other.data_  = nullptr;
    other.count_ = 0;
  }

  inline dynamic_array& operator=(dynamic_array&& other) noexcept
  {
    clear();
    data_        = other.data_;
    count_       = other.count_;
    other.data_  = nullptr;
    other.count_ = 0;
    return *this;
  }

  inline dynamic_array(dynamic_array const& other) noexcept : dynamic_array(other.begin(), other.end()) {}

  inline dynamic_array& operator=(dynamic_array const& other) noexcept
  {
    clear();
    count_ = other.count_;
    if (count_)
    {
      data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
      std::uninitialized_copy_n(other.begin(), count_, data_);
    }
    return *this;
  }

  template <typename It>
  inline dynamic_array(It first, It last) noexcept : count_(static_cast<uint32_t>(std::distance(first, last)))
  {
    if (count_)
    {
      data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
      std::uninitialized_copy_n(first, count_, data_);
    }
  }

  inline dynamic_array(uint32_t n, T const& fill = T()) noexcept : count_(n)
  {
    if (count_)
    {
      data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
      std::uninitialized_fill_n(data_, count_, fill);
    }
  }

  inline ~dynamic_array() noexcept
  {
    clear();
  }

  void clear() noexcept
  {
    if (data_)
    {
      if constexpr (!std::is_trivially_destructible_v<T>)
        std::destroy_n(data_, count_);
      Allocator::deallocate(data_, sizeof(T) * count_, alignarg<T>);
      data_  = nullptr;
      count_ = 0;
    }
  }

  T& operator[](size_t i) noexcept
  {
    assert(i < count_);
    return data_[i];
  }

  T const& operator[](size_t i) const noexcept
  {
    assert(i < count_);
    return data_[i];
  }

  T* begin() noexcept
  {
    return data_;
  }

  T* end() noexcept
  {
    return data_ + count_;
  }

  T const* begin() const noexcept
  {
    return data_;
  }

  T const* end() const noexcept
  {
    return data_ + count_;
  }

  T* data() noexcept
  {
    return data_;
  }

  T const* data() const noexcept
  {
    return data_;
  }

  uint32_t size() const noexcept
  {
    return count_;
  }

private:
  T*       data_  = nullptr;
  uint32_t count_ = 0;
};

template <typename T, typename Allocator, uint32_t N>
class dynamic_array : public Allocator
{

public:
  inline static constexpr uint32_t count_ = N;
  static_assert(count_ > 0);

  dynamic_array() = default;

  inline dynamic_array(dynamic_array&& other) noexcept : data_(other.data_)
  {
    other.data_ = nullptr;
  }

  inline dynamic_array& operator=(dynamic_array&& other) noexcept
  {
    clear();
    data_       = other.data_;
    other.data_ = nullptr;
    return *this;
  }

  inline dynamic_array(dynamic_array const& other) noexcept : dynamic_array(other.begin(), other.end()) {}

  inline dynamic_array& operator=(dynamic_array const& other) noexcept
  {
    clear();
    data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
    std::uninitialized_copy_n(other.begin(), count_, data_);
    return *this;
  }

  template <typename It>
  inline dynamic_array(It first, It last) noexcept
  {
    auto count = (static_cast<uint32_t>(std::distance(first, last)));
    data_      = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
    std::uninitialized_copy_n(first, std::min(count, count_), data_);
    if (count < count_)
      std::uninitialized_fill_n(data_ + count, count_ - count, T());
  }

  inline dynamic_array(uint32_t count, T const& fill = T()) noexcept
  {
    data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
    std::uninitialized_fill_n(data_, std::min(count, count_), fill);
    if (count < count_)
      std::uninitialized_fill_n(data_ + count, count_ - count, T());
  }

  inline ~dynamic_array() noexcept
  {
    clear();
  }

  void clear() noexcept
  {
    if (data_)
    {
      if constexpr (!std::is_trivially_destructible_v<T>)
        std::destroy_n(data_, count_);
      Allocator::deallocate(data_, sizeof(T) * count_, alignarg<T>);
      data_ = nullptr;
    }
  }

  T& operator[](size_t i) noexcept
  {
    assert(i < count_);
    return data_[i];
  }

  T const& operator[](size_t i) const noexcept
  {
    assert(i < count_);
    return data_[i];
  }

  T* begin() noexcept
  {
    return data_;
  }

  T* end() noexcept
  {
    return data_ + count_;
  }

  T const* begin() const noexcept
  {
    return data_;
  }

  T const* end() const noexcept
  {
    return data_ + count_;
  }

  T* data() noexcept
  {
    return data_;
  }

  T const* data() const noexcept
  {
    return data_;
  }

  uint32_t size() const noexcept
  {
    return count_;
  }

private:
  T* data_ = nullptr;
};
} // namespace acl
