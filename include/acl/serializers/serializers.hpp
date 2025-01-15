#pragma once

#include <acl/serializers/detail/binary_input_serializer.hpp>
#include <acl/serializers/detail/binary_output_serializer.hpp>
#include <acl/serializers/detail/structured_input_serializer.hpp>
#include <acl/serializers/detail/structured_output_serializer.hpp>
#include <acl/utility/transforms.hpp>

namespace acl
{

template <typename V>
concept StructuredInputStream = requires(V v) {
  // function: Must return object_type
  { v.is_object() } -> ::std::same_as<bool>;

  // function: Must return object_type
  { v.is_array() } -> ::std::same_as<bool>;

  // function: Must return object_type
  { v.is_null() } -> ::std::same_as<bool>;

  // size
  { v.size() } -> ::std::convertible_to<std::size_t>;

  // function for_each: Should accept a lambda that accepts a value_type
  // This function should consume a field that is an array of values
  {
    v.for_each_entry(
     [](V) -> bool
     {
       return false;
     })
  } -> std::same_as<void>;

  // function object: Must return object_type
  { v.at(::std::string_view()) } -> std::same_as<std::optional<std::decay_t<V>>>;

  // function object: Must return object_type
  { v.at(std::declval<size_t>()) } -> std::same_as<std::optional<std::decay_t<V>>>;

  // Must convert value_type to double
  { v.as_double() } -> acl::detail::OptionalValueLike;

  // Must convert value_type to float
  { v.as_uint64() } -> acl::detail::OptionalValueLike;

  // Must convert value_type to float
  { v.as_int64() } -> acl::detail::OptionalValueLike;

  // Must convert value_type to float
  { v.as_bool() } -> acl::detail::OptionalValueLike;

  // Must convert value_type to float
  { v.as_string() } -> acl::detail::OptionalValueLike;
};

template <typename V>
concept StructuredOutputStream = requires(V v) {
  // begin array
  v.begin_array();

  // end array
  v.end_array();

  // begin object
  v.begin_object();

  // end array
  v.end_object();

  // key
  v.key(std::string_view());

  // value
  v.as_string(std::string_view());

  v.as_uint64(uint64_t());

  v.as_int64(int64_t());

  v.as_double(double());

  v.as_bool(bool());

  v.as_null();

  // begin of next key
  v.next_map_entry();
  v.next_array_entry();
};

template <typename V>
concept BinaryInputStream = requires(V v, std::size_t N) {
  // function: Must have a read function to read bytes
  { v.read(std::declval<std::byte*>(), N) } -> std::same_as<void>;
  { v.skip(N) } -> std::same_as<void>;
};

template <typename V>
concept BinaryOutputStream = requires(V v, std::size_t N) {
  // function: Must have a write function to write bytes
  v.write(std::declval<std::byte const*>(), N);
};

template <typename Config = acl::config<>>
void read(auto& stream, auto&& obj)
  requires StructuredInputStream<decltype(stream)>
{
  auto state = detail::structured_input_serializer<std::decay_t<decltype(stream)>, Config>(stream);
  visit(obj, state);
}

template <typename Config = acl::config<>>
void write(auto& stream, auto&& obj)
  requires StructuredOutputStream<decltype(stream)>
{
  auto state = detail::structured_output_serializer<std::decay_t<decltype(stream)>, Config>(stream);
  visit(obj, state);
}

template <std::endian Endian = std::endian::little>
void read(auto& stream, auto&& obj)
  requires BinaryInputStream<decltype(stream)>
{
  auto state = detail::binary_input_serializer<std::decay_t<decltype(stream)>, Endian>(stream);
  visit(obj, state);
}

template <std::endian Endian = std::endian::little>
void write(auto& stream, auto&& obj)
  requires BinaryOutputStream<decltype(stream)>
{
  auto state = detail::binary_output_serializer<std::decay_t<decltype(stream)>, Endian>(stream);
  visit(obj, state);
}

} // namespace acl