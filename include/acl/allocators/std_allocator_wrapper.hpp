
#pragma once

#include <acl/allocators/default_allocator.hpp>
#include <acl/allocators/detail/allocator_wrapper.hpp>
#include <acl/utility/type_traits.hpp>

#include <memory_resource>

namespace acl
{

template <typename T, typename UA>
struct allocator_wrapper : public acl::detail::allocator_common<T>, public UA
{
  using typename acl::detail::allocator_common<T>::value_type;
  using typename acl::detail::allocator_common<T>::size_type;
  using typename acl::detail::allocator_common<T>::difference_type;
  using typename acl::detail::allocator_common<T>::reference;
  using typename acl::detail::allocator_common<T>::const_reference;
  using typename acl::detail::allocator_common<T>::pointer;
  using typename acl::detail::allocator_common<T>::const_pointer;

  template <typename U>
  struct rebind
  {
    using other = allocator_wrapper<U, UA>;
  };

  allocator_wrapper() noexcept = default;
  template <typename U>
  allocator_wrapper(allocator_wrapper<U, UA> const& other) : UA((UA const&)other)
  {}
  template <typename U>
  allocator_wrapper(allocator_wrapper<U, UA>&& other) : UA(std::move((UA&)other)) // NOLINT
  {}

  [[nodiscard]] auto allocate(size_type cnt) const -> pointer
  {
    auto ret = static_cast<pointer>(UA::allocate(static_cast<size_type>(sizeof(T) * cnt), alignarg<T>));
    return ret;
  }

  void deallocate(pointer p, size_type cnt) const
  {
    UA::deallocate(p, static_cast<size_type>(sizeof(T) * cnt), alignarg<T>);
  }
};

template <typename T, typename UA>
struct allocator_ref : public acl::detail::allocator_common<T>
{

  using typename acl::detail::allocator_common<T>::value_type;
  using typename acl::detail::allocator_common<T>::size_type;
  using typename acl::detail::allocator_common<T>::difference_type;
  using typename acl::detail::allocator_common<T>::reference;
  using typename acl::detail::allocator_common<T>::const_reference;
  using typename acl::detail::allocator_common<T>::pointer;
  using typename acl::detail::allocator_common<T>::const_pointer;

  template <typename U>
  struct rebind
  {
    using other = allocator_ref<U, UA>;
  };

  allocator_ref() noexcept = default;
  allocator_ref(UA& ref) noexcept : ref_(&ref) {}
  template <typename U>
  allocator_ref(allocator_ref<U, UA>&& ref) noexcept : ref_(std::move(ref.ref_))
  {
    ref.ref_ = nullptr;
  }
  template <typename U>
  auto operator=(allocator_ref<U, UA>&& ref) noexcept -> allocator_ref&
  {
    ref_     = std::move(ref.ref_);
    ref.ref_ = nullptr;
    return *this;
  }
  template <typename U>
  allocator_ref(allocator_ref<U, UA> const& ref) noexcept : ref_(ref.ref_)
  {}
  template <typename U>
  auto operator=(allocator_ref<U, UA> const& ref) noexcept -> allocator_ref&
  {
    ref_ = ref.ref_;
    return *this;
  }

  [[nodiscard]] auto allocate(size_type cnt) const -> pointer
  {
    assert(ref_);
    auto ret = static_cast<pointer>(ref_->allocate(static_cast<size_type>(sizeof(T) * cnt), alignarg<T>));
    return ret;
  }

  void deallocate(pointer p, size_type cnt) const
  {
    assert(ref_);
    ref_->deallocate(p, static_cast<size_type>(sizeof(T) * cnt), alignarg<T>);
  }

  UA* ref_ = nullptr;
};

template <typename UA>
class memory_resource_ref : public std::pmr::memory_resource
{
public:
  memory_resource_ref(const memory_resource_ref&) = delete;
  memory_resource_ref(UA* impl) : impl_(impl) {}
  memory_resource_ref(memory_resource_ref&& other) noexcept : impl_(other.impl_)
  {
    other.impl_ = nullptr;
  }
  ~memory_resource_ref() noexcept override                           = default;
  auto operator=(const memory_resource_ref&) -> memory_resource_ref& = delete;
  auto operator=(memory_resource_ref&& other) noexcept -> memory_resource_ref&
  {
    impl_       = other.impl_;
    other.impl_ = nullptr;
    return *this;
  }

  /**
   * \thread_safe
   */
  [[nodiscard]] auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override
  {
    return impl_->allocate(bytes, alignment);
  }
  /**
   * \thread_safe
   */
  void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
  {
    return impl_->deallocate(ptr, bytes, alignment);
  }
  /**
   * \thread_safe
   */
  [[nodiscard]] auto do_is_equal(const memory_resource& other) const noexcept -> bool override
  {
    // TODO
    auto pother = dynamic_cast<memory_resource_ref<UA> const*>(&other);
    return !static_cast<bool>(!pother || pother->impl_ != impl_);
  }

private:
  UA* impl_;
};

template <typename UA>
class memory_resource : public std::pmr::memory_resource
{
public:
  template <typename... Args>
  memory_resource(Args&&... args) : impl_(std::forward<Args>(args)...)
  {}

  memory_resource(const memory_resource&) = delete;
  memory_resource(memory_resource&& r) noexcept : impl_(std::move(r.impl_)) {}
  ~memory_resource() noexcept override                       = default;
  auto operator=(const memory_resource&) -> memory_resource& = delete;
  auto operator=(memory_resource&& r) noexcept -> memory_resource&
  {
    impl_ = std::move(r.impl_);
    return *this;
  }
  /**
   * \thread_safe
   */
  [[nodiscard]] auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override
  {
    return impl_.allocate(bytes, alignment);
  }
  /**
   * \thread_safe
   */
  void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
  {
    return impl_.deallocate(ptr, bytes, alignment);
  }
  /**
   * \thread_safe
   */
  auto do_is_equal(const memory_resource& other) const noexcept -> bool override
  {
    // TODO
    auto pother = dynamic_cast<memory_resource<UA> const*>(&other);
    return !static_cast<bool>(!pother || &(pother->impl_) != &impl_);
  }

private:
  UA impl_;
};
} // namespace acl
