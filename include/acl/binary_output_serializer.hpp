
#pragma once

#include "detail/reflection_utils.hpp"
#include "reflection.hpp"
#include "type_traits.hpp"
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

  template <detail::BoundClass Class>
  void operator()(Class const& obj) noexcept
  {
    uint32_t h = type_hash<Class>();
    (*this)(h);
    for_each_field(*this, obj);
  }

  template <detail::OutputSerializableClass<Serializer> Class>
  void operator()(Class& obj) noexcept
  {
    uint32_t h = type_hash<Class>();
    (*this)(h);
    get() << obj;
  }

  template <detail::TupleLike Class>
  void operator()(Class const& obj) noexcept
  {
    constexpr auto tup_size = std::tuple_size_v<Class>;
    static_assert(tup_size < 256, "Tuple is too big, please customize the serailization!");
    uint8_t size = static_cast<uint8_t>(tup_size);
    (*this)(size);

    [ this, &obj ]<size_t... N>(std::index_sequence<N...>)
    {
      return (at<N>(obj), ...);
    }
    (std::make_index_sequence<std::tuple_size_v<Class>>());
  }

  template <detail::ContainerLike Class>
  void operator()(Class const& obj) noexcept
  {
    uint32_t h = type_hash<Class>();
    (*this)(h);
    // Invalid type is unexpected
    uint32_t count = static_cast<uint32_t>(obj.size());
    (*this)(count);
    if constexpr (detail::LinearArrayLike<Class, Serializer> && has_fast_path)
    {
      return get().write(obj.data(), sizeof(typename Class::value_type) * count);
    }
    else
    {
      for (auto const& value : obj)
      {
        (*this)(value);
      }
    }
  }

  template <detail::VariantLike Class>
  void operator()(Class const& obj) noexcept
  {
    // Invalid type is unexpected
    auto idx = static_cast<uint8_t>(obj.index());
    (*this)(idx);
    std::visit(
      [this](auto const& arg)
      {
        (*this)(arg);
      },
      obj);
  }

  template <detail::CastableToStringView Class>
  void operator()(Class const& obj) noexcept
  {
    write_string(std::string_view(obj));
  }

  template <detail::CastableToString Class>
  void operator()(Class const& obj) noexcept
  {
    write_string(std::string(obj));
  }

  template <detail::TransformToString Class>
  void operator()(Class const& obj) noexcept
  {
    write_string(acl::to_string(obj));
  }

  template <detail::TransformToStringView Class>
  void operator()(Class const& obj) noexcept
  {
    write_string(acl::to_string_view(obj));
  }

  template <detail::BoolLike Class>
  void operator()(Class const& obj) noexcept
  {
    get().write(&obj, sizeof(obj));
  }

  template <detail::IntegerLike Class>
  void operator()(Class obj) noexcept
  {
    if constexpr (has_fast_path)
      get().write(&obj, sizeof(obj));
    else
    {
      obj = detail::byteswap(obj);
      get().write(&obj, sizeof(obj));
    }
  }

  void operator()(float obj) noexcept
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

  void operator()(double obj) noexcept
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

  template <detail::StringLike Class>
  void operator()(Class const& obj) noexcept
  {
    write_string(obj);
  }

  template <detail::PointerLike Class>
  void operator()(Class const& obj) noexcept
  {
    bool is_null = !(bool)(obj);
    (*this)(is_null);
    if (obj)
      (*this)(*obj);
  }

  template <detail::OptionalLike Class>
  void operator()(Class const& obj) noexcept
  {
    bool is_null = !(bool)obj;
    (*this)(is_null);
    if (obj)
      (*this)(*obj);
  }

  template <detail::MonostateLike Class>
  void operator()(Class const& obj) noexcept
  {}

  template <typename Class, typename Decl, std::size_t I>
  inline void operator()(Class const& obj, Decl const& decl, std::integral_constant<size_t, I>) noexcept
  {
    (*this)(decl.value(obj));
  }

private:
  inline void write_string(std::string_view sv)
  {
    uint32_t length = static_cast<uint32_t>(sv.length());
    (*this)(length);
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

  template <size_t N, typename Class>
  void at(Class const& obj) noexcept
  {
    (*this)(std::get<N>(obj));
  }
};

} // namespace acl
