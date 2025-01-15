//
// Created by obhi on 9/18/20.
//
#pragma once

#include "acl/reflection/visitor.hpp"
#include <acl/reflection/detail/container_utils.hpp>
#include <acl/reflection/detail/derived_concepts.hpp>
#include <acl/reflection/detail/visitor_helpers.hpp>
#include <acl/reflection/reflection.hpp>
#include <acl/reflection/type_name.hpp>
#include <acl/reflection/visitor_impl.hpp>
#include <acl/utility/detail/concepts.hpp>

#include <cassert>

namespace acl::detail
{

// Given an input serializer, load
// a bound class
template <typename Stream, typename Config = acl::config<>>
class structured_input_serializer
{

private:
  std::optional<Stream> serializer_;

public:
  using serializer_type              = Stream;
  using serializer_tag               = reader_tag;
  using transform_type               = transform_t<Config>;
  using config_type                  = Config;
  static constexpr bool mutate_enums = requires { typename Config::mutate_enums_type; };

  auto operator=(const structured_input_serializer&) -> structured_input_serializer& = default;
  auto operator=(structured_input_serializer&&) -> structured_input_serializer&      = default;
  structured_input_serializer(structured_input_serializer const&) noexcept           = default;
  structured_input_serializer(structured_input_serializer&& i_other) noexcept : serializer_(i_other.serializer_) {}
  structured_input_serializer(Stream ser) noexcept : serializer_(std::move(ser)) {}
  ~structured_input_serializer() noexcept = default;

  structured_input_serializer(acl::detail::field_visitor_tag /*unused*/, structured_input_serializer& ser,
                              std::string_view key)
      : serializer_(ser.get().at(key))

  {}
  structured_input_serializer(acl::detail::field_visitor_tag /*unused*/, structured_input_serializer& ser,
                              std::size_t index)
      : serializer_(ser.get().at(index))

  {}

  structured_input_serializer(acl::detail::object_visitor_tag /*unused*/, structured_input_serializer& ser)
      : serializer_{ser.serializer_}
  {
    // No-op
    if (serializer_.has_value())
    {
      if (!serializer_->is_object())
      {
        serializer_.reset();
      }
    }
  }

  structured_input_serializer(acl::detail::array_visitor_tag /*unused*/, structured_input_serializer& ser)
      : serializer_{ser.serializer_}
  {
    // No-op
    if (serializer_.has_value())
    {
      if (!serializer_->is_array())
      {
        serializer_.reset();
      }
    }
  }

  template <typename Class>
  auto can_visit(Class& obj) -> continue_token
  {
    return serializer_.has_value();
  }

  void visit(auto&& fn)
  {
    auto value = get().as_string();
    if (value.has_value())
    {
      fn(*value);
    }
    else
    {
      throw visitor_error(visitor_error::invalid_value);
    }
  }

  template <acl::detail::InputSerializableClass<Stream> T>
  void visit(T& obj)
  {
    (*serializer_) >> obj;
  }

  template <typename Class>
  void for_each_entry(Class& obj, auto&& fn)
  {
    if constexpr (!ContainerCanAppendValue<Class>)
    {
      acl::detail::resize(obj, get().size());
    }

    try
    {
      get().for_each_entry(
       [&](Stream& value)
       {
         structured_input_serializer visitor{value};
         fn(visitor);
       });
    }
    catch (visitor_error const& e)
    {
      obj = {};
      throw;
    }
  }

  template <acl::detail::BoolLike Class>
  auto visit(Class& obj)
  {
    auto value = get().as_bool();
    if (!value.has_value())
    {
      throw visitor_error(visitor_error::invalid_value);
    }
    obj = *value;
  }

  template <typename Class>
    requires(acl::detail::IntegerLike<Class> || acl::detail::EnumLike<Class>)
  void visit(Class& obj)
  {
    if constexpr (std::is_unsigned_v<Class>)
    {
      auto value = get().as_uint64();
      if (!value.has_value())
      {
        throw visitor_error(visitor_error::invalid_value);
      }
      obj = static_cast<std::decay_t<Class>>(*value);
    }
    else
    {
      auto value = get().as_int64();
      if (!value.has_value())
      {
        throw visitor_error(visitor_error::invalid_value);
      }
      obj = static_cast<std::decay_t<Class>>(*value);
    }
  }

  template <acl::detail::FloatLike Class>
  void visit(Class& obj)
  {
    auto value = get().as_double();
    if (!value.has_value())
    {
      throw visitor_error(visitor_error::invalid_value);
    }
    obj = *value;
  }

  [[nodiscard]] auto is_null() -> bool
  {
    return (!serializer_.has_value()) || get().is_null();
  }

private:
  auto get() -> Stream&
  {
    assert(serializer_.has_value());
    return *serializer_;
  }
};

} // namespace acl::detail
