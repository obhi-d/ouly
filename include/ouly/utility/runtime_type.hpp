// SPDX-License-Identifier: MIT
#pragma once

/**
 * @file runtime_type.hpp
 * @brief A context-driven dynamic value for serialization.
 *
 * A runtime_type pairs a runtime type_id with a type-erased value (ouly::any).
 * The mapping from type_id to a concrete C++ type is supplied at (de)serialization
 * time by a registry/context (see ouly::stream_type_registry), rather than
 * being fixed at compile time as with std::variant.
 *
 * Like std::variant it is serialized as a { type, value } object. The "type" and
 * "value" key names follow the serialization Config's string transform, so they can
 * be customized exactly as variant's keys are (see ouly::stream_type_registry).
 */

#include "ouly/utility/any.hpp"

#include <cstdint>

namespace ouly
{

/**
 * @brief A lightweight runtime type identifier.
 */
struct type_id
{
  std::uint32_t id_ = 0;
};

/**
 * @brief A dynamically typed value: a type_id plus a type-erased payload.
 *
 * @tparam Any The type-erased storage type (defaults to ouly::any).
 */
template <typename Any = ouly::any>
struct basic_runtime_type
{
  /// Tag used by ouly::detail::RuntimeTypeLike to recognize this type.
  using is_runtime_type = void;

  type_id id_{};
  Any     value_{};
};

using runtime_type = basic_runtime_type<>;

} // namespace ouly
