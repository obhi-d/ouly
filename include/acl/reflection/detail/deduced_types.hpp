#pragma once

#include <acl/reflection/detail/base_concepts.hpp>
#include <acl/utility/string_literal.hpp>

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

} // namespace acl::detail