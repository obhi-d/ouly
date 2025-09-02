// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/utility/user_config.hpp"
#include <compare>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace ouly
{

template <typename T>
concept ReferenceCounted = requires(T* a) {
  // Should return the previous value
  { intrusive_count_add(a) } -> std::integral;
  // Should return the previous value
  { intrusive_count_sub(a) } -> std::integral;
  // Should return the current value
  { intrusive_count_get(a) } -> std::integral;
};

template <typename T, typename Del = std::default_delete<T>>
class intrusive_ptr
{
public:
  using element_type = T;

  constexpr intrusive_ptr() noexcept = default;
  constexpr intrusive_ptr(std::nullptr_t) noexcept {}

  template <typename D>
  explicit constexpr intrusive_ptr(D* self) noexcept
    requires(std::is_base_of_v<T, D>)
      : self_(self)
  {
    static_assert(ReferenceCounted<T>, "Type must be reference counted.");
    if (self_ != nullptr)
    {
      intrusive_count_add(self_);
    }
  }

  template <typename D, typename Del2>
  constexpr intrusive_ptr(ouly::intrusive_ptr<D, Del2> self) noexcept
    requires(std::is_base_of_v<T, D>)
      : self_(self.get())
  {
    static_assert(ReferenceCounted<T>, "Type must be reference counted.");
    self.release();
  }

  explicit constexpr intrusive_ptr(T* self) noexcept : self_(self)
  {
    static_assert(ReferenceCounted<T>, "Type must be reference counted.");
    if (self_ != nullptr)
    {
      intrusive_count_add(self_);
    }
  }
  constexpr intrusive_ptr(intrusive_ptr const& rhs) noexcept : intrusive_ptr(rhs.self_) {}
  constexpr intrusive_ptr(intrusive_ptr&& rhs) noexcept : self_(rhs.self_)
  {
    rhs.self_ = nullptr;
  }

  template <typename D, typename Del2>
  constexpr auto operator=(intrusive_ptr<D, Del2> const& rhs) noexcept
   -> intrusive_ptr& requires(std::is_base_of_v<T, D>) {
     reset(rhs.self_);
     return *this;
   }

  constexpr auto operator=(intrusive_ptr const& rhs) noexcept -> intrusive_ptr&
  {
    if (this != &rhs)
    {
      reset(rhs.self_);
    }
    return *this;
  }

  constexpr auto operator=(T* rhs) noexcept -> intrusive_ptr&
  {
    reset(rhs);
    return *this;
  }

  template <typename D>
  constexpr auto operator=(D* rhs) noexcept -> intrusive_ptr& requires(std::is_base_of_v<T, D>) {
    reset(rhs);
    return *this;
  }

  constexpr auto operator=(intrusive_ptr&& rhs) noexcept -> intrusive_ptr&
  {
    reset();
    self_     = rhs.self_;
    rhs.self_ = nullptr;
    return *this;
  }

  constexpr ~intrusive_ptr() noexcept
  {
    static_assert(ReferenceCounted<T>, "Type must be reference counted.");
    reset();
  }

  constexpr void reset(T* other = nullptr) noexcept
  {
    if (self_ != nullptr && intrusive_count_sub(self_) == 1)
    {
      Del d;
      d(self_);
    }
    self_ = other;
    if (self_)
    {
      intrusive_count_add(self_);
    }
  }

  constexpr auto release() noexcept -> T*
  {
    T* r  = self_;
    self_ = nullptr;
    return r;
  }

  constexpr void swap(intrusive_ptr& other) noexcept
  {
    std::swap(other.self_, self_);
  }

  auto use_count() const noexcept
  {
    return intrusive_count_get(self_);
  }

  auto get() const noexcept -> T*
  {
    return self_;
  }

  operator T*() const noexcept
  {
    return self_;
  }

  auto operator->() const noexcept -> T*
  {
    OULY_ASSERT(self_);
    return self_;
  }

  operator T&() const noexcept
  {
    OULY_ASSERT(self_);
    return *self_;
  }

  constexpr auto operator==(std::nullptr_t) const noexcept -> bool
  {
    return self_ == nullptr;
  }

  constexpr auto operator!=(std::nullptr_t) const noexcept -> bool
  {
    return self_ != nullptr;
  }

  auto operator<=>(intrusive_ptr const&) const noexcept = default;

  template <typename U, template <typename> typename TDeleter>
  friend auto static_pointer_cast(intrusive_ptr<U, TDeleter<U>>&& r) noexcept -> intrusive_ptr<T, TDeleter<T>>
  {
    auto p = static_cast<typename intrusive_ptr<T, TDeleter<T>>::element_type*>(std::move(r).release());
    return intrusive_ptr<T, TDeleter<T>>(std::false_type{}, p);
  }

  template <typename U, typename Deleter>
  friend auto static_pointer_cast(intrusive_ptr<U, Deleter>&& r) noexcept -> intrusive_ptr<T, Deleter>
  {
    auto p = static_cast<typename intrusive_ptr<T, Deleter>::element_type*>(std::move(r).release());
    return intrusive_ptr<T, Deleter>(std::false_type{}, p);
  }

private:
  explicit constexpr intrusive_ptr(std::false_type /*unused*/, T* self) noexcept : self_(self) {}

  T* self_ = nullptr;
};

template <typename T, typename U, template <typename> typename Del>
auto static_pointer_cast(const intrusive_ptr<U, Del<U>>& r) noexcept -> intrusive_ptr<T, Del<T>>
{
  auto p = static_cast<typename intrusive_ptr<T, Del<T>>::element_type*>(r.get());
  return intrusive_ptr<T, Del<T>>(p);
}

template <typename T, typename U, typename Del>
auto static_pointer_cast(const intrusive_ptr<U, Del>& r) noexcept -> intrusive_ptr<T, Del>
{
  auto p = static_cast<typename intrusive_ptr<T, Del>::element_type*>(r.get());
  return intrusive_ptr<T, Del>(p);
}

} // namespace ouly
