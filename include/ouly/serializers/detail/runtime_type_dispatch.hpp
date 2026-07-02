// SPDX-License-Identifier: MIT
#pragma once

/**
 * @file runtime_type_dispatch.hpp
 * @brief Abstract type-erasure boundaries used to (de)serialize ouly::runtime_type.
 *
 * The yml parser (parser_state) and the output serializer hold pointers to these
 * abstract bases so they remain decoupled from the concrete, user-populated
 * registry (ouly::stream_type_registry<Config>), which derives from them.
 *
 * Only forward declarations of the serializer/parser types are needed here because
 * the bases reference them solely through pointers/references in their declarations.
 */

#include "ouly/utility/runtime_type.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ouly::detail
{
class parser_state;
struct in_context_base;

template <typename Stream, typename Config>
class structured_output_serializer;

class writer_state;

struct erased_binary_output
{
  virtual void write(std::byte const* data, std::size_t size) = 0;

  erased_binary_output()                                               = default;
  erased_binary_output(const erased_binary_output&)                    = default;
  erased_binary_output(erased_binary_output&&)                         = default;
  auto operator=(const erased_binary_output&) -> erased_binary_output& = default;
  auto operator=(erased_binary_output&&) -> erased_binary_output&      = default;
  virtual ~erased_binary_output()                                      = default;
};

struct erased_binary_input
{
  virtual void read(std::byte* data, std::size_t size) = 0;
  virtual void skip(std::size_t size)                  = 0;

  erased_binary_input()                                              = default;
  erased_binary_input(const erased_binary_input&)                    = default;
  erased_binary_input(erased_binary_input&&)                         = default;
  auto operator=(const erased_binary_input&) -> erased_binary_input& = default;
  auto operator=(erased_binary_input&&) -> erased_binary_input&      = default;
  virtual ~erased_binary_input()                                     = default;
};

template <std::endian Endian>
class erased_binary_output_stream
{
  erased_binary_output* stream_ = nullptr;

public:
  explicit erased_binary_output_stream(erased_binary_output& stream) : stream_(&stream) {}

  void write(std::byte const* data, std::size_t size)
  {
    stream_->write(data, size);
  }
};

template <std::endian Endian>
class erased_binary_input_stream
{
  erased_binary_input* stream_ = nullptr;

public:
  explicit erased_binary_input_stream(erased_binary_input& stream) : stream_(&stream) {}

  void read(std::byte* data, std::size_t size)
  {
    stream_->read(data, size);
  }

  void skip(std::size_t size)
  {
    stream_->skip(size);
  }
};

template <std::endian Endian>
struct binary_any_reader_base
{
  virtual void read_value(std::uint32_t type, ouly::any& storage, erased_binary_input_stream<Endian>& stream) = 0;

  binary_any_reader_base()                                                 = default;
  binary_any_reader_base(const binary_any_reader_base&)                    = default;
  binary_any_reader_base(binary_any_reader_base&&)                         = default;
  auto operator=(const binary_any_reader_base&) -> binary_any_reader_base& = default;
  auto operator=(binary_any_reader_base&&) -> binary_any_reader_base&      = default;
  virtual ~binary_any_reader_base()                                        = default;
};

template <std::endian Endian>
struct binary_any_writer_base
{
  virtual void write_value(std::uint32_t type, ouly::any const& storage,
                           erased_binary_output_stream<Endian>& stream) = 0;

  binary_any_writer_base()                                                 = default;
  binary_any_writer_base(const binary_any_writer_base&)                    = default;
  binary_any_writer_base(binary_any_writer_base&&)                         = default;
  auto operator=(const binary_any_writer_base&) -> binary_any_writer_base& = default;
  auto operator=(binary_any_writer_base&&) -> binary_any_writer_base&      = default;
  virtual ~binary_any_writer_base()                                        = default;
};

/**
 * @brief Read-side boundary: given a type_id, construct the bound type into the
 *        runtime_type's value storage and return a context that continues parsing it.
 *
 * Not templated on Config: the concrete in_context_impl<T, Config> is produced inside
 * the registry override and returned as the type-erased in_context_base*.
 */
struct runtime_type_reader_base
{
  virtual auto parse_value(type_id id, ouly::any& storage, parser_state* parser) -> in_context_base* = 0;

  /// True when the registry resolves the serialized "type" scalar from a string name.
  [[nodiscard]] virtual auto reads_type_by_name() const -> bool
  {
    return false;
  }

  /// Resolves a string name to a type_id (only called when reads_type_by_name() is true).
  virtual auto type_id_from_name(std::string_view /*name*/) -> type_id
  {
    return {};
  }

  runtime_type_reader_base()                                                   = default;
  runtime_type_reader_base(const runtime_type_reader_base&)                    = default;
  runtime_type_reader_base(runtime_type_reader_base&&)                         = default;
  auto operator=(const runtime_type_reader_base&) -> runtime_type_reader_base& = default;
  auto operator=(runtime_type_reader_base&&) -> runtime_type_reader_base&      = default;
  virtual ~runtime_type_reader_base()                                          = default;
};

/**
 * @brief Write-side boundary: given a type_id, emit the bound type held in the
 *        runtime_type's value storage to the provided yml field visitor.
 *
 * Templated on Config because the yml field visitor type depends on it.
 */
template <typename Config>
struct runtime_type_writer_base
{
  virtual void write_value(type_id id, ouly::any const& storage,
                           structured_output_serializer<writer_state, Config>& visitor) = 0;

  /// True when the registry emits the "type" scalar as a string name.
  [[nodiscard]] virtual auto writes_type_by_name() const -> bool
  {
    return false;
  }

  /// Maps a type_id to its string name (only called when writes_type_by_name() is true).
  /// The returned view must outlive the serialization call.
  virtual auto type_name_from_id(type_id /*id*/) -> std::string_view
  {
    return {};
  }

  runtime_type_writer_base()                                                   = default;
  runtime_type_writer_base(const runtime_type_writer_base&)                    = default;
  runtime_type_writer_base(runtime_type_writer_base&&)                         = default;
  auto operator=(const runtime_type_writer_base&) -> runtime_type_writer_base& = default;
  auto operator=(runtime_type_writer_base&&) -> runtime_type_writer_base&      = default;
  virtual ~runtime_type_writer_base()                                          = default;
};

} // namespace ouly::detail
