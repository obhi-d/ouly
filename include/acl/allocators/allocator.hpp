
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <new>

namespace acl
{
/**
 * @brief Alignment for allocations is passed using tag dispatching. It is strictly a type when the alignment can be
 * determined at compile time, otherwise a generic unsigned value is accepted as a parameter and the alignment type is
 * assumed to be the underlying unsigned type.
 * @example Allocate using alignarg
 * auto pointer = acl::allocate<std::string>(allocator, sizeof(std::string), alignarg<std::string>);
 *
 * @note alignarg is an alias for a constexpr declaration of alignment.
 */
template <std::size_t Value = 0>
struct alignment
{
  static constexpr std::align_val_t value = std::align_val_t{Value};
  constexpr alignment() noexcept          = default;

  constexpr explicit operator bool() const noexcept
  {
    return Value > alignof(void*);
  }

  constexpr operator std::size_t() const noexcept
  {
    return Value;
  }

  static constexpr auto log2() noexcept
  {
    auto constexpr half = Value >> 1;
    return (Value != 0U) ? 1 + alignment<half>::log2() : -1;
  }
};

/** @brief constexpr value of alignment for a given type using alignof */
template <typename T>
constexpr auto alignarg = alignment<alignof(T)>();

/**
 * @brief Allocates memory based on the allocator provided and casts the memory to a type for use. Note that this
 * function **does not** call the constructor for the given type. If the caller needs to call in-place new on the type,
 * it is probaly better to stick to `allocator.allocate(...)`
 */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
[[nodiscard]] auto allocate(Allocator& allocator, typename Allocator::size_type size_in_bytes, Alignment alignment = {})
 -> Ty*
{
  return static_cast<Ty*>(allocator.allocate(size_in_bytes, alignment));
}

/**
 * @brief Allocates zeroed out memory based on the allocator provided and casts the memory to a type for use. Note that
 * this function **does not** call the constructor for the given type. If the caller needs to call in-place new on the
 * type, it is probaly better to stick to `allocator.allocate(...)`
 */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
[[nodiscard]] auto zallocate(Allocator& allocator, typename Allocator::size_type size_in_bytes,
                             Alignment alignment = {}) -> Ty*
{
  return static_cast<Ty*>(allocator.zero_allocate(size_in_bytes, alignment));
}

/** @brief Deallocates memory allocated by allocate or zallocate */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
void deallocate(Allocator& allocator, Ty* data, typename Allocator::size_type size_in_bytes, Alignment alignment = {})
{
  allocator.deallocate(data, size_in_bytes, alignment);
}

/**
 * @brief Allocates memory based on the allocator provided and casts the memory to a type for use. The allocator can be
 * a constant allocator, like the default_allocator. Note that this function **does not** call the constructor for the
 * given type. If the caller needs to call in-place new on the type, it is probaly better to stick to
 * `allocator.allocate(...)`
 */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
[[nodiscard]] auto allocate(Allocator const& allocator, typename Allocator::size_type size_in_bytes,
                            Alignment alignment = {}) -> Ty*
{
  return static_cast<Ty*>(allocator.allocate(size_in_bytes, alignment));
}

/**
 * @brief Allocates zeroed out memory based on the allocator provided and casts the memory to a type for use. The
 * allocator can be a constant allocator, like the default_allocator. Note that this function **does not** call the
 * constructor for the given type. If the caller needs to call in-place new on the type, it is probaly better to stick
 * to `allocator.allocate(...)`
 */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
[[nodiscard]] auto zallocate(Allocator const& allocator, typename Allocator::size_type size_in_bytes,
                             Alignment alignment = {}) -> Ty*
{
  return static_cast<Ty*>(allocator.zero_allocate(size_in_bytes, alignment));
}

/**
 * @brief Deallocates memory allocated by allocate or zallocate. The allocator can be a constant allocator, like the
 * default_allocator */
template <typename Ty, typename Allocator, typename Alignment = alignment<alignof(Ty)>>
void deallocate(Allocator const& allocator, Ty* data, typename Allocator::size_type size_in_bytes,
                Alignment alignment = {})
{
  allocator.deallocate(data, size_in_bytes, alignment);
}

} // namespace acl
