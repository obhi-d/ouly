
#pragma once
#include <acl/allocators/linear_arena_allocator.hpp>
#include <acl/dsl/yaml.hpp>
#include <acl/utils/error_codes.hpp>
#include <acl/utils/reflection.hpp>
#include <acl/utils/reflection_utils.hpp>
#include <acl/utils/type_traits.hpp>
#include <functional>

namespace acl
{
namespace detail
{
class parser_state;

struct in_context_base : public acl::yaml::context
{
	using pop_fn = void (*)(in_context_base*);

	parser_state*		 parser_state_;
	in_context_base* parent_	= nullptr;
	pop_fn					 pop_fn_	= nullptr;
	uint32_t				 xvalue_	= 0;
	bool						 is_null_ = false;

	in_context_base(parser_state& parser_state, in_context_base* parent) noexcept
			: parser_state_(&parser_state), parent_(parent)
	{}
};
constexpr uint32_t default_parser_buffer_size = 8096;
class parser_state
{
	yaml::istream									stream_;
	acl::linear_arena_allocator<> allocator_;
	in_context_base*							context_ = nullptr;

public:
	auto					operator=(const parser_state&) -> parser_state& = delete;
	auto					operator=(parser_state&&) -> parser_state& = default;
	parser_state(const parser_state&)												 = delete;
	parser_state(parser_state&&)														 = default;
	parser_state(std::string_view content) noexcept : stream_(content), allocator_(default_parser_buffer_size) {}
	~parser_state() noexcept
	{
		while (context_ != nullptr)
		{
			auto* parent = context_->parent_;
			if (context_->pop_fn_ != nullptr)
			{
				context_->pop_fn_(context_);
			}
			context_ = parent;
		}
	}

	template <typename C>
	void parse(C& handler)
	{
		stream_.set_handler(&handler);
		handler.setup_proxy();
		stream_.parse();
	}

	template <typename Context, typename... Args>
	auto push(Args&&... args) -> Context*
	{
		void* cursor = allocator_.allocate(sizeof(Context), alignof(Context));
		// NOLINTNEXTLINE
		auto icontext = std::construct_at(reinterpret_cast<Context*>(cursor), std::forward<Args>(args)...);
		stream_.set_handler(icontext);
		icontext->setup_proxy();
		context_ = icontext;
		return icontext;
	}

	template <typename Context>
	void pop(Context* ptr, in_context_base* parent)
	{
		std::destroy_at(ptr);
		allocator_.deallocate(ptr, sizeof(Context), alignof(Context));
		stream_.set_handler(parent);
		context_ = parent;
	}
};

template <typename Class, typename Opt>
class icontext : public in_context_base
{
	using pop_fn					 = std::function<void(void*)>;
	using class_type			 = detail::remove_cref<Class>;
	using key_field_name	 = detail::key_field_name_t<Opt>;
	using value_field_name = detail::value_field_name_t<Opt>;
	using type_field_name	 = detail::type_field_name_t<Opt>;

public:
	icontext(class_type& obj, parser_state& state, in_context_base* parent) noexcept
		requires(std::is_reference_v<Class>)
			: obj_(obj), in_context_base(state, parent)
	{}

	icontext(parser_state& state, in_context_base* parent) noexcept
		requires(!std::is_reference_v<Class>)
			: in_context_base(state, parent)
	{}

	auto get() noexcept -> class_type&
	{
		return obj_;
	}

	void begin_key(std::string_view key) override
	{
		if constexpr (detail::BoundClass<class_type>)
		{
			if constexpr (detail::StringMapValueType<class_type>)
			{
				read_string_map_value(key);
			}
			else
			{
				read_bound_class(key);
			}
		}
		else if constexpr (detail::VariantLike<class_type>)
		{
			read_variant(key);
		}
	}

	void end_key() override
	{
		if (pop_fn_)
		{
			pop_fn_(this);
		}
	}

