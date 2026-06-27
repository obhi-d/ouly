// SPDX-License-Identifier: MIT

#pragma once
#include "ouly/dsl/lite_yml.hpp"
#include "ouly/serializers/detail/lite_yml_parser_context.hpp"
#include "ouly/serializers/detail/lite_yml_writer_context.hpp"
#include "ouly/serializers/detail/structured_output_serializer.hpp"
#include "ouly/serializers/serializers.hpp"
#include "ouly/utility/convert.hpp"
#include "ouly/utility/runtime_type.hpp"

#include <functional>
#include <string_view>
#include <unordered_map>

namespace ouly::yml
{

/**
 * @brief Registry resolving ouly::type_id values to concrete C++ types for
 *        runtime_type (de)serialization.
 *
 * Each bound type contributes a read trampoline (construct the type into the
 * runtime_type's value storage and return a parse context) and a write trampoline
 * (emit the stored value). Both are fully type-erased behind this registry so the
 * parser/writer stay decoupled from the concrete bound types.
 *
 * A runtime_type is serialized just like a std::variant, as a two-field object:
 * @code{.yaml}
 * field:
 *   type: 1        # the ouly::type_id
 *   value: ...     # parsed/emitted as the bound concrete type
 * @endcode
 *
 * Field names ("type" / "value") are configurable through the same mechanism as
 * variant: they are run through the Config's string transform (transform_t<Config>).
 * Choosing a Config with a string transform therefore renames these keys (along with
 * every other key in the document). For example, with
 * @code
 * using upper = ouly::config<ouly::to_upper>;
 * ouly::yml::runtime_type_registry<upper> reg; // emits/parses TYPE: and VALUE:
 * @endcode
 * the keys become "TYPE" and "VALUE". The Config used here must match the one passed
 * to from_string/to_string (it is deduced from this registry's template parameter).
 *
 * @tparam Config The serialization configuration (must match the from_string/to_string call).
 */
template <typename Config = ouly::config<>>
class runtime_type_registry : public ouly::detail::runtime_type_reader_base,
                              public ouly::detail::runtime_type_writer_base<Config>
{
  using writer_visitor = ouly::detail::structured_output_serializer<ouly::detail::writer_state, Config>;
  using read_fn        = auto (*)(ouly::any&, ouly::detail::parser_state*) -> ouly::detail::in_context_base*;
  using write_fn       = void (*)(ouly::any const&, writer_visitor&);

  struct entry
  {
    read_fn  read_  = nullptr;
    write_fn write_ = nullptr;
  };

  std::unordered_map<int, entry>                 map_;
  std::function<ouly::type_id(std::string_view)> from_name_;
  std::function<std::string_view(ouly::type_id)> to_name_;

public:
  /**
   * @brief Installs name<->type_id resolvers used to (de)serialize the "type" scalar
   *        as a string name instead of an integer.
   *
   * When set, the serializers route the "type" field through these functors. When left
   * unset, the default path is used (a convert<type_id> specialization if present,
   * otherwise a plain integer). The view returned by @p to_name must outlive the
   * serialization call (e.g. a string literal or a string owned by the caller).
   */
  void set_name_resolver(std::function<ouly::type_id(std::string_view)> from_name,
                         std::function<std::string_view(ouly::type_id)> to_name)
  {
    from_name_ = std::move(from_name);
    to_name_   = std::move(to_name);
  }

  [[nodiscard]] auto reads_type_by_name() const -> bool override
  {
    return static_cast<bool>(from_name_);
  }

  auto type_id_from_name(std::string_view name) -> ouly::type_id override
  {
    return from_name_(name);
  }

  [[nodiscard]] auto writes_type_by_name() const -> bool override
  {
    return static_cast<bool>(to_name_);
  }

  auto type_name_from_id(ouly::type_id id) -> std::string_view override
  {
    return to_name_(id);
  }

  /**
   * @brief Associates a type_id with a concrete type T.
   */
  template <typename T>
  void bind(ouly::type_id id)
  {
    entry e;
    e.read_ = [](ouly::any& storage, ouly::detail::parser_state* parser) -> ouly::detail::in_context_base*
    {
      T& ref = storage.template emplace<T>();
      return parser->template create<ouly::detail::in_context_impl<T&, Config>>(ref);
    };
    e.write_ = [](ouly::any const& storage, writer_visitor& visitor) -> void
    {
      auto const* value = storage.template get_if<T>();
      if (value != nullptr)
      {
        ouly::visit(*value, visitor);
      }
    };
    map_[id.id_] = e;
  }

  auto parse_value(ouly::type_id id, ouly::any& storage, ouly::detail::parser_state* parser)
   -> ouly::detail::in_context_base* override
  {
    auto found = map_.find(id.id_);
    if (found == map_.end())
    {
      throw ouly::visitor_error(ouly::visitor_error::unknown_runtime_type);
    }
    return found->second.read_(storage, parser);
  }

  void write_value(ouly::type_id id, ouly::any const& storage, writer_visitor& visitor) override
  {
    auto found = map_.find(id.id_);
    if (found == map_.end())
    {
      throw ouly::visitor_error(ouly::visitor_error::unknown_runtime_type);
    }
    found->second.write_(storage, visitor);
  }
};

template <typename Class, typename Config = ouly::config<>>
auto to_string(Class const& obj) -> std::string
{
  auto state = ouly::detail::writer_state();
  ouly::write<Config>(state, obj);
  return state.get();
}

template <typename Class, typename Config = ouly::config<>>
auto to_string(Class const& obj, runtime_type_registry<Config>& registry) -> std::string
{
  auto stream = ouly::detail::writer_state();
  auto state  = ouly::detail::structured_output_serializer<ouly::detail::writer_state, Config>(stream);
  state.set_runtime_writer(&registry);
  ouly::visit(obj, state);
  return stream.get();
}

template <typename Class, typename Config = ouly::config<>>
void from_string(Class& obj, std::string_view data)
{
  auto in_context_impl = ouly::detail::in_context_impl<Class&, Config>(obj);
  {
    auto state = ouly::detail::parser_state(data);
    state.parse(in_context_impl);
  }
}

template <typename Class, typename Config = ouly::config<>>
void from_string(Class& obj, std::string_view data, runtime_type_registry<Config>& registry)
{
  auto in_context_impl = ouly::detail::in_context_impl<Class&, Config>(obj);
  {
    auto state = ouly::detail::parser_state(data);
    state.set_runtime(&registry);
    state.parse(in_context_impl);
  }
}
} // namespace ouly::yml
