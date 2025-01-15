#pragma once

#include <acl/allocators/linear_arena_allocator.hpp>
#include <acl/dsl/lite_yml.hpp>
#include <acl/reflection/visitor.hpp>
#include <acl/serializers/config.hpp>
#include <acl/utility/detail/concepts.hpp>
#include <cstddef>
#include <string>
#include <type_traits>

namespace acl::detail
{
class parser_state;

struct in_context_base
{
  using post_init_fn          = void (*)(in_context_base*, parser_state*);
  in_context_base* parent_    = nullptr;
  post_init_fn     post_init_ = nullptr;
  uint32_t         xvalue_    = 0;

  in_context_base() noexcept                                 = default;
  in_context_base(const in_context_base&)                    = default;
  in_context_base(in_context_base&&)                         = delete;
  auto operator=(const in_context_base&) -> in_context_base& = default;
  auto operator=(in_context_base&&) -> in_context_base&      = delete;

  in_context_base(in_context_base* parent) noexcept : parent_(parent) {}

  virtual auto set_key(parser_state* parser, std::string_view ikey) -> in_context_base* = 0;
  virtual void set_value(parser_state* parser, std::string_view slice)                  = 0;
  virtual void post_init_object(parser_state* parser)                                   = 0;
  virtual auto add_item(parser_state* parser) -> in_context_base*                       = 0;
  virtual ~in_context_base() noexcept                                                   = default;
};

class parser_state final : public acl::yml::context
{
  acl::yml::lite_stream         stream_;
  acl::linear_arena_allocator<> allocator_;
  in_context_base*              context_ = nullptr;

public:
  auto operator=(const parser_state&) -> parser_state& = delete;
  auto operator=(parser_state&&) -> parser_state&      = delete;
  parser_state(const parser_state&)                    = delete;
  parser_state(parser_state&&)                         = delete;
  parser_state(std::string_view content) noexcept
      : stream_(content, this), allocator_(cfg::default_lite_yml_parser_buffer_size)
  {}
  ~parser_state() noexcept final
  {
    clear();
  }

  [[nodiscard]] auto get_stored_value() const noexcept -> uint32_t
  {
    return context_->xvalue_;
  }

  template <typename C>
  void parse(C& handler)
  {
    context_ = &handler;
    stream_.parse();
  }

  template <typename Context, typename... Args>
  auto create(Args&&... args) -> Context*
  {
    void* cursor = allocator_.allocate(sizeof(Context), alignof(Context));
    // NOLINTNEXTLINE
    return std::construct_at(reinterpret_cast<Context*>(cursor), context_, std::forward<Args>(args)...);
  }

  void pop()
  {
    if (context_ != nullptr)
    {
      auto* parent = context_->parent_;
      context_->post_init_object(this);
      context_ = parent;
    }
  }

  template <typename Context>
  void destroy(Context* ptr)
  {
    std::destroy_at(ptr);
    allocator_.deallocate(ptr, sizeof(Context), alignof(Context));
  }

  void begin_array() final {}

  void end_array() final
  {
    pop();
  }

  void begin_object() final {}

  void end_object() final
  {
    pop();
  }

  void clear()
  {
    while (context_ != nullptr)
    {
      pop();
    }
  }

  void begin_new_array_item() final
  {
    context_ = context_->add_item(this);
  }

  void set_key(std::string_view ikey) final
  {
    context_ = context_->set_key(this, ikey);
  }

  void set_value(std::string_view slice) final
  {
    context_->set_value(this, slice);
    pop();
  }
};

template <typename Class, typename Config>
class in_context_impl : public in_context_base
{
  using pop_fn         = std::function<void(void*)>;
  using class_type     = std::decay_t<Class>;
  using transform_type = transform_t<Config>;
  using this_type      = in_context_impl<Class, Config>;

public:
  in_context_impl(class_type& obj) noexcept
    requires(std::is_reference_v<Class>)
      : obj_(obj)
  {}

  in_context_impl(in_context_base* parent, class_type& obj) noexcept
    requires(std::is_reference_v<Class>)
      : in_context_base(parent), obj_(obj)
  {}

  in_context_impl(in_context_base* parent) noexcept
    requires(!std::is_reference_v<Class>)
      : in_context_base(parent)
  {}

  auto get() noexcept -> class_type&
  {
    return obj_;
  }

