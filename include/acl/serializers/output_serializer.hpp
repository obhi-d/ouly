
//
// Created by obhi on 9/18/20.
//
#pragma once

#include <acl/utils/reflection.hpp>
#include <acl/utils/reflection_utils.hpp>
#include <acl/utils/type_traits.hpp>
#include <cassert>
#include <memory>
#include <optional>

namespace acl
{

template <typename V>
concept OutputSerializer = requires(V v) {
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
	v.next_map_entry();
	v.next_array_entry();
};

// Given an input serializer, load
// a bound class
template <OutputSerializer Serializer, typename Opt = acl::options<>>
class output_serializer
{
	using key_field_name	 = detail::key_field_name_t<Opt>;
	using value_field_name = detail::value_field_name_t<Opt>;
	using type_field_name	 = detail::type_field_name_t<Opt>;

private:
	std::reference_wrapper<Serializer> ser_;

public:
	auto operator=(const output_serializer&) -> output_serializer& = default;
	auto operator=(output_serializer&&) -> output_serializer&			 = default;
	output_serializer(output_serializer const&) noexcept					 = default;
	output_serializer(output_serializer&& i_other) noexcept : ser_(i_other.ser_) {}
	output_serializer(Serializer& ser) noexcept : ser_(ser) {}
	~output_serializer() noexcept = default;

	template <typename Class>
	auto operator<<(Class& obj) -> auto&
	{
		write(obj);
		return *this;
	}

	template <typename Class>
	void write(Class const& obj) noexcept
	{
		// Ensure ordering with multiple matches
		if constexpr (detail::BoundClass<Class>)
		{
			write_bound_class(obj);
		}
		else if constexpr (detail::OutputSerializableClass<Class, Serializer>)
		{
			write_serializable(obj);
		}
		else if constexpr (detail::TransformToStringView<Class>)
		{
			write_string_view_transformable(obj);
		}
		else if constexpr (detail::TupleLike<Class>)
		{
			write_tuple(obj);
		}
		else if constexpr (detail::ContainerLike<Class>)
		{
			write_container(obj);
		}
		else if constexpr (detail::VariantLike<Class>)
		{
			write_variant(obj);
		}
		else if constexpr (detail::CastableToStringView<Class>)
		{
			write_string_view_castable(obj);
		}
		else if constexpr (detail::CastableToString<Class>)
		{
			write_string_castable(obj);
		}
		else if constexpr (detail::TransformToString<Class>)
		{
			write_string_transformable(obj);
		}
		else if constexpr (detail::ContainerIsStringLike<Class>)
		{
			write_string(obj);
		}
		else if constexpr (detail::BoolLike<Class>)
		{
			write_bool(obj);
		}
		else if constexpr (detail::IntegerLike<Class>)
		{
			write_integer(obj);
		}
		else if constexpr (detail::EnumLike<Class>)
		{
			write_enum(obj);
		}
		else if constexpr (detail::FloatLike<Class>)
		{
			write_float(obj);
		}
		else if constexpr (detail::PointerLike<Class>)
		{
			write_pointer(obj);
		}
		else if constexpr (detail::OptionalLike<Class>)
		{
			write_optional(obj);
		}
		else if constexpr (detail::MonostateLike<Class>)
		{
			write_monostate(obj);
		}
		else
		{
			[]<bool Flag = false>()
			{
				static_assert(Flag, "This type is not serializable");
			}();
		}
	}

private:
	template <detail::BoundClass Class>
	void write_bound_class(Class const& obj) noexcept
	{
		get().begin_object();
		for_each_field(*this, obj);
		get().end_object();
	}

	template <detail::OutputSerializableClass<Serializer> Class>
	void write_serializable(Class& obj) noexcept
	{
		get() << obj;
	}

	template <detail::TupleLike Class>
	void write_tuple(Class const& obj) noexcept
	{
		// Invalid type is unexpected
		get().begin_array();
		[this, &obj]<std::size_t... N>(std::index_sequence<N...>)
		{
			return (at<N>(obj), ...);
		}(std::make_index_sequence<std::tuple_size_v<Class>>());
		get().end_array();
	}

	template <detail::StringMapLike Class>
	void write_container(Class const& obj) noexcept
	{

		using key_type		= detail::remove_cref<typename Class::key_type>;
		using mapped_type = detail::remove_cref<typename Class::mapped_type>;

		get().begin_object();
		bool comma = false;
		for (auto const& [key, value] : obj)
		{
			if (comma)
			{
				get().next_map_entry();
			}
			get().key(key);
			write(value);
			comma = true;
		}

		get().end_object();
	}

