// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/detail/memory_stats.hpp"
#include "ouly/allocators/detail/platform_memory.hpp"
#include "ouly/allocators/tags.hpp"
#include "ouly/utility/common.hpp"

#include <cstddef>
#include <filesystem>
#include <iterator>
#include <system_error>

namespace ouly
{

/**
 * @brief Memory-mapped file object with interface similar to mio library
 *
 * Provides a container-like interface to memory-mapped files with read-only
 * or read-write access modes. The interface is inspired by the mio library
 * (https://github.com/vimpunk/mio) but adapted for the OULY framework.
 *
 * @tparam AccessMode The access mode (read or write)
 * @tparam Config Configuration for the allocator
 *
 * Features:
 * - Container-like interface (begin, end, size, data, operator[])
 * - Cross-platform file mapping (Windows/POSIX)
 * - Error handling via std::error_code
 * - Automatic file handle management
 * - Sync operations for write mappings
 * - Move semantics (single ownership)
 *
 * Example usage:
 * @code
 * // Read-only mapping
 * std::error_code error;
 * ouly::mmap_source source;
 * source.map("data.txt", error);
 * if (!error) {
 *   for (auto byte : source) {
 *     // Process byte
 *   }
 * }
 *
 * // Read-write mapping
 * ouly::mmap_sink sink;
 * sink.map("output.dat", error);
 * if (!error) {
 *   std::fill(sink.begin(), sink.end(), 0);
 *   sink.sync(error);
 * }
 * @endcode
 */

enum class access_mode : std::uint8_t
{
  read,
  write
};

// Map entire file constant
constexpr std::size_t map_entire_file = 0;

template <access_mode AccessMode, typename Config = ouly::config<>>
class basic_mmap_file : ouly::detail::statistics<mmap_allocator_tag, Config>
{
public:
  using tag                    = mmap_allocator_tag;
  using statistics             = ouly::detail::statistics<mmap_allocator_tag, Config>;
  using value_type             = std::uint8_t;
  using size_type              = std::size_t;
  using difference_type        = std::ptrdiff_t;
  using reference              = value_type&;
  using const_reference        = const value_type&;
  using pointer                = value_type*;
  using const_pointer          = const value_type*;
  using iterator               = pointer;
  using const_iterator         = const_pointer;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr auto align              = 1;
  static constexpr auto default_protection = cfg::protection::read_write;

  static constexpr auto null() -> pointer
  {
    return nullptr;
  }

  /**
   * @brief Default constructor - creates an unmapped file object
   */
  basic_mmap_file() noexcept = default;

  /**
   * @brief Constructor that maps a file immediately
   * @param path Path to the file to map
   * @param offset Offset within the file (0 for start)
   * @param length Length to map (map_entire_file for entire file)
   * @throws std::system_error on mapping failure (when exceptions enabled)
   */
  basic_mmap_file(std::filesystem::path const& path, size_type offset = 0, size_type length = map_entire_file)
  {
    map(path, offset, length);
  }

  /**
   * @brief Move constructor
   */
  basic_mmap_file(basic_mmap_file&& other) noexcept
      : data_(other.data_), length_(other.length_), mapped_length_(other.mapped_length_),
        mapped_ptr_(other.mapped_ptr_), filename_(std::move(other.filename_))
  {
    other.data_          = nullptr;
    other.length_        = 0;
    other.mapped_length_ = 0;
    other.mapped_ptr_    = nullptr;
  }

  /**
   * @brief Move assignment operator
   */
  auto operator=(basic_mmap_file&& other) noexcept -> basic_mmap_file&
  {
    if (this != &other)
    {
      unmap();
      data_          = other.data_;
      length_        = other.length_;
      mapped_length_ = other.mapped_length_;
      mapped_ptr_    = other.mapped_ptr_;
      filename_      = std::move(other.filename_);

      other.data_          = nullptr;
      other.length_        = 0;
      other.mapped_length_ = 0;
      other.mapped_ptr_    = nullptr;
    }
    return *this;
  }

  /**
   * @brief Deleted copy operations (move-only semantics)
   */
  basic_mmap_file(const basic_mmap_file&)                    = delete;
  auto operator=(const basic_mmap_file&) -> basic_mmap_file& = delete;

  /**
   * @brief Destructor - automatically syncs and unmaps
   */
  ~basic_mmap_file() noexcept
  {
    if constexpr (AccessMode == access_mode::write)
    {
      try
      {
        sync(); // Best effort sync in destructor
      }
      catch (const std::system_error& e)
      {
        // Log or handle sync error if needed
        OULY_PRINT_ERROR(std::string("Failed to sync mmap file: ") + e.what());
      }
    }
    unmap();
  }

  // Container interface

  /**
   * @brief Returns true if no mapping is established
   */
  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return length_ == 0;
  }

