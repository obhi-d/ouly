
#pragma once

#include <acl/utils/type_name.hpp>
#include <string>
#include <system_error>

// enums

namespace acl
{

enum class serializer_error : uint8_t
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
	[[nodiscard]] auto name() const noexcept -> const char* final
	{
		return detail::type_name<E>().data();
	}

	[[nodiscard]] auto message(int ev) const -> std::string final
	{
		return std::to_string(ev);
	}

	static auto instance() -> auto&
	{
		static error_category<E> inst;
		return inst;
	}
};

template <typename E>
inline auto make_error_code(E e) -> std::error_code
{
	return {static_cast<int>(e), error_category<E>::instance()};
}

} // namespace acl

template <>
struct std::is_error_code_enum<acl::serializer_error> : true_type
{};
