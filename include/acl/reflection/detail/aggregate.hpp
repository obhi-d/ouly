#pragma once

#include <acl/utils/string_literal.hpp>
#include <cstddef>

namespace acl::detail
{

struct anyinit
{
  template <typename T>
  operator T() const;
};

template <auto M>
struct aggregate_field_name
{};

} // namespace acl::detail