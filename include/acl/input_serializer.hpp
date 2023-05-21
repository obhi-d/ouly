//
// Created by obhi on 9/18/20.
//
#pragma once

#include "detail/reflection_utils.hpp"
#include "error_codes.hpp"
#include "reflection.hpp"
#include "type_traits.hpp"
#include <cassert>
#include <memory>
#include <optional>

namespace acl
{

// clang-format off
template <typename V>
concept InputSerializer = requires(V v) 
{  
  // function: Must return object_type
  { v.is_object() } -> ::std::same_as<bool>;

  // function: Must return object_type
  { v.is_array() } -> ::std::same_as<bool>;

  // function: Must return object_type
  { v.is_null() } -> ::std::same_as<bool>;

  // function: Must return true if fail bit is set
  { v.failed() } -> ::std::same_as<bool>;

  // size
  { v.size() } -> ::std::convertible_to<size_t>;
                                 
  // function for_each: Should accept a lambda that accepts a key and value_type
  // This function should consume a field that is a map of key, values
  { v.for_each([](::std::string_view key, V) -> bool { return false; }) } -> std::same_as<bool>;
    
  // function for_each: Should accept a lambda that accepts a value_type
  // This function should consume a field that is an array of values
  { v.for_each([](V) -> bool { return false; }) } -> std::same_as<bool>;
    
  // function object: Must return object_type
  { v.at(::std::string_view()) } -> detail::OptionalValueLike<V>;
    
  // function object: Must return object_type given an array index
  { v.at(uint32_t(0)) } -> detail::OptionalValueLike<V>;

  // Must convert value_type to double
  { v.as_double() } -> detail::OptionalValueLike<double>;
  
  // Must convert value_type to float
  { v.as_uint64() } -> detail::OptionalValueLike<uint64_t>;

  // Must convert value_type to float
  { v.as_int64() } -> detail::OptionalValueLike<int64_t>;

  // Must convert value_type to float
  { v.as_bool() } -> detail::OptionalValueLike<bool>;

  // Must convert value_type to float
  { v.as_string() } -> detail::OptionalValueLike<std::string_view>;

  // error handler: context, error
  v.error(std::string_view(), std::error_code()); 
};
// clang-format on

// Given an input serializer, load
// a bound class
template <InputSerializer Serializer>
class input_serializer
{
protected:
  std::reference_wrapper<Serializer> ser_;

public:
  input_serializer(input_serializer const&) noexcept = delete;
  input_serializer(input_serializer&& i_other) noexcept : ser_(i_other.ser_) {}
  inline input_serializer(Serializer& ser) noexcept : ser_(ser) {}

  template <typename Class>
  inline bool operator()(Class& obj) noexcept
  {
    // Ensure ordering with multiple matches
    if constexpr (detail::BoundClass<Class>)
      return read_bound_class(obj);
    else if constexpr (detail::InputSerializableClass<Class, Serializer>)
      return read_serializable(obj);
    else if constexpr (detail::TupleLike<Class>)
      return read_tuple(obj);
    else if constexpr (detail::ContainerLike<Class>)
      return read_container(obj);
    else if constexpr (detail::VariantLike<Class>)
      return read_variant(obj);
    else if constexpr (detail::ConstructedFromStringView<Class>)
      return read_string_constructed(obj);
    else if constexpr (detail::TransformFromString<Class>)
      return read_string_transformed(obj);
    else if constexpr (detail::StringLike<Class>)
      return read_string(obj);
    else if constexpr (detail::BoolLike<Class>)
      return read_bool(obj);
    else if constexpr (detail::IntegerLike<Class>)
      return read_integer(obj);
    else if constexpr (detail::FloatLike<Class>)
      return read_float(obj);
    else if constexpr (detail::PointerLike<Class>)
      return read_pointer(obj);
    else if constexpr (detail::OptionalLike<Class>)
      return read_optional(obj);
    else if constexpr (detail::MonostateLike<Class>)
      return read_monostate(obj);
    else
    {
      []<bool flag = false>()
      {
        static_assert(flag, "This type is not serializable");
      }
      ();
      return false;
    }
  }

private:
  template <detail::BoundClass Class>
  bool read_bound_class(Class& obj) noexcept
  {
    bool status = true;
    for_each_field(
      [this, &status]<typename Decl>(Class& obj, Decl const& decl, auto) noexcept
      {
        auto key_val = get().at(decl.key());
        if (!key_val)
          return;

        using value_t = typename Decl::MemTy;
        value_t load;
        if (input_serializer(*key_val)(load))
        {
          decl.value(obj, std::move(load));
          return;
        }
        status = false;
      },
      obj);
    return status;
  }

