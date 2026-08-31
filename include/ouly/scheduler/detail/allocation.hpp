// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/alignment.hpp"
#include "ouly/utility/user_config.hpp"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#ifdef OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR
#include <memory_resource>
#endif
#include <new>
#include <type_traits>
#include <utility>

namespace ouly
{

namespace detail
{

/**
 * @brief Scheduler-owned allocation policy for internal task and continuation nodes.
 *
 * Internal execution nodes have lifetimes governed by scheduler queues rather than user-visible
 * task handles. Keeping their blocks in the scheduler makes that lifetime explicit. It also keeps
 * a cooperative task-scope join safe when an already-claimed node remains referenced by a queued
 * delegate. The default policy uses new/delete; the optional synchronized PMR policy provides
 * size-segregated reuse for differently sized lambda captures and permits nodes to be returned by
 * any worker.
 */
class scheduler_node_pool
{
public:
  scheduler_node_pool() noexcept                                     = default;
  scheduler_node_pool(scheduler_node_pool const&)                    = delete;
  auto operator=(scheduler_node_pool const&) -> scheduler_node_pool& = delete;
  scheduler_node_pool(scheduler_node_pool&&)                         = delete;
  auto operator=(scheduler_node_pool&&) -> scheduler_node_pool&      = delete;
  ~scheduler_node_pool() noexcept                                    = default;

  template <typename T, typename... Args>
  [[nodiscard]] auto make(Args&&... args) -> T*
  {
#ifdef OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR
    auto* memory = resource_.allocate(sizeof(T), alignof(T));
    try
    {
      return std::construct_at(static_cast<T*>(memory), std::forward<Args>(args)...);
    }
    catch (...)
    {
      resource_.deallocate(memory, sizeof(T), alignof(T));
      throw;
    }
#else
    void* memory = allocate_global<T>();
    try
    {
      return std::construct_at(static_cast<T*>(memory), std::forward<Args>(args)...);
    }
    catch (...)
    {
      deallocate_global<T>(memory);
      throw;
    }
#endif
  }

  template <typename T>
#ifdef OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR
  void destroy(T* object) noexcept
  {
    std::destroy_at(object);
    resource_.deallocate(object, sizeof(T), alignof(T));
  }
#else
  static void destroy(T* object) noexcept
  {
    std::destroy_at(object);
    deallocate_global<T>(object);
  }
#endif

private:
#ifdef OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR
  std::pmr::synchronized_pool_resource resource_;
#else
  template <typename T>
  [[nodiscard]] static auto allocate_global() -> void*
  {
    if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    {
      return ::operator new(sizeof(T), std::align_val_t{alignof(T)});
    }
    else
    {
      return ::operator new(sizeof(T));
    }
  }

  template <typename T>
  static void deallocate_global(void* memory) noexcept
  {
    if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    {
      ::operator delete(memory, std::align_val_t{alignof(T)});
    }
    else
    {
      ::operator delete(memory);
    }
  }
#endif
};

/** @brief Remembers pooled storage ownership only when the PMR node allocator is enabled. */
class scheduler_node_owner
{
protected:
  scheduler_node_owner() noexcept = default;

#ifdef OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR
  explicit scheduler_node_owner(scheduler_node_pool& pool) noexcept : pool_(&pool) {}

  template <typename T>
  void release_node(T* object) noexcept
  {
    OULY_ASSERT(pool_ != nullptr);
    pool_->destroy(object);
  }

private:
  scheduler_node_pool* pool_ = nullptr;
#else
  explicit scheduler_node_owner([[maybe_unused]] scheduler_node_pool& pool) noexcept {}

