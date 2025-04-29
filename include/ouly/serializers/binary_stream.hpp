// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstring>
#include <istream>
#include <ostream>
#include <ouly/serializers/serializers.hpp>
#include <span>
#include <vector>

namespace ouly
{
/**
 * @brief A vector-based binary data stream for storing serialized data
 *
 * This is a typedef for a standard vector of bytes that serves as the underlying
 * storage for serialized binary data.
 */
using binary_stream = std::vector<std::byte>;

/**
 * @brief A non-owning view over binary data
 *
 * This is a typedef for a span of constant bytes, providing a non-owning
 * view of binary data, typically used for reading operations.
 */
using binary_stream_view = std::span<std::byte const>;

/**
 * @brief A stream for writing binary data
 *
 * This class provides facilities for writing binary data to a stream,
 * with operations optimized for serialization.
 */
class binary_output_stream
{
public:
  /**
   * @brief Default constructor that creates an empty stream
   */
  binary_output_stream() = default;

  /**
   * @brief Writes raw binary data to the stream
   *
   * @param sdata Pointer to the source data
   * @param size Number of bytes to write
   */
  void write(std::byte const* sdata, std::size_t size)
  {
    stream_.insert(stream_.end(), sdata, sdata + size);
  }

  /**
   * @brief Gets a view of the current stream content
   *
   * @return A non-owning view of the stream's content
   */
  [[nodiscard]] auto get_string() const -> binary_stream_view
  {
    return stream_;
  }

  /**
   * @brief Releases ownership of the stream content
   *
   * @return The underlying stream, transferring ownership to the caller
   */
  auto release() -> binary_stream
  {
    return std::move(stream_);
  }

  /**
   * @brief Writes a value to the stream using the global serialization functions
   *
   * @tparam T Type of the value to write
   * @param value The value to write to the stream
   */
  template <typename T>
  void stream_out(T const& value)
  {
    ouly::write(*this, value);
  }

  /**
   * @brief Gets a pointer to the raw data in the stream
   *
   * @return Pointer to the beginning of the stream's data
   */
  [[nodiscard]] auto data() const -> std::byte const*
  {
    return stream_.data();
  }

  /**
   * @brief Gets the current size of the stream in bytes
   *
   * @return The number of bytes in the stream
   */
  [[nodiscard]] auto size() const -> std::size_t
  {
    return stream_.size();
  }

private:
  binary_stream stream_; ///< The underlying binary data storage
};

/**
 * @brief A stream for reading binary data
 *
 * This class provides facilities for reading binary data from a stream,
 * with operations optimized for deserialization.
 */
class binary_input_stream
{
public:
  /**
   * @brief Constructs a stream from a raw pointer and size
   *
   * @param data Pointer to the binary data
   * @param size Number of bytes in the data
   */
  binary_input_stream(std::byte const* data, std::size_t size) : stream_(data, size) {}

  /**
   * @brief Constructs a stream from a binary_stream_view
   *
   * @param str View of the binary data to read from
   */
  binary_input_stream(binary_stream_view str) : stream_(str) {}

  /**
   * @brief Reads binary data from the stream into a buffer
   *
   * @param sdata Pointer to the destination buffer
   * @param size Number of bytes to read
   */
  void read(std::byte* sdata, std::size_t size)
  {
    if (size > stream_.size())
    {
      throw std::out_of_range("Not enough data in the stream");
    }
    std::memcpy(sdata, stream_.data(), size);
    stream_ = stream_.subspan(size);
  }

  /**
   * @brief Skips a specified number of bytes in the stream
   *
   * @param size Number of bytes to skip
   */
  void skip(std::size_t size)
  {
    if (size > stream_.size())
    {
      throw std::out_of_range("Not enough data in the stream");
    }
    stream_ = stream_.subspan(size);
  }

  /**
   * @brief Gets a copy of the stream's current content
   *
   * @return A copy of the remaining binary data
   */
  [[nodiscard]] auto get_string() const -> binary_stream_view
  {
    return stream_;
  }

  /**
   * @brief Reads a value from the stream using the global deserialization functions
   *
   * @tparam T Type of the value to read
   * @param value Reference to the variable that will receive the deserialized value
   */
  template <typename T>
  void stream_in(T& value)
  {
    ouly::read(*this, value);
  }

  /**
   * @brief Gets a pointer to the raw data in the stream
   *
   * @return Pointer to the beginning of the remaining data
   */
  [[nodiscard]] auto data() const -> std::byte const*
  {
    return stream_.data();
  }

  /**
   * @brief Gets the remaining size of the stream in bytes
   *
   * @return The number of bytes left to read
   */
  [[nodiscard]] auto size() const -> std::size_t
  {
    return stream_.size();
  }

private:
  binary_stream_view stream_; ///< The underlying binary data view
};

/**
 * @brief A stream adapter for writing binary data to std::ostream
 *
 * This class provides serialization capabilities using a standard output stream
 * as the underlying storage mechanism.
 */
class binary_ostream
{
public:
  /**
   * @brief Constructs a binary output stream adapter
   *
   * @param os Reference to the output stream to write to
   */
  explicit binary_ostream(std::ostream& os) : os_(&os) {}

  /**
   * @brief Writes raw binary data to the stream
   *
   * @param sdata Pointer to the source data
   * @param size Number of bytes to write
   */
  void write(std::byte const* sdata, std::size_t size)
  {
    // NOLINTNEXTLINE
    os_->write(reinterpret_cast<char const*>(sdata), static_cast<std::streamsize>(size));
  }

  /**
   * @brief Writes a value to the stream using the global serialization functions
   *
   * @tparam T Type of the value to write
   * @param value The value to write to the stream
   */
  template <typename T>
  void stream_out(T const& value)
  {
    ouly::write(*this, value);
  }

private:
  std::ostream* os_ = nullptr; ///< The underlying output stream
};

/**
 * @brief A stream adapter for reading binary data from std::istream
 *
 * This class provides deserialization capabilities using a standard input stream
 * as the data source.
 */
class binary_istream
{
public:
  /**
   * @brief Constructs a binary input stream adapter
   *
   * @param is Reference to the input stream to read from
   */
  explicit binary_istream(std::istream& is) : is_(&is) {}

  /**
   * @brief Reads binary data from the stream into a buffer
   *
   * @param sdata Pointer to the destination buffer
   * @param size Number of bytes to read
   */
  void read(std::byte* sdata, std::size_t size)
  {
    // NOLINTNEXTLINE
    is_->read(reinterpret_cast<char*>(sdata), static_cast<std::streamsize>(size));
    if (is_->gcount() != static_cast<std::streamsize>(size))
    {
      throw std::out_of_range("Not enough data in the stream");
    }
  }

  /**
   * @brief Skips a specified number of bytes in the stream
   *
   * @param size Number of bytes to skip
   */
  void skip(std::size_t size)
  {
    is_->ignore(static_cast<std::streamsize>(size));
    if (is_->gcount() != static_cast<std::streamsize>(size))
    {
      throw std::out_of_range("Not enough data in the stream");
    }
  }

  /**
   * @brief Reads a value from the stream using the global deserialization functions
   *
   * @tparam T Type of the value to read
   * @param value Reference to the variable that will receive the deserialized value
   */
  template <typename T>
  void stream_in(T& value)
  {
    ouly::read(*this, value);
  }

private:
  std::istream* is_ = nullptr; ///< The underlying input stream
};
} // namespace ouly