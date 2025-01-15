
#pragma once

#include <acl/reflection/detail/container_utils.hpp>
#include <acl/reflection/detail/derived_concepts.hpp>
#include <acl/reflection/detail/visitor_helpers.hpp>
#include <acl/reflection/reflection.hpp>
#include <acl/serializers/byteswap.hpp>
#include <acl/serializers/config.hpp>
#include <acl/utility/config.hpp>
#include <acl/utility/transforms.hpp>
#include <acl/utility/type_traits.hpp>
#include <cassert>
#include <string_view>

namespace acl::detail
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
  using transform_type               = acl::pass_through_transform;
  using size_type                    = acl::cfg::container_size_type;
  using config_type                  = acl::config<>;
  using binary                       = std::true_type;
  static constexpr bool mutate_enums = false;

  auto operator=(const binary_output_serializer&) -> binary_output_serializer&     = default;
  auto operator=(binary_output_serializer&&) noexcept -> binary_output_serializer& = default;
  binary_output_serializer(binary_output_serializer const&)                        = default;
  binary_output_serializer(binary_output_serializer&& i_other) noexcept : serializer_(i_other.serializer_) {}
  binary_output_serializer(Stream& ser) : serializer_(&ser) {}
  ~binary_output_serializer() noexcept = default;

  binary_output_serializer(acl::detail::field_visitor_tag /*unused*/, binary_output_serializer& ser,
                           std::string_view /*key*/)
      : serializer_{ser.serializer_}, may_fast_path_(ser.may_fast_path_), type_{type::field}
  {
    // No-op
  }

  binary_output_serializer(acl::detail::field_visitor_tag /*unused*/, binary_output_serializer& ser, size_t /*index*/)
      : serializer_{ser.serializer_}, may_fast_path_(ser.may_fast_path_), type_{type::field}
  {
    // No-op
  }

  binary_output_serializer(acl::detail::object_visitor_tag /*unused*/, binary_output_serializer& ser)
      : serializer_{ser.serializer_}, may_fast_path_(ser.may_fast_path_), type_{type::object}
  {
    // No-op
  }

  binary_output_serializer(acl::detail::array_visitor_tag /*unused*/, binary_output_serializer& ser)
      : serializer_{ser.serializer_}, may_fast_path_(ser.may_fast_path_), type_{type::array}
  {
    // No-op
  }

  template <typename Class>
  auto can_visit(Class const& obj) -> continue_token
  {
    using class_type = std::decay_t<Class>;
    if (!may_fast_path_)
    {
      if constexpr (requires { Class::magic_type_header; })
      {
        write_id(Class::magic_type_header);
      }
      else if constexpr (acl::cfg::magic_type_header<std::decay_t<Class>>)
      {
        write_id(cfg::magic_type_header<std::decay_t<Class>>);
      }
    }
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
    using type                   = std::decay_t<Class>;
    constexpr bool may_fast_path = acl::detail::LinearArrayLike<type, Stream>;

    // First time entering a fast path container
    may_fast_path_ = may_fast_path;

    size_type count = std::size(obj);
    visit(count);

    if constexpr (may_fast_path && has_fast_path)
    {
      // NOLINTNEXTLINE
      get().write(reinterpret_cast<std::byte const*>(std::data(obj)), sizeof(typename Class::value_type) * count);
      return;
    }

    for (auto const& value : obj)
    {
      fn(value, *this);
    }
  }

  void visit(std::string_view str)
  {
    auto count = static_cast<size_type>(str.length());
    visit(count);
    // NOLINTNEXTLINE
    get().write(reinterpret_cast<std::byte const*>(str.data()), str.length());
  }

  template <acl::detail::BoolLike Class>
  auto visit(Class const& obj)
  {
    // NOLINTNEXTLINE
    get().write(reinterpret_cast<std::byte const*>(&obj), sizeof(obj));
  }

  template <typename Class>
    requires(acl::detail::IntegerLike<Class> || acl::detail::FloatLike<Class> || acl::detail::EnumLike<Class>)
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
    assert(serializer_ != nullptr);
    return *serializer_;
  }

  auto get() const -> auto const&
  {
    assert(serializer_ != nullptr);
    return *serializer_;
  }
};

struct empty_output_streamer
{
  void write(std::byte* data, size_t s) {}
};

} // namespace acl::detail
