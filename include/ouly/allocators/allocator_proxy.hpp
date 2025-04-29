// SPDX-License-Identifier: MIT

#pragma once
#include <ouly/allocators/allocator.hpp>

namespace ouly
{
template <typename T>
class allocator_proxy
{
public:
  using allocator_type = T;
  using value_type     = typename allocator_type::value_type;
  using size_type      = typename allocator_type::size_type;
  using address        = typename allocator_type::address;

  allocator_proxy() noexcept = default;
  allocator_proxy(allocator_type* i_allocator) noexcept : allocator_(i_allocator) {}
  allocator_proxy(allocator_proxy const& other) noexcept : allocator_(other.allocator_) {}
  allocator_proxy(allocator_proxy&& other) noexcept : allocator_(other.allocator_)
  {
    other.allocator_ = nullptr;
  }
  auto operator=(allocator_proxy const& other) noexcept -> allocator_proxy&
  {
    if (this == &other)
    {
      return *this;
    }
    allocator_ = other.allocator_;
    return *this;
  }
  auto operator=(allocator_proxy&& other) noexcept -> allocator_proxy&
  {
    allocator_       = other.allocator_;
    other.allocator_ = nullptr;
    return *this;
  }
  auto get() const noexcept -> allocator_type*
  {
    return allocator_;
  }

  static constexpr auto null() -> void*
  {
    return nullptr;
  }

  static constexpr auto null_v = nullptr;

  template <typename Alignment = alignment<>>
  [[nodiscard]] auto allocate(size_type i_size, Alignment i_alignment = {}) -> address
  {
    OULY_ASSERT(allocator_ != nullptr);
    return allocator_->allocate(i_size, i_alignment);
  }

  template <typename Alignment = alignment<>>
  [[nodiscard]] auto zero_allocate(size_type i_size, Alignment i_alignment = {}) -> address
  {
    OULY_ASSERT(allocator_ != nullptr);
    return allocator_->zero_allocate(i_size, i_alignment);
  }
  template <typename Alignment = alignment<>>
  void deallocate(address i_data, size_type i_size, Alignment i_alignment = {})
  {
    OULY_ASSERT(allocator_ != nullptr);
    allocator_->deallocate(i_data, i_size, i_alignment);
  }

private:
  allocator_type* allocator_ = nullptr;
};

} // namespace ouly