	void begin_array() override
	{
		if constexpr (detail::ContainerLike<class_type>)
		{
			push_container_item();
		}
		else if constexpr (detail::TupleLike<class_type>)
		{
			read_tuple();
		}
	}

	void end_array() override
	{
		if (pop_fn_)
		{
			pop_fn_(this);
		}
	}

	void setup_proxy()
	{
		if constexpr (detail::PointerLike<class_type>)
		{
			return read_pointer();
		}
		else if constexpr (detail::OptionalLike<class_type>)
		{
			return read_optional();
		}
	}

	void set_value(std::string_view slice) override
	{
		if (slice == "null")
		{
			is_null_ = true;
			return;
		}

		if constexpr (detail::TransformFromString<class_type>)
		{
			acl::from_string(obj_, slice);
		}
		else if constexpr (detail::ConstructedFromStringView<class_type> || detail::ContainerIsStringLike<class_type>)
		{
			obj_ = class_type(slice);
		}
		else if constexpr (detail::BoolLike<class_type>)
		{
			return read_bool(slice);
		}
		else if constexpr (detail::IntegerLike<class_type>)
		{
			return read_integer(slice);
		}
		else if constexpr (detail::EnumLike<class_type>)
		{
			return read_enum(slice);
		}
		else if constexpr (detail::FloatLike<class_type>)
		{
			return read_float(slice);
		}
		else if constexpr (detail::MonostateLike<class_type>)
		{
		}
	}

	void read_bool(std::string_view slice)
	{
		using namespace std::string_view_literals;
		obj_ = slice == "true"sv || slice == "True"sv;
	}

	void error_check(std::from_chars_result result)
	{
		if (result.ec != std::errc())
		{
			throw std::runtime_error("Failed to parse value");
		}
	}

	void read_integer(std::string_view slice)
	{
		constexpr uint32_t base_10 = 10;
		constexpr uint32_t base_16 = 10;
		using namespace std::string_view_literals;
		if (slice.starts_with("0x"sv))
		{
			error_check(std::from_chars(slice.data(), slice.data() + slice.size(), obj_, base_16));
		}
		else
		{
			error_check(std::from_chars(slice.data(), slice.data() + slice.size(), obj_, base_10));
		}
	}

	void read_float(std::string_view slice)
	{
		using namespace std::string_view_literals;
		if (slice == ".nan"sv || slice == "nan"sv)
		{
			obj_ = std::numeric_limits<class_type>::quiet_NaN();
		}
		else if (slice == ".inf"sv || slice == "inf"sv)
		{
			obj_ = std::numeric_limits<class_type>::infinity();
		}
		else if (slice == "-.inf"sv || slice == "-inf"sv)
		{
			obj_ = -std::numeric_limits<class_type>::infinity();
		}
		else
		{
			error_check(std::from_chars(slice.data(), slice.data() + slice.size(), obj_));
		}
	}

	void read_enum(std::string_view slice)
	{
		constexpr uint32_t base_10 = 10;
		constexpr uint32_t base_16 = 10;

		std::underlying_type_t<class_type> value;
		if (slice.starts_with("0x"))
		{
			error_check(std::from_chars(slice.data(), slice.data() + slice.size(), value, base_16));
		}
		else
		{
			error_check(std::from_chars(slice.data(), slice.data() + slice.size(), value, base_10));
		}
		obj_ = static_cast<class_type>(value);
	}

	void read_pointer()
	{
		using class_type	= detail::remove_cref<class_type>;
		using pvalue_type = detail::pointer_class_type<class_type>;
		if (!obj_)
		{
			if constexpr (std::same_as<class_type, std::shared_ptr<pvalue_type>>)
			{
				obj_ = std::make_shared<pvalue_type>();
			}
			else
			{
				obj_ = class_type(new detail::pointer_class_type<class_type>());
			}
		}

		auto mapping		 = parser_state_->push<icontext<pvalue_type&, Opt>>(*obj_, parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			auto* object = reinterpret_cast<icontext<pvalue_type&, Opt>*>(mapping);
			auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);

			bool nullify = object->is_null_;
			parent->parser_state_->pop(object, parent);

			if (nullify)
			{
				parent->obj_ = nullptr;
			}

			if (parent->pop_fn_)
			{
				parent->pop_fn_(parent);
			}
		};
	}

