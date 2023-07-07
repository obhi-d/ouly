export module acl.utils:intrusive_ptr;

import <cassert>;
import <compare>;
import <concepts>;

export namespace acl
{

// clang-format off
template <typename T>
concept ReferenceCounted = requires(T* a) {
                             // Should return the previous value
                             {intrusive_count_add(a)}->std::integral;
                             // Should return the previous value
                             {intrusive_count_sub(a)}->std::integral;
                             // Should return the current value
                             {intrusive_count_get(a)}->std::integral;
                           };
// clang-format on

template <typename T>
class intrusive_ptr
{
public:
  using element_type = T;

  inline constexpr intrusive_ptr() noexcept = default;
  inline constexpr intrusive_ptr(std::nullptr_t) noexcept {}
  inline explicit constexpr intrusive_ptr(T* self) noexcept : self_(self)
  {
    static_assert(ReferenceCounted<T>, "Type must be reference counted.");
    if (self_ != nullptr)
      intrusive_count_add(self_);
  }
  inline constexpr intrusive_ptr(intrusive_ptr const& rhs) noexcept : intrusive_ptr(rhs.self_) {}
  inline constexpr intrusive_ptr(intrusive_ptr&& rhs) noexcept : self_(rhs.self_)
  {
    rhs.self_ = nullptr;
  }

  inline constexpr intrusive_ptr& operator=(intrusive_ptr const& rhs) noexcept
  {
    reset(rhs.self_);
    return *this;
  }
  inline constexpr intrusive_ptr& operator=(T* rhs) noexcept
  {
    reset(rhs);
    return *this;
  }
  inline constexpr intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept
  {
    reset();
    self_     = rhs.self_;
    rhs.self_ = nullptr;
    return *this;
  }

  inline constexpr ~intrusive_ptr() noexcept
  {
    static_assert(ReferenceCounted<T>, "Type must be reference counted.");
    reset();
  }

  inline constexpr void reset(T* other = nullptr) noexcept
  {
    if (self_ != nullptr && intrusive_count_sub(self_) == 1)
    {
      delete self_;
    }
    self_ = other;
    if (self_)
      intrusive_count_add(self_);
  }

  inline constexpr T* release() noexcept
  {
    T* r  = self_;
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

  inline operator T*() const noexcept
  {
    return self_;
  }

  inline T* operator->() const noexcept
  {
    ACL_ASSERT(self_);
    return self_;
  }

  inline operator T&() const noexcept
  {
    ACL_ASSERT(self_);
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

  inline auto operator<=>(intrusive_ptr const&) const noexcept = default;

private:
  T* self_ = nullptr;
};

template <typename T, typename U>
intrusive_ptr<T> static_pointer_cast(const intrusive_ptr<U>& r) noexcept
{
  auto p = static_cast<typename intrusive_ptr<T>::element_type*>(r.get());
  return intrusive_ptr<T>(p);
}

} // namespace acl
