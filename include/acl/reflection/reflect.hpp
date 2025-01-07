
#pragma once

#include <acl/reflection/bind.hpp>
#include <acl/reflection/detail/concepts.hpp>

namespace acl
{
template <detail::ClassWithReflect Class>
auto reflect() noexcept
{
  return Class::reflect();
}

template <typename Class = void>
auto reflect() noexcept
{
  return std::tuple<>();
}
} // namespace acl