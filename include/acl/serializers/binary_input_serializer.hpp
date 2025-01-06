//
// Created by obhi on 9/18/20.
//
#pragma once

#include <acl/utils/error_codes.hpp>
#include <acl/utils/reflection.hpp>
#include <acl/utils/reflection_utils.hpp>
#include <acl/utils/type_traits.hpp>
#include <bit>
#include <cassert>
#include <cstddef>
#include <ios>
#include <limits>
#include <memory>
#include <optional>

namespace acl
{

template <typename V>
concept BinaryInputStream = requires(V v, std::size_t N) {
  // function: Must have a read function to read bytes
  { v.read(std::declval<std::byte*>(), N) } -> std::same_as<bool>;

  // function that accepts error
  v.error(std::string_view(), std::error_code());

  // function: Must return true if fail bit is set
  { v.failed() } -> ::std::same_as<bool>;
};

/**
 * @brief Given an input serializer, load a bound class
 * @note The endian parameter should match between output and input
 */
template <BinaryInputStream Serializer, std::endian Endian = std::endian::little>
class binary_input_serializer
{
private:
  std::reference_wrapper<Serializer> ser_;

  static constexpr bool has_fast_path = (Endian == std::endian::native);

public:
  auto operator=(const binary_input_serializer&) -> binary_input_serializer& = default;
  auto operator=(binary_input_serializer&&) -> binary_input_serializer&      = default;
  binary_input_serializer(binary_input_serializer const&) noexcept           = default;
  binary_input_serializer(binary_input_serializer&& i_other) noexcept : ser_(i_other.ser_) {}
  binary_input_serializer(Serializer& ser) noexcept : ser_(ser) {}
  ~binary_input_serializer() noexcept = default;

  template <typename Class>
  auto operator>>(Class& obj) -> auto&
  {
    if (!ser_.get().failed())
    {
      if (!read(obj) && !ser_.get().failed())
      {
        ser_.get().error(type_name<Class>(), make_error_code(serializer_error::failed_to_parse_value));
      }
    }
    return *this;
  }

