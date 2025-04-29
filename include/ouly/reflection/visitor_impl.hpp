// SPDX-License-Identifier: MIT

#pragma once
#include "ouly/reflection/detail/aggregate.hpp"
#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/reflection/detail/visitor_helpers.hpp"
#include "ouly/reflection/visitor.hpp"
#include "ouly/utility/always_false.hpp"

namespace ouly
{

/** @see ouly::visit */
template <typename Class, typename Visitor>
void visit(Class& obj, Visitor& visitor)
{
  using type            = std::decay_t<Class>;
  using visitor_type    = std::decay_t<Visitor>;
  using serializer_tag  = typename visitor_type::serializer_tag;
  using serializer_type = typename visitor_type::serializer_type;

  if constexpr (ouly::detail::ExplicitlyReflected<type>)
  {
    return ouly::detail::visit_explicitly_reflected(obj, visitor);
  }
  else if constexpr ((std::same_as<serializer_tag, reader_tag> &&
                      ouly::detail::InputSerializableClass<type, serializer_type>) ||
                     (std::same_as<serializer_tag, writer_tag> &&
                      ouly::detail::OutputSerializableClass<type, serializer_type>))
  {
    return ouly::detail::visit_serializable(obj, visitor);
  }
  else if constexpr (ouly::detail::Convertible<type>)
  {
    return ouly::detail::visit_convertible(obj, visitor);
  }
  else if constexpr (ouly::detail::TupleLike<type>)
  {
    return ouly::detail::visit_tuple(obj, visitor);
  }
  else if constexpr (ouly::detail::ContainerLike<type>)
  {
    return ouly::detail::visit_container(obj, visitor);
  }
  else if constexpr (ouly::detail::VariantLike<type>)
  {
    return ouly::detail::visit_variant(obj, visitor);
  }
  else if constexpr (ouly::detail::BoolLike<type> || ouly::detail::IntegerLike<type> || ouly::detail::FloatLike<type>)
  {
    return ouly::detail::visit_value(obj, visitor);
  }
  else if constexpr (ouly::detail::EnumLike<type>)
  {
    return ouly::detail::visit_enum(obj, visitor);
  }
  else if constexpr (ouly::detail::PointerLike<type>)
  {
    return ouly::detail::visit_pointer(obj, visitor);
  }
  else if constexpr (ouly::detail::OptionalLike<type>)
  {
    return ouly::detail::visit_optional(obj, visitor);
  }
  else if constexpr (ouly::detail::MonostateLike<type>)
  {
    return ouly::detail::visit_monostate(obj, visitor);
  }
  else if constexpr (ouly::detail::Aggregate<type>)
  {
    return ouly::detail::visit_aggregate(obj, visitor);
  }
  else
  {
    static_assert(always_false<type>, "This type is not serializable");
  }
}

} // namespace ouly