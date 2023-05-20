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
template <typename T, typename Allocator = acl::default_allocator<>>
class dynamic_array : public Allocator
{

public:
  using value_type     = T;
  using allocator_type = Allocator;

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

  template <typename OT>
  inline dynamic_array(std::initializer_list<OT> data) noexcept : dynamic_array(data.begin(), data.end())
  {}

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

  inline void resize(uint32_t n, T const& fill = T()) noexcept
  {
    if (n != count_)
    {
      auto data = n > 0 ? (T*)Allocator::allocate(sizeof(T) * n, alignarg<T>) : nullptr;
      std::uninitialized_move_n(begin(), std::min(count_, n), data);
      if (n > count_)
        std::uninitialized_fill_n(data + count_, n - count_, fill);
      clear();
      data_  = data;
      count_ = n;
    }
  }

  inline bool empty() const noexcept
  {
    return count_ == 0;
  }

  inline bool operator==(dynamic_array const& other) const noexcept
  {
    return count_ == other.size() && std::ranges::equal(*this, other);
  }

  inline bool operator!=(dynamic_array const& other) const noexcept
  {
    return !(*this == other);
  }

private:
  T*       data_  = nullptr;
  uint32_t count_ = 0;
};

/// @brief This is a fixed sized array, but dynamically allocated
/// @tparam T Type of the object
/// @tparam Allocator Allocator allocator type
/// @tparam N size of the array
template <typename T, uint32_t N, typename Allocator = acl::default_allocator<>>
class fixed_array : public Allocator
{

public:
  using value_type     = T;
  using allocator_type = Allocator;

  inline static constexpr uint32_t count_ = N;
  static_assert(count_ > 0);

  fixed_array() = default;

  inline fixed_array(fixed_array&& other) noexcept : data_(other.data_)
  {
    other.data_ = nullptr;
  }

  inline fixed_array& operator=(fixed_array&& other) noexcept
  {
    clear();
    data_       = other.data_;
    other.data_ = nullptr;
    return *this;
  }

  inline fixed_array(fixed_array const& other) noexcept : fixed_array(other.begin(), other.end()) {}

  inline fixed_array& operator=(fixed_array const& other) noexcept
  {
    clear();
    data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
    std::uninitialized_copy_n(other.begin(), count_, data_);
    return *this;
  }

  template <typename It>
  inline fixed_array(It first, It last) noexcept
  {
    auto count = (static_cast<uint32_t>(std::distance(first, last)));
    data_      = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
    std::uninitialized_copy_n(first, std::min(count, count_), data_);
    if (count < count_)
      std::uninitialized_fill_n(data_ + count, count_ - count, T());
  }

  template <typename OT>
  inline fixed_array(std::initializer_list<OT> data) noexcept : fixed_array(data.begin(), data.end())
  {}

  inline fixed_array(uint32_t count, T const& fill = T()) noexcept
  {
    data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignarg<T>);
    std::uninitialized_fill_n(data_, std::min(count, count_), fill);
    if (count < count_)
      std::uninitialized_fill_n(data_ + count, count_ - count, T());
  }

  inline ~fixed_array() noexcept
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

  inline bool operator==(fixed_array const& other) const noexcept
  {
    return count_ == other.size() && std::ranges::equal(*this, other);
  }

  inline bool operator!=(fixed_array const& other) const noexcept
  {
    return !(*this == other);
  }

private:
  T* data_ = nullptr;
};
} // namespace acl
