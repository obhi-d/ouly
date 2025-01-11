
#pragma once
#include <acl/reflection/detail/field_helpers.hpp>

namespace acl
{
struct reader_tag
{};
struct writer_tag
{};

using continue_token = bool;

/**  Visitor concept is not enforced as some visitors may not need to implement all functions */
template <typename T, typename Class>
concept Visitor = requires(T& visitor, Class& obj) {
  { visitor.begin_object(obj) } -> std::same_as<bool>;
  { visitor.end_object(obj) } -> std::same_as<void>;
  { visitor.begin_array(obj) } -> std::same_as<bool>;
  { visitor.end_array(obj) } -> std::same_as<void>;
  { visitor.begin_field(obj, std::string_view{}, false) } -> std::same_as<void>;
  { visitor.end_field(obj) } -> std::same_as<void>;
  { visitor.is_null() } -> std::same_as<bool>;
  { visitor.null() } -> std::same_as<void>;
  {
    visitor.read_string([](std::string_view s) {})
  } -> std::same_as<void>;
  { visitor.write_string(std::string_view{}) } -> std::same_as<void>;
  { visitor.value(obj) } -> std::same_as<void>;
  {
    visitor.for_each_map_entry(obj, [](std::string_view key) {})
  } -> std::same_as<void>;
  {
    visitor.for_each_array_entry(obj, []() {})
  } -> std::same_as<void>;
};

/**
 * @brief Visits an object with a visitor using compile-time type detection to determine visitation strategy
 *
 * This function uses compile-time type traits to detect the type category of the object and delegates
 * to the appropriate specialized visitor implementation. It supports various C++ types including:
 * - Explicitly reflected types
 * - Serializable types (both input and output)
 * - Transformable types
 * - Tuple-like types
 * - Container-like types
 * - Variant-like types
 * - Primitive types (bool, integer, float)
 * - Enum types
 * - Pointer-like types
 * - Optional-like types
 * - Monostate types
 * - Aggregate types
 *
 * If none of the supported categories match, a static assertion failure is triggered.
 *
 * @tparam Class The type of the object to visit
 * @tparam Visitor The type of the visitor
 * @param obj The object to visit
 * @param visitor The visitor to use
 * @throws Throws any exceptions that may be thrown by the specialized visitor implementations
 */
template <typename Class, typename Visitor>
auto visit(Class& obj, Visitor& visitor) -> void;

} // namespace acl