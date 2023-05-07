
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

// clang-format off
template <typename V>
concept OutputSerializer = requires(V v) 
{   
  // begin array
  v.begin_array();

  // end array
  v.end_array();
  
  // begin object
  v.begin_object();

  // end array
  v.end_object();

  // key
  v.key(std::string_view());

  // value
  v.as_string(std::string_view());
    
  v.as_uint64(uint64_t());

  v.as_int64(int64_t());

  v.as_double(double());

  v.as_bool(bool());

  v.as_null();

  // begin of next key
  v.next();

};
// clang-format on

// Given an input serializer, load
// a bound class
template <OutputSerializer Serializer>
class output_serializer
{
protected:
  std::reference_wrapper<Serializer> ser_;

public:
  output_serializer(output_serializer const&) noexcept = delete;
  output_serializer(output_serializer&& i_other) noexcept : ser_(i_other.ser_) {}
  inline output_serializer(Serializer& ser) noexcept : ser_(ser) {}

  template <detail::BoundClass Class>
  void operator()(Class const& obj) noexcept
  {
    get().begin_object();
    detail::get_all<Class>(*this, obj);
    get().end_object();
  }

  template <detail::OutputSerializableClass<Serializer> Class>
  void operator()(Class& obj) noexcept
  {
    get() << obj;
  }

  template <detail::TupleLike Class>
  void operator()(Class const& obj) noexcept
  {
    // Invalid type is unexpected
    get().begin_array();
    [ this, &obj ]<size_t... N>(std::index_sequence<N...>)
    {
      return (at<N>(obj), ...);
    }
    (std::make_index_sequence<std::tuple_size_v<Class>>());
    get().end_array();
  }

  template <detail::StringMapLike Class>
  void operator()(Class const& obj) noexcept
  {

    using key_type    = detail::remove_cref<typename Class::key_type>;
    using mapped_type = detail::remove_cref<typename Class::mapped_type>;

    get().begin_object();
    bool comma = false;
    for (auto const& [key, value] : obj)
    {
      if (comma)
        get().next();
      get().key(key);
      (*this)(value);
      comma = true;
    }

    get().end_object();
  }

  template <detail::ArrayLike Class>
  void operator()(Class const& obj) noexcept
  {
    // Invalid type is unexpected
    get().begin_array();
    bool comma = false;
    for (auto const& value : obj)
    {
      if (comma)
        get().next();
      (*this)(value);
      comma = true;
    }
    get().end_array();
  }

  template <detail::VariantLike Class>
  void operator()(Class const& obj) noexcept
  {
    // Invalid type is unexpected
    get().begin_array();
    (*this)(obj.index());
    get().next();
    std::visit(
      [this](auto const& arg)
      {
        (*this)(arg);
      },
      obj);
    get().end_array();
  }

  template <detail::CastableToStringView Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_string(std::string_view(obj));
  }

  template <detail::CastableToString Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_string(std::string(obj));
  }

  template <detail::TransformToString Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_string(acl::to_string(obj));
  }

  template <detail::TransformToStringView Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_string(acl::to_string_view(obj));
  }

  template <detail::BoolLike Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_bool(obj);
  }

  template <detail::SignedIntLike Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_int64(obj);
  }

  template <detail::UnsignedIntLike Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_uint64(obj);
  }

  template <detail::FloatLike Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_double(obj);
  }

  template <detail::StringLike Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_string(obj);
  }

  template <detail::PointerLike Class>
  void operator()(Class const& obj) noexcept
  {
    if (obj)
      (*this)(*obj);
    else
      get().as_null();
  }

  template <detail::OptionalLike Class>
  void operator()(Class const& obj) noexcept
  {
    if (obj)
      (*this)(*obj);
    else
      get().as_null();
  }

  template <detail::MonostateLike Class>
  void operator()(Class const& obj) noexcept
  {
    get().as_null();
  }

  template <typename Class, typename Decl, std::size_t I>
  inline void operator()(Class const& obj, Decl const& decl, std::integral_constant<size_t, I>) noexcept
  {
    if constexpr (I != 0)
      get().next();
    get().key(decl.key());
    (*this)(decl.value(obj));
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
  void at(Class const& obj) noexcept
  {
    if constexpr (N != 0)
      get().next();
    (*this)(std::get<N>(obj));
  }
};

} // namespace acl
