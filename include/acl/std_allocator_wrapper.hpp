
#pragma once

#include <memory>
#include <memory_resource>

namespace acl
{
namespace detail
{

template <typename T, typename UA, bool static_ua>
struct std_allocator_wrapper;

template <typename T, typename UA>
class std_allocator_wrapper<T, UA, false> : public std::allocator<T>
{
public:
  //    typedefs
  using pointer       = typename std::allocator_traits<std::allocator<T>>::pointer;
  using const_pointer = typename std::allocator_traits<std::allocator<T>>::const_pointer;
  using size_type     = typename std::allocator_traits<std::allocator<T>>::size_type;
  using value_type    = typename std::allocator_traits<std::allocator<T>>::value_type;

  using underlying_size_t = typename UA::size_type;

public:
  //    convert an allocator<T> to allocator<U>

  template <typename U>
  struct rebind
  {
    typedef std_allocator_wrapper<U, UA, false> other;
  };

public:
  inline explicit std_allocator_wrapper(UA& impl) : impl_(&impl) {}

  inline ~std_allocator_wrapper() {}

  /*	inline explicit std_allocator_wrapper(std_allocator_wrapper const&) {}*/
  template <typename U>
  inline std_allocator_wrapper(std_allocator_wrapper<U, UA, false> const& other) : impl_(other.get_impl())
  {}

  //    memory allocation

  inline pointer allocate(size_type cnt, const_pointer = 0) const
  {
    if constexpr (alignof(T) > alignof(void*))
    {
      pointer ret =
        reinterpret_cast<pointer>(impl_->allocate(static_cast<underlying_size_t>(sizeof(T) * cnt), alignof(T)));
      return ret;
    }
    else
    {
      pointer ret = reinterpret_cast<pointer>(impl_->allocate(static_cast<underlying_size_t>(sizeof(T) * cnt)));
      return ret;
    }
  }

  inline void deallocate(pointer p, size_type cnt) const
  {
    if constexpr (alignof(T) > alignof(void*))
      impl_->deallocate(p, static_cast<underlying_size_t>(sizeof(T) * cnt), alignof(T));
    else
      impl_->deallocate(p, static_cast<underlying_size_t>(sizeof(T) * cnt));
  }
  //    construction/destruction

  template <typename... Args>
  inline void construct(pointer p, Args&&... args) const
  {
    new (p) T(std::forward<Args>(args)...);
  }

  inline void destroy(pointer p) const
  {
    reinterpret_cast<T*>(p)->~T();
  }
  inline bool operator==(const std_allocator_wrapper& x)
  {
    return impl_ == x.impl_;
  }
  inline bool operator!=(const std_allocator_wrapper& x)
  {
    return impl_ != x.impl_;
  }

  UA* get_impl() const
  {
    return impl_;
  }

protected:
  UA* impl_;
};

template <typename T, typename UA>
class std_allocator_wrapper<T, UA, true> : public std::allocator<T>, public UA
{
public:
  //    typedefs
  using pointer       = typename std::allocator_traits<std::allocator<T>>::pointer;
  using const_pointer = typename std::allocator_traits<std::allocator<T>>::const_pointer;
  using size_type     = typename std::allocator_traits<std::allocator<T>>::size_type;
  using value_type    = typename std::allocator_traits<std::allocator<T>>::value_type;

  using underlying_size_t = typename UA::size_type;

public:
  //    convert an allocator<T> to allocator<U>

  template <typename U>
  struct rebind
  {
    typedef std_allocator_wrapper<U, UA, true> other;
  };

public:
  inline std_allocator_wrapper() {}

  inline explicit std_allocator_wrapper(UA const& impl) : UA(impl) {}
  inline std_allocator_wrapper(std_allocator_wrapper<T, UA, true> const& impl) : UA((UA const&)impl) {}

  inline ~std_allocator_wrapper() {}

  /*	inline explicit std_allocator_wrapper(std_allocator_wrapper const&) {}*/
  template <typename U>
  inline std_allocator_wrapper(std_allocator_wrapper<U, UA, true> const& other) : UA((UA&)other)
  {}

  //    memory allocation

  inline pointer allocate(size_type cnt, const_pointer = 0) const
  {
    if constexpr (alignof(T) > alignof(void*))
    {
      pointer ret =
        reinterpret_cast<pointer>(UA::allocate(static_cast<underlying_size_t>(sizeof(T) * cnt), alignof(T)));
      return ret;
    }
    else
    {
      pointer ret = reinterpret_cast<pointer>(UA::allocate(static_cast<underlying_size_t>(sizeof(T) * cnt)));
      return ret;
    }
  }

  inline void deallocate(pointer p, size_type cnt) const
  {
    if constexpr (alignof(T) > alignof(void*))
      UA::deallocate(p, static_cast<underlying_size_t>(sizeof(T) * cnt), alignof(T));
    else
      UA::deallocate(p, static_cast<underlying_size_t>(sizeof(T) * cnt));
  }
  //    construction/destruction

  template <typename... Args>
  inline void construct(pointer p, Args&&... args) const
  {
    new (p) T(std::forward<Args>(args)...);
  }

  inline void destroy(pointer p) const
  {
    reinterpret_cast<T*>(p)->~T();
  }
  inline bool operator==(const std_allocator_wrapper& x)
  {
    return true;
  }
  inline bool operator!=(const std_allocator_wrapper& x)
  {
    return false;
  }
};

} // namespace detail

template <typename T, typename UA>
class std_allocator_wrapper : public detail::std_allocator_wrapper<T, UA, traits::is_static_v<traits::tag_t<UA>>>
{
public:
  //    typedefs
  using base_type     = detail::std_allocator_wrapper<T, UA, traits::is_static_v<traits::tag_t<UA>>>;
  using pointer       = typename std::allocator_traits<std::allocator<T>>::pointer;
  using const_pointer = typename std::allocator_traits<std::allocator<T>>::const_pointer;
  using size_type     = typename std::allocator_traits<std::allocator<T>>::size_type;
  using value_type    = typename std::allocator_traits<std::allocator<T>>::value_type;

  using underlying_size_t = typename UA::size_type;

  std_allocator_wrapper() = default;

  template <typename... Args>
  inline std_allocator_wrapper(Args&&... args) : base_type(std::forward<Args>(args)...)
  {}
};

template <typename T, typename UA>
auto make_std_allocator(UA& impl)
{
  return std_allocator_wrapper<T, UA>(impl);
}

template <typename T, typename UA>
auto make_std_allocator()
{
  return std_allocator_wrapper<T, UA>();
}

template <typename UA>
class std_memory_resource : public std::pmr::memory_resource
{
public:
  std_memory_resource(UA* impl) : impl_(impl) {}

  /// \thread_safe
  inline void* do_allocate(std::size_t bytes, std::size_t alignment) override
  {
    return impl_->allocate(bytes, alignment);
  }
  /// \thread_safe
  inline void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
  {
    return impl_->deallocate(ptr, bytes, alignment);
  }
  /// \thread_safe
  inline bool do_is_equal(const memory_resource& other) const noexcept override
  {
    // TODO
    auto pother = dynamic_cast<std_memory_resource<UA> const*>(&other);
    if (!pother || pother->impl_ != impl_)
      return false;
    return true;
  }

private:
  UA* impl_;
};

} // namespace acl