  template <typename Base, typename TClassType>
  static auto read_key(TClassType& obj, parser_state* parser, std::string_view ikey) -> in_context_base*
  {
    using tclass_type = std::decay_t<TClassType>;
    auto key          = transform_type::transform(ikey);

    if constexpr (ExplicitlyReflected<tclass_type>)
    {
      return read_explicitly_reflected<Base>(obj, parser, key);
    }
    else if constexpr (Aggregate<tclass_type>)
    {
      return read_aggregate<Base>(obj, parser, key);
    }
    else if constexpr (VariantLike<tclass_type>)
    {
      return read_variant(obj, parser, key);
    }
    else if constexpr (PointerLike<tclass_type>)
    {
      return read_key_in_pointer<Base>(obj, parser, key);
    }
    else if constexpr (OptionalLike<tclass_type>)
    {
      return read_key_in_optional<Base>(obj, parser, key);
    }
    else
    {
      throw visitor_error(visitor_error::type_is_not_an_object);
    }
  }

  void post_init_object(parser_state* parser) final
  {
    if (post_init_)
    {
      post_init_(this, parser);
    }

    post_read(obj_);
    if (parent_ != nullptr)
    {
      parser->destroy(this);
    }
  }

  auto set_key(parser_state* parser, std::string_view ikey) -> in_context_base* final
  {
    return read_key<this_type>(obj_, parser, ikey);
  }

  void set_value(parser_state* parser, std::string_view slice) final
  {
    read_value(obj_, parser, slice);
  }

  auto add_item(parser_state* parser) -> in_context_base* final
  {
    if constexpr (ContainerLike<class_type>)
    {
      return push_container_item(parser);
    }
    else if constexpr (TupleLike<class_type>)
    {
      return read_tuple(parser);
    }
    else
    {
      throw visitor_error(visitor_error::type_is_not_an_array);
    }
    return nullptr;
  }

  template <typename TClassType>
  static void read_value(TClassType& obj, parser_state* parser, std::string_view slice)
  {
    using tclass_type = std::decay_t<TClassType>;
    if constexpr (Convertible<tclass_type>)
    {
      if constexpr (requires { typename Config::mutate_enums_type; } && std::is_enum_v<tclass_type>)
      {
        acl::convert<tclass_type>::from_string(obj, transform_type::transform(slice));
      }
      else
      {
        acl::convert<tclass_type>::from_string(obj, slice);
      }
    }
    else if constexpr (PointerLike<tclass_type>)
    {
      read_pointer(obj, parser, slice);
    }
    else if constexpr (OptionalLike<tclass_type>)
    {
      read_optional(obj, parser, slice);
    }
    else if constexpr (BoolLike<tclass_type>)
    {
      read_bool(obj, slice);
    }
    else if constexpr (IntegerLike<tclass_type>)
    {
      read_integer(obj, slice);
    }
    else if constexpr (EnumLike<tclass_type>)
    {
      read_enum(obj, slice);
    }
    else if constexpr (FloatLike<tclass_type>)
    {
      read_float(obj, slice);
    }
    else if constexpr (MonostateLike<tclass_type>)
    {
    }
  }

  static void error_check(std::from_chars_result result)
  {
    if (result.ec != std::errc())
    {
      throw visitor_error(visitor_error::invalid_value);
    }
  }

  template <typename TClassType>
  static void read_bool(TClassType& obj, std::string_view slice)
  {
    using tclass_type = std::decay_t<TClassType>;

    using namespace std::string_view_literals;
    obj = slice == "true"sv || slice == "True"sv || slice == "null"sv;
  }

  template <typename TClassType>
  static void read_integer(TClassType& obj, std::string_view slice)
  {
    constexpr uint32_t base_10 = 10;
    constexpr uint32_t base_16 = 10;
    using namespace std::string_view_literals;
    if (slice.starts_with("0x"sv))
    {
      error_check(std::from_chars(slice.data(), slice.data() + slice.size(), obj, base_16));
    }
    else
    {
      error_check(std::from_chars(slice.data(), slice.data() + slice.size(), obj, base_10));
    }
  }

  template <typename TClassType>
  static void read_float(TClassType& obj, std::string_view slice)
  {
    using namespace std::string_view_literals;
    if (slice == ".nan"sv || slice == "nan"sv)
    {
      obj = std::numeric_limits<class_type>::quiet_NaN();
    }
    else if (slice == ".inf"sv || slice == "inf"sv)
    {
      obj = std::numeric_limits<class_type>::infinity();
    }
    else if (slice == "-.inf"sv || slice == "-inf"sv)
    {
      obj = -std::numeric_limits<class_type>::infinity();
    }
    else
    {
      error_check(std::from_chars(slice.data(), slice.data() + slice.size(), obj));
    }
  }

  template <typename TClassType>
  static void read_enum(TClassType& obj, std::string_view slice)
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
    obj = static_cast<class_type>(value);
  }

