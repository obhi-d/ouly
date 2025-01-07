#pragma once

#include <acl/reflection/detail/concepts.hpp>
#include <acl/utils/string_literal.hpp>

namespace acl::detail
{

template <HasValueType Class>
using container_value_type = typename Class::value_type;

template <typename T>
constexpr auto get_pointer_class_type()
{
  if constexpr (IsBasicPointer<T>)
  {
    return std::decay_t<std::remove_pointer_t<std::decay_t<T>>>();
  }
  else if constexpr (IsSmartPointer<T>)
  {
    return std::decay_t<std::decay_t<typename T::element_type>>();
  }
}

template <typename T>
using pointer_class_type = decltype(get_pointer_class_type<T>());

template <string_literal Name>
struct field_name
{
  static constexpr auto value = Name;
};

template <typename T>
struct key_field_name
{
  using type = field_name<"key">;
};

template <HasKeyFieldName T>
struct key_field_name<T>
{
  using type = typename T::key_field_name_t;
};

template <typename T>
using key_field_name_t = typename key_field_name<T>::type;

template <typename T>
struct value_field_name
{
  using type = field_name<"value">;
};

template <HasValueFieldName T>
struct value_field_name<T>
{
  using type = typename T::value_field_name_t;
};

template <typename T>
using value_field_name_t = typename value_field_name<T>::type;

template <typename T>
struct type_field_name
{
  using type = field_name<"type">;
};

template <HasValueFieldName T>
struct type_field_name<T>
{
  using type = typename T::type_field_name_t;
};

template <typename T>
using type_field_name_t = typename type_field_name<T>::type;

} // namespace acl::detail