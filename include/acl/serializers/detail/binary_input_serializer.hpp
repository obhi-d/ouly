//
// Created by obhi on 9/18/20.
//
#pragma once

#include "acl/reflection/detail/base_concepts.hpp"
#include "acl/reflection/visitor.hpp"
#include <acl/reflection/detail/container_utils.hpp>
#include <acl/reflection/detail/derived_concepts.hpp>
#include <acl/reflection/detail/visitor_helpers.hpp>
#include <acl/reflection/reflection.hpp>
#include <acl/reflection/visitor_impl.hpp>
#include <acl/serializers/byteswap.hpp>
#include <acl/serializers/config.hpp>

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace acl::detail
{

/**
 * @brief Given an input serializer, load a bound class
 * @note The endian parameter should match between output and input
 */
template <typename Stream, std::endian Endian = std::endian::little>
class binary_input_serializer
{
private:
  enum class type : uint8_t
  {
    object,
    array,
    field
  };

  Stream*  serializer_    = nullptr;
  uint32_t object_id_     = 0;
  type     type_          = type::object;
  bool     may_fast_path_ = false;

  static constexpr bool has_fast_path = (Endian == std::endian::native);

public:
  using serializer_type              = Stream;
  using serializer_tag               = reader_tag;
  using transform_type               = acl::pass_through_transform;
  using size_type                    = cfg::container_size_type;
  using binary                       = std::true_type;
  static constexpr bool mutate_enums = false;

  auto operator=(const binary_input_serializer&) -> binary_input_serializer&     = default;
  auto operator=(binary_input_serializer&&) noexcept -> binary_input_serializer& = default;
  binary_input_serializer(binary_input_serializer const&)                        = default;
  binary_input_serializer(binary_input_serializer&& i_other) noexcept            = default;
  binary_input_serializer(Stream& ser) : serializer_{&ser} {}
  ~binary_input_serializer() noexcept = default;

  binary_input_serializer(acl::detail::field_visitor_tag /*unused*/, binary_input_serializer& ser, std::string_view key)
      : serializer_{ser.serializer_}, object_id_(ser.object_id_), may_fast_path_(ser.may_fast_path_), type_{type::field}
  {
    // No-op
  }

  binary_input_serializer(acl::detail::field_visitor_tag /*unused*/, binary_input_serializer& ser, size_t /*index*/)
      : serializer_{ser.serializer_}, object_id_(ser.object_id_), may_fast_path_(ser.may_fast_path_), type_{type::field}
  {
    // No-op
  }

  binary_input_serializer(acl::detail::object_visitor_tag /*unused*/, binary_input_serializer& ser)
      : serializer_{ser.serializer_}, may_fast_path_(ser.may_fast_path_), type_{type::object}
  {
    // No-op
  }

  binary_input_serializer(acl::detail::array_visitor_tag /*unused*/, binary_input_serializer& ser)
      : serializer_{ser.serializer_}, object_id_(ser.object_id_), may_fast_path_(ser.may_fast_path_), type_{type::array}
  {
    // No-op
  }

  template <typename Class>
  auto can_visit(Class& obj) -> continue_token
  {
    using class_type = std::decay_t<Class>;
    if (!may_fast_path_)
    {
      if constexpr (requires { Class::magic_type_header; })
      {
        return (read_id() == Class::magic_type_header);
      }
      else if constexpr (cfg::magic_type_header<std::decay_t<Class>>)
      {
        constexpr uint32_t match_id = cfg::magic_type_header<std::decay_t<Class>>;
        return (read_id() == match_id);
      }
    }
    return true;
  }

  void visit(auto&& fn)
  {
    fn(read_string());
  }

  template <acl::detail::InputSerializableClass<Stream> T>
  void visit(T& obj)
  {
    (*serializer_) >> obj;
  }

  template <typename Class>
  void for_each_entry(Class& obj, auto&& fn)
  {

    using type                   = std::decay_t<Class>;
    constexpr bool may_fast_path = acl::detail::LinearArrayLike<type, Stream>;

    // First time entering a fast path container
    may_fast_path_ = may_fast_path;

    size_type count = 0;
    visit(count);
    acl::detail::reserve(obj, count);

    if constexpr (may_fast_path && has_fast_path)
    {
      acl::detail::resize(obj, count);
      // NOLINTNEXTLINE
      get().read(reinterpret_cast<std::byte*>(std::data(obj)),
                 sizeof(typename Class::value_type) *
                  std::min<size_type>(static_cast<size_type>(std::size(obj)), count));
      if (count > std::size(obj))
      {
        get().skip((count - std::size(obj)) * sizeof(typename Class::value_type));
      }
      return;
    }
    else if constexpr (!acl::detail::ContainerCanAppendValue<Class>)
    {
      acl::detail::resize(obj, count);
    }

    for (size_type i = 0; i < count; ++i)
    {
      fn(*this);
    }
  }

  template <acl::detail::BoolLike Class>
  auto visit(Class& obj)
  {
    // NOLINTNEXTLINE
    get().read(reinterpret_cast<std::byte*>(&obj), sizeof(obj));
  }

  template <typename Class>
    requires(acl::detail::IntegerLike<Class> || acl::detail::FloatLike<Class> || acl::detail::EnumLike<Class>)
  void visit(Class& obj)
  {
    if constexpr (has_fast_path)
    {
      // NOLINTNEXTLINE
      get().read(reinterpret_cast<std::byte*>(&obj), sizeof(obj));
    }
    else
    {
      // NOLINTNEXTLINE
      get().read(reinterpret_cast<std::byte*>(&obj), sizeof(obj));
      obj = byteswap(obj);
    }
  }

  [[nodiscard]] auto is_null() -> bool
  {
    uint8_t value = 0;
    visit(value);
    if (value == cfg::null_sentinel)
    {
      return true;
    }
    if (value != cfg::not_null_sentinel)
    {
      throw visitor_error(visitor_error::invalid_null_sentinel);
    }
    return false;
  }

private:
  auto read_id() -> uint32_t
  {
    if (object_id_ == 0)
    {
      visit(object_id_);
    }
    return object_id_;
  }

  auto read_string() -> std::string
  {
    size_type count = 0;
    visit(count);
    std::string buffer;
    buffer.resize(count);
    // NOLINTNEXTLINE
    serializer_->read(reinterpret_cast<std::byte*>(buffer.data()), count);
    return buffer;
  }

  auto get() -> auto&
  {
    assert(serializer_ != nullptr);
    return *serializer_;
  }

  auto get() const -> auto const&
  {
    assert(serializer_ != nullptr);
    return *serializer_;
  }
};

struct empty_input_streamer
{
  static void read(std::byte* data, size_t s) {}
};

} // namespace acl::detail