	template <detail::ComplexMapLike Class>
	void write_container(Class const& obj) noexcept
	{

		using key_type		= detail::remove_cref<typename Class::key_type>;
		using mapped_type = detail::remove_cref<typename Class::mapped_type>;

		get().begin_array();
		bool comma = false;
		for (auto const& [key, value] : obj)
		{
			if (comma)
			{
				get().next_array_entry();
			}
			get().begin_object();
			get().key(key_field_name::value);
			write(key);
			get().next_map_entry();
			get().key(value_field_name::value);
			write(value);
			get().end_object();
			comma = true;
		}
		get().end_array();
	}

	template <detail::ArrayLike Class>
	void write_container(Class const& obj) noexcept
	{
		// Invalid type is unexpected
		get().begin_array();
		bool comma = false;
		for (auto const& value : obj)
		{
			if (comma)
			{
				get().next_array_entry();
			}
			write(value);
			comma = true;
		}
		get().end_array();
	}

	template <detail::VariantLike Class>
	void write_variant(Class const& obj) noexcept
	{
		// Invalid type is unexpected
		get().begin_object();
		get().key(type_field_name::value);
		if constexpr (detail::HasVariantTypeTransform<Class>)
		{
			get().as_string(acl::from_variant_index<Class>(obj.index()));
		}
		else
		{
			get().as_uint64(obj.index());
		}
		get().next_map_entry();
		get().key(value_field_name::value);
		std::visit(
		 [this](auto const& arg)
		 {
			 write(arg);
		 },
		 obj);
		get().end_object();
	}

	template <detail::CastableToStringView Class>
	void write_string_view_castable(Class const& obj) noexcept
	{
		get().as_string(std::string_view(obj));
	}

	template <detail::CastableToString Class>
	void write_string_castable(Class const& obj) noexcept
	{
		get().as_string(std::string(obj));
	}

	template <detail::TransformToString Class>
	void write_string_transformable(Class const& obj) noexcept
	{
		get().as_string(acl::to_string(obj));
	}

	template <detail::TransformToStringView Class>
	void write_string_view_transformable(Class const& obj) noexcept
	{
		get().as_string(acl::to_string_view(obj));
	}

	template <detail::ContainerIsStringLike Class>
	void write_string(Class const& obj) noexcept
	{
		get().as_string(obj);
	}

	template <detail::BoolLike Class>
	void write_bool(Class const& obj) noexcept
	{
		get().as_bool(obj);
	}

	template <detail::SignedIntLike Class>
	void write_integer(Class const& obj) noexcept
	{
		get().as_int64(obj);
	}

	template <detail::UnsignedIntLike Class>
	void write_integer(Class const& obj) noexcept
	{
		get().as_uint64(obj);
	}

	template <detail::EnumLike Class>
	void write_enum(Class const& obj) noexcept
	{
		get().as_uint64(static_cast<uint64_t>(obj));
	}

	template <detail::FloatLike Class>
	void write_float(Class const& obj) noexcept
	{
		get().as_double(obj);
	}

	template <detail::ContainerIsStringLike Class>
	void write_float(Class const& obj) noexcept
	{
		get().as_string(obj);
	}

	template <detail::PointerLike Class>
	void write_pointer(Class const& obj) noexcept
	{
		if (obj)
		{
			write(*obj);
		}
		else
		{
			get().as_null();
		}
	}

	template <detail::OptionalLike Class>
	void write_optional(Class const& obj) noexcept
	{
		if (obj)
		{
			write(*obj);
		}
		else
		{
			get().as_null();
		}
	}

	template <detail::MonostateLike Class>
	void write_monostate(Class const& obj) noexcept
	{
		get().as_null();
	}

public:
	template <typename Class, typename Decl, std::size_t I>
	void operator()(Class const& obj, Decl const& decl, std::integral_constant<std::size_t, I> /*unused*/) noexcept
	{
		if constexpr (I != 0)
		{
			get().next_map_entry();
		}
		get().key(decl.key());
		write(decl.value(obj));
	}

private:
	auto get() noexcept -> auto&
	{
		return ser_.get();
	}

	auto get() const noexcept -> auto const&
	{
		return ser_.get();
	}

	template <std::size_t N, typename Class>
	void at(Class const& obj) noexcept
	{
		if constexpr (N != 0)
		{
			get().next_array_entry();
		}
		write(std::get<N>(obj));
	}
};

} // namespace acl
