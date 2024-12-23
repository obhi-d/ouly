
#pragma once

#include <acl/utils/type_name.hpp>
#include <string>
#include <system_error>

// enums

namespace acl
{

enum class serializer_error
{
  none,
  invalid_type,
  failed_streaming_map,
  failed_streaming_array,
  failed_streaming_variant,
  failed_to_parse_value,
  variant_missing_index,
  variant_missing_value,
  variant_index_is_not_int,
  variant_invalid_index,
  invalid_key,
  invalid_tuple_size,
  corrupt_array_item,
  corrupt_string_length,
  corrupt_string,
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
