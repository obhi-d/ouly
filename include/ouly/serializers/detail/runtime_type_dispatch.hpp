// SPDX-License-Identifier: MIT
#pragma once

/**
 * @file runtime_type_dispatch.hpp
 * @brief Abstract type-erasure boundaries used to (de)serialize ouly::runtime_type.
 *
 * The yml parser (parser_state) and the output serializer hold pointers to these
 * abstract bases so they remain decoupled from the concrete, user-populated
 * registry (ouly::yml::runtime_type_registry<Config>), which derives from them.
 *
 * Only forward declarations of the serializer/parser types are needed here because
 * the bases reference them solely through pointers/references in their declarations.
 */

#include "ouly/utility/runtime_type.hpp"

namespace ouly::detail
{
class parser_state;
struct in_context_base;

template <typename Stream, typename Config>
class structured_output_serializer;

class writer_state;

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

  runtime_type_writer_base()                                                   = default;
  runtime_type_writer_base(const runtime_type_writer_base&)                    = default;
  runtime_type_writer_base(runtime_type_writer_base&&)                         = default;
  auto operator=(const runtime_type_writer_base&) -> runtime_type_writer_base& = default;
  auto operator=(runtime_type_writer_base&&) -> runtime_type_writer_base&      = default;
  virtual ~runtime_type_writer_base()                                          = default;
};

} // namespace ouly::detail