  template <typename Class>
  auto read(Class& obj) noexcept -> bool
  {
    if (ser_.get().failed())
    {
      return false;
    }
    // Ensure ordering with multiple matches
    if constexpr (detail::BoundClass<Class>)
    {
      return read_bound_class(obj);
    }
    else if constexpr (detail::InputSerializableClass<Class, Serializer>)
    {
      return read_serializable(obj);
    }
    else if constexpr (detail::TransformFromString<Class>)
    {
      return read_string_transformed(obj);
    }
    else if constexpr (detail::TupleLike<Class>)
    {
      return read_tuple(obj);
    }
    else if constexpr (detail::ContainerLike<Class>)
    {
      return read_container(obj);
    }
    else if constexpr (detail::VariantLike<Class>)
    {
      return read_variant(obj);
    }
    else if constexpr (detail::ConstructedFromStringView<Class>)
    {
      return read_string_constructed(obj);
    }
    else if constexpr (detail::BoolLike<Class>)
    {
      return read_bool(obj);
    }
    else if constexpr (detail::IntegerLike<Class>)
    {
      return read_integer(obj);
    }
    else if constexpr (detail::EnumLike<Class>)
    {
      return read_enum(obj);
    }
    else if constexpr (detail::FloatLike<Class>)
    {
      return read_float(obj);
    }
    else if constexpr (detail::PointerLike<Class>)
    {
      return read_pointer(obj);
    }
    else if constexpr (detail::OptionalLike<Class>)
    {
      return read_optional(obj);
    }
    else if constexpr (detail::MonostateLike<Class>)
    {
      return read_monostate(obj);
    }
    else
    {
      []<bool Flag = false>()
      {
        static_assert(Flag, "This type is not serializable");
      }();
      return false;
    }
  }

private:
  template <detail::BoundClass Class>
  auto read_bound_class(Class& obj) noexcept -> bool
  {
    uint32_t h = 0;
    read(h);
    if (h != type_hash<Class>())
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::invalid_key));
      return false;
    }

    bool status = true;
    for_each_field(
     [this, &status]<typename Decl>(Class& obj, Decl const& decl, auto) noexcept
     {
       using value_t = typename Decl::MemTy;
       value_t load;
       if (read(load))
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
  auto read_serializable(Class& obj) noexcept -> bool
  {
    uint32_t h = 0;
    read(h);
    if (h != type_hash<Class>())
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::invalid_key));
      return false;
    }

    get() >> obj;
    return !get().failed();
  }

  template <detail::TupleLike Class>
  auto read_tuple(Class& obj) noexcept -> bool
  {
    constexpr auto tup_size = std::tuple_size_v<Class>;
    static_assert(tup_size < std::numeric_limits<uint8_t>::max(),
                  "Tuple is too big, please customize the serailization!");
    uint8_t size = 0;
    read(size);
    if (size != tup_size)
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::invalid_tuple_size));
      return false;
    }
    return [this, &obj]<std::size_t... N>(std::index_sequence<N...>)
    {
      return (at<N>(obj) && ...);
    }(std::make_index_sequence<std::tuple_size_v<Class>>());
  }

  template <detail::ContainerLike Class>
  auto read_container(Class& obj) noexcept -> bool
  {
    uint32_t h = 0;
    read(h);
    if (h != type_hash<Class>())
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::invalid_key));
      return false;
    }

    uint32_t count = 0;
    read(count);

    detail::reserve(obj, count);
    if constexpr (!detail::ContainerCanAppendValue<Class>)
    {
      detail::resize(obj, count);
    }
    if constexpr (detail::LinearArrayLike<Class, Serializer> && has_fast_path)
    {
      detail::resize(obj, count);
      return get().read(obj.data(), sizeof(typename Class::value_type) * count);
    }
    else
    {
      for (uint32_t index = 0; index < count; ++index)
      {
        detail::array_value_type<Class> stream_val;
        if (!read(stream_val))
        {
          get().error(type_name<Class>(), make_error_code(serializer_error::corrupt_array_item));
          return false;
        }
        if constexpr (detail::ContainerCanAppendValue<Class>)
        {
          detail::emplace(obj, std::move(stream_val));
        }
        else
        {
          obj[index] = std::move(stream_val);
        }
      }
    }

    return true;
  }

  template <detail::VariantLike Class>
  auto read_variant(Class& obj) noexcept -> bool
  {
    uint8_t index = 0;
    read(index);
    if (index >= std::variant_size_v<Class>)
    {
      get().error(type_name<Class>(), make_error_code(serializer_error::variant_invalid_index));
      return false;
    }
    return find_alt<std::variant_size_v<Class> - 1, Class>(
     static_cast<uint32_t>(index),
     [this, &obj](auto I) -> bool
     {
       using type = std::variant_alternative_t<I, Class>;
       type load;
       if (read(load))
       {
         obj = std::move(load);
         return true;
       }
       get().error(type_name<type>(), make_error_code(serializer_error::failed_streaming_variant));
       return false;
     });
  }

  template <detail::ConstructedFromStringView Class>
  auto read_string_constructed(Class& obj) noexcept -> bool
  {
    auto value = read_string();
    if (value)
    {
      obj = Class(*value);
      return true;
    }

    get().error("string", make_error_code(serializer_error::failed_to_parse_value));
    return false;
  }

  template <detail::TransformFromString Class>
  auto read_string_transformed(Class& obj) noexcept -> bool
  {
    auto value = read_string();
    if (value)
    {
      acl::from_string(obj, *value);
      return true;
    }

    get().error("string", make_error_code(serializer_error::failed_to_parse_value));
    return false;
  }

  template <detail::BoolLike Class>
  auto read_bool(Class& obj) noexcept -> bool
  {
    return get().read(&obj, sizeof(obj));
  }

  template <detail::IntegerLike Class>
  auto read_integer(Class& obj) noexcept -> bool
  {
    if constexpr (has_fast_path)
    {
      return get().read(&obj, sizeof(obj));
    }
    else
    {
      bool result = get().read(&obj, sizeof(obj));
      obj         = detail::byteswap(obj);
      return result;
    }
  }

  template <detail::EnumLike Class>
  auto read_enum(Class& obj) noexcept -> bool
  {
    using type = std::underlying_type_t<Class>;
    if constexpr (has_fast_path)
    {
      return get().read(&obj, sizeof(obj));
    }
    else
    {
      bool result = get().read(&obj, sizeof(obj));
      obj         = static_cast<Class>(detail::byteswap(static_cast<type>(obj)));
      return result;
    }
  }

  auto read_float(float& obj) noexcept -> bool
  {
    if constexpr (has_fast_path)
    {
      return get().read(&obj, sizeof(obj));
    }
    else
    {
      union
      {
        float    val_;
        uint32_t ref_;
      } data      = {obj};
      bool result = get().read(&data.ref_, sizeof(data.ref_));
      data.ref_   = detail::byteswap(data.ref_);
      obj         = data.val_;
      return result;
    }
  }

  auto read_float(double& obj) noexcept -> bool
  {
    if constexpr (has_fast_path)
    {
      return get().read(&obj, sizeof(obj));
    }
    else
    {
      union
      {
        double   val_;
        uint64_t ref_;
      } data      = {obj};
      bool result = get().read(&data.ref_, sizeof(data.ref_));
      data.ref_   = detail::byteswap(data.ref_);
      obj         = data.val_;
      return result;
    }
  }

  template <detail::PointerLike Class>
  auto read_pointer(Class& obj) noexcept -> bool
  {
    bool is_null = false;
    if (get().read(&is_null, sizeof(is_null)))
    {
      if (!is_null)
      {
        using class_type  = detail::remove_cref<Class>;
        using pvalue_type = detail::pointer_class_type<Class>;
        if constexpr (std::same_as<class_type, std::shared_ptr<pvalue_type>>)
        {
          obj = std::make_shared<pvalue_type>();
        }
        else
        {
          obj = Class(new detail::pointer_class_type<Class>());
        }
        return read(*obj);
      }
      obj = nullptr;
      return true;
    }
    return false;
  }

  template <detail::OptionalLike Class>
  auto read_optional(Class& obj) noexcept -> bool
  {
    bool is_null = false;
    if (get().read(&is_null, sizeof(is_null)))
    {
      if (!is_null)
      {
        obj.emplace();
        return read(*obj);
      }
      obj.reset();
      return true;
    }
    return false;
  }

  template <detail::MonostateLike Class>
  auto read_monostate(Class& obj) noexcept -> bool
  {
    return true;
  }

  auto get() noexcept -> auto&
  {
    return ser_.get();
  }

  auto get() const noexcept -> auto const&
  {
    return ser_.get();
  }

  auto read_string() noexcept -> std::optional<std::string>
  {
    std::optional<std::string> result;
    uint32_t                   length = 0;
    if (!read(length))
    {
      get().error("string", make_error_code(serializer_error::corrupt_string_length));
    }
    else
    {
      result.emplace(length, 0);
      if (!get().read(result->data(), length))
      {
        get().error("string", make_error_code(serializer_error::corrupt_string));
        result.reset();
      }
    }
    return result;
  }

  template <std::size_t N, typename Class>
  auto at(Class& obj) noexcept -> bool
  {
    using type = detail::remove_cref<std::tuple_element_t<N, Class>>;
    return read(const_cast<type&>(std::get<N>(obj)));
  }

  template <std::size_t const I, typename Class, typename L>
  static constexpr auto find_alt(std::size_t i, L&& lambda) noexcept -> bool
  {
    if (I == i)
    {
      return std::forward<L>(lambda)(std::integral_constant<uint32_t, I>{});
    }
    if constexpr (I > 0)
    {
      return find_alt<I - 1, Class>(i, std::forward<L>(lambda));
    }
    else
    {
      return false;
    }
  }
};

namespace detail
{
struct empty_input_streamer
{
  static auto read(std::byte* data, size_t s) -> bool
  {
    return true;
  }

  void error(std::string_view /*unused*/, std::error_code /*unused*/) {}

  static auto failed() -> bool
  {
    return false;
  }
};

} // namespace detail

template <typename Class>
concept InputSerializable =
 detail::BoundClass<Class> || detail::InputSerializableClass<Class, detail::empty_input_streamer> ||
 detail::TupleLike<Class> || detail::ContainerLike<Class> || detail::VariantLike<Class> ||
 detail::ConstructedFromStringView<Class> || detail::TransformFromString<Class> || detail::BoolLike<Class> ||
 detail::IntegerLike<Class> || detail::EnumLike<Class> || detail::FloatLike<Class> || detail::PointerLike<Class> ||
 detail::OptionalLike<Class> || detail::MonostateLike<Class>;

} // namespace acl
