
#include "default_allocator.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace acl
{

template <typename T, typename Allocator = default_allocator<>>
class dynamic_array : public Allocator
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
    data_  = (T*)Allocator::allocate(sizeof(T) * count_, alignof(T));
    std::uninitialized_copy_n(other.begin(), count_, data_);
    return *this;
  }

  template <typename It>
  inline dynamic_array(It first, It last) noexcept : count_(static_cast<uint32_t>(std::distance(first, last)))
  {
    data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignof(T));
    std::uninitialized_copy_n(first, count_, data_);
  }

  inline dynamic_array(uint32_t n, T const& fill = T()) noexcept : count_(n)
  {
    data_ = (T*)Allocator::allocate(sizeof(T) * count_, alignof(T));
    std::uninitialized_fill_n(data_, count_, fill);
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
      Allocator::deallocate(data_, sizeof(T) * count_, alignof(T));
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
} // namespace acl