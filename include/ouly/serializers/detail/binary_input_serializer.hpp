// SPDX-License-Identifier: MIT
//
// Created by obhi on 9/18/20.
//
#pragma once

#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/reflection/detail/container_utils.hpp"
#include "ouly/reflection/detail/derived_concepts.hpp"
#include "ouly/reflection/detail/visitor_helpers.hpp"
#include "ouly/reflection/reflection.hpp"
#include "ouly/reflection/visitor.hpp"
#include "ouly/reflection/visitor_impl.hpp"
#include "ouly/serializers/byteswap.hpp"
#include "ouly/serializers/config.hpp"
#include "ouly/serializers/detail/runtime_type_dispatch.hpp"

#include "ouly/utility/user_config.hpp"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ouly::detail
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

  Stream*                         serializer_    = nullptr;
  binary_any_reader_base<Endian>* any_reader_    = nullptr;
  uint32_t                        object_id_     = 0;
  type                            type_          = type::object;
  bool                            may_fast_path_ = false;

  static constexpr bool has_fast_path = (Endian == std::endian::native);

public:
  using serializer_type              = Stream;
  using serializer_tag               = reader_tag;
  using transform_type               = ouly::pass_through_transform;
  using size_type                    = cfg::container_size_type;
  using binary                       = std::true_type;
  static constexpr bool mutate_enums = false;

  auto operator=(const binary_input_serializer&) -> binary_input_serializer&     = default;
  auto operator=(binary_input_serializer&&) noexcept -> binary_input_serializer& = default;
  binary_input_serializer(binary_input_serializer const&)                        = default;
  binary_input_serializer(binary_input_serializer&& i_other) noexcept            = default;
  binary_input_serializer(Stream& ser) : serializer_{&ser} {}
  ~binary_input_serializer() noexcept = default;

  binary_input_serializer(ouly::detail::field_visitor_tag /*unused*/, binary_input_serializer& ser,
                          [[maybe_unused]] std::string_view key)
      : serializer_{ser.serializer_}, any_reader_{ser.any_reader_}, object_id_(ser.object_id_), type_{type::field},
        may_fast_path_(ser.may_fast_path_)
  {
    // No-op
  }

  binary_input_serializer(ouly::detail::field_visitor_tag /*unused*/, binary_input_serializer& ser, size_t /*index*/)
      : serializer_{ser.serializer_}, any_reader_{ser.any_reader_}, object_id_(ser.object_id_), type_{type::field},
        may_fast_path_(ser.may_fast_path_)
  {
    // No-op
  }

  binary_input_serializer(ouly::detail::object_visitor_tag /*unused*/, binary_input_serializer& ser)
      : serializer_{ser.serializer_}, any_reader_{ser.any_reader_}, type_{type::object},
        may_fast_path_(ser.may_fast_path_)
  {
    // No-op
  }

  binary_input_serializer(ouly::detail::array_visitor_tag /*unused*/, binary_input_serializer& ser)
      : serializer_{ser.serializer_}, any_reader_{ser.any_reader_}, object_id_(ser.object_id_), type_{type::array},
        may_fast_path_(ser.may_fast_path_)
  {
    // No-op
  }

  template <typename Class>
  auto can_visit([[maybe_unused]] Class& obj) -> continue_token
  {
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

  template <ouly::detail::InputSerializableClass<Stream> T>
  void visit(T& obj)
  {
    (*serializer_) >> obj;
  }

  template <typename Class>
  void for_each_entry(Class& obj, auto&& fn)
  {

    using decay_class_type       = std::decay_t<Class>;
    constexpr bool may_fast_path = ouly::detail::LinearArrayLike<decay_class_type, Stream>;

    // First time entering a fast path container
    may_fast_path_ = may_fast_path;

    size_type count = 0;
    visit(count);
    ouly::detail::reserve(obj, count);

    if constexpr (may_fast_path && has_fast_path)
    {
      ouly::detail::resize(obj, count);
      // NOLINTNEXTLINE
      get().read(reinterpret_cast<std::byte*>(std::data(obj)),
                 sizeof(typename Class::value_type) *
                  std::min<size_type>(static_cast<size_type>(std::size(obj)), count));
      if (count > std::size(obj))
      {
        get().skip((count - std::size(obj)) * sizeof(typename Class::value_type));
      }
    }
    else
    {
      if constexpr (!ouly::detail::ContainerCanAppendValue<Class>)
      {
        ouly::detail::resize(obj, count);
      }

      for (size_type i = 0; i < count; ++i)
      {
        fn(*this);
      }
    }
  }

  template <ouly::detail::BoolLike Class>
  auto visit(Class& obj)
  {
    // NOLINTNEXTLINE
    get().read(reinterpret_cast<std::byte*>(&obj), sizeof(obj));
  }

  template <typename Class>
    requires(ouly::detail::IntegerLike<Class> || ouly::detail::FloatLike<Class> || ouly::detail::EnumLike<Class>)
  void visit(Class& obj)
  {
    // NOLINTNEXTLINE
    get().read(reinterpret_cast<std::byte*>(&obj), sizeof(obj));
    if constexpr (!has_fast_path)
    {
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

  void set_any_reader(binary_any_reader_base<Endian>* reader) noexcept
  {
    any_reader_ = reader;
  }

  template <typename Class>
    requires(ouly::detail::AnyLike<Class>)
  void read_any(Class& obj)
  {
    std::uint32_t type_id = 0;
    visit(type_id);
    if (type_id == 0)
    {
      obj.reset();
      return;
    }
    if (any_reader_ == nullptr)
    {
      throw visitor_error(visitor_error::unknown_runtime_type);
    }

    struct input_adapter final : erased_binary_input
    {
      Stream* stream_ = nullptr;

      explicit input_adapter(Stream& stream) : stream_(&stream) {}

      void read(std::byte* data, std::size_t size) override
      {
        stream_->read(data, size);
      }

      void skip(std::size_t size) override
      {
        stream_->skip(size);
      }
    };

    input_adapter                      adapter{get()};
    erased_binary_input_stream<Endian> erased_stream{adapter};
    any_reader_->read_value(type_id, obj, erased_stream);
  }

  template <typename Class>
    requires(ouly::detail::RuntimeTypeLike<Class>)
  void read_runtime_type(Class& obj)
  {
    std::uint32_t type_id = 0;
    visit(type_id);
    obj.id_.id_ = type_id;
    if (type_id == 0)
    {
      obj.value_.reset();
      return;
    }
    if (any_reader_ == nullptr)
    {
      throw visitor_error(visitor_error::unknown_runtime_type);
    }

    struct input_adapter final : erased_binary_input
    {
      Stream* stream_ = nullptr;

      explicit input_adapter(Stream& stream) : stream_(&stream) {}

      void read(std::byte* data, std::size_t size) override
      {
        stream_->read(data, size);
      }

      void skip(std::size_t size) override
      {
        stream_->skip(size);
      }
    };

    input_adapter                      adapter{get()};
    erased_binary_input_stream<Endian> erased_stream{adapter};
    any_reader_->read_value(type_id, obj.value_, erased_stream);
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
    get().read(reinterpret_cast<std::byte*>(buffer.data()), count);
    return buffer;
  }

  auto get() -> auto&
  {
    OULY_ASSERT(serializer_ != nullptr);
    return *serializer_;
  }

  auto get() const -> auto const&
  {
    OULY_ASSERT(serializer_ != nullptr);
    return *serializer_;
  }
};

struct empty_input_streamer
{
  static void read([[maybe_unused]] std::byte* data, [[maybe_unused]] size_t s) {}
};

} // namespace ouly::detail