  /**
   * @brief Returns the number of mapped bytes
   */
  [[nodiscard]] auto size() const noexcept -> size_type
  {
    return length_;
  }

  /**
   * @brief Returns the number of mapped bytes (same as size)
   */
  [[nodiscard]] auto length() const noexcept -> size_type
  {
    return length_;
  }

  /**
   * @brief Returns the actual mapped length (may be larger due to page alignment)
   */
  [[nodiscard]] auto mapped_length() const noexcept -> size_type
  {
    return mapped_length_;
  }

  /**
   * @brief Returns pointer to the mapped data
   */
  template <access_mode A = AccessMode>
  [[nodiscard]] auto data() noexcept -> pointer
    requires(A == access_mode::write)
  {
    return data_;
  }

  /**
   * @brief Returns const pointer to the mapped data
   */
  [[nodiscard]] auto data() const noexcept -> const_pointer
  {
    return data_;
  }

  /**
   * @brief Iterator to beginning of mapped data
   */
  template <access_mode A = AccessMode>
  [[nodiscard]] auto begin() noexcept -> iterator
    requires(A == access_mode::write)
  {
    return data_;
  }

  /**
   * @brief Const iterator to beginning of mapped data
   */
  [[nodiscard]] auto begin() const noexcept -> const_iterator
  {
    return data_;
  }

  /**
   * @brief Const iterator to beginning of mapped data
   */
  [[nodiscard]] auto cbegin() const noexcept -> const_iterator
  {
    return data_;
  }

  /**
   * @brief Iterator to end of mapped data
   */
  template <access_mode A = AccessMode>
  [[nodiscard]] auto end() noexcept -> iterator
    requires(A == access_mode::write)
  {
    return data_ + length_;
  }

  /**
   * @brief Const iterator to end of mapped data
   */
  [[nodiscard]] auto end() const noexcept -> const_iterator
  {
    return data_ + length_;
  }

  /**
   * @brief Const iterator to end of mapped data
   */
  [[nodiscard]] auto cend() const noexcept -> const_iterator
  {
    return data_ + length_;
  }

  /**
   * @brief Reverse iterator to end of mapped data
   */
  template <access_mode A = AccessMode>
  [[nodiscard]] auto rbegin() noexcept -> reverse_iterator
    requires(A == access_mode::write)
  {
    return reverse_iterator(end());
  }

  /**
   * @brief Const reverse iterator to end of mapped data
   */
  [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(end());
  }

  /**
   * @brief Const reverse iterator to end of mapped data
   */
  [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(end());
  }

  /**
   * @brief Reverse iterator to beginning of mapped data
   */
  template <access_mode A = AccessMode>
  [[nodiscard]] auto rend() noexcept -> reverse_iterator
    requires(A == access_mode::write)
  {
    return reverse_iterator(begin());
  }

  /**
   * @brief Const reverse iterator to beginning of mapped data
   */
  [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(begin());
  }

  /**
   * @brief Const reverse iterator to beginning of mapped data
   */
  [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(begin());
  }

  /**
   * @brief Access element at index
   */
  template <access_mode A = AccessMode>
  [[nodiscard]] auto operator[](size_type i) noexcept -> reference
    requires(A == access_mode::write)
  {
    return data_[i];
  }

  /**
   * @brief Access element at index (const)
   */
  [[nodiscard]] auto operator[](size_type i) const noexcept -> const_reference
  {
    return data_[i];
  }

  // File mapping interface

  /**
   * @brief Returns true if a file is currently mapped
   */
  [[nodiscard]] auto is_open() const noexcept -> bool
  {
    return data_ != nullptr;
  }

  /**
   * @brief Returns true if a file is currently mapped (alias for is_open)
   */
  [[nodiscard]] auto is_mapped() const noexcept -> bool
  {
    return is_open();
  }

  /**
   * @brief Map a file with specified offset and length
   * @param path Path to the file to map
   * @param offset Offset within the file (bytes)
   * @param length Length to map (map_entire_file for entire file)
   * @param error Error code output parameter
   */
  void map(std::filesystem::path const& path, size_type offset, size_type length)
  {
    // Unmap any existing mapping
    unmap();

    // Store filename for potential error reporting
    filename_ = path;

    // Determine actual length to map
    size_type actual_length = length;
    auto      result =
     detail::map_file(filename_.c_str(), actual_length,
                      AccessMode == access_mode::write ? cfg::protection::read_write : cfg::protection::read,
                      detail::map_flags::shared, length == map_entire_file);

    if (!result)
    {
      throw std::system_error(std::make_error_code(std::errc::invalid_argument));
    }

    mapped_ptr_    = result.address_;
    mapped_length_ = result.size_;

    // Handle offset within the mapping
    if (offset > result.size_)
    {
      unmap();
      throw std::system_error(std::make_error_code(std::errc::invalid_argument));
    }

    data_   = static_cast<pointer>(mapped_ptr_) + offset;
    length_ = (length == map_entire_file) ? (mapped_length_ - offset) : std::min(length, mapped_length_ - offset);

    [[maybe_unused]] auto measure = statistics::report_allocate(length_);
  }

