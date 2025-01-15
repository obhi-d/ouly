#pragma once

#include <acl/reflection/reflect.hpp>
#include <tuple>

namespace acl::detail
{
// Concepts
// Decl
template <typename Class>
using bind_type = decltype(reflect<Class>());

// Utils
template <typename Class>
inline constexpr std::size_t field_count = std::tuple_size_v<bind_type<std::decay_t<Class>>>;

template <typename Class>
concept ExplicitlyReflected = (field_count<Class>) > 0;

template <typename Class>
concept ByteStreamable =
 std::is_standard_layout_v<Class> && std::is_trivial_v<Class> && std::has_unique_object_representations_v<Class>;

template <typename Class, typename Serializer>
concept ByteStreambleClass = ByteStreamable<Class> && !ExplicitlyReflected<Class> &&
                             !OutputSerializableClass<Class, Serializer> && !InputSerializableClass<Class, Serializer>;

template <typename Class, typename Serializer>
concept LinearArrayLike = requires(Class c) {
  typename Class::value_type;
  c.data();
  { c.size() } -> std::convertible_to<std::size_t>;
} && ByteStreambleClass<typename Class::value_type, Serializer>;

} // namespace acl::detail
