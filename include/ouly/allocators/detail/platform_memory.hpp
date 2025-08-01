// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/config.hpp"
#include "ouly/utility/common.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <type_traits>

namespace ouly::detail
{

enum class map_flags : std::uint8_t
{
  none        = 0,
  private_map = 1, // Changes are private to the process
  shared      = 2, // Changes are visible to other processes
  anonymous   = 4, // Not backed by a file
  fixed       = 8  // Map at exact address
};

struct memory_info
{
  std::size_t page_size_              = 0;
  std::size_t allocation_granularity_ = 0;
};

struct mapped_file_info
{
  void*           address_ = nullptr;               // Mapped address
  std::size_t     size_    = 0;                     // Size of the mapping
  cfg::protection prot_    = cfg::protection::none; // Protection flags

  explicit operator bool() const noexcept
  {
    return address_ != nullptr && size_ > 0;
  }
};

/**
 * @brief Get system memory information
 */
OULY_API auto get_memory_info() noexcept -> memory_info;

/**
 * @brief Allocate virtual memory
 * @param size Size in bytes to allocate
 * @param prot Memory cfg::protection flags
 * @param preferred_address Preferred address (may be ignored)
 * @return Pointer to allocated memory or nullptr on failure
 */
OULY_API auto virtual_alloc(std::size_t size, cfg::protection prot = cfg::protection::read_write,
                            void* preferred_address = nullptr) noexcept -> void*;

/**
 * @brief Deallocate virtual memory
 * @param ptr Pointer to memory to deallocate
 * @param size Size of the memory region
 * @return true on success, false on failure
 */
OULY_API auto virtual_free(void* ptr, std::size_t size) noexcept -> bool;

/**
 * @brief Change memory cfg::protection on a region
 * @param ptr Pointer to memory region
 * @param size Size of the region
 * @param new_prot New cfg::protection flags
 * @return true on success, false on failure
 */
OULY_API auto virtual_protect(void* ptr, std::size_t size, cfg::protection new_prot) noexcept -> bool;

/**
 * @brief Create or open a memory-mapped file
 * @param filename Path to the file
 * @param size Size to map (0 for entire file)
 * @param prot Protection flags
 * @param flags Mapping flags
 * @param create_if_missing Create file if it doesn't exist
 * @return Pointer to mapped memory or nullptr on failure
 */
OULY_API auto map_file(std::filesystem::path const& filename, std::size_t size,
                       cfg::protection prot = cfg::protection::read_write, map_flags flags = map_flags::shared,
                       bool create_if_missing = false) noexcept -> mapped_file_info;

/**
 * @brief Create anonymous memory mapping
 * @param size Size to map
 * @param prot Protection flags
 * @param preferred_address Preferred address (may be ignored)
 * @return Pointer to mapped memory or nullptr on failure
 */
OULY_API auto map_anonymous(std::size_t size, cfg::protection prot = cfg::protection::read_write,
                            void* preferred_address = nullptr) noexcept -> void*;

/**
 * @brief Unmap memory-mapped region
 * @param ptr Pointer to mapped memory
 * @param size Size of the mapped region
 * @return true on success, false on failure
 */
OULY_API auto unmap(void* ptr, std::size_t size) noexcept -> bool;

/**
 * @brief Synchronize mapped memory with underlying storage
 * @param ptr Pointer to mapped memory
 * @param size Size of the region to sync
 * @param async Whether to perform asynchronous sync
 * @return true on success, false on failure
 */
OULY_API auto sync(void* ptr, std::size_t size, bool async = false) noexcept -> bool;

OULY_API auto advise(void* ptr, std::size_t size, cfg::advice advice_type) noexcept -> bool;

constexpr auto operator|(cfg::protection lhs, cfg::protection rhs) noexcept -> cfg::protection
{
  return static_cast<cfg::protection>(static_cast<std::underlying_type_t<cfg::protection>>(lhs) |
                                      static_cast<std::underlying_type_t<cfg::protection>>(rhs));
}

constexpr auto operator&(cfg::protection lhs, cfg::protection rhs) noexcept -> cfg::protection
{
  return static_cast<cfg::protection>(static_cast<std::underlying_type_t<cfg::protection>>(lhs) &
                                      static_cast<std::underlying_type_t<cfg::protection>>(rhs));
}

constexpr auto operator|(map_flags lhs, map_flags rhs) noexcept -> map_flags
{
  return static_cast<map_flags>(static_cast<std::underlying_type_t<map_flags>>(lhs) |
                                static_cast<std::underlying_type_t<map_flags>>(rhs));
}

constexpr auto operator&(map_flags lhs, map_flags rhs) noexcept -> map_flags
{
  return static_cast<map_flags>(static_cast<std::underlying_type_t<map_flags>>(lhs) &
                                static_cast<std::underlying_type_t<map_flags>>(rhs));
}

} // namespace ouly::detail
