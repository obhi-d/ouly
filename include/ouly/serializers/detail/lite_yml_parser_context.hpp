// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/linear_arena_allocator.hpp"
#include "ouly/dsl/lite_yml.hpp"
#include "ouly/reflection/visitor.hpp"
#include "ouly/serializers/config.hpp"
#include "ouly/utility/detail/concepts.hpp"
#include "ouly/utility/from_chars.hpp"
#include <charconv>
#include <cstddef>
#include <string>
#include <type_traits>

namespace ouly::detail
{
class parser_state;

/**
 * @brief Base class for context objects in the YAML parsing system
 *
 * This abstract base class defines the interface for tracking parser state
 * during YAML document parsing. Derived contexts handle specific element types
 * and maintain parent-child relationships in the parsing hierarchy.
 */
struct in_context_base
{
  using post_init_fn               = void (*)(in_context_base*, parser_state*);
  in_context_base* parent_         = nullptr; ///< Parent context in the hierarchy
  in_context_base* proxy_          = nullptr; ///< Proxy context for type conversion
  post_init_fn     post_init_      = nullptr; ///< Function called after initialization
  uint32_t         xvalue_         = 0;       ///< Utility value used by contexts
  bool             is_proxy_       = false;   ///< Whether this is a proxy context
  bool             is_post_inited_ = false;   ///< Whether post-initialization has been performed
  bool             has_value_      = false;   ///< Whether the context has a value

  in_context_base() noexcept                                 = default;
  in_context_base(const in_context_base&)                    = default;
  in_context_base(in_context_base&&)                         = delete;
  auto operator=(const in_context_base&) -> in_context_base& = default;
  auto operator=(in_context_base&&) -> in_context_base&      = delete;

  /**
   * @brief Constructs a context with a specified parent
   *
   * @param parent Pointer to the parent context
   */
  in_context_base(in_context_base* parent) noexcept : parent_(parent) {}

  /**
   * @brief Handles a key in the YAML document
   *
   * @param parser The parser state
   * @param ikey The key as a string view
   * @return A new context for the associated value, or null
   */
  virtual auto set_key(parser_state* parser, std::string_view ikey) -> in_context_base* = 0;

  /**
   * @brief Handles a scalar value in the YAML document
   *
   * @param parser The parser state
   * @param slice The value as a string view
   */
  virtual void set_value(parser_state* parser, std::string_view slice) = 0;

  /**
   * @brief Performs post-initialization tasks after an object is fully parsed
   *
   * @param parser The parser state
   */
  virtual void post_init_object(parser_state* parser) = 0;

  /**
   * @brief Adds a new item to a sequence context
   *
   * @param parser The parser state
   * @return A new context for the array item
   */
  virtual auto add_item(parser_state* parser) -> in_context_base* = 0;

  /**
   * @brief Virtual destructor for proper cleanup
   */
  virtual ~in_context_base() noexcept = default;
};

/**
 * @brief Main parser state that manages the YAML parsing process
 *
 * This class maintains the current parsing state, manages a memory arena for
 * context objects, and implements the YAML context interface. It handles the
 * parsing of YAML documents into C++ objects through context handlers.
 */
class parser_state final : public ouly::yml::context
{
  ouly::yml::lite_stream         stream_;            ///< The YAML stream being parsed
  ouly::linear_arena_allocator<> allocator_;         ///< Allocator for context objects
  in_context_base*               context_ = nullptr; ///< Current parsing context

public:
  auto operator=(const parser_state&) -> parser_state& = delete;
  auto operator=(parser_state&&) -> parser_state&      = delete;
  parser_state(const parser_state&)                    = delete;
  parser_state(parser_state&&)                         = delete;

  /**
   * @brief Constructs a parser state for a YAML content string
   *
   * @param content The YAML content to parse
   */
  parser_state(std::string_view content) noexcept
      : stream_(content, this), allocator_(cfg::default_lite_yml_parser_buffer_size)
  {}

