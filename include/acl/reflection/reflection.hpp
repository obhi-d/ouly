
#pragma once

#include <acl/reflection/reflect.hpp>
#include <acl/reflection/visitor_impl.hpp>

namespace acl
{

/**
 * @file reflection.hpp
 * @brief Provides compile-time reflection utilities for user-defined types.
 *
 * This header defines various concepts, utility types, and functions intended to enable
 * reflection of struct members, free functions, and class accessors. It is designed to
 * serve as a customization point for types, allowing them to be:
 *   - Bound for static introspection via @c bind(...)
 *   - Serialized, deserialized, or otherwise processed based on compile-time type properties
 */

} // namespace acl