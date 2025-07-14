// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/reflection/detail/container_utils.hpp"
#include "ouly/reflection/detail/derived_concepts.hpp"
#include "ouly/reflection/detail/visitor_helpers.hpp"
#include "ouly/reflection/reflection.hpp"
#include "ouly/serializers/byteswap.hpp"
#include "ouly/serializers/config.hpp"
#include "ouly/utility/config.hpp"
#include "ouly/utility/convert.hpp"
#include "ouly/utility/type_traits.hpp"
#include "ouly/utility/user_config.hpp"
#include <string_view>

namespace ouly::detail
{

template <typename Stream, std::endian Endian = std::endian::little>
class binary_output_serializer
{
private:
  enum class type : uint8_t
  {
    object,
    array,
    field
  };

  Stream* serializer_    = nullptr;
  type    type_          = type::object;
  bool    may_fast_path_ = false;

  static constexpr bool has_fast_path = (Endian == std::endian::native);

public:
  using serializer_type              = Stream;
  using serializer_tag               = writer_tag;
  using transform_type               = ouly::pass_through_transform;
  using size_type                    = ouly::cfg::container_size_type;
  using config_type                  = ouly::config<>;
  using binary                       = std::true_type;
  static constexpr bool mutate_enums = false;

  auto operator=(const binary_output_serializer&) -> binary_output_serializer&     = default;
  auto operator=(binary_output_serializer&&) noexcept -> binary_output_serializer& = default;
  binary_output_serializer(binary_output_serializer const&)                        = default;
  binary_output_serializer(binary_output_serializer&& i_other) noexcept : serializer_(i_other.serializer_) {}
  binary_output_serializer(Stream& ser) : serializer_(&ser) {}
  ~binary_output_serializer() noexcept = default;

  binary_output_serializer(ouly::detail::field_visitor_tag /*unused*/, binary_output_serializer& ser,
                           std::string_view /*key*/)
      : serializer_{ser.serializer_}, type_{type::field}, may_fast_path_(ser.may_fast_path_)
  {
    // No-op
  }

  binary_output_serializer(ouly::detail::field_visitor_tag /*unused*/, binary_output_serializer& ser, size_t /*index*/)
      : serializer_{ser.serializer_}, type_{type::field}, may_fast_path_(ser.may_fast_path_)
  {
    // No-op
  }

  binary_output_serializer(ouly::detail::object_visitor_tag /*unused*/, binary_output_serializer& ser)
      : serializer_{ser.serializer_}, type_{type::object}, may_fast_path_(ser.may_fast_path_)
  {
    // No-op
  }

  binary_output_serializer(ouly::detail::array_visitor_tag /*unused*/, binary_output_serializer& ser)
      : serializer_{ser.serializer_}, type_{type::array}, may_fast_path_(ser.may_fast_path_)
  {
    // No-op
  }

  template <typename Class>
  auto can_visit([[maybe_unused]] Class const& obj) -> continue_token
  {
    if (!may_fast_path_)
    {
      if constexpr (requires { Class::magic_type_header; })
      {
        write_id(Class::magic_type_header);
      }
      else if constexpr (ouly::cfg::magic_type_header<std::decay_t<Class>>)
      {
        write_id(cfg::magic_type_header<std::decay_t<Class>>);
      }
    }
    return true;
  }

  template <ouly::detail::OutputSerializableClass<Stream> T>
  void visit(T& obj)
  {
    (*serializer_) << obj;
  }

  template <typename Class>
  void for_each_entry(Class const& obj, auto&& fn)
  {
    using decay_class_type       = std::decay_t<Class>;
    constexpr bool may_fast_path = ouly::detail::LinearArrayLike<decay_class_type, Stream>;

    // First time entering a fast path container
    may_fast_path_ = may_fast_path;

    size_type count = static_cast<size_type>(std::size(obj));
    visit(count);

    if constexpr (may_fast_path && has_fast_path)
    {
      // NOLINTNEXTLINE
      get().write(reinterpret_cast<std::byte const*>(std::data(obj)), sizeof(typename Class::value_type) * count);
    }
    else
    {
      for (auto const& value : obj)
      {
        fn(value, *this);
      }
    }
  }

  void visit(std::string_view str)
  {
    auto count = static_cast<size_type>(str.length());
    visit(count);
    // NOLINTNEXTLINE
    get().write(reinterpret_cast<std::byte const*>(str.data()), str.length());
  }

  template <ouly::detail::BoolLike Class>
  auto visit(Class const& obj)
  {
    // NOLINTNEXTLINE
    get().write(reinterpret_cast<std::byte const*>(&obj), sizeof(obj));
  }

  template <typename Class>
    requires(ouly::detail::IntegerLike<Class> || ouly::detail::FloatLike<Class> || ouly::detail::EnumLike<Class>)
  auto visit(Class const& obj)
  {
    if constexpr (has_fast_path)
    {
      // NOLINTNEXTLINE
      get().write(reinterpret_cast<std::byte const*>(&obj), sizeof(obj));
    }
    else
    {
      auto obj_swapped = byteswap(obj);
      // NOLINTNEXTLINE
      get().write(reinterpret_cast<std::byte const*>(&obj_swapped), sizeof(obj_swapped));
    }
  }

  void set_null()
  {
    visit(cfg::null_sentinel);
  }

  void set_not_null()
  {
    visit(cfg::not_null_sentinel);
  }

private:
  auto write_id(uint32_t id)
  {
    visit(id);
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

struct empty_output_streamer
{
  void write([[maybe_unused]] std::byte* data, [[maybe_unused]] size_t s) {}
};

} // namespace ouly::detail