  /**
   * @brief Destroys the parser state and cleans up any resources
   */
  ~parser_state() noexcept final
  {
    clear();
  }

  /**
   * @brief Sets the current context
   *
   * @param ctx The new context to use
   */
  void set_context(in_context_base* ctx) noexcept
  {
    context_ = ctx;
  }

  /**
   * @brief Retrieves the stored value from the current context
   *
   * @return The stored value
   */
  [[nodiscard]] auto get_stored_value() const noexcept -> uint32_t
  {
    return context_->xvalue_;
  }

  /**
   * @brief Parses YAML content using the specified handler
   *
   * @tparam C Type of the handler/context
   * @param handler The handler to use for parsing
   */
  template <typename C>
  void parse(C& handler)
  {
    context_ = &handler;
    stream_.parse();
  }

  /**
   * @brief Creates a new context object in the arena
   *
   * @tparam Context The type of context to create
   * @tparam Args Argument types for the context constructor
   * @param args Arguments to pass to the context constructor
   * @return Pointer to the newly created context
   */
  template <typename Context, typename... Args>
  auto create(Args&&... args) -> Context*
  {
    void* cursor = allocator_.allocate(sizeof(Context), alignof(Context));
    // NOLINTNEXTLINE
    return std::construct_at(reinterpret_cast<Context*>(cursor), context_, std::forward<Args>(args)...);
  }

  /**
   * @brief Pops the current context from the stack
   *
   * Handles proxy contexts by traversing up to a non-proxy parent.
   * Calls post_init_object on the context being popped.
   */
  void pop()
  {
    if (context_ != nullptr)
    {
      while (context_->is_proxy_ && (context_->parent_ != nullptr))
      {
        context_ = context_->parent_;
      }
      auto* parent = context_->parent_;
      context_->post_init_object(this);
      context_ = parent;
    }
  }

  /**
   * @brief Destroys a context object and releases its memory
   *
   * @tparam Context The type of context to destroy
   * @param ptr Pointer to the context to destroy
   */
  template <typename Context>
  void destroy(Context* ptr)
  {
    std::destroy_at(ptr);
    allocator_.deallocate(ptr, sizeof(Context), alignof(Context));
  }

  // YAML context interface implementation

  /**
   * @brief Called when a new array begins in the YAML document
   */
  void begin_array() final {}

  /**
   * @brief Called when an array ends in the YAML document
   *
   * Handles proper context cleanup for arrays
   */
  void end_array() final
  {
    if (context_ != nullptr)
    {
      if (!context_->has_value_)
      {
        pop();
      }
    }
    pop();
  }

  /**
   * @brief Called when a new object begins in the YAML document
   */
  void begin_object() final {}

  /**
   * @brief Called when an object ends in the YAML document
   */
  void end_object() final
  {
    pop();
  }

  /**
   * @brief Clears all contexts and resets the parser state
   */
  void clear()
  {
    while (context_ != nullptr)
    {
      pop();
    }
  }

  /**
   * @brief Called when a new array item is encountered
   *
   * Creates a new context for the array item
   */
  void begin_new_array_item() final
  {
    context_ = context_->add_item(this);
  }

  /**
   * @brief Called when a key is encountered in a mapping
   *
   * @param ikey The key as a string view
   */
  void set_key(std::string_view ikey) final
  {
    context_ = context_->set_key(this, ikey);
  }

