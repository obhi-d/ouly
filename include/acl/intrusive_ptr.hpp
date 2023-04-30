
#pragma once

#include <compare>
#include <cassert>

namespace acl
{

template <typename T>
concept ReferenceCounted = requires(T* a) {
                             intrusive_count_add(a);
                             intrusive_count_sub(a);
                             intrusive_count_get(a);
                           };

template <ReferenceCounted T>
class intrusive_ptr
{
public:
  inline constexpr intrusive_ptr() noexcept = default;
  inline constexpr intrusive_ptr(std::nullptr_t) noexcept {}
  inline constexpr intrusive_ptr(T* self) noexcept : self_(self)
  {
    if (self_ != nullptr)
      intrusive_count_sub(self_);
  }
  inline constexpr intrusive_ptr(intrusive_ptr const& rhs) noexcept : intrusive_ptr(rhs.self_) {}
  inline constexpr intrusive_ptr(intrusive_ptr&& rhs) noexcept : self_(rhs.self_) {}
  inline constexpr intrusive_ptr& operator=(intrusive_ptr const& rhs) noexcept 
  {
    if (self_ != nullptr)
      intrusive_count_sub(self_);
    self_ = rhs.self_;
    if (self_ != nullptr)
      intrusive_count_add(self_);
    return *this;
  }
  inline constexpr intrusive_ptr& operator=(T* rhs) noexcept
  {
    if (self_ != nullptr)
      intrusive_count_sub(self_);
    self_ = rhs;
    if (self_ != nullptr)
      intrusive_count_add(self_);
    return *this;
  }
  inline constexpr intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept 
  {
    if (self_ != nullptr)
      intrusive_count_sub(self_);
    self_ = rhs.self_;
    rhs.self_ = nullptr;
  }

  inline constexpr ~intrusive_ptr() noexcept 
  {
    if (self_ != nullptr)
      intrusive_count_sub(self_);
  }

  inline constexpr void reset() noexcept
  {
    if (self_ != nullptr)
    {
      intrusive_count_sub(self_);
      self_ = nullptr;
    }
  }

  inline constexpr T* release() noexcept
  {
    T* r =  self_;
    self_ = nullptr;
    return r;
  }

  inline constexpr void swap(intrusive_ptr& other) noexcept
  {
    std::swap(other.self_, self_);
  }

  inline auto use_count() const noexcept
  {
    return intrusive_count_get(self_);
  }

  inline T* get() const noexcept
  {
    return self_;
  }

  inline operator T* () const noexcept 
  {
    return self_;
  }

  inline T*  operator ->() const noexcept
  {
    assert(self_);
    return self_;
  }
    
  inline operator T&() const noexcept
  {
    assert(self_);
    return *self_;
  }

  constexpr inline bool operator==(std::nullptr_t) const noexcept
  {
    return self_ == nullptr;
  }

  constexpr inline bool operator!=(std::nullptr_t) const noexcept
  {
    return self_ != nullptr;
  }

  inline auto operator <=>(intrusive_ptr const&) const noexcept = default;

private:

  T* self_ = nullptr;
};
} // namespace acl
