// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/alignment.hpp"
#include "ouly/allocators/config.hpp"
#include "ouly/allocators/detail/default_allocator_defs.hpp"
#include "ouly/allocators/detail/memory_stats.hpp"
#include "ouly/allocators/detail/platform_memory.hpp"
#include "ouly/allocators/tags.hpp"
#include <cstddef>

namespace ouly
{

/**
 * @brief Virtual memory allocator using platform-specific virtual memory APIs
 *
 * This allocator uses virtual memory allocation functions (VirtualAlloc on Windows,
 * mmap with MAP_ANONYMOUS on Unix) to allocate large regions of memory directly from
 * the operating system. It's particularly useful for:
 * - Large allocations that bypass malloc/new overhead
 * - Memory regions that need specific protection (read-only, executable, etc.)
 * - Cases where fine-grained control over memory commitment is needed
 * - Applications requiring specific memory alignment or placement
 *
 * Features:
 * - Allocates memory directly from OS virtual memory system
 * - Supports memory protection flags (read/write/execute combinations)
 * - Page-aligned allocations (automatically rounds up to page boundaries)
 * - Zero-initialized memory by default
 * - Cross-platform (Windows and POSIX systems)
 * - Move constructible/assignable but not copy constructible/assignable
 *
 * @tparam Config Configuration template for the allocator
 *
 * Example usage:
 * @code
 * virtual_allocator alloc;
 *
 * // Allocate 1MB of read-write memory
 * void* ptr = alloc.allocate(1024 * 1024);
 *
 * // Allocate executable memory (for JIT code)
 * using ExecConfig = ouly::config<ouly::cfg::protection<protection::read_write_execute>>;
 * virtual_allocator<ExecConfig> exec_alloc;
 * void* code_ptr = exec_alloc.allocate(4096);
 *
 * alloc.deallocate(ptr, 1024 * 1024);
 * exec_alloc.deallocate(code_ptr, 4096);
 * @endcode
 *
 * @note All allocations are rounded up to the system page size
 * @note Memory is automatically zeroed on allocation
 * @warning Deallocating with wrong size or pointer may cause undefined behavior
 */
template <typename Config = ouly::config<>>
class virtual_allocator : ouly::detail::statistics<virtual_memory_allocator_tag, Config>
{
public:
  using tag        = virtual_memory_allocator_tag;
  using statistics = ouly::detail::statistics<virtual_memory_allocator_tag, Config>;
  using size_type  = std::size_t;
  using address    = void*;

  static constexpr auto align              = ouly::detail::min_alignment_v<Config>;
  static constexpr auto default_protection = cfg::protection::read_write;

  static constexpr auto null() -> address
  {
    return nullptr;
  }

  /**
   * @brief Default constructor
   *
   * Initializes the allocator and queries system memory information.
   */
  virtual_allocator() noexcept
  {
    auto info               = detail::get_memory_info();
    page_size_              = info.page_size_;
    allocation_granularity_ = info.allocation_granularity_;
  }

  /**
   * @brief Deleted copy constructor
   */
  virtual_allocator(const virtual_allocator&) = delete;

  /**
   * @brief Move constructor
   */
  virtual_allocator(virtual_allocator&& other) noexcept
      : statistics(std::move(other)), page_size_(other.page_size_),
        allocation_granularity_(other.allocation_granularity_)
  {}

  /**
   * @brief Deleted copy assignment
   */
  auto operator=(const virtual_allocator&) -> virtual_allocator& = delete;

  /**
   * @brief Move assignment
   */
  auto operator=(virtual_allocator&& other) noexcept -> virtual_allocator&
  {
    if (this != &other)
    {
      statistics::operator=(std::move(other));
      page_size_              = other.page_size_;
      allocation_granularity_ = other.allocation_granularity_;
    }
    return *this;
  }

  /**
   * @brief Destructor
   */
  ~virtual_allocator() = default;