	void read_optional()
	{
		using pvalue_type = typename class_type::value_type;
		if (!obj_)
		{
			obj_.emplace();
		}

		auto mapping		 = parser_state_->push<icontext<pvalue_type&, Opt>>(*obj_, parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			auto* object = reinterpret_cast<icontext<pvalue_type&, Opt>*>(mapping);
			auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);

			bool nullify = object->is_null_;
			parent->parser_state_->pop(object, parent);

			if (nullify)
			{
				parent->obj_.reset();
			}

			if (parent->pop_fn_)
			{
				parent->pop_fn_(parent);
			}
		};
	}

	void read_tuple()
	{
		read_tuple_value<std::tuple_size_v<class_type> - 1>(xvalue_++);
	}

	void read_variant(std::string_view key)
	{
		if (key == type_field_name::value)
		{
			read_variant_type();
		}
		else if (key == value_field_name::value)
		{
			read_variant_value<std::variant_size_v<class_type> - 1>(xvalue_);
		}
	}

	void read_variant_type()
	{
		using variant_tt = std::conditional_t<detail::HasVariantTypeTransform<class_type>, std::string_view, uint32_t>;

		auto mapping		 = parser_state_->push<icontext<variant_tt, Opt>>(parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			auto object = static_cast<icontext<uint32_t, Opt>*>(mapping);
			if constexpr (detail::HasVariantTypeTransform<class_type>)
			{
				object->parent_->xvalue_ = acl::to_variant_index<class_type>(object->get());
			}
			else
			{
				object->parent_->xvalue_ = object->get();
			}
			object->parent_->parser_state_->pop(object, object->parent_);
		};
	}

