#pragma once

#include <acl/serializers/serializers.hpp>
#include <cstddef>
#include <cstring>
#include <span>
#include <vector>

namespace acl
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
    stream.insert(stream.end(), sdata, sdata + size);
  }

  /**
   * @brief Gets a view of the current stream content
   *
   * @return A non-owning view of the stream's content
   */
  [[nodiscard]] auto get_string() const -> binary_stream_view
  {
    return stream;
  }

  /**
   * @brief Releases ownership of the stream content
   *
   * @return The underlying stream, transferring ownership to the caller
   */
  auto release() -> binary_stream
  {
    return std::move(stream);
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
    acl::write(*this, value);
  }

  /**
   * @brief Gets a pointer to the raw data in the stream
   *
   * @return Pointer to the beginning of the stream's data
   */
  [[nodiscard]] auto data() const -> std::byte const*
  {
    return reinterpret_cast<std::byte const*>(stream.data());
  }

  /**
   * @brief Gets the current size of the stream in bytes
   *
   * @return The number of bytes in the stream
   */
  [[nodiscard]] auto size() const -> std::size_t
  {
    return stream.size();
  }

private:
  binary_stream stream; ///< The underlying binary data storage
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
  binary_input_stream(std::byte const* data, std::size_t size) : stream(reinterpret_cast<uint8_t const*>(data), size) {}

  /**
   * @brief Constructs a stream from a binary_stream_view
   *
   * @param str View of the binary data to read from
   */
  binary_input_stream(binary_stream_view str) : stream(str) {}

  /**
   * @brief Reads binary data from the stream into a buffer
   *
   * @param sdata Pointer to the destination buffer
   * @param size Number of bytes to read
   */
  void read(std::byte* sdata, std::size_t size)
  {
    std::memcpy(sdata, stream.data(), size);
    stream.remove_prefix(size);
  }

  /**
   * @brief Skips a specified number of bytes in the stream
   *
   * @param size Number of bytes to skip
   */
  void skip(std::size_t size)
  {
    stream.remove_prefix(size);
  }

  /**
   * @brief Gets a copy of the stream's current content
   *
   * @return A copy of the remaining binary data
   */
  [[nodiscard]] auto get_string() const -> binary_stream
  {
    return stream;
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
    acl::read(*this, value);
  }

  /**
   * @brief Gets a pointer to the raw data in the stream
   *
   * @return Pointer to the beginning of the remaining data
   */
  [[nodiscard]] auto data() const -> std::byte const*
  {
    return reinterpret_cast<std::byte const*>(stream.data());
  }

  /**
   * @brief Gets the remaining size of the stream in bytes
   *
   * @return The number of bytes left to read
   */
  [[nodiscard]] auto size() const -> std::size_t
  {
    return stream.size();
  }

private:
  binary_stream_view stream; ///< The underlying binary data view
};

} // namespace acl