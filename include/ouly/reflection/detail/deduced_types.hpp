// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/utility/string_literal.hpp"

namespace ouly::detail
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

} // namespace ouly::detail