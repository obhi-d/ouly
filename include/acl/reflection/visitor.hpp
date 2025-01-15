
#pragma once
#include <acl/reflection/detail/field_helpers.hpp>
#include <exception>
#include <format>

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
 * - Convertible types
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

struct visitor_error : std::exception
{
  enum code : uint8_t
  {
    unknown,
    invalid_tuple,
    invalid_container,
    invalid_variant,
    invalid_variant_type,
    invalid_aggregate,
    invalid_null_sentinel,
    invalid_value,
    invalid_key,
    type_is_not_an_object,
    type_is_not_an_array
  };

  explicit visitor_error(code errc) noexcept : code_(errc) {}

  [[nodiscard]] auto what() const noexcept -> const char* override
  {
    switch (code_)
    {
    case invalid_tuple:
      return "Invalid tuple";
    case invalid_container:
      return "Invalid container";
    case invalid_variant:
      return "Invalid variant";
    case invalid_variant_type:
      return "Invalid variant type";
    case invalid_aggregate:
      return "Invalid aggregate";
    case invalid_null_sentinel:
      return "Invalid null sentinel";
    case invalid_value:
      return "Invalid value";
    case invalid_key:
      return "Invalid key";
    case type_is_not_an_object:
      return "Type is not an object";
    case type_is_not_an_array:
      return "Type is not an array";
    default:
      return "Unknown visitor error";
    }
  }

  [[nodiscard]] auto get_code() const noexcept -> code
  {
    return code_;
  }

private:
  code code_ = code::unknown;
};

/**
 * @brief Post-read an object, visitors may use this to perform cleanup or finalization
 */
template <typename T>
void post_read(T& obj)
{
  if constexpr (std::is_pointer_v<T>)
  {
    if (obj)
    {
      post_read(*obj);
    }
  }
}

} // namespace acl