	template <std::size_t const I>
	void read_variant_value(uint32_t i) noexcept
	{
		if (I == i)
		{
			using type = std::variant_alternative_t<I, class_type>;

			auto mapping		 = parser_state_->push<icontext<type, Opt>>(parser_state_, this);
			mapping->pop_fn_ = [](in_context_base* mapping)
			{
				auto* object = static_cast<icontext<type, Opt>*>(mapping);
				auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);
				parent->get().template emplace<type>(std::move(object->get()));
				parent->parser_state_->pop(object, parent);
			};
		}
		else if constexpr (I > 0)
		{
			read_variant_value<I - 1>(i);
		}
		else
		{
			return;
		}
	}

	template <std::size_t const I>
	void read_tuple_value(uint32_t i) noexcept
	{
		if (I == i)
		{
			using type = std::tuple_element_t<I, class_type>;

			auto mapping		 = parser_state_->push<icontext<type, Opt>>(parser_state_, this);
			mapping->pop_fn_ = [](in_context_base* mapping)
			{
				auto* object							 = static_cast<icontext<type, Opt>*>(mapping);
				auto	parent							 = static_cast<icontext<Class, Opt>*>(object->parent_);
				std::get<I>(parent->get()) = std::move(object->get());
				parent->parser_state_->pop(object, parent);
			};
		}
		else if constexpr (I > 0)
		{
			read_tuple_value<I - 1>(i);
		}
		else
		{
			return;
		}
	}

	void push_container_item()
		requires(!ContainerHasEmplaceBack<class_type> && ContainerHasArrayValueAssignable<class_type>)
	{
		auto mapping		 = parser_state_->push<icontext<array_value_type<class_type>, Opt>>(parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			auto* object = static_cast<icontext<array_value_type<class_type>, Opt>*>(mapping);
			auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);
			if (parent->xvalue_ < parent->get().size())
			{
				parent->get()[parent->xvalue_++] = std::move(object->get());
			}
			parent->parser_state_->pop(object, parent);
		};
	}

	void push_container_item()
		requires(ContainerHasEmplaceBack<class_type>)
	{
		auto mapping		 = parser_state_->push<icontext<typename class_type::value_type, Opt>>(parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			auto* object = static_cast<icontext<typename class_type::value_type, Opt>*>(mapping);
			auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);
			parent->get().emplace_back(std::move(object->get()));
			parent->parser_state_->pop(object, parent);
		};
	}

	void push_container_item()
		requires(ContainerHasEmplace<class_type> && !MapLike<class_type>)
	{
		using value_t		 = typename class_type::value_type;
		auto mapping		 = parser_state_->push<icontext<value_t, Opt>>(parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			auto* object = static_cast<icontext<value_t, Opt>*>(mapping);
			auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);
			parent->get().emplace(std::move(object->get()));
			parent->parser_state_->pop(object, parent);
		};
	}

	void push_container_item()
		requires(detail::ComplexMapLike<class_type>)
	{
		using value_t		 = map_value_type<typename class_type::key_type, typename class_type::mapped_type, Opt>;
		auto mapping		 = parser_state_->push<icontext<value_t, Opt>>(parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			auto* object = static_cast<icontext<value_t, Opt>*>(mapping);
			auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);
			auto& pair	 = object->get();
			parent->get().emplace(std::move(pair.key), std::move(pair.value));
			parent->parser_state_->pop(object, parent);
		};
	}

	void push_container_item()
		requires(detail::StringMapLike<class_type>)
	{
		using value_t		 = string_map_value_type<typename class_type::mapped_type, Opt>;
		auto mapping		 = parser_state_->push<icontext<value_t, Opt>>(parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			auto* object = static_cast<icontext<value_t, Opt>*>(mapping);
			auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);
			auto& pair	 = object->get();
			parent->get().emplace(std::move(pair.key), std::move(pair.value));
			parent->parser_state_->pop(object, parent);
		};
	}

	void read_string_map_value(std::string_view key)
	{
		obj_.key				 = key;
		using value_t		 = typename class_type::value_type;
		auto mapping		 = parser_state_->push<icontext<value_t&, Opt>>(obj_.value, parser_state_, this);
		mapping->pop_fn_ = [](in_context_base* mapping)
		{
			auto* object = static_cast<icontext<value_t&, Opt>*>(mapping);
			auto	parent = static_cast<icontext<Class, Opt>*>(object->parent_);
			parent->parser_state_->pop(object, parent);
		};
	}

	void read_bound_class(std::string_view key)
	{
		for_each_field(
		 [this, key]<typename Decl>(class_type& obj, Decl const& decl, auto) noexcept
		 {
			 if (decl.key() == key)
			 {
				 using value_t = typename Decl::MemTy;

				 auto mapping			= parser_state_->push<icontext<value_t, Opt>>(parser_state_, this);
				 mapping->pop_fn_ = [](in_context_base* mapping)
				 {
					 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
					 auto object = reinterpret_cast<icontext<value_t, Opt>*>(mapping);
					 auto parent = static_cast<icontext<Class, Opt>*>(object->parent_);
					 Decl decl;
					 decl.value(parent->get(), std::move(object->get()));
					 parent->parser_state_->pop(object, parent);
				 };
			 }
		 },
		 obj_);
	}

private:
	Class obj_;
};

} // namespace detail

namespace yaml
{
template <typename Class, typename Opt = acl::options<>>
void from_string(Class& obj, std::string_view data)
{
	auto state		= detail::parser_state(data);
	auto icontext = detail::icontext<Class&, Opt>(obj, state, nullptr);
	state.parse(icontext);
}

} // namespace yaml
} // namespace acl