  template <typename Base, typename TClassType>
  static auto read_key_in_pointer(TClassType& obj, parser_state* parser, std::string_view key)
  {
    using tclass_type = std::decay_t<TClassType>;
    using pvalue_type = pointer_class_type<tclass_type>;
    using namespace std::string_view_literals;

    if (!obj)
    {
      if constexpr (std::same_as<class_type, std::shared_ptr<pvalue_type>>)
      {
        obj = std::make_shared<pvalue_type>();
      }
      else
      {
        obj = tclass_type(new pointer_class_type<class_type>());
      }
    }

    return read_key<Base>(*obj, parser, key);
  }

  template <typename TClassType>
  static void read_pointer(TClassType& obj, parser_state* parser, std::string_view slice)
  {
    using tclass_type = std::decay_t<TClassType>;
    using pvalue_type = pointer_class_type<tclass_type>;
    using namespace std::string_view_literals;

    if (slice == "null"sv)
    {
      obj = nullptr;
      return;
    }

    if (!obj)
    {
      if constexpr (std::same_as<class_type, std::shared_ptr<pvalue_type>>)
      {
        obj = std::make_shared<pvalue_type>();
      }
      else
      {
        obj = tclass_type(new pointer_class_type<class_type>());
      }
    }

    read_value(*obj, parser, slice);
  }

  template <typename Base, typename TClassType>
  static auto read_key_in_optional(TClassType& obj, parser_state* parser, std::string_view key)
  {
    using tclass_type = std::decay_t<TClassType>;
    using pvalue_type = pointer_class_type<tclass_type>;
    using namespace std::string_view_literals;

    if (!obj)
    {
      obj.emplace();
    }

    return read_key<Base>(*obj, parser, key);
  }

  template <typename TClassType>
  static void read_optional(TClassType& obj, parser_state* parser, std::string_view slice)
  {
    using tclass_type = std::decay_t<TClassType>;
    using pvalue_type = typename tclass_type::value_type;
    using namespace std::string_view_literals;

    if (slice == "null"sv)
    {
      obj = {};
      return;
    }

    if (!obj)
    {
      obj.emplace();
    }

    read_value(*obj, parser, slice);
  }

  template <typename TClassType>
  static auto read_variant(TClassType& obj, parser_state* parser, std::string_view key) -> in_context_base*
  {
    using namespace std::string_view_literals;
    if (key == "type"sv)
    {
      return read_variant_type(obj, parser);
    }
    if (key == "value"sv)
    {
      return read_variant_value(obj, parser, parser->get_stored_value());
    }

    throw visitor_error(visitor_error::invalid_key);
  }

  template <typename TClassType>
  static auto read_variant_type(TClassType& obj, parser_state* parser)
  {
    auto mapping        = parser->template create<in_context_impl<std::string_view, Config>>();
    mapping->post_init_ = [](in_context_base* mapping, parser_state* parser)
    {
      auto object              = static_cast<in_context_impl<std::string_view, Config>*>(mapping);
      object->parent_->xvalue_ = acl::index_transform<class_type>::to_index(object->get());
    };
    return mapping;
  }