  template <typename T>
  static void release_node(T* object) noexcept
  {
    scheduler_node_pool::destroy(object);
  }
#endif
};

} // namespace detail

/**
 * @brief An allocator that can honour an alignment that is only known at runtime
 *
 * The alignment travels as a `std::align_val_t`, which every ouly allocator accepts in place of the
 * compile time `ouly::alignment<N>` tag.
 */
template <typename Allocator>
concept AlignedSchedulerAllocator = requires(Allocator& allocator, std::size_t size, std::align_val_t align) {
  { allocator.allocate(size, align) } -> std::convertible_to<void*>;
};

/**
 * @brief Non-owning, type-erased allocation source used by scheduler task state and coroutine frames.
 *
 * The referenced allocator must outlive every allocation made through this object. Allocators only
 * need allocate(size); an allocate(size, std::align_val_t) overload is used when available and lets
 * the request be satisfied without reserving any padding of our own. deallocate(pointer, size) -
 * with a trailing alignment when the allocator takes one - is used when available and may return
 * either void or a status value. Individual allocations carry their source, so destruction does not
 * need the original scheduler_allocator object.
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
      : instance_(&allocator), allocate_(&allocate_from<Allocator>), deallocate_(&deallocate_from<Allocator>),
        guaranteed_alignment_(guaranteed_alignment_of<Allocator>),
        honors_alignment_(AlignedSchedulerAllocator<Allocator>)
  {}

  /**
   * @brief Allocate `size` bytes aligned to `alignment`
   *
   * The alignment is handed to the source allocator, so no slack has to be reserved to align by
   * hand; the payload simply starts one header past the block. Only a source that can neither
   * honour the alignment nor guarantee it falls back to over-allocating by `alignment - 1`.
   */
  [[nodiscard]] auto allocate_bytes(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) const -> void*
  {
    OULY_ASSERT(std::has_single_bit(alignment));
    if (!std::has_single_bit(alignment))
    {
      throw std::bad_alloc();
    }
    // the header sits right before the payload, so the payload can never be less aligned than it
    alignment = std::max(alignment, alignof(allocation_header));

    // The source hands out blocks aligned to `guaranteed_alignment_` on its own, and honours
    // anything beyond that when it accepts an alignment of its own
    bool const already_aligned = alignment <= guaranteed_alignment_;
    bool const exact           = already_aligned || honors_alignment();
    // nothing to ask for when the block the source hands out is aligned enough anyway
    bool const tell_source = exact && !already_aligned;
    // when the block is aligned already the payload only has to clear the header
    auto const overhead = exact ? ouly::detail::align_size(sizeof(allocation_header), alignment)
                                : alignment - 1U + sizeof(allocation_header);
    if (size > std::numeric_limits<std::size_t>::max() - overhead)
    {
      throw std::bad_alloc();
    }

    auto const total = size + overhead;
    auto*      base  = static_cast<std::byte*>(allocate_raw(total, tell_source ? alignment : 0U));
    if (base == nullptr)
    {
      throw std::bad_alloc();
    }

    // NOLINTNEXTLINE
    auto const begin = reinterpret_cast<std::uintptr_t>(base);
    // exact: the block is already aligned, so the header padded up to `alignment` lands the payload
    // on the boundary. Otherwise walk up from the first byte that can hold the header.
    auto const aligned =
     exact ? begin + overhead
           : ((begin + sizeof(allocation_header) + alignment - 1U) & ~(static_cast<std::uintptr_t>(alignment) - 1U));
    OULY_ASSERT((aligned & (alignment - 1U)) == 0U);
    // NOLINTNEXTLINE
    auto* result = reinterpret_cast<void*>(aligned);
    // NOLINTNEXTLINE
    auto* header = reinterpret_cast<allocation_header*>(aligned - sizeof(allocation_header));
    std::construct_at(header, allocation_header{.instance_   = instance_,
                                                .deallocate_ = deallocate_,
                                                .size_       = total,
                                                .offset_     = static_cast<std::uint32_t>(aligned - begin),
                                                .alignment_ = static_cast<std::uint32_t>(tell_source ? alignment : 0)});
    return result;
  }

