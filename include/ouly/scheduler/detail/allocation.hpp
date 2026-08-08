// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/utility/user_config.hpp"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace ouly
{

/**
 * @brief Non-owning, type-erased allocation source used by scheduler task state and coroutine frames.
 *
 * The referenced allocator must outlive every allocation made through this object. Allocators only
 * need allocate(size); deallocate(pointer, size) is used when available and may return either void
 * or a status value. Individual allocations carry their source, so destruction does not need the
 * original scheduler_allocator object.
 */
class scheduler_allocator
{
public:
  scheduler_allocator() noexcept = default;

  template <typename Allocator>
    requires requires(Allocator& allocator, std::size_t size) {
      { allocator.allocate(size) } -> std::convertible_to<void*>;
    }
  scheduler_allocator(Allocator& allocator) noexcept
      : instance_(&allocator), allocate_(&allocate_from<Allocator>), deallocate_(&deallocate_from<Allocator>)
  {}

  [[nodiscard]] auto allocate_bytes(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) const -> void*
  {
    OULY_ASSERT(std::has_single_bit(alignment));
    if (!std::has_single_bit(alignment))
    {
      throw std::bad_alloc();
    }
    alignment = std::max(alignment, alignof(allocation_header));

    auto const overhead = alignment - 1U + sizeof(allocation_header);
    if (size > std::numeric_limits<std::size_t>::max() - overhead)
    {
      throw std::bad_alloc();
    }
    auto const total = size + overhead;
    auto*      base  = static_cast<std::byte*>(allocate_raw(total));
    if (base == nullptr)
    {
      throw std::bad_alloc();
    }

    auto const begin   = reinterpret_cast<std::uintptr_t>(base + sizeof(allocation_header));
    auto const aligned = (begin + alignment - 1U) & ~(static_cast<std::uintptr_t>(alignment) - 1U);
    auto*      result  = reinterpret_cast<void*>(aligned);
    auto*      header  = reinterpret_cast<allocation_header*>(aligned - sizeof(allocation_header));
    std::construct_at(
     header, allocation_header{.instance_ = instance_, .deallocate_ = deallocate_, .base_ = base, .size_ = total});
    return result;
  }

  static void deallocate_bytes(void* ptr) noexcept
  {
    if (ptr == nullptr)
    {
      return;
    }

    auto const address    = reinterpret_cast<std::uintptr_t>(ptr);
    auto*      header     = reinterpret_cast<allocation_header*>(address - sizeof(allocation_header));
    auto*      instance   = header->instance_;
    auto       deallocate = header->deallocate_;
    auto*      base       = header->base_;
    auto const total      = header->size_;
    std::destroy_at(header);
    if (deallocate != nullptr)
    {
      deallocate(instance, base, total);
    }
    else
    {
      ::operator delete(base);
    }
  }

  template <typename T, typename... Args>
  [[nodiscard]] auto make(Args&&... args) const -> T*
  {
    auto* memory = allocate_bytes(sizeof(T), alignof(T));
    try
    {
      return std::construct_at(static_cast<T*>(memory), std::forward<Args>(args)...);
    }
    catch (...)
    {
      deallocate_bytes(memory);
      throw;
    }
  }

  template <typename T>
  static void destroy(T* object) noexcept
  {
    if (object == nullptr)
    {
      return;
    }
    std::destroy_at(object);
    deallocate_bytes(object);
  }

private:
  using allocate_fn   = auto (*)(void*, std::size_t) -> void*;
  using deallocate_fn = void (*)(void*, void*, std::size_t) noexcept;

  // Only what deallocate_bytes() needs: the allocation source and the raw block to hand back.
  struct allocation_header
  {
    void*         instance_   = nullptr;
    deallocate_fn deallocate_ = nullptr;
    void*         base_       = nullptr;
    std::size_t   size_       = 0;
  };

  template <typename Allocator>
  static auto allocate_from(void* instance, std::size_t size) -> void*
  {
    return static_cast<Allocator*>(instance)->allocate(size);
  }

  template <typename Allocator>
  static void deallocate_from([[maybe_unused]] void* instance, [[maybe_unused]] void* ptr,
                              [[maybe_unused]] std::size_t size) noexcept
  {
    if constexpr (requires(Allocator& allocator) { allocator.deallocate(ptr, size); })
    {
      static_cast<void>(static_cast<Allocator*>(instance)->deallocate(ptr, size));
    }
  }

  [[nodiscard]] auto allocate_raw(std::size_t size) const -> void*
  {
    if (allocate_ != nullptr)
    {
      return allocate_(instance_, size);
    }
    return ::operator new(size);
  }

  void*         instance_   = nullptr;
  allocate_fn   allocate_   = nullptr;
  deallocate_fn deallocate_ = nullptr;
};

} // namespace ouly