  /**
   * @brief Allocate virtual memory
   *
   * @param size Size in bytes to allocate (will be rounded up to page size)
   * @param alignment Memory alignment requirement
   * @return Pointer to allocated memory or nullptr on failure
   */
  template <typename Alignment = alignment<align>>
  [[nodiscard]] auto allocate(size_type size, Alignment /* alignment_hint */ = {}) noexcept -> address
  {
    if (size == 0)
    {
      return nullptr;
    }

    // Round up to page size for virtual memory allocation
    size_type aligned_size = round_up_to_page_size(size);

    // Get protection from config or use default
    auto protection = get_protection_from_config();

    void* ptr = detail::virtual_alloc(aligned_size, protection);

    if (ptr != nullptr)
    {
      [[maybe_unused]] auto measure = statistics::report_allocate(aligned_size);
    }

    return ptr;
  }

  /**
   * @brief Allocate zeroed virtual memory
   *
   * Virtual memory is already zeroed by the OS, so this is equivalent to allocate().
   *
   * @param size Size in bytes to allocate
   * @param alignment Memory alignment requirement
   * @return Pointer to allocated memory or nullptr on failure
   */
  template <typename Alignment = alignment<align>>
  [[nodiscard]] auto zero_allocate(size_type size, Alignment /* alignment_hint */ = {}) noexcept -> address
  {
    // Virtual memory is already zero-initialized by the OS
    return allocate(size);
  }

  /**
   * @brief Deallocate virtual memory
   *
   * @param ptr Pointer to memory to deallocate
   * @param size Size of the memory region (should match allocation size)
   * @param alignment Memory alignment (ignored for virtual memory)
   */
  template <typename Alignment = alignment<align>>
  void deallocate(address ptr, size_type size, Alignment /* alignment_hint */ = {}) noexcept
  {
    if (ptr == nullptr)
    {
      return;
    }

    size_type aligned_size = round_up_to_page_size(size);

    if (detail::virtual_free(ptr, aligned_size))
    {
      [[maybe_unused]] auto measure = statistics::report_deallocate(size);
    }
  }

  /**
   * @brief Change memory protection on a region
   *
   * @param ptr Pointer to memory region
   * @param size Size of the region
   * @param new_protection New protection flags
   * @return true on success, false on failure
   */
  [[nodiscard]] auto protect(address ptr, size_type size, cfg::protection new_protection) noexcept -> bool
  {
    if (ptr == nullptr)
    {
      return false;
    }

    size_type aligned_size = round_up_to_page_size(size);
    return detail::virtual_protect(ptr, aligned_size, new_protection);
  }

  /**
   * @brief Advise the kernel about memory usage patterns
   *
   * @param ptr Pointer to memory region
   * @param size Size of the region
   * @param advice_type Memory usage advice
   * @return true on success, false on failure
   */
  [[nodiscard]] auto advise(address ptr, size_type size, cfg::advice advice_type) noexcept -> bool
  {
    if (ptr == nullptr)
    {
      return false;
    }

    size_type aligned_size = round_up_to_page_size(size);
    return detail::advise(ptr, aligned_size, advice_type);
  }

  /**
   * @brief Get system page size
   */
  [[nodiscard]] auto page_size() const noexcept -> size_type
  {
    return page_size_;
  }

  /**
   * @brief Get system allocation granularity
   */
  [[nodiscard]] auto allocation_granularity() const noexcept -> size_type
  {
    return allocation_granularity_;
  }

private:
  size_type page_size_{0};
  size_type allocation_granularity_{0};

  [[nodiscard]] auto round_up_to_page_size(size_type size) const noexcept -> size_type
  {
    return ((size + page_size_ - 1) / page_size_) * page_size_;
  }

  [[nodiscard]] auto get_protection_from_config() const noexcept -> cfg::protection
  {
    // Check if config specifies protection, otherwise use default
    if constexpr (ouly::detail::has_protection_v<Config>)
    {
      return ouly::detail::protection_v<Config>;
    }
    else
    {
      return default_protection;
    }
  }
};

} // namespace ouly
