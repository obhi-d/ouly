
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace acl
{

template <std::size_t value_ = 0>
struct alignment
{
  static constexpr auto value = value_;
  inline constexpr alignment() noexcept {}
  inline constexpr operator std::size_t() const noexcept
  {
    return value;
  }

  inline constexpr auto log2() const noexcept
  {
    auto constexpr half = value_ >> 1;
    return value_ ? 1 + alignment<half>::log2() : -1;
  }
};

template <typename T>
concept HasLog2 = requires(T a) {
                    {
                      a.log2()
                      } -> std::same_as<std::size_t>;
                  };

template <typename T>
constexpr auto alignarg = alignment<alignof(T)>();

//! Define Allocator concept
//! template <typename T>
//! concept Allocator = requires(Allocator a) {
//! 		typename Allocator::size_type;
//! 		typename Allocator::address;
//! 		a.allocate(typename Allocator::size_type, Args&&...i_args)->typename
//! Allocator::address; 		a.deallocate(typename Allocator::address, typename
//! Allocator::size_type)->void;
//! }
//!

template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
Ty* allocate(Allocator& allocator, typename Allocator::size_type size_in_bytes, Alignment alignment = {})
{
  return reinterpret_cast<Ty*>(allocator.allocate(size_in_bytes, alignment));
}

template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
Ty* zallocate(Allocator& allocator, typename Allocator::size_type size_in_bytes, Alignment alignment = {})
{
  return reinterpret_cast<Ty*>(allocator.zero_allocate(size_in_bytes, alignment));
}

template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
void deallocate(Allocator& allocator, Ty* data, typename Allocator::size_type size_in_bytes, Alignment alignment = {})
{
  allocator.deallocate(data, size_in_bytes, alignment);
}

} // namespace acl