  template <detail::InputSerializableClass<Serializer> Class>
  bool read_serializable(Class& obj) noexcept
  {
    get() >> obj;
    return !get().failed();
  }

  template <detail::TupleLike Class>
  bool read_tuple(Class& obj) noexcept
  {
    // Invalid type is unexpected
    if (!get().is_array())
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::invalid_type));
      return false;
    }
    return [ this, &obj ]<size_t... N>(std::index_sequence<N...>)
    {
      return (at<N>(obj) && ...);
    }
    (std::make_index_sequence<std::tuple_size_v<Class>>());
  }

  template <detail::StringMapLike Class>
  bool read_container(Class& obj) noexcept
  {
    // Invalid is not unexpected
    // Invalid type is unexpected
    if (!get().is_object())
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::invalid_type));
      return false;
    }

    using key_type    = detail::remove_cref<typename Class::key_type>;
    using mapped_type = detail::remove_cref<typename Class::mapped_type>;

    detail::reserve(obj, get().size());

    return get().for_each(
      [this, &obj](std::string_view key, Serializer value) -> bool
      {
        mapped_type stream_val;

        if (input_serializer(value)(stream_val))
        {
          detail::emplace(obj, key_type{key}, std::move(stream_val));
          return true;
        }
        value.error(type_name<mapped_type>(), make_error_code(serializer_error::failed_streaming_map));
        return false;
      });
  }

  template <detail::ArrayLike Class>
  bool read_container(Class& obj) noexcept
  {
    // Invalid type is unexpected
    if (!get().is_array())
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::invalid_type));
      return false;
    }

    detail::reserve(obj, get().size());
    if constexpr (detail::HasEmplaceFn<Class, detail::array_value_type<Class>>)
    {
      return get().for_each(

        [this, &obj](Serializer value)
        {
          detail::array_value_type<Class> stream_val;
          bool                            result = input_serializer(value)(stream_val);
          if (result)
          {
            detail::emplace(obj, std::move(stream_val));
            return true;
          }
          value.error(type_name<Class>(), make_error_code(serializer_error::failed_streaming_array));
          return false;
        });
    }
    else
    {
      detail::resize(obj, get().size());
      std::uint32_t index = 0;
      if (!get().for_each(

            [&obj, &index](Serializer value)
            {
              detail::array_value_type<Class> stream_val;
              bool                            result = input_serializer(value)(stream_val);
              if (result)
              {
                obj[index++] = std::move(stream_val);
                return true;
              }
              value.error(type_name<Class>(), make_error_code(serializer_error::failed_streaming_array));
              return false;
            }))
      {
        detail::resize(obj, index);
        return false;
      }
      return true;
    }
  }

  template <detail::VariantLike Class>
  bool read_variant(Class& obj) noexcept
  {
    if (get().is_null())
      return true;

    // Invalid type is unexpected
    if (!get().is_array())
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::invalid_type));
      return false;
    }

    if (get().size() != 2)
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::variant_invalid_format));
      return false;
    }

    auto index_opt = get().at(0);
    // Missing value is not an error
    ACL_ASSERT(index_opt);

    auto index = (*index_opt).as_uint64();
    if (!index)
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::variant_index_is_not_int));
      return false;
    }

    auto value_opt = get().at(1);

    ACL_ASSERT(value_opt);

    auto value = *value_opt;
    return find_alt<std::variant_size_v<Class> - 1, Class>(
      static_cast<uint32_t>(*index),
      [&obj, &value](auto I) -> bool
      {
        using type = std::variant_alternative_t<I, Class>;
        type load;
        if (input_serializer(value)(load))
        {
          obj = std::move(load);
          return true;
        }
        value.error(type_name<type>(), make_error_code(serializer_error::failed_streaming_variant));
        return false;
      });
  }

  template <detail::ConstructedFromStringView Class>
  bool read_string_constructed(Class& obj) noexcept
  {
    auto value = get().as_string();
    if (value)
    {
      obj = Class(*value);
      return true;
    }
    else
    {
      get().error("string", make_error_code(serializer_error::failed_to_parse_value));
      return false;
    }
  }

  template <detail::TransformFromString Class>
  bool read_string_transformed(Class& obj) noexcept
  {
    auto value = get().as_string();
    if (value)
    {
      acl::from_string(obj, *value);
      return true;
    }
    else
    {
      get().error("string", make_error_code(serializer_error::failed_to_parse_value));
      return false;
    }
  }

  template <detail::StringLike Class>
  bool read_string(Class& obj) noexcept
  {
    auto value = get().as_string();
    if (value)
    {
      obj = Class(*value);
      return true;
    }
    else
    {
      get().error("string", make_error_code(serializer_error::failed_to_parse_value));
      return false;
    }
  }

  template <detail::BoolLike Class>
  bool read_bool(Class& obj) noexcept
  {
    auto value = get().as_bool();
    if (value)
    {
      obj = *value;
      return true;
    }
    else
    {
      get().error("bool", make_error_code(serializer_error::failed_to_parse_value));
      return false;
    }
  }

  template <detail::SignedIntLike Class>
  bool read_integer(Class& obj) noexcept
  {
    auto value = get().as_int64();
    if (value)
    {
      obj = static_cast<Class>(*value);
      return true;
    }
    else
    {
      get().error("int64", make_error_code(serializer_error::failed_to_parse_value));
      return false;
    }
  }

  template <detail::UnsignedIntLike Class>
  bool read_integer(Class& obj) noexcept
  {
    auto value = get().as_uint64();
    if (value)
    {
      obj = static_cast<Class>(*value);
      return true;
    }
    else
    {
      get().error("uint64", make_error_code(serializer_error::failed_to_parse_value));
      return false;
    }
  }

  template <detail::FloatLike Class>
  bool read_float(Class& obj) noexcept
  {
    auto value = get().as_double();
    if (value)
    {
      obj = static_cast<Class>(*value);
      return true;
    }
    else
    {
      get().error("float", make_error_code(serializer_error::failed_to_parse_value));
      return false;
    }
  }

  template <detail::PointerLike Class>
  bool read_pointer(Class& obj) noexcept
  {
    if (!get().is_null())
    {
      using class_type  = detail::remove_cref<Class>;
      using pvalue_type = detail::pointer_class_type<Class>;
      if constexpr (std::same_as<class_type, std::shared_ptr<pvalue_type>>)
        obj = std::make_shared<pvalue_type>();
      else
        obj = Class(new detail::pointer_class_type<Class>());
      return (*this)(*obj);
    }
    obj = nullptr;
    return true;
  }

  template <detail::OptionalLike Class>
  bool read_optional(Class& obj) noexcept
  {
    if (!get().is_null())
    {
      obj.emplace();
      return (*this)(*obj);
    }
    else
      obj.reset();
    return true;
  }

  template <detail::MonostateLike Class>
  bool read_monostate(Class& obj) noexcept
  {
    return true;
  }

private:
  inline auto& get() noexcept
  {
    return ser_.get();
  }

  inline auto const& get() const noexcept
  {
    return ser_.get();
  }

  template <size_t N, typename Class>
  bool at(Class& obj) noexcept
  {
    auto ser = get().at(N);
    if (ser)
    {
      using type = detail::remove_cref<std::tuple_element_t<N, Class>>;
      return input_serializer(*ser)(const_cast<type&>(std::get<N>(obj)));
    }
    return true;
  }

  template <size_t const I, typename Class, typename L>
  static constexpr auto find_alt(size_t i, L&& lambda) noexcept -> bool
  {
    if (I == i)
      return std::forward<L>(lambda)(std::integral_constant<uint32_t, I>{});
    if constexpr (I > 0)
      return find_alt<I - 1, Class>(i, std::forward<L>(lambda));
    else
      return false;
  }
};

} // namespace acl