  /**
   * @brief Map an entire file
   * @param path Path to the file to map
   * @param error Error code output parameter
   */
  void map(std::filesystem::path const& path)
  {
    map(path, 0, map_entire_file);
  }

  /**
   * @brief Unmap the currently mapped file
   */
  void unmap() noexcept
  {
    if (mapped_ptr_ != nullptr)
    {
      [[maybe_unused]] auto measure = statistics::report_deallocate(length_);
      detail::unmap(mapped_ptr_, mapped_length_);

      data_          = nullptr;
      length_        = 0;
      mapped_length_ = 0;
      mapped_ptr_    = nullptr;
      filename_.clear();
    }
  }

  /**
   * @brief Sync mapped data to disk (only available for write mappings)
   * @param error Error code output parameter
   */
  template <access_mode A = AccessMode>
  auto sync() -> void
    requires(A == access_mode::write)
  {
    if (!is_mapped())
    {
      throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
    }

    if (!detail::sync(mapped_ptr_, mapped_length_, true))
    {
      throw std::system_error(std::make_error_code(std::errc::io_error));
    }
  }

  /**
   * @brief Get the filename of the mapped file
   */
  [[nodiscard]] auto filename() const noexcept -> const std::filesystem::path&
  {
    return filename_;
  }

  /**
   * @brief Swap with another mmap file object
   */
  void swap(basic_mmap_file& other) noexcept
  {
    if (this != &other)
    {
      using std::swap;
      swap(data_, other.data_);
      swap(length_, other.length_);
      swap(mapped_length_, other.mapped_length_);
      swap(mapped_ptr_, other.mapped_ptr_);
      swap(filename_, other.filename_);
    }
  }

private:
  pointer               data_          = nullptr; // Points to the user-requested data (with offset applied)
  size_type             length_        = 0;       // User-requested length
  size_type             mapped_length_ = 0;       // Actual mapped length (page-aligned)
  void*                 mapped_ptr_    = nullptr; // Actual mapping start (for unmapping)
  std::filesystem::path filename_;                // Filename for error reporting
};

// Type aliases for common use cases

/**
 * @brief Read-only memory-mapped file
 */
template <typename Config = ouly::config<>>
using basic_mmap_source = basic_mmap_file<access_mode::read, Config>;

/**
 * @brief Read-write memory-mapped file
 */
template <typename Config = ouly::config<>>
using basic_mmap_sink = basic_mmap_file<access_mode::write, Config>;

// Convenience aliases with default config
using mmap_source = basic_mmap_source<>;
using mmap_sink   = basic_mmap_sink<>;

// Factory functions (mio-compatible interface)

/**
 * @brief Factory function to create a read-only mapped file
 * @param path Path to the file to map
 * @param offset Offset within the file
 * @param length Length to map (map_entire_file for entire file)
 * @return Mapped file object
 */
[[nodiscard]] auto make_mmap_source(std::filesystem::path const& path, std::size_t offset, std::size_t length)
 -> mmap_source
{
  mmap_source source;
  source.map(path, offset, length);
  return source;
}

/**
 * @brief Factory function to create a read-only mapped file (entire file)
 * @param path Path to the file to map
 * @return Mapped file object
 */
[[nodiscard]] auto make_mmap_source(std::filesystem::path const& path) -> mmap_source
{
  return make_mmap_source(path, 0, map_entire_file);
}

/**
 * @brief Factory function to create a read-write mapped file
 * @param path Path to the file to map
 * @param offset Offset within the file
 * @param length Length to map (map_entire_file for entire file)
 * @return Mapped file object
 */
[[nodiscard]] auto make_mmap_sink(std::filesystem::path const& path, std::size_t offset, std::size_t length)
 -> mmap_sink
{
  mmap_sink sink;
  sink.map(path, offset, length);
  return sink;
}

/**
 * @brief Factory function to create a read-write mapped file (entire file)
 * @param path Path to the file to map
 * @return Mapped file object
 */
[[nodiscard]] auto make_mmap_sink(std::filesystem::path const& path) -> mmap_sink
{
  return make_mmap_sink(path, 0, map_entire_file);
}

// Free function for swapping
template <access_mode AccessMode, typename Config>
void swap(basic_mmap_file<AccessMode, Config>& a, basic_mmap_file<AccessMode, Config>& b) noexcept
{
  a.swap(b);
}

} // namespace ouly
