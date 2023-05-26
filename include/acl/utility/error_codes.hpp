
#pragma once

#include <acl/detail/type_name.hpp>
#include <string>
#include <system_error>

// enums

namespace acl
{

enum class serializer_error
{
  none                     = 0,
  invalid_type             = 1,
  failed_streaming_map     = 2,
  failed_streaming_array   = 3,
  failed_streaming_variant = 4,
  failed_to_parse_value    = 5,
  variant_invalid_format   = 6,
  variant_index_is_not_int = 7,
  variant_invalid_index    = 8,
  invalid_key              = 9,
  invalid_tuple_size       = 10,
  corrupt_array_item       = 11,
  corrupt_string_length    = 12,
  corrupt_string           = 13,
};

template <typename E>
struct error_category : std::error_category
{
  inline const char* name() const noexcept final
  {
    return detail::type_name<E>().data();
  }

  inline std::string message(int ev) const final
  {
    return std::to_string(ev);
  }

  static inline auto& instance()
  {
    static error_category<E> inst;
    return inst;
  }
};

template <typename E>
inline std::error_code make_error_code(E e)
{
  return {static_cast<int>(e), error_category<E>::instance()};
}

} // namespace acl

template <>
struct std::is_error_code_enum<acl::serializer_error> : true_type
{};