  static void deallocate_bytes(void* ptr) noexcept
  {
    if (ptr == nullptr)
    {
      return;
    }

    // NOLINTNEXTLINE
    auto const address = reinterpret_cast<std::uintptr_t>(ptr);
    // NOLINTNEXTLINE
    auto*      header     = reinterpret_cast<allocation_header*>(address - sizeof(allocation_header));
    auto*      instance   = header->instance_;
    auto       deallocate = header->deallocate_;
    auto const total      = header->size_;
    auto const alignment  = header->alignment_;
    // NOLINTNEXTLINE
    auto* base = reinterpret_cast<void*>(address - header->offset_);
    std::destroy_at(header);
    if (deallocate != nullptr)
    {
      deallocate(instance, base, total, alignment);
    }
    else if (alignment != 0U)
    {
      ::operator delete(base, std::align_val_t{alignment});
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
  // An alignment of 0 means the block was requested without one
  using allocate_fn   = auto (*)(void*, std::size_t, std::size_t) -> void*;
  using deallocate_fn = void (*)(void*, void*, std::size_t, std::size_t) noexcept;

  // Only what deallocate_bytes() needs: the allocation source and how to get back to the raw block.
  struct allocation_header
  {
    void*         instance_   = nullptr;
    deallocate_fn deallocate_ = nullptr;
    std::size_t   size_       = 0; ///< Total bytes handed out by the source
    std::uint32_t offset_     = 0; ///< Distance from the raw block to the payload
    std::uint32_t alignment_  = 0; ///< Alignment requested from the source, 0 when none was passed
  };

  /**
   * @brief Alignment every block from `Allocator` already carries without being asked
   *
   * Allocators advertise it as a static `alignment` or `align` member. One that can be told an
   * alignment but advertises nothing is assumed to guarantee none, so the request is always passed
   * down; one that can not be told is assumed to behave like the global operator new.
   */
  template <typename Allocator>
  static constexpr std::uint32_t guaranteed_alignment_of = []() -> std::uint32_t
  {
    if constexpr (requires {
                    { Allocator::alignment } -> std::convertible_to<std::size_t>;
                  })
    {
      return static_cast<std::uint32_t>(Allocator::alignment);
    }
    else if constexpr (requires {
                         { Allocator::align } -> std::convertible_to<std::size_t>;
                       })
    {
      return static_cast<std::uint32_t>(Allocator::align);
    }
    else if constexpr (AlignedSchedulerAllocator<Allocator>)
    {
      return std::uint32_t{1};
    }
    else
    {
      return static_cast<std::uint32_t>(alignof(std::max_align_t));
    }
  }();

  [[nodiscard]] auto honors_alignment() const noexcept -> bool
  {
    return honors_alignment_;
  }

  template <typename Allocator>
  static auto allocate_from(void* instance, std::size_t size, std::size_t alignment) -> void*
  {
    if constexpr (AlignedSchedulerAllocator<Allocator>)
    {
      if (alignment != 0U)
      {
        return static_cast<Allocator*>(instance)->allocate(size, std::align_val_t{alignment});
      }
    }
    return static_cast<Allocator*>(instance)->allocate(size);
  }

  template <typename Allocator>
  static void deallocate_from([[maybe_unused]] void* instance, [[maybe_unused]] void* ptr,
                              [[maybe_unused]] std::size_t size, [[maybe_unused]] std::size_t alignment) noexcept
  {
    // The alignment has to travel back with the block: a source that allocated through an aligned
    // operator new has to be released through the matching one
    constexpr bool takes_alignment =
     requires(Allocator& allocator) { allocator.deallocate(ptr, size, std::align_val_t{alignment}); };
    constexpr bool takes_size = requires(Allocator& allocator) { allocator.deallocate(ptr, size); };

    if constexpr (takes_alignment)
    {
      if (alignment != 0U || !takes_size)
      {
        static_cast<void>(static_cast<Allocator*>(instance)->deallocate(ptr, size, std::align_val_t{alignment}));
        return;
      }
    }
    if constexpr (takes_size)
    {
      static_cast<void>(static_cast<Allocator*>(instance)->deallocate(ptr, size));
    }
  }

  [[nodiscard]] auto allocate_raw(std::size_t size, std::size_t alignment) const -> void*
  {
    if (allocate_ != nullptr)
    {
      return allocate_(instance_, size, alignment);
    }
    if (alignment != 0U)
    {
      return ::operator new(size, std::align_val_t{alignment});
    }
    return ::operator new(size);
  }

  void*         instance_   = nullptr;
  allocate_fn   allocate_   = nullptr;
  deallocate_fn deallocate_ = nullptr;
  /** @brief What the source aligns to on its own; the global operator new when there is no source */
  std::uint32_t guaranteed_alignment_ = static_cast<std::uint32_t>(alignof(std::max_align_t));
  /** @brief Whether the source can be told an alignment; the global aligned operator new always can */
  bool honors_alignment_ = true;
};

} // namespace ouly
