
#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>

#pragma once

namespace acl
{
template <typename T, typename auto_delete = std::true_type>
struct nullable_optional
{
  ~nullable_optional() noexcept
  {
    if constexpr (auto_delete::value)
    {
      if (*this)
        reset();
    }
  }

  template <typename... Args>
  inline void emplace(Args&&... args)
  {
    new (memory()) T(std::forward<Args>(args)...);
  }

  inline T& get() noexcept
  {
    return *memory();
  }

  inline T const& get() const noexcept
  {
    return *memory();
  }

  inline explicit operator bool() const noexcept
  {
    return std::any_of((uint8_t*)&bytes_, ((uint8_t*)&bytes_) + sizeof(bytes_),
                       [](uint8_t nonZero)
                       {
                         return nonZero;
                       });
  }

  inline void reset() noexcept
  {
    memory()->~T();
  }

  inline T* memory() noexcept
  {
    return reinterpret_cast<T*>(&bytes_);
  }

  inline T const* memory() const noexcept
  {
    return reinterpret_cast<T const*>(&bytes_);
  }

  alignas(alignof(T)) std::array<uint8_t, sizeof(T)> bytes_ = {};
};
} // namespace acl