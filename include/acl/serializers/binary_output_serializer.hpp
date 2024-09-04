
#pragma once

#include <acl/utils/reflection.hpp>
#include <acl/utils/reflection_utils.hpp>
#include <acl/utils/type_traits.hpp>
#include <cassert>
#include <memory>
#include <optional>

namespace acl
{ // clang-format off
template <typename V>
concept BinaryOutputStream = requires(V v, std::size_t N) 
{   
  // function: Must have a write function to write bytes
  v.write(std::declval<std::byte const*>(), N);

};
// clang-format on

template <BinaryOutputStream Serializer, std::endian Endian = std::endian::little>
class binary_output_serializer
{
protected:
  std::reference_wrapper<Serializer> ser_;

  static constexpr bool has_fast_path = (Endian == std::endian::native);

public:
  binary_output_serializer(binary_output_serializer const&) noexcept = delete;
  binary_output_serializer(binary_output_serializer&& i_other) noexcept : ser_(i_other.ser_) {}
  inline binary_output_serializer(Serializer& ser) noexcept : ser_(ser) {}

  template <typename Class>
  inline auto& operator<<(Class& obj)
  {
    write(obj);
    return *this;
  }

  template <typename Class>
  inline void write(Class const& obj) noexcept
  {
    // Ensure ordering with multiple matches
    if constexpr (detail::BoundClass<Class>)
      write_bound_class(obj);
    else if constexpr (detail::OutputSerializableClass<Class, Serializer>)
      write_serializable(obj);
    else if constexpr (detail::TupleLike<Class>)
      write_tuple(obj);
    else if constexpr (detail::ContainerLike<Class>)
      write_container(obj);
    else if constexpr (detail::VariantLike<Class>)
      write_variant(obj);
    else if constexpr (detail::CastableToStringView<Class>)
      write_string_view_castable(obj);
    else if constexpr (detail::CastableToString<Class>)
      write_string_castable(obj);
    else if constexpr (detail::TransformToStringView<Class>)
      write_string_view_transformable(obj);
    else if constexpr (detail::TransformToString<Class>)
      write_string_transformable(obj);
    else if constexpr (detail::StringLike<Class>)
      write_string(obj);
    else if constexpr (detail::BoolLike<Class>)
      write_bool(obj);
    else if constexpr (detail::IntegerLike<Class>)
      write_integer(obj);
    else if constexpr (detail::EnumLike<Class>)
      write_enum(obj);
    else if constexpr (detail::FloatLike<Class>)
      write_float(obj);
    else if constexpr (detail::PointerLike<Class>)
      write_pointer(obj);
    else if constexpr (detail::OptionalLike<Class>)
      write_optional(obj);
    else if constexpr (detail::MonostateLike<Class>)
      write_monostate(obj);
    else
    {
      []<bool flag = false>()
      {
        static_assert(flag, "This type is not serializable");
      }
      ();
    }
  }

private:
  template <detail::BoundClass Class>
  void write_bound_class(Class const& obj) noexcept
  {
    constexpr uint32_t h = type_hash<Class>();
    write(h);
    for_each_field(*this, obj);
  }

  template <detail::OutputSerializableClass<Serializer> Class>
  void write_serializable(Class& obj) noexcept
  {
    constexpr uint32_t h = type_hash<Class>();
    write(h);
    get() << obj;
  }

  template <detail::TupleLike Class>
  void write_tuple(Class const& obj) noexcept
  {
    constexpr auto tup_size = std::tuple_size_v<Class>;
    static_assert(tup_size < 256, "Tuple is too big, please customize the serailization!");
    uint8_t size = static_cast<uint8_t>(tup_size);
    write(size);

    [this, &obj]<std::size_t... N>(std::index_sequence<N...>)
    {
      return (at<N>(obj), ...);
    }(std::make_index_sequence<std::tuple_size_v<Class>>());
  }

  template <detail::ContainerLike Class>
  void write_container(Class const& obj) noexcept
  {
    constexpr uint32_t h = type_hash<Class>();
    write(h);
    // Invalid type is unexpected
    uint32_t count = static_cast<uint32_t>(obj.size());
    write(count);
    if constexpr (detail::LinearArrayLike<Class, Serializer> && has_fast_path)
    {
      return get().write(obj.data(), sizeof(typename Class::value_type) * count);
    }
    else
    {
      for (auto const& value : obj)
      {
        write(value);
      }
    }
  }

