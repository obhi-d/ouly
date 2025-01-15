
//
// Created by obhi on 9/18/20.
//
#pragma once

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
class structured_output_serializer
{

private:
  enum class type : uint8_t
  {
    none,
    object,
    array,
    field
  };

  Stream* serializer_ = nullptr;
  type    type_       = type::none;
  bool    first_      = true;

public:
  using serializer_type              = Stream;
  using serializer_tag               = writer_tag;
  using transform_type               = transform_t<Config>;
  using config_type                  = Config;
  static constexpr bool mutate_enums = requires { typename Config::mutate_enums_type; };

  auto operator=(const structured_output_serializer&) -> structured_output_serializer&     = default;
  auto operator=(structured_output_serializer&&) noexcept -> structured_output_serializer& = default;
  structured_output_serializer(structured_output_serializer const&)                        = default;
  structured_output_serializer(structured_output_serializer&& i_other) noexcept : serializer_(i_other.serializer_) {}
  structured_output_serializer(Stream& ser) : serializer_(&ser) {}
  ~structured_output_serializer() noexcept
  {
    if (serializer_)
    {
      switch (type_)
      {
      case type::object:
        serializer_->end_object();
        break;
      case type::array:
        serializer_->end_array();
        break;
      case type::field:
        break;
      }
    }
  }

  structured_output_serializer(acl::detail::field_visitor_tag /*unused*/, structured_output_serializer& ser,
                               std::string_view key)
      : serializer_{ser.serializer_}, type_{type::field}
  {
    if (ser.first_)
    {
      ser.first_ = false;
    }
    else
    {
      serializer_->next_map_entry();
    }

    if (serializer_)
    {
      serializer_->key(key);
    }
  }

  structured_output_serializer(acl::detail::field_visitor_tag /*unused*/, structured_output_serializer& ser,
                               size_t index)
      : serializer_{ser.serializer_}, type_{type::field}
  {
    if (ser.first_)
    {
      ser.first_ = false;
    }
    else
    {
      serializer_->next_array_entry();
    }
  }

  structured_output_serializer(acl::detail::object_visitor_tag /*unused*/, structured_output_serializer& ser)
      : serializer_{ser.serializer_}, type_{type::object}
  {
    if (serializer_)
    {
      serializer_->begin_object();
    }
  }

  structured_output_serializer(acl::detail::array_visitor_tag /*unused*/, structured_output_serializer& ser)
      : serializer_{ser.serializer_}, type_{type::array}
  {
    if (serializer_)
    {
      serializer_->begin_array();
    }
  }

  template <typename Class>
  auto can_visit(Class const& obj) -> continue_token
  {
    return true;
  }

  template <acl::detail::OutputSerializableClass<Stream> T>
  void visit(T& obj)
  {
    (*serializer_) << obj;
  }

  template <typename Class>
  void for_each_entry(Class const& obj, auto&& fn)
  {
    bool first = true;
    for (auto const& value : obj)
    {
      if (!first)
      {
        get().next_array_entry();
      }
      first = false;
      fn(value, *this);
    }
  }

  void visit(std::string_view str)
  {
    get().as_string(str);
  }

  template <acl::detail::BoolLike Class>
  void visit(Class const& obj)
  {
    get().as_bool(obj);
  }

  template <typename Class>
    requires(acl::detail::IntegerLike<Class> || acl::detail::EnumLike<Class>)
  void visit(Class const& obj)
  {
    if constexpr (std::is_unsigned_v<Class>)
    {
      get().as_uint64(static_cast<uint64_t>(obj));
    }
    else
    {
      get().as_int64(static_cast<int64_t>(obj));
    }
  }

  template <acl::detail::FloatLike Class>
  void visit(Class const& obj)
  {
    get().as_double(obj);
  }

  void set_null()
  {
    get().as_null();
  }

  void set_not_null() {}

private:
  auto get() -> Stream&
  {
    assert(serializer_ != nullptr);
    return *serializer_;
  }
};

} // namespace acl::detail
