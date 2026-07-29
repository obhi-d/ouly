// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/allocators/alignment.hpp"
#include "ouly/utility/user_config.hpp"
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace ouly
{

namespace detail
{
/** @brief Alignment every scratch allocation is made with */
inline constexpr std::size_t scratch_alignment = alignof(std::max_align_t);

/** @brief Allocate from an allocator that either takes an alignment argument or aligns implicitly */
template <typename A>
auto scratch_allocate(A& allocator, std::size_t size) -> void*
{
  if constexpr (requires { allocator.allocate(size, ouly::alignment<scratch_alignment>{}); })
  {
    return allocator.allocate(size, ouly::alignment<scratch_alignment>{});
  }
  else
  {
    return allocator.allocate(size);
  }
}

/** @brief Resize a block of an allocator that either takes an alignment argument or aligns implicitly */
template <typename A>
auto scratch_realloc(A& allocator, void* pointer, std::size_t old_size, std::size_t new_size) -> void*
{
  if constexpr (requires { allocator.realloc(pointer, old_size, new_size, ouly::alignment<scratch_alignment>{}); })
  {
    return allocator.realloc(pointer, old_size, new_size, ouly::alignment<scratch_alignment>{});
  }
  else
  {
    return allocator.realloc(pointer, old_size, new_size);
  }
}
} // namespace detail

/**
 * @brief An allocator usable as a source of short lived scratch memory.
 *
 * The allocator must be able to resize an existing block through `realloc`, growing it in place when
 * the block is the most recent allocation. All linear allocators of the library model this concept.
 *
 * @note Scratch memory is reclaimed in bulk: buffers handed out to `scratch_vector` are never
 * released individually, so the allocator is expected to be a linear/arena allocator that is
 * rewound or destroyed once the work using it is done.
 */
template <typename T>
concept ScratchAllocator = requires(T allocator, void* pointer, std::size_t size) {
  { ouly::detail::scratch_allocate(allocator, size) } -> std::convertible_to<void*>;
  { ouly::detail::scratch_realloc(allocator, pointer, size, size) } -> std::convertible_to<void*>;
};

/**
 * @brief Non-owning view over a `ScratchAllocator`.
 *
 * The view lets code compiled out of line accept any scratch allocator without being a template. It
 * refers to the allocator, so the allocator must outlive every buffer taken from the view.
 */
class scratch_allocator_ref
{
public:
  template <ScratchAllocator A>
    requires(!std::same_as<std::remove_cvref_t<A>, scratch_allocator_ref>)
  // NOLINTNEXTLINE(hicpp-explicit-conversions)
  scratch_allocator_ref(A& allocator) noexcept : instance_(&allocator), vtable_(&vtable_for<A>)
  {}

  /** @brief Allocate `size` bytes aligned to `ouly::detail::scratch_alignment` */
  [[nodiscard]] auto allocate(std::size_t size) const -> void*
  {
    return vtable_->allocate_(instance_, size);
  }

  /** @brief Resize a block previously obtained from this view */
  [[nodiscard]] auto realloc(void* pointer, std::size_t old_size, std::size_t new_size) const -> void*
  {
    return vtable_->realloc_(instance_, pointer, old_size, new_size);
  }

private:
  struct vtable
  {
    void* (*allocate_)(void*, std::size_t)                    = nullptr;
    void* (*realloc_)(void*, void*, std::size_t, std::size_t) = nullptr;
  };

  template <ScratchAllocator A>
  static constexpr vtable vtable_for = {
   .allocate_ = [](void* instance, std::size_t size) -> void*
   {
     return ouly::detail::scratch_allocate(*static_cast<A*>(instance), size);
   },
   .realloc_ = [](void* instance, void* pointer, std::size_t old_size, std::size_t new_size) -> void*
   {
     return ouly::detail::scratch_realloc(*static_cast<A*>(instance), pointer, old_size, new_size);
   }};

  void*         instance_ = nullptr;
  vtable const* vtable_   = nullptr;
};

/**
 * @brief Minimal growable array of trivially copyable objects backed by a scratch allocator.
 *
 * Growth goes through `realloc`, so a linear allocator extends the buffer in place instead of
 * copying it whenever the buffer is still the most recent allocation. The buffer is never released
 * individually; it is reclaimed when the backing scratch allocator is rewound or destroyed. The
 * container is therefore trivially copyable itself and can be nested inside another
 * `scratch_vector`.
 */
template <typename T>
  requires(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
class scratch_vector
{
public:
  using value_type = T;
  using size_type  = std::size_t;

  static constexpr size_type initial_capacity = 8;

  explicit scratch_vector(scratch_allocator_ref scratch) noexcept : scratch_(scratch) {}

  scratch_vector(scratch_allocator_ref scratch, size_type capacity) : scratch_(scratch)
  {
    reserve(capacity);
  }

  /** @brief Make room for at least `capacity` objects without changing the size */
  void reserve(size_type capacity)
  {
    if (capacity <= capacity_)
    {
      return;
    }

    auto const new_bytes = capacity * sizeof(T);
    auto*      buffer =
     data_ == nullptr ? scratch_.allocate(new_bytes) : scratch_.realloc(data_, capacity_ * sizeof(T), new_bytes);
    data_     = static_cast<T*>(buffer);
    capacity_ = capacity;
  }

  void push_back(T const& value)
  {
    reserve_for(size_ + 1);
    data_[size_++] = value;
  }

  /** @brief Insert `value` before position `index`, shifting the tail up */
  void insert(size_type index, T const& value)
  {
    OULY_ASSERT(index <= size_);
    reserve_for(size_ + 1);
    if (index != size_)
    {
      std::memmove(data_ + index + 1, data_ + index, (size_ - index) * sizeof(T));
    }
    data_[index] = value;
    ++size_;
  }

  /** @brief Remove the object at position `index`, shifting the tail down */
  void erase(size_type index)
  {
    OULY_ASSERT(index < size_);
    --size_;
    if (index != size_)
    {
      std::memmove(data_ + index, data_ + index + 1, (size_ - index) * sizeof(T));
    }
  }

  void clear() noexcept
  {
    size_ = 0;
  }

  auto operator[](size_type index) -> T&
  {
    OULY_ASSERT(index < size_);
    return data_[index];
  }

  auto operator[](size_type index) const -> T const&
  {
    OULY_ASSERT(index < size_);
    return data_[index];
  }

  [[nodiscard]] auto size() const noexcept -> size_type
  {
    return size_;
  }

  [[nodiscard]] auto capacity() const noexcept -> size_type
  {
    return capacity_;
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return size_ == 0;
  }

  [[nodiscard]] auto data() noexcept -> T*
  {
    return data_;
  }

  [[nodiscard]] auto data() const noexcept -> T const*
  {
    return data_;
  }

  [[nodiscard]] auto begin() noexcept -> T*
  {
    return data_;
  }

  [[nodiscard]] auto end() noexcept -> T*
  {
    return data_ + size_;
  }

  [[nodiscard]] auto begin() const noexcept -> T const*
  {
    return data_;
  }

  [[nodiscard]] auto end() const noexcept -> T const*
  {
    return data_ + size_;
  }

private:
  void reserve_for(size_type required)
  {
    if (required <= capacity_)
    {
      return;
    }
    reserve(capacity_ == 0 ? std::max(required, initial_capacity) : std::max(required, capacity_ * 2));
  }

  scratch_allocator_ref scratch_;
  T*                    data_     = nullptr;
  size_type             size_     = 0;
  size_type             capacity_ = 0;
};

} // namespace ouly
