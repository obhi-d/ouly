
#pragma once

#include <acl/serializers/serializers.hpp>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>

namespace acl
{
using binary_stream      = std::basic_string<std::byte>;
using binary_stream_view = std::basic_string_view<std::byte>;
class binary_output_stream
{
public:
  binary_output_stream() = default;

  void write(std::byte const* sdata, std::size_t size)
  {
    stream.append(sdata, size);
  }

  binary_stream_view get_string() const
  {
    return stream;
  }

  binary_stream release()
  {
    return std::move(stream);
  }

  template <typename T>
  void stream_out(T const& value)
  {
    acl::write(*this, value);
  }

  std::byte const* data() const
  {
    return stream.data();
  }

  std::size_t size() const
  {
    return stream.size();
  }

private:
  std::basic_string<std::byte> stream;
};

class binary_input_stream
{
public:
  binary_input_stream(std::byte const* data, std::size_t size) : stream(data, size) {}
  binary_input_stream(binary_stream_view str) : stream(str) {}

  void read(std::byte* sdata, std::size_t size)
  {
    std::memcpy(sdata, stream.data(), size);
    stream.remove_prefix(size);
  }

  void skip(std::size_t size)
  {
    stream.remove_prefix(size);
  }

  binary_stream_view get_string() const
  {
    return stream;
  }

  template <typename T>
  void stream_in(T& value)
  {
    acl::read(*this, value);
  }

  std::byte const* data() const
  {
    return stream.data();
  }

  std::size_t size() const
  {
    return stream.size();
  }

private:
  binary_stream_view stream;
};

} // namespace acl