  /**
   * @brief Called when a scalar value is encountered
   *
   * @param slice The value as a string view
   */
  void set_value(std::string_view slice) final
  {
    context_->has_value_ = true;
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
  static auto read_key(TClassType& obj, parser_state* parser, std::string_view key) -> in_context_base*
  {
    using tclass_type = std::decay_t<TClassType>;

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
    if (is_post_inited_)
    {
      return;
    }

    is_post_inited_ = true;

    if (proxy_ != nullptr)
    {
      proxy_->post_init_object(parser);
    }

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
    has_value_ = true;
    if (parent_ != nullptr)
    {
      parent_->has_value_ = true;
    }
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
    else if constexpr (Convertible<class_type>)
    {
      return read_convertible_container(parser);
    }
    else
    {
      throw visitor_error(visitor_error::type_is_not_an_array);
    }
  }

  auto read_convertible_container(parser_state* parser) -> in_context_base*
  {
    if (proxy_ == nullptr)
    {
      using value_type   = convertible_to_type<class_type>;
      proxy_             = parser->template create<in_context_impl<value_type, Config>>();
      proxy_->is_proxy_  = true;
      proxy_->post_init_ = [](in_context_base* mapping, parser_state* /*parser_ptr*/)
      {
        auto object = static_cast<in_context_impl<value_type, Config>*>(mapping);
        auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
        ouly::convert<class_type>::from_type(parent->get(), object->get());
      };
    }
    if (proxy_ == nullptr)
    {
      throw visitor_error(visitor_error::type_is_not_an_array);
    }
    parser->set_context(proxy_);
    return proxy_->add_item(parser);
  }

  template <typename TClassType>
  static void read_value(TClassType& obj, parser_state* parser, std::string_view slice)
  {
    using tclass_type = std::decay_t<TClassType>;
    if constexpr (Convertible<tclass_type>)
    {
      if constexpr (requires { typename Config::mutate_enums_type; } && std::is_enum_v<tclass_type> &&
                    ouly::detail::ConvertibleFrom<tclass_type, std::string_view>)
      {
        ouly::convert<tclass_type>::from_type(obj, transform_type::transform(slice));
      }
      else if constexpr (ouly::detail::ConvertibleFrom<class_type, std::string_view>)
      {
        ouly::convert<tclass_type>::from_type(obj, slice);
      }
      else
      {
        ouly::detail::convertible_to_type<tclass_type> value = {};
        read_value(value, parser, slice);
        ouly::convert<tclass_type>::from_type(obj, value);
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

  template <typename TClassType>
  static void read_bool(TClassType& obj, std::string_view slice)
  {
    using namespace std::string_view_literals;
    obj = slice == "true"sv || slice == "True"sv || slice == "null"sv;
  }

  template <typename TClassType>
  static void read_integer(TClassType& obj, std::string_view slice)
  {
    using namespace std::string_view_literals;
    from_chars(slice, obj);
  }

  template <typename TClassType>
  static void read_float(TClassType& obj, std::string_view slice)
  {
    from_chars(slice, obj);
  }

  template <typename TClassType>
  static void read_enum(TClassType& obj, std::string_view slice)
  {

    std::underlying_type_t<class_type> value;
    from_chars(slice, value);

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
    if (key == cache_key<transform_type, "type">())
    {
      return read_variant_type(obj, parser);
    }
    if (key == cache_key<transform_type, "value">())
    {
      return read_variant_value(obj, parser, parser->get_stored_value());
    }

    throw visitor_error(visitor_error::invalid_key);
  }

  template <typename TClassType>
  static auto read_variant_type([[maybe_unused]] TClassType& obj, parser_state* parser)
  {
    auto mapping        = parser->template create<in_context_impl<std::string_view, Config>>();
    mapping->post_init_ = [](in_context_base* mapping_base, parser_state* /* parser_ptr */)
    {
      auto object              = static_cast<in_context_impl<std::string_view, Config>*>(mapping_base);
      object->parent_->xvalue_ = static_cast<uint32_t>(ouly::index_transform<class_type>::to_index(object->get()));
    };
    return mapping;
  }

  template <typename TClassType, std::size_t const I>
  static auto read_variant_at([[maybe_unused]] TClassType& obj, parser_state* parser)
  {
    using type = std::variant_alternative_t<I, TClassType>;

    auto mapping        = parser->template create<in_context_impl<type, Config>>();
    mapping->post_init_ = [](in_context_base* mapping_base, parser_state* /*parser_ptr*/)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping_base);
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
    ret->post_init_ = [](in_context_base* mapping, parser_state* /*parser_ptr*/)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      if (parent->xvalue_ < std::size(parent->get()) && object->has_value_)
      {
        parent->has_value_               = true;
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
    ret->post_init_ = [](in_context_base* mapping, parser_state* /*parser_ptr*/)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      if (object->has_value_)
      {
        parent->has_value_ = true;
        parent->get().emplace_back(std::move(object->get()));
      }
    };
    return ret;
  }

  auto push_container_item(parser_state* parser)
    requires(ContainerHasEmplace<class_type> && !MapLike<class_type>)
  {
    using type      = typename class_type::value_type;
    auto ret        = parser->template create<in_context_impl<type, Config>>();
    ret->post_init_ = [](in_context_base* mapping, parser_state* /*parser_ptr*/)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      if (object->has_value_)
      {
        parent->has_value_ = true;
        parent->get().emplace(std::move(object->get()));
      }
    };
    return ret;
  }

  auto push_container_item(parser_state* parser)
    requires(MapLike<class_type>)
  {
    using type      = std::pair<typename class_type::key_type, typename class_type::mapped_type>;
    auto ret        = parser->template create<in_context_impl<type, Config>>();
    ret->post_init_ = [](in_context_base* mapping, parser_state* /*parser_ptr*/)
    {
      auto object = static_cast<in_context_impl<type, Config>*>(mapping);
      auto parent = static_cast<in_context_impl<Class, Config>*>(object->parent_);
      if (object->has_value_)
      {
        parent->has_value_ = true;
        parent->get().emplace(std::move(object->get()));
      }
    };
    return ret;
  }

  template <typename Base, typename TClassType>
  static auto read_explicitly_reflected(TClassType& obj, parser_state* parser, std::string_view key)
  {
    using tclass_type    = std::decay_t<TClassType>;
    in_context_base* ret = nullptr;
    for_each_field(
     [&]<typename Decl>(tclass_type& /*lobj*/, Decl const& decl, auto)
     {
       if (decl.template cache_key<transform_type>() == key)
       {
         using mem_value_type = typename Decl::MemTy;

         ret             = parser->template create<in_context_impl<mem_value_type, Config>>();
         ret->post_init_ = [](in_context_base* mapping, parser_state* /*parser_ptr*/)
         {
           auto object = static_cast<in_context_impl<mem_value_type, Config>*>(mapping);
           auto parent = static_cast<Base*>(object->parent_);
           Decl ldecl;
           if constexpr (PointerLike<typename Base::class_type> || OptionalLike<typename Base::class_type>)
           {
             ldecl.value(*parent->get(), std::move(object->get()));
           }
           else
           {
             ldecl.value(parent->get(), std::move(object->get()));
           }

           parent->has_value_ = true;
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
  static void post_init_aggregate(in_context_base* mapping, [[maybe_unused]] parser_state* parser)
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
    parent->has_value_ = true;
  };

  template <typename Base, typename TClassType, std::size_t I>
  static inline void get_aggregate_field(in_context_base*& ret, parser_state* parser, std::string_view field_key,
                                         auto const& field_names)
  {
    if (ret)
      return;
    if (std::get<I>(field_names) == field_key)
    {
      using type      = field_type<I, TClassType>;
      ret             = parser->template create<in_context_impl<type, Config>>();
      ret->post_init_ = &post_init_aggregate<Base, TClassType, I>;
    }
  }

  template <typename Base, typename TClassType>
  static auto read_aggregate([[maybe_unused]] TClassType& obj, parser_state* parser, std::string_view field_key)
  {
    using tclass_type = std::decay_t<TClassType>;

    auto const&      field_names = get_cached_field_names<tclass_type, transform_type>();
    in_context_base* ret         = nullptr;

    [&]<std::size_t... I>(std::index_sequence<I...>, std::string_view key)
    {
      (get_aggregate_field<Base, tclass_type, I>(ret, parser, key, field_names), ...);
    }(std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(field_names)>>>(), field_key);

    if (ret == nullptr)
    {
      throw visitor_error(visitor_error::invalid_key);
    }

    return ret;
  }

private:
  Class obj_;
};
} // namespace ouly::detail