  template <detail::VariantLike Class>
  void write_variant(Class const& obj) noexcept
  {
    // Invalid type is unexpected
    auto idx = static_cast<uint8_t>(obj.index());
    write(idx);
    std::visit(
      [this](auto const& arg)
      {
        write(arg);
      },
      obj);
  }

  template <detail::CastableToStringView Class>
  void write_string_view_castable(Class const& obj) noexcept
  {
    write_string(std::string_view(obj));
  }

  template <detail::CastableToString Class>
  void write_string_castable(Class const& obj) noexcept
  {
    write_string(std::string(obj));
  }

  template <detail::TransformToString Class>
  void write_string_transformable(Class const& obj) noexcept
  {
    write_string(acl::to_string(obj));
  }

  template <detail::TransformToStringView Class>
  void write_string_view_transformable(Class const& obj) noexcept
  {
    write_string(acl::to_string_view(obj));
  }

  template <detail::BoolLike Class>
  void write_bool(Class const& obj) noexcept
  {
    get().write(&obj, sizeof(obj));
  }

  template <detail::IntegerLike Class>
  void write_integer(Class obj) noexcept
  {
    if constexpr (has_fast_path)
      get().write(&obj, sizeof(obj));
    else
    {
      obj = detail::byteswap(obj);
      get().write(&obj, sizeof(obj));
    }
  }

  template <detail::EnumLike Class>
  void write_enum(Class obj) noexcept
  {
    using type = std::underlying_type_t<Class>;
    if constexpr (has_fast_path)
      get().write(&obj, sizeof(obj));
    else
    {
      auto data = static_cast<type>(obj);
      data      = detail::byteswap(data);
      get().write(&data, sizeof(data));
    }
  }

  void write_float(float obj) noexcept
  {
    if constexpr (has_fast_path)
      get().write(&obj, sizeof(obj));
    else
    {
      auto& ref = (uint32_t&)obj;
      ref       = detail::byteswap(ref);
      get().write(&ref, sizeof(ref));
    }
  }

  void write_float(double obj) noexcept
  {
    if constexpr (has_fast_path)
      get().write(&obj, sizeof(obj));
    else
    {
      auto& ref = (uint64_t&)obj;
      ref       = detail::byteswap(ref);
      get().write(&ref, sizeof(ref));
    }
  }

  template <detail::PointerLike Class>
  void write_pointer(Class const& obj) noexcept
  {
    bool is_null = !(bool)(obj);
    write(is_null);
    if (obj)
      write(*obj);
  }

  template <detail::OptionalLike Class>
  void write_optional(Class const& obj) noexcept
  {
    bool is_null = !(bool)obj;
    write(is_null);
    if (obj)
      write(*obj);
  }

  template <detail::MonostateLike Class>
  void write_monostate(Class const& obj) noexcept
  {}

public:
  template <typename Class, typename Decl, std::size_t I>
  inline void operator()(Class const& obj, Decl const& decl, std::integral_constant<std::size_t, I>) noexcept
  {
    write(decl.value(obj));
  }

private:
  inline void write_string(std::string_view sv)
  {
    uint32_t length = static_cast<uint32_t>(sv.length());
    write(length);
    get().write(sv.data(), length);
  }

  inline auto& get() noexcept
  {
    return ser_.get();
  }

  inline auto const& get() const noexcept
  {
    return ser_.get();
  }

  template <std::size_t N, typename Class>
  void at(Class const& obj) noexcept
  {
    write(std::get<N>(obj));
  }
};

namespace detail
{
struct empty_output_streamer
{
  void write(std::byte* data, size_t s) {}
};

} // namespace detail

template <typename Class>
concept OutputSerializable =
  detail::BoundClass<Class> || detail::OutputSerializableClass<Class, detail::empty_output_streamer> ||
  detail::TupleLike<Class> || detail::ContainerLike<Class> || detail::VariantLike<Class> ||
  detail::CastableToStringView<Class> || detail::CastableToString<Class> || detail::TransformToStringView<Class> ||
  detail::TransformToString<Class> || detail::StringLike<Class> || detail::BoolLike<Class> ||
  detail::IntegerLike<Class> || detail::EnumLike<Class> || detail::FloatLike<Class> || detail::PointerLike<Class> ||
  detail::OptionalLike<Class> || detail::MonostateLike<Class>;

} // namespace acl
