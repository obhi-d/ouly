#pragma once

#include <acl/reflection/detail/deduced_types.hpp>

namespace acl::opt
{
template <string_literal Name>
struct key_field_name
{
  using key_field_name_t = detail::field_name<Name>;
};

template <string_literal Name>
struct value_field_name
{
  using value_field_name_t = detail::field_name<Name>;
};

template <string_literal Name>
struct type_field_name
{
  using type_field_name_t = detail::field_name<Name>;
};
} // namespace acl::opt