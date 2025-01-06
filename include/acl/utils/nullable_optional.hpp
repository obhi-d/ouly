
#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>

#pragma once

namespace acl
{
template <typename T, typename AutoDelete = std::true_type>
struct nullable_optional
{
  nullable_optional() noexcept                                   = default;
  nullable_optional(nullable_optional const&)                    = delete;
  auto operator=(nullable_optional const&) -> nullable_optional& = delete;
  auto operator=(nullable_optional&&) -> nullable_optional&      = default;
  nullable_optional(nullable_optional&&) noexcept                = default;

  ~nullable_optional() noexcept
  {
    if constexpr (AutoDelete::value)
    {
      if (*this)
      {
        reset();
      }
    }
  }

  template <typename... Args>
  void emplace(Args&&... args)
  {
    new (memory()) T(std::forward<Args>(args)...);
  }

  auto get() noexcept -> T&
  {
    return *memory();
  }

  auto get() const noexcept -> T const&
  {
    return *memory();
  }

  explicit operator bool() const noexcept
  {
    return std::any_of((uint8_t*)&bytes_, ((uint8_t*)&bytes_) + sizeof(bytes_),
                       [](uint8_t nonZero)
                       {
                         return nonZero;
                       });
  }

  void reset() noexcept
  {
    memory()->~T();
  }

  auto memory() noexcept -> T*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<T*>(&bytes_);
  }

  auto memory() const noexcept -> T const*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<T const*>(&bytes_);
  }

  alignas(alignof(T)) std::array<uint8_t, sizeof(T)> bytes_ = {};
};
} // namespace acl