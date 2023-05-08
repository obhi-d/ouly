//
// Created by obhi on 9/18/20.
//
#pragma once

#include "detail/reflection_utils.hpp"
#include "reflection.hpp"
#include "type_traits.hpp"
#include <cassert>
#include <memory>
#include <optional>

namespace acl
{

enum class input_error_code
{
  none,
  invalid_type,
  failed_streaming_map,
  failed_streaming_array,
  failed_streaming_variant,
  failed_to_parse_value,
  variant_invalid_format,
  variant_index_is_not_int
};

// clang-format off
template <typename V>
concept InputSerializer = requires(V v) 
{  
  // function object: Must return object_type
  { v.is_object() } -> ::std::same_as<bool>;

  // function object: Must return object_type
  { v.is_array() } -> ::std::same_as<bool>;

  // function object: Must return object_type
  { v.is_null() } -> ::std::same_as<bool>;

  // function object: Must return true if fail bit is set
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
  v.error(std::string_view(), input_error_code()); 
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

  template <detail::BoundClass Class>
  bool operator()(Class& obj) noexcept
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
  bool operator()(Class& obj) noexcept
  {
    get() >> obj;
    return !get().failed();
  }

  template <detail::TupleLike Class>
  bool operator()(Class& obj) noexcept
  {
    // Invalid type is unexpected
    if (!get().is_array())
    {
      get().error(type_name<Class>(), input_error_code::invalid_type);
      return false;
    }
    return [ this, &obj ]<size_t... N>(std::index_sequence<N...>)
    {
      return (at<N>(obj) && ...);
    }
    (std::make_index_sequence<std::tuple_size_v<Class>>());
  }

  template <detail::StringMapLike Class>
  bool operator()(Class& obj) noexcept
  {
    // Invalid is not unexpected
    // Invalid type is unexpected
    if (!get().is_object())
    {
      get().error(type_name<Class>(), input_error_code::invalid_type);
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
        value.error(type_name<mapped_type>(), input_error_code::failed_streaming_map);
        return false;
      });
  }

  template <detail::ArrayLike Class>
  bool operator()(Class& obj) noexcept
  {
    // Invalid type is unexpected
    if (!get().is_array())
    {
      get().error(type_name<Class>(), input_error_code::invalid_type);
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
          value.error(type_name<Class>(), input_error_code::failed_streaming_array);
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
              value.error(type_name<Class>(), input_error_code::failed_streaming_array);
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
  bool operator()(Class& obj) noexcept
  {
    if (get().is_null())
      return true;

    // Invalid type is unexpected
    if (!get().is_array())
    {
      get().error(type_name<Class>(), input_error_code::invalid_type);
      return false;
    }

    if (get().size() != 2)
    {
      get().error(type_name<Class>(), input_error_code::variant_invalid_format);
      return false;
    }

    auto index_opt = get().at(0);
    // Missing value is not an error
    assert(index_opt);

    auto index = (*index_opt).as_uint64();
    if (!index)
    {
      get().error(type_name<Class>(), input_error_code::variant_index_is_not_int);
      return false;
    }

    auto value_opt = get().at(1);

    assert(value_opt);

    auto value = *value_opt;
    return find_alt<std::variant_size_v<Class> - 1, Class>(static_cast<uint32_t>(*index),
                                                           [&obj, &value](auto I) -> bool
                                                           {
                                                             using type = std::variant_alternative_t<I, Class>;
                                                             type load;
                                                             if (input_serializer(value)(load))
                                                             {
                                                               obj = std::move(load);
                                                               return true;
                                                             }
                                                             value.error(type_name<type>(),
                                                                         input_error_code::failed_streaming_variant);
                                                             return false;
                                                           });
  }

  template <detail::ConstructedFromStringView Class>
  bool operator()(Class& obj) noexcept
  {
    auto value = get().as_string();
    if (value)
    {
      obj = Class(*value);
      return true;
    }
    else
    {
      get().error("string", input_error_code::failed_to_parse_value);
      return false;
    }
  }

  template <detail::TransformFromString Class>
  bool operator()(Class& obj) noexcept
  {
    auto value = get().as_string();
    if (value)
    {
      acl::from_string(obj, *value);
      return true;
    }
    else
    {
      get().error("string", input_error_code::failed_to_parse_value);
      return false;
    }
  }

  template <detail::BoolLike Class>
  bool operator()(Class& obj) noexcept
  {
    auto value = get().as_bool();
    if (value)
    {
      obj = *value;
      return true;
    }
    else
    {
      get().error("bool", input_error_code::failed_to_parse_value);
      return false;
    }
  }

  template <detail::SignedIntLike Class>
  bool operator()(Class& obj) noexcept
  {
    auto value = get().as_int64();
    if (value)
    {
      obj = static_cast<Class>(*value);
      return true;
    }
    else
    {
      get().error("int64", input_error_code::failed_to_parse_value);
      return false;
    }
  }

  template <detail::UnsignedIntLike Class>
  bool operator()(Class& obj) noexcept
  {
    auto value = get().as_uint64();
    if (value)
    {
      obj = static_cast<Class>(*value);
      return true;
    }
    else
    {
      get().error("uint64", input_error_code::failed_to_parse_value);
      return false;
    }
  }

  template <detail::FloatLike Class>
  bool operator()(Class& obj) noexcept
  {
    auto value = get().as_double();
    if (value)
    {
      obj = static_cast<Class>(*value);
      return true;
    }
    else
    {
      get().error("float", input_error_code::failed_to_parse_value);
      return false;
    }
  }

  template <detail::PointerLike Class>
  bool operator()(Class& obj) noexcept
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
  bool operator()(Class& obj) noexcept
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
  bool operator()(Class& obj) noexcept
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
