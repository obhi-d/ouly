
#pragma once

namespace acl
{

template <typename size_type = std::size_t>
constexpr size_type default_alignment = 0;

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

template <typename Ty, typename Allocator>
Ty* allocate(Allocator& allocator, typename Allocator::size_type size_in_bytes,
             typename Allocator::size_type alignment = 0)
{
  return reinterpret_cast<Ty*>(allocator.allocate(size_in_bytes, alignment));
}

template <typename Ty, typename Allocator>
void deallocate(Allocator& allocator, Ty* data, typename Allocator::size_type size_in_bytes,
                typename Allocator::size_type alignment = 0)
{
  allocator.deallocate(data, size_in_bytes, alignment);
}

template <typename Ty, typename Allocator>
Ty* allocate_count(Allocator& allocator, typename Allocator::size_type size,
                   typename Allocator::size_type alignment = 0)
{
  return reinterpret_cast<Ty*>(allocator.allocate(size * sizeof(Ty), alignment));
}

template <typename Ty, typename Allocator>
void deallocate_count(Allocator& allocator, Ty* data, typename Allocator::size_type size,
                      typename Allocator::size_type alignment = 0)
{
  allocator.deallocate(data, size * sizeof(Ty), alignment);
}
} // namespace acl