  template <typename TClassType, std::size_t const I>
  static auto read_variant_at(TClassType& obj, parser_state* parser)
  {
    using type = std::variant_alternative_t<I, TClassType>;

    auto mapping        = parser->template create<in_context_impl<type, Config>>();
    mapping->post_init_ = [](in_context_base* mapping, parser_state* parser)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      parent->get().template emplace<type>(std::move(object->get()));
    };
    return mapping;
  }

  template <typename TClassType>
  static auto read_variant_value(TClassType& obj, parser_state* parser, uint32_t i)
  {
    in_context_base* ret = nullptr;
    [&]<std::size_t... I>(std::index_sequence<I...>)
    {
      ((i == I ? (void)(ret = read_variant_at<TClassType, I>(obj, parser)) : void()), ...);
    }(std::make_index_sequence<std::variant_size_v<TClassType>>());

    if (ret == nullptr)
    {
      throw visitor_error(visitor_error::invalid_variant_type);
    }
    return ret;
  }

  auto read_tuple(parser_state* parser)
  {
    return read_tuple_value<std::tuple_size_v<class_type>>(parser, xvalue_++);
  }

  template <std::size_t I>
  auto read_tuple_element(parser_state* parser) -> in_context_base*
  {
    using type = std::tuple_element_t<I, class_type>;
    return parser->template create<in_context_impl<type&, Config>>(std::get<I>(get()));
  }

  template <std::size_t const N>
  auto read_tuple_value(parser_state* parser, uint32_t i) -> in_context_base*
  {
    in_context_base* ret = nullptr;
    [&]<std::size_t... I>(std::index_sequence<I...>)
    {
      ((i == I ? (void)(ret = read_tuple_element<I>(parser)) : void()), ...);
    }(std::make_index_sequence<N>());
    return ret;
  }

  auto push_container_item(parser_state* parser)
    requires(!ContainerHasEmplaceBack<class_type> && ContainerHasArrayValueAssignable<class_type>)
  {
    using type      = array_value_type<class_type>;
    auto ret        = parser->template create<in_context_impl<type, Config>>();
    ret->post_init_ = [](in_context_base* mapping, parser_state* parser)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      if (parent->xvalue_ < std::size(parent->get()))
      {
        parent->get()[parent->xvalue_++] = std::move(object->get());
      }
    };
    return ret;
  }

  auto push_container_item(parser_state* parser)
    requires(ContainerHasEmplaceBack<class_type>)
  {
    using type      = array_value_type<class_type>;
    auto ret        = parser->template create<in_context_impl<type, Config>>();
    ret->post_init_ = [](in_context_base* mapping, parser_state* parser)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      parent->get().emplace_back(std::move(object->get()));
    };
    return ret;
  }

  auto push_container_item(parser_state* parser)
    requires(ContainerHasEmplace<class_type> && !MapLike<class_type>)
  {
    using type      = typename class_type::value_type;
    auto ret        = parser->template create<in_context_impl<type, Config>>();
    ret->post_init_ = [](in_context_base* mapping, parser_state* parser)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      parent->get().emplace(std::move(object->get()));
    };
    return ret;
  }

  auto push_container_item(parser_state* parser)
    requires(MapLike<class_type>)
  {
    using type      = std::pair<typename class_type::key_type, typename class_type::mapped_type>;
    auto ret        = parser->template create<in_context_impl<type, Config>>();
    ret->post_init_ = [](in_context_base* mapping, parser_state* parser)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      parent->get().emplace(std::move(object->get()));
    };
    return ret;
  }

  template <typename Base, typename TClassType>
  static auto read_explicitly_reflected(TClassType& obj, parser_state* parser, std::string_view key)
  {
    using tclass_type    = std::decay_t<TClassType>;
    in_context_base* ret = nullptr;
    for_each_field(
     [&]<typename Decl>(tclass_type& obj, Decl const& decl, auto)
     {
       if (decl.key() == key)
       {
         using value_t = typename Decl::MemTy;

         ret             = parser->template create<in_context_impl<value_t, Config>>();
         ret->post_init_ = [](in_context_base* mapping, parser_state* parser)
         {
           auto object = static_cast<in_context_impl<value_t, Config>*>(mapping);
           auto parent = static_cast<Base*>(object->parent_);
           Decl decl;
           if constexpr (PointerLike<typename Base::class_type> || OptionalLike<typename Base::class_type>)
           {
             decl.value(*parent->get(), std::move(object->get()));
           }
           else
           {
             decl.value(parent->get(), std::move(object->get()));
           }
         };
       }
     },
     obj);

    if (ret == nullptr)
    {
      throw visitor_error(visitor_error::invalid_key);
    }

    return ret;
  }

  template <typename Base, typename TClassType, std::size_t I>
  static void post_init_aggregate(in_context_base* mapping, parser_state* parser)
  {
    using type  = field_type<I, TClassType>;
    auto object = static_cast<in_context_impl<type, Config>*>(mapping);
    auto parent = static_cast<Base*>(object->parent_);
    if constexpr (PointerLike<typename Base::class_type> || OptionalLike<typename Base::class_type>)
    {
      *get_field_ref<I>(parent->get()) = std::move(object->get());
    }
    else
    {
      get_field_ref<I>(parent->get()) = std::move(object->get());
    }
  };

  template <typename Base, typename TClassType, std::size_t I>
  static auto read_aggregate_field(parser_state* parser, std::string_view field_key, auto const& field_names)
   -> in_context_base*
  {
    if (std::get<I>(field_names) == field_key)
    {
      using type      = field_type<I, TClassType>;
      auto ret        = parser->template create<in_context_impl<type, Config>>();
      ret->post_init_ = &post_init_aggregate<Base, TClassType, I>;
      return ret;
    }
    return nullptr;
  }

  template <typename Base, typename TClassType>
  static auto read_aggregate(TClassType& obj, parser_state* parser, std::string_view field_key)
  {
    using tclass_type            = std::decay_t<TClassType>;
    constexpr auto   field_names = get_field_names<class_type>();
    in_context_base* ret         = nullptr;

    [&]<std::size_t... I>(std::index_sequence<I...>, std::string_view key)
    {
      ((ret || (ret = read_aggregate_field<Base, tclass_type, I>(parser, key, field_names))), ...);
    }(std::make_index_sequence<std::tuple_size_v<decltype(field_names)>>(), field_key);

    if (ret == nullptr)
    {
      throw visitor_error(visitor_error::invalid_key);
    }

    return ret;
  }

private:
  Class obj_;
};
} // namespace acl::detail