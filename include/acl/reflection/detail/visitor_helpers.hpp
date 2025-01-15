
#pragma once

#include "field_helpers.hpp"
#include <acl/reflection/detail/aggregate.hpp>
#include <acl/reflection/detail/base_concepts.hpp>
#include <acl/reflection/detail/container_utils.hpp>
#include <acl/reflection/visitor.hpp>
#include <acl/utility/config.hpp>
#include <acl/utility/transforms.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace acl::detail
{
template <typename Visitor>
constexpr bool is_reader = std::same_as<typename Visitor::serializer_tag, reader_tag>;
template <typename Visitor>
constexpr bool is_writer = std::same_as<typename Visitor::serializer_tag, writer_tag>;

struct field_visitor_tag
{};
struct object_visitor_tag
{};
struct array_visitor_tag
{};

template <typename Class, typename Visitor, typename Decl>
void process_field(Class& obj, Visitor& visitor, Decl const& decl)
{
  using value_t = typename Decl::MemTy;

  Visitor field_visitor{field_visitor_tag{}, visitor, Visitor::transform_type::transform(decl.key())};

  if (!field_visitor.can_visit(obj))
  {
    return;
  }

  if constexpr (is_reader<Visitor>)
  {
    value_t load;
    visit(load, field_visitor);
    decl.value(obj, std::move(load));
  }
  else if constexpr (is_writer<Visitor>)
  {
    visit(decl.value(obj), field_visitor);
  }
}

template <typename Class, typename Visitor, typename Ref>
void process_field(Class& obj, Ref& ref, Visitor& visitor, std::string_view name)
{
  using value_t = std::decay_t<Ref>;

  Visitor field_visitor{field_visitor_tag{}, visitor, name};

  if (!field_visitor.can_visit(obj))
  {
    return;
  }

  if constexpr (is_reader<Visitor>)
  {
    value_t load;
    visit(load, field_visitor);
    ref = std::move(load);
  }
  else if constexpr (is_writer<Visitor>)
  {
    visit(ref, field_visitor);
  }
}

template <typename Class, typename Visitor>
void visit_explicitly_reflected(Class& obj, Visitor& visitor)
{

  Visitor object_visitor{object_visitor_tag{}, visitor};

  if (!object_visitor.can_visit(obj))
  {
    return;
  }

  for_each_field(
   [&]<typename Decl, std::size_t I>(Class& obj, Decl const& decl, std::integral_constant<std::size_t, I> /*unused*/)
   {
     process_field(obj, object_visitor, decl);
   },
   obj);

  if constexpr (is_reader<Visitor>)
  {
    post_read(obj);
  }
}

template <typename Class, typename Visitor>
void visit_convertible(Class& obj, Visitor& visitor)
{
  using class_type = std::decay_t<Class>;
  if constexpr (is_reader<Visitor>)
  {
    visitor.visit(
     [&](std::string_view str) -> void
     {
       if constexpr (Visitor::mutate_enums && std::is_enum_v<class_type>)
       {
         acl::convert<class_type>::from_string(obj, Visitor::transform_type::transform(str));
       }
       else
       {
         acl::convert<class_type>::from_string(obj, str);
       }
     });
  }
  else if constexpr (is_writer<Visitor>)
  {
    if constexpr (Visitor::mutate_enums && std::is_enum_v<class_type>)
    {
      visitor.visit(Visitor::transform_type::transform(acl::convert<class_type>::to_string(obj)));
    }
    else
    {
      visitor.visit(acl::convert<class_type>::to_string(obj));
    }
  }
}

template <typename Class, typename Visitor>
void visit_serializable(Class& obj, Visitor& visitor)
{
  visitor.visit(obj);
}

template <typename Class, typename Visitor>
void visit_at(Class& obj, std::size_t index, Visitor& visitor)
{
  using value_t = std::decay_t<Class>;

  Visitor field_visitor{field_visitor_tag{}, visitor, index};

  if (!field_visitor.can_visit(obj))
  {
    return;
  }

  visit(obj, field_visitor);
}

template <typename Class, typename Visitor>
void visit_tuple(Class& obj, Visitor& visitor)
{
  Visitor array_visitor{array_visitor_tag{}, visitor};
  if (!array_visitor.can_visit(obj))
  {
    throw visitor_error(visitor_error::invalid_tuple);
  }
  [&]<std::size_t... I>(std::index_sequence<I...>)
  {
    (visit_at(std::get<I>(obj), I, array_visitor), ...);
  }(std::make_index_sequence<std::tuple_size_v<std::decay_t<Class>>>{});
}

template <acl::detail::MapLike Class, typename Visitor>
  requires(is_reader<Visitor>)
void visit_container(Class& obj, Visitor& visitor)
{
  using key_type    = std::decay_t<typename Class::key_type>;
  using mapped_type = std::decay_t<typename Class::mapped_type>;

  Visitor array_visitor{array_visitor_tag{}, visitor};

  if (!array_visitor.can_visit(obj))
  {
    throw visitor_error(visitor_error::invalid_container);
  }

  size_t index = 0;
  array_visitor.for_each_entry(obj,
                               [&](auto& field_visitor)
                               {
                                 std::pair<key_type, mapped_type> value;

                                 visit(value, field_visitor);

                                 acl::detail::emplace(obj, index++, std::move(value.first), std::move(value.second));
                               });
}

template <acl::detail::MapLike Class, typename Visitor>
  requires(is_writer<Visitor>)
void visit_container(Class const& obj, Visitor& visitor)
{
  using key_type    = std::decay_t<typename Class::key_type>;
  using mapped_type = std::decay_t<typename Class::mapped_type>;

  Visitor array_visitor{array_visitor_tag{}, visitor};

  if (!array_visitor.can_visit(obj))
  {
    throw visitor_error(visitor_error::invalid_container);
  }

  array_visitor.for_each_entry(obj,
                               [&](auto const& value, auto& field_visitor)
                               {
                                 visit(value, field_visitor);
                               });
}

template <acl::detail::ArrayLike Class, typename Visitor>
  requires(is_reader<Visitor>)
void visit_container(Class& obj, Visitor& visitor)
{
  using value_type = std::decay_t<typename Class::value_type>;

  Visitor array_visitor{array_visitor_tag{}, visitor};

  if (!array_visitor.can_visit(obj))
  {
    throw visitor_error(visitor_error::invalid_container);
  }

  size_t index = 0;
  array_visitor.for_each_entry(obj,
                               [&](auto& field_visitor)
                               {
                                 value_type stream_val;

                                 visit(stream_val, field_visitor);

                                 acl::detail::emplace(obj, index++, std::move(stream_val));
                               });
}
template <acl::detail::ArrayLike Class, typename Visitor>
  requires(is_writer<Visitor>)
void visit_container(Class const& obj, Visitor& visitor)
{
  using value_type = std::decay_t<typename Class::value_type>;

  Visitor array_visitor{array_visitor_tag{}, visitor};

  if (!array_visitor.can_visit(obj))
  {
    throw visitor_error(visitor_error::invalid_container);
  }

  size_t index = 0;
  array_visitor.for_each_entry(obj,
                               [&](value_type const& stream_val, auto& field_visitor)
                               {
                                 visit(stream_val, field_visitor);

                                 acl::detail::emplace(obj, index++, std::move(stream_val));
                               });
}

template <typename Class, typename Visitor>
  requires(is_reader<Visitor>)
void visit_variant(Class& obj, Visitor& visitor)
{
  using type = std::decay_t<Class>;
  Visitor object_visitor{object_visitor_tag{}, visitor};

  if (!object_visitor.can_visit(obj))
  {
    throw visitor_error(visitor_error::invalid_variant);
  }

  constexpr auto variant_size  = std::variant_size_v<type>;
  uint8_t        variant_index = std::numeric_limits<uint8_t>::max();
  {
    Visitor field_visitor{field_visitor_tag{}, object_visitor, Visitor::transform_type::transform("type")};

    if (!field_visitor.can_visit(obj))
    {
      throw visitor_error(visitor_error::invalid_variant_type);
    }

    if constexpr (requires { typename Visitor::binary; })
    {
      visit(variant_index, field_visitor);
    }
    else
    {
      try
      {
        std::string variant_index_str;
        visit(variant_index_str, field_visitor);
        variant_index = static_cast<uint8_t>(index_transform<type>::to_index(variant_index_str));
      }
      catch (visitor_error const& e)
      {
        visit(variant_index, field_visitor);
      }
    }

    if (variant_index >= variant_size)
    {
      throw visitor_error(visitor_error::invalid_variant_type);
    }
  }

  {
    Visitor field_visitor{field_visitor_tag{}, object_visitor, Visitor::transform_type::transform("value")};

    if (!field_visitor.can_visit(obj))
    {
      throw visitor_error(visitor_error::invalid_variant);
    }

    auto emplace_item = [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
    {
      std::variant_alternative_t<I, Class> value;
      visit(value, field_visitor);
      obj = std::move(value);
    };

    [&]<std::size_t... I>(std::index_sequence<I...>)
    {
      (((I == variant_index ? (emplace_item(std::integral_constant<std::size_t, I>()), true) : false) || ...), false);
    }(std::make_index_sequence<variant_size>{});

    post_read(obj);
  }
}

template <typename Class, typename Visitor>
  requires(is_writer<Visitor>)
void visit_variant(Class const& obj, Visitor& visitor)
{
  using type = std::decay_t<Class>;
  Visitor object_visitor{object_visitor_tag{}, visitor};

  if (!object_visitor.can_visit(obj))
  {
    return;
  }

  auto variant_index = static_cast<uint8_t>(obj.index());
  {
    Visitor field_visitor{field_visitor_tag{}, object_visitor, Visitor::transform_type::transform("type")};

    if (!field_visitor.can_visit(obj))
    {
      throw visitor_error(visitor_error::invalid_variant_type);
    }

    if constexpr (requires { typename Visitor::binary; })
    {
      visit(variant_index, field_visitor);
    }
    else
    {
      auto variant_index_str = index_transform<type>::from_index(variant_index);
      visit(variant_index_str, field_visitor);
    }
  }

  {
    Visitor field_visitor{field_visitor_tag{}, object_visitor, Visitor::transform_type::transform("value")};

    if (!field_visitor.can_visit(obj))
    {
      throw visitor_error(visitor_error::invalid_variant);
    }

    std::visit(
     [&](auto const& value)
     {
       visit(value, field_visitor);
     },
     obj);

    post_read(obj);
  }
}

template <typename Class, typename Visitor>
void visit_value(Class& obj, Visitor& visitor)
{
  visitor.visit(obj);
}

template <typename Class, typename Visitor>
void visit_enum(Class& obj, Visitor& visitor)
{
  visitor.visit(obj);
}

template <typename Class, typename Visitor>
  requires(is_reader<Visitor>)
void visit_pointer(Class& obj, Visitor& visitor)
{
  if (visitor.is_null())
  {
    obj = nullptr;
    return;
  }

  if (!obj)
  {
    using class_type  = std::decay_t<Class>;
    using pvalue_type = acl::detail::pointer_class_type<Class>;

    if constexpr (is_specialization_of<std::shared_ptr, class_type>::value)
    {
      obj = std::make_shared<pvalue_type>();
    }
    else if constexpr (std::is_same_v<class_type, std::unique_ptr<pvalue_type>>)
    {
      obj = std::make_unique<pvalue_type>();
    }
    else if constexpr (std::is_same_v<class_type, std::unique_ptr<pvalue_type[]>>)
    {
      obj = std::make_unique<pvalue_type[]>(1);
    }
    else
    {
      static_assert(always_false<Class>, "Unsupported pointer type");
    }
  }

  visit(*obj, visitor);
}

template <typename Class, typename Visitor>
  requires(is_writer<Visitor>)
void visit_pointer(Class const& obj, Visitor& visitor)
{
  if (!obj)
  {
    visitor.set_null();
    return;
  }

  visitor.set_not_null();
  visit(*obj, visitor);
}

template <typename Class, typename Visitor>
  requires(is_reader<Visitor>)
void visit_optional(Class& obj, Visitor& visitor)
{
  if (visitor.is_null())
  {
    obj.reset();
    return;
  }

  if (!obj)
  {
    obj.emplace();
  }

  visit(*obj, visitor);
}

template <typename Class, typename Visitor>
  requires(is_writer<Visitor>)
void visit_optional(Class const& obj, Visitor& visitor)
{
  if (!obj)
  {
    visitor.set_null();
    return;
  }

  visitor.set_not_null();

  visit(*obj, visitor);
}

template <typename Class, typename Visitor>
void visit_monostate(Class& obj, Visitor& visitor)
{
  if constexpr (is_writer<Visitor>)
  {
    visitor.set_null();
  }
  else
  {
    if (!visitor.is_null())
    {
      throw visitor_error(visitor_error::invalid_value);
    }
  }
}

template <typename Class, typename Visitor>
void visit_aggregate(Class& obj, Visitor& visitor)
{
  Visitor object_visitor{object_visitor_tag{}, visitor};

  if (!object_visitor.can_visit(obj))
  {
    throw visitor_error(visitor_error::invalid_aggregate);
  }

  constexpr auto field_names = get_field_names<std::decay_t<Class>>();
  // auto           field_refs  = get_field_refs<std::decay_t<Class>>(obj);

  [&]<std::size_t... I>(std::index_sequence<I...>)
  {
    ((process_field(obj, get_field_ref<I>(obj), object_visitor, std::get<I>(field_names))), ...);
  }(std::make_index_sequence<std::tuple_size_v<decltype(field_names)>>());

  if constexpr (is_reader<Visitor>)
  {
    post_read(obj);
  }
}
} // namespace acl::detail