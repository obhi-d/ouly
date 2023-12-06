
#pragma once

#include "parameter.hpp"
#include <acl/allocators/linear_stack_allocator.hpp>
#include <acl/utils/reflection.hpp>
#include <acl/utils/reflection_utils.hpp>
#include <acl/utils/string_literal.hpp>
#include <charconv>
#include <functional>
#include <ostream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace acl
{

class scli;
using cmd_execute = bool (*)(scli&);
using cmd_enter   = bool (*)(scli&);
using cmd_exit    = void (*)(scli&);
struct param_context;
using text_content = std::variant<std::string_view, std::string>;

inline std::string_view view(text_content const& tc)
{
  return tc.index() == 0 ? std::string_view(std::get<0>(tc)) : std::string_view(std::get<1>(tc));
}

using inner_param_context = param_context* (*)(scli&, int param_pos, std::string_view param_name);

struct cmd_state;

struct base_context : public std::enable_shared_from_this<base_context>
{
  virtual ~base_context() noexcept = default;
};

struct param_context : base_context
{
  inline virtual std::pair<param_context*, cmd_state*> enter_param_context(scli&, int, std::string_view,
                                                                           cmd_state* cstate)
  {
    return {nullptr, cstate};
  }

  inline virtual void exit_param_context(scli&, int param_pos, cmd_state* cstate_inner, cmd_state* cstate_cur) {}

  inline virtual void parse_param(scli&, std::string_view value, cmd_state*) {}
  inline virtual void parse_param(scli&, int param_pos, std::string_view param_name, std::string_view value, cmd_state*)
  {}
};

struct cmd_context : param_context
{
  inline virtual cmd_state* construct(scli&)
  {
    return nullptr;
  }

  inline virtual void destroy(scli&, cmd_state*) {}

  inline virtual bool execute(scli&, cmd_state*)
  {
    return true;
  }

  inline virtual void enter_region(scli&, std::string_view id, std::string_view name) {}

  inline virtual void enter_region(scli&, std::string_view id, std::string_view name, text_content&& content) {}

  inline virtual bool is_text_context() const noexcept
  {
    return false;
  }

  inline virtual bool enter(scli&, cmd_state*)
  {
    return true;
  }

  inline virtual void exit(scli&, cmd_state*) {}

  inline virtual cmd_context* get_context(scli const&, std::string_view cmd_name)
  {
    return nullptr;
  }

  inline virtual void         add_sub_command(std::string_view name, std::shared_ptr<cmd_context> cmd) {}
  inline virtual cmd_context* get_sub_command(std::string_view name)
  {
    return nullptr;
  }
};

using stack_allocator                     = linear_stack_allocator<>;
static constexpr uint32_t scli_stack_size = 2048;

class scli
{
public:
  struct position
  {
    uint32_t    line                                        = 1;
    uint32_t    character                                   = 1;
    inline auto operator<=>(position const&) const noexcept = default;

    inline friend std::ostream& operator<<(std::ostream& yyo, position const& l) noexcept
    {
      yyo << l.line << ':' << l.character;
      return yyo;
    }
  };

  class location
  {
  public:
    inline void step() noexcept
    {
      begin = end;
    }

    inline void columns(std::uint32_t l) noexcept
    {
      end.character += l;
    }

    inline void lines(std::uint32_t l) noexcept
    {
      end.line += l;
      end.character = 0;
    }

    inline operator std::string() const noexcept

    {
      std::string value = std::string(source_name.empty() ? "buffer" : source_name);
      value += "(" + std::to_string(begin.line) + ":" + std::to_string(begin.character) + "-" +
               std::to_string(end.line) + ":" + std::to_string(end.character) + "): ";
      return value;
    }

    inline friend std::ostream& operator<<(std::ostream& yyo, location const& l) noexcept

    {
      std::string value = std::string(l.source_name.empty() ? "buffer" : l.source_name);
      if (l.begin == l.end)
        yyo << "<" << value << '-' << l.begin << ">";
      else
        yyo << "<" << value << '-' << l.begin << "-" << l.end << ">";
      return yyo;
    }

    inline auto operator<=>(scli::location const&) const noexcept = default;

    std::string_view source_name;
    position         begin;
    position         end;
  };

  using def_imported_string_map = std::unordered_map<std::string_view, std::string>;
  using error_handler_lambda =
    std::function<void(scli::location const&, std::string_view error, std::string_view context)>;
  using import_handler_lambda = std::function<std::string_view(std::string_view)>;

  class context;
  class builder;
  class parser;

  struct shared_state
  {
    void*                    user_ctx = nullptr;
    context&                 ctx;
    std::vector<std::string> include_paths;
    def_imported_string_map  imports;
    error_handler_lambda     error_handler;
    import_handler_lambda    import_handler;
    stack_allocator          allocator;

    inline shared_state(context& cctx) : ctx(cctx), allocator(scli_stack_size)
    {
      import_handler = [this](std::string_view imp) -> std::string_view
      {
        return scli::default_import_handler(*this, imp);
      };
    }
  };

  inline scli(shared_state& ss) noexcept : sstate(ss) {}

  /**
   * @par API
   */
  template <typename UserContext, typename L = std::false_type>
  inline static void parse(context& c, UserContext& uc, std::string_view src_name, std::string_view content,
                           std::vector<std::string> include_paths = {}, error_handler_lambda ehl = {},
                           import_handler_lambda ihl = {}, L&& pre_parse_cbk = {}) noexcept
  {
    shared_state ss(c);
    ss.user_ctx = &uc;
    if (ehl)
      ss.error_handler = ehl;
    if (ihl)
      ss.import_handler = ihl;
    ss.include_paths = std::move(include_paths);
    auto scli_inst   = scli(ss);
    if constexpr (!std::is_same_v<L, std::false_type>)
      pre_parse_cbk(scli_inst);
    scli_inst.parse(src_name, content);
  }

  template <typename UserContext>
  inline UserContext& get() noexcept
  {
    return *reinterpret_cast<UserContext*>(sstate.user_ctx);
  }

  /**
   * @par Command management
   */
  template <typename T>
  inline T* create_cmd_state()
  {
    auto rewind   = sstate.allocator.get_rewind_point();
    auto state    = new (sstate.allocator.allocate(sizeof(T), alignarg<T>)) T;
    state->rewind = rewind;
    return state;
  }

  template <typename T>
  inline void destroy_cmd_state(T* state)
  {
    sstate.allocator.rewind(state->rewind);
  }

  inline std::string_view get_command_name() noexcept
  {
    return command;
  }

  /**
   * @par API
   */
  ACL_API void parse(std::string_view src_name, std::string_view content) noexcept;

  /**
   * @par Parser utilities
   */
  void set_next_command(std::string_view name) noexcept;
  void execute_command();
  void enter_command_scope();
  void exit_command_scope();
  void set_next_param_name(std::string_view) noexcept;
  void set_param(std::string_view);
  void set_param(text_content&&);
  void enter_param_scope();
  void exit_param_scope();
  void enter_region(std::string_view);
  void enter_text_region(std::string_view, text_content&&);
  void import_script(text_content&&);
  void destroy_comamnd_state();

  static std::string_view default_import_handler(shared_state&, std::string_view) noexcept;

  /**
   * @par Lexer utilities
   */
  int                     read(char* buffer, int siz) noexcept;
  void                    begin_scan() noexcept;
  void                    end_scan() noexcept;
  void                    put(int32_t len) noexcept;
  void                    skip_len(std::int32_t len) noexcept;
  static std::string_view trim(std::string_view str, std::string_view whitespace = " \t");
  std::string_view        make_token() noexcept;
  void                    escape_sequence(std::string_view ss) noexcept;
  text_content            make_text() noexcept;
  void                    set_current_reg_id(std::string_view name) noexcept;
  void                    error(scli::location const&, std::string_view error, std::string_view context);
  bool                    is_code_region(std::string_view) const noexcept;
  std::string_view        get_file_name() const noexcept
  {
    return source_name;
  }

  void* get_scanner() const noexcept
  {
    return scanner;
  }

  scli::location source;
  std::string    token;

private:
  std::string_view get() const noexcept;

  using command_stack = std::vector<std::pair<cmd_context*, cmd_state*>>;
  using param_stack   = std::vector<std::tuple<param_context*, cmd_state*, int>>;

  shared_state& sstate;
  void*         scanner           = nullptr;
  cmd_context*  current_cmd_ctx   = nullptr;
  cmd_context*  current_cmd       = nullptr;
  cmd_state*    current_cmd_state = nullptr;

  param_context* parent_param_ctx = nullptr;
  param_context* param_ctx        = nullptr;

  std::string_view parameter;
  std::string_view command;
  std::string_view contents;
  std::string_view region_id;
  std::string_view source_name;
  param_stack      param_ctx_stack;
  command_stack    cmd_ctx_stack;
  int              param_pos           = 0;
  int              skip_depth          = 0;
  int              pos                 = 0;
  int              pos_commit          = 0;
  int              len_reading         = 0;
  bool             skip_if_cmd_missing = true;
};

namespace detail
{
template <typename C>
concept AutoCommand = requires(C c, scli& s) {
  {
    c.execute(s)
  } -> std::same_as<bool>;
};

template <typename C>
concept SimpleCommand = requires(C c, scli& s, parameter_list params) {
  {
    c.execute(s, params)
  } -> std::same_as<bool>;
};

template <typename C>
concept ClassicCommand = requires(C c, scli& s, parameter_list params) {
  {
    c.execute(s)
  } -> std::same_as<bool>;

  typename C::classic_command_type;
};

template <typename C>
concept HasGetPointer = requires(C c) {
  typename C::pointer;
  {
    c.get()
  } -> std::same_as<typename C::pointer>;
};

template <typename C>
concept HasEntry = requires(C c, scli& s) {
  {
    c.enter(s)
  } -> std::same_as<bool>;
};

template <typename C>
concept HasExit = requires(C c, scli& s) {
  {
    c.exit(s)
  } -> std::same_as<void>;
};

template <typename C>
concept CommandType = requires(C c) {
  typename C::command_type;
  {
    c.name()
  } -> std::same_as<std::string_view>;
};

template <typename Type>
concept CodeRegionHandler = requires(acl::scli& s) {
  {
    Type::enter(s, std::declval<std::string_view>(), std::declval<std::string_view>())
  } -> std::same_as<void>;
};

template <typename Type>
concept TextRegionHandler = requires(acl::scli& s) {
  {
    Type::enter(s, std::declval<std::string_view>(), std::declval<std::string_view>(),
                std::move(std::declval<text_content>()))
  } -> std::same_as<void>;
};

template <typename Type>
concept RegionHandler = CodeRegionHandler<Type> || TextRegionHandler<Type>;
// Subclasses
struct classic_param_context
{
  static param_context* get_instance();
};

template <typename ParamClass>
struct param_context_impl
{
  static param_context* get_instance();
};

template <detail::BoundClass ParamClass>
struct param_context_impl_bc : param_context
{
  static constexpr uint32_t size = field_size<ParamClass>();
  using cmd_state_offsetter      = cmd_state* (*)(cmd_state*);

  struct type_erased_member_ref
  {
    std::string_view    name;
    param_context*      internal_ctx = nullptr;
    cmd_state_offsetter offset       = nullptr;
  };

  std::array<type_erased_member_ref, size> members;

  param_context_impl_bc() noexcept
  {
    for_each_field<ParamClass>(
      [this]<typename Decl>(Decl const& decl, auto ii) noexcept
      {
        using value_t            = typename Decl::MemTy;
        members[ii].name         = decl.key();
        members[ii].internal_ctx = param_context_impl<value_t>::get_instance();
        members[ii].offset       = [](cmd_state* state) -> cmd_state*
        {
          return reinterpret_cast<cmd_state*>(Decl::offset(*reinterpret_cast<ParamClass*>(state)));
        };
      });
  }

  std::pair<param_context*, cmd_state*> enter_param_context(scli&, int param_pos, std::string_view param_name,
                                                            cmd_state* cstate) override
  {
    if (!param_name.empty())
    {
      for (auto& m : members)
      {
        if (m.name == param_name)
        {
          return std::pair<param_context*, cmd_state*>{m.internal_ctx, m.offset(cstate)};
        }
      }
    }
    else if (param_pos < (int)size)
    {
      auto& m = members[param_pos];
      return std::pair<param_context*, cmd_state*>{m.internal_ctx, m.offset(cstate)};
    }
    return {nullptr, cstate};
  }

  void parse_param(scli&, std::string_view value, cmd_state*) override {}
  void parse_param(scli& scli, int param_pos, std::string_view param_name, std::string_view value,
                   cmd_state* cstate) override
  {
    if (!param_name.empty())
    {
      for (auto& m : members)
      {
        if (m.name == param_name)
        {
          m.internal_ctx->parse_param(scli, value, m.offset(cstate));
          return;
        }
      }
    }
    else if (param_pos < (int)size)
    {
      auto& m = members[param_pos];
      m.internal_ctx->parse_param(scli, value, m.offset(cstate));
    }
  }
};

template <detail::TupleLike ParamClass>
struct param_context_impl_tl : param_context
{
  static constexpr auto size = std::tuple_size_v<ParamClass>;
  using cmd_state_offsetter  = cmd_state* (*)(cmd_state*);

  struct type_erased_member_ref
  {
    param_context*      internal_ctx = nullptr;
    cmd_state_offsetter offset       = nullptr;
  };

  std::array<type_erased_member_ref, size> members;

  param_context_impl_tl() noexcept
  {
    [&]<typename L, std::size_t... I>(std::index_sequence<I...>, L&& fn)
    {
      (fn(std::integral_constant<std::size_t, I>()), ...);
    }(std::make_index_sequence<size>(),

      [this]<typename I>(I ii) noexcept
      {
        using value_t            = std::tuple_element_t<I::value, ParamClass>;
        members[ii].internal_ctx = param_context_impl<value_t>::get_instance();
        members[ii].offset       = [](cmd_state* state) -> cmd_state*
        {
          return reinterpret_cast<cmd_state*>(&std::get<I::value>(*reinterpret_cast<ParamClass*>(state)));
        };
      });
  }

  std::pair<param_context*, cmd_state*> enter_param_context(scli&, int param_pos, std::string_view param_name,
                                                            cmd_state* cstate) override
  {
    if (param_pos < (int)size)
    {
      auto& m = members[param_pos];
      return std::pair<param_context*, cmd_state*>{m.internal_ctx, m.offset(cstate)};
    }
    return std::pair<param_context*, cmd_state*>{nullptr, cstate};
  }

  void parse_param(scli&, std::string_view value, cmd_state* cstate) override {}
  void parse_param(scli& scli, int param_pos, std::string_view param_name, std::string_view value,
                   cmd_state* cstate) override
  {
    if (param_pos < (int)size)
    {
      auto& m = members[param_pos];
      m.internal_ctx->parse_param(scli, value, m.offset(cstate));
    }
  }
};

template <detail::ContainerLike ParamClass>
struct param_context_impl_cl : param_context
{
  using value_t            = detail::array_value_type<ParamClass>;
  param_context* value_ctx = nullptr;

  struct state
  {
    value_t                       data;
    stack_allocator::rewind_point rewind;
  };

  param_context_impl_cl() noexcept
  {
    value_ctx = param_context_impl<value_t>::get_instance();
  }

  std::pair<param_context*, cmd_state*> enter_param_context(scli& scli, int param_pos, std::string_view param_name,
                                                            cmd_state* cstate) override
  {
    return std::pair<param_context*, cmd_state*>{value_ctx, (cmd_state*)scli.create_cmd_state<state>()};
    // container like will be called sequentially, so we can ignore param_pos
  }

  void exit_param_context(scli& scli, int param_pos, cmd_state* cstate_inner, cmd_state* cstate) override
  {
    auto pcs = (reinterpret_cast<ParamClass*>(cstate));
    auto cs  = (reinterpret_cast<state*>(cstate_inner));
    if constexpr (detail::HasEmplaceFn<ParamClass, detail::array_value_type<ParamClass>>)
    {
      detail::emplace(*pcs, std::move(cs->data));
    }
    else
    {
      detail::resize(*pcs, static_cast<std::size_t>(param_pos + 1));
      (*pcs)[param_pos] = std::move(cs->data);
    }
    scli.destroy_cmd_state(cs);
  }

  void parse_param(scli& scli, std::string_view value, cmd_state* cstate) override
  {
    parse_param(scli, 0, {}, value, cstate);
  }

  void parse_param(scli& scli, int param_pos, std::string_view param_name, std::string_view value,
                   cmd_state* cstate) override
  {
    auto    pcs = (reinterpret_cast<ParamClass*>(cstate));
    value_t data;
    value_ctx->parse_param(scli, value, (cmd_state*)&data);
    if constexpr (detail::HasEmplaceFn<ParamClass, detail::array_value_type<ParamClass>>)
    {
      detail::emplace(*pcs, std::move(data));
    }
    else
    {
      detail::resize(*pcs, static_cast<std::size_t>(param_pos + 1));
      (*pcs)[param_pos] = std::move(data);
    }
  }
};

template <detail::VariantLike ParamClass>
struct param_context_impl_vl : param_context
{
  static constexpr auto size = std::variant_size_v<ParamClass>;
  using cmd_state_offsetter  = cmd_state* (*)(cmd_state*);
  using cmd_state_emplace    = void (*)(cmd_state*);

  struct type_erased_member_ref
  {
    param_context*      internal_ctx = nullptr;
    cmd_state_offsetter offset       = nullptr;
    cmd_state_emplace   emplace      = nullptr;
  };

  std::array<type_erased_member_ref, size> members;

  param_context_impl_vl() noexcept
  {
    [&]<typename L, std::size_t... I>(std::index_sequence<I...>, L&& fn)
    {
      (fn(std::integral_constant<std::size_t, I>()), ...);
    }(std::make_index_sequence<size>(),

      [this]<typename I>(I ii) noexcept
      {
        using value_t            = std::variant_alternative_t<I::value, ParamClass>;
        members[ii].internal_ctx = param_context_impl<value_t>::get_instance();
        members[ii].offset       = [](cmd_state* cstate) -> cmd_state*
        {
          return (cmd_state*)(&std::get<I::value>(*reinterpret_cast<ParamClass*>(cstate)));
        };
        members[ii].emplace = [](cmd_state* cstate)
        {
          (*reinterpret_cast<ParamClass*>(cstate)).template emplace<I::value>();
        };
      });
  }

  std::pair<param_context*, cmd_state*> enter_param_context(scli& scli, int param_pos, std::string_view param_name,
                                                            cmd_state* cstate) override
  {
    if (param_name == "value" || param_pos == 1)
    {
      auto idx = reinterpret_cast<ParamClass*>(cstate)->index();
      return std::pair<param_context*, cmd_state*>{members[idx].internal_ctx, members[idx].offset(cstate)};
    }
    scli.error(scli.source, "Type must have been set for variant parameter.", param_name);
    return std::pair<param_context*, cmd_state*>{nullptr, nullptr};
    // container like will be called sequentially, so we can ignore param_pos
  }

  void parse_param(scli& scli, int param_pos, std::string_view param_name, std::string_view value,
                   cmd_state* cstate) override
  {
    if (param_name == "index" || param_pos == 0)
    {
      uint32_t id = 0;
      std::from_chars(value.data(), value.data() + value.size(), id);
      if (id < size)
        members[id].emplace(cstate);
    }
    else if (param_name == "value" || param_pos == 1)
    {
      auto idx = reinterpret_cast<ParamClass*>(cstate)->index();
      members[idx].internal_ctx->parse_param(scli, value, members[idx].offset(cstate));
    }
  }
};

template <detail::ConstructedFromStringView ParamClass>
struct param_context_impl_cfsv : param_context
{
  void parse_param(scli& scli, std::string_view value, cmd_state* cstate) override
  {
    *reinterpret_cast<ParamClass*>(cstate) = ParamClass(value);
  }
};

template <detail::TransformFromString ParamClass>
struct param_context_impl_tfs : param_context
{
  void parse_param(scli& scli, std::string_view value, cmd_state* cstate) override
  {
    acl::from_string(*reinterpret_cast<ParamClass*>(cstate), value);
  }
};

template <detail::StringLike ParamClass>
struct param_context_impl_sl : param_context
{
  void parse_param(scli& scli, std::string_view value, cmd_state* cstate) override
  {
    *reinterpret_cast<ParamClass*>(cstate) = ParamClass(value);
  }
};

template <detail::BoolLike ParamClass>
struct param_context_impl_bl : param_context
{
  void parse_param(scli& scli, std::string_view value, cmd_state* cstate) override
  {
    *reinterpret_cast<ParamClass*>(cstate) = value == "true" || value == "1";
  }
};

template <detail::IntegerLike ParamClass>
struct param_context_impl_il : param_context
{
  void parse_param(scli& scli, std::string_view value, cmd_state* cstate) override
  {
    std::from_chars(value.data(), value.data() + value.size(), *reinterpret_cast<ParamClass*>(cstate));
  }
};

template <detail::FloatLike ParamClass>
struct param_context_impl_fl : param_context
{
  void parse_param(scli& scli, std::string_view value, cmd_state* cstate) override
  {
    std::from_chars(value.data(), value.data() + value.size(), *reinterpret_cast<ParamClass*>(cstate));
  }
};

template <detail::PointerLike ParamClass>
struct param_context_impl_pl : param_context
{
  using value_t            = std::remove_pointer_t<ParamClass>;
  param_context* value_ctx = nullptr;

  param_context_impl_pl() noexcept
  {
    value_ctx = param_context_impl<value_t>::get_instance();
  }

  void emplace(cmd_state* state)
  {
    if constexpr (std::same_as<ParamClass, std::shared_ptr<value_t>>)
      *reinterpret_cast<ParamClass*>(state) = std::make_shared<value_t>();
    else
      *reinterpret_cast<ParamClass*>(state) = ParamClass(new detail::pointer_class_type<ParamClass>());
  }

  cmd_state* redirect(cmd_state* state)
  {
    if constexpr (detail::HasGetPointer<ParamClass>)
      return (cmd_state*)reinterpret_cast<ParamClass*>(state)->get();
    else
      return (cmd_state*)(*reinterpret_cast<ParamClass*>(state));
  }

  virtual std::pair<param_context*, cmd_state*> enter_param_context(scli& scli, int param_pos,
                                                                    std::string_view param_name, cmd_state* cstate)
  {
    if (param_pos == 0)
      emplace(cstate);
    return value_ctx->enter_param_context(scli, param_pos, param_name, redirect(cstate));
  }

  virtual void exit_param_context(scli& scli, int param_pos, cmd_state* cstate_inner, cmd_state* cstate_cur)
  {
    return value_ctx->exit_param_context(scli, param_pos, cstate_inner, redirect(cstate_cur));
  }

  virtual void parse_param(scli& scli, std::string_view value, cmd_state* cstate)
  {
    emplace(cstate);
    value_ctx->parse_param(scli, value, redirect(cstate));
  }

  virtual void parse_param(scli& scli, int param_pos, std::string_view param_name, std::string_view value,
                           cmd_state* cstate)
  {
    if (param_pos == 0)
      emplace(cstate);
    value_ctx->parse_param(scli, param_pos, param_name, value, redirect(cstate));
  }
};

template <detail::OptionalLike ParamClass>
struct param_context_impl_ol : param_context
{
  using value_t            = std::remove_pointer_t<ParamClass>;
  param_context* value_ctx = nullptr;

  param_context_impl_ol() noexcept
  {
    value_ctx = param_context_impl<value_t>::get_instance();
  }

  void emplace(cmd_state* state)
  {
    reinterpret_cast<ParamClass*>(state)->emplace();
  }

  cmd_state* redirect(cmd_state* state)
  {
    return (cmd_state*)(&reinterpret_cast<ParamClass*>(state)->value());
  }

  virtual std::pair<param_context*, cmd_state*> enter_param_context(scli& scli, int param_pos,
                                                                    std::string_view param_name, cmd_state* cstate)
  {
    if (param_pos == 0)
      emplace(cstate);
    return value_ctx->enter_param_context(scli, param_pos, param_name, redirect(cstate));
  }

  virtual void exit_param_context(scli& scli, int param_pos, cmd_state* cstate_inner, cmd_state* cstate_cur)
  {
    return value_ctx->exit_param_context(scli, param_pos, cstate_inner, redirect(cstate_cur));
  }

  virtual void parse_param(scli& scli, std::string_view value, cmd_state* cstate)
  {
    emplace(cstate);
    value_ctx->parse_param(scli, value, redirect(cstate));
  }

  virtual void parse_param(scli& scli, int param_pos, std::string_view param_name, std::string_view value,
                           cmd_state* cstate)
  {
    if (param_pos == 0)
      emplace(cstate);
    value_ctx->parse_param(scli, param_pos, param_name, value, redirect(cstate));
  }
};

template <detail::MonostateLike ParamClass>
struct param_context_impl_ml : param_context
{};

struct classic_param_data
{
  parameter_main               main;
  parameter_list*              current = nullptr;
  std::vector<parameter_list*> stack;
};

template <typename ParamClass>
auto build_map()
{}

template <typename Class>
inline param_context* param_context_impl<Class>::get_instance()
{
  // Ensure ordering with multiple matches
  if constexpr (detail::ClassicCommand<Class>)
  {
    return detail::classic_param_context::get_instance();
  }
  else if constexpr (detail::BoundClass<Class>)
  {
    static param_context_impl_bc<Class> context;
    return &context;
  }
  else if constexpr (detail::TupleLike<Class>)
  {
    static param_context_impl_tl<Class> context;
    return &context;
  }
  else if constexpr (detail::ContainerLike<Class>)
  {
    static param_context_impl_cl<Class> context;
    return &context;
  }
  else if constexpr (detail::VariantLike<Class>)
  {
    static param_context_impl_vl<Class> context;
    return &context;
  }
  else if constexpr (detail::ConstructedFromStringView<Class>)
  {
    static param_context_impl_cfsv<Class> context;
    return &context;
  }
  else if constexpr (detail::TransformFromString<Class>)
  {
    static param_context_impl_tfs<Class> context;
    return &context;
  }
  else if constexpr (detail::StringLike<Class>)
  {
    static param_context_impl_sl<Class> context;
    return &context;
  }
  else if constexpr (detail::BoolLike<Class>)
  {
    static param_context_impl_bl<Class> context;
    return &context;
  }
  else if constexpr (detail::IntegerLike<Class>)
  {
    static param_context_impl_il<Class> context;
    return &context;
  }
  else if constexpr (detail::FloatLike<Class>)
  {
    static param_context_impl_fl<Class> context;
    return &context;
  }
  else if constexpr (detail::PointerLike<Class>)
  {
    static param_context_impl_pl<Class> context;
    return &context;
  }
  else if constexpr (detail::OptionalLike<Class>)
  {
    static param_context_impl_ol<Class> context;
    return &context;
  }
  else if constexpr (detail::MonostateLike<Class>)
  {
    static param_context_impl_ml<Class> context;
    return &context;
  }
  else
  {
    []<bool flag = false>()
    {
      static_assert(flag, "This type is not parameterized");
    }
    ();
    return nullptr;
  }
}

struct cmd_group : cmd_context
{
  void add_sub_command(std::string_view name, std::shared_ptr<cmd_context> cmd) override
  {
    if (name == "*")
      default_executer = std::move(cmd);
    else

      sub_objects[name] = std::move(cmd);
  }
  inline cmd_context* get_sub_command(std::string_view name) override
  {
    if (name == "*")
      return default_executer.get();
    auto it = sub_objects.find(name);
    if (it != sub_objects.end())
      return it->second.get();
    return nullptr;
  }

  cmd_context* get_context(scli const&, std::string_view cmd_name) override
  {
    auto it = sub_objects.find(cmd_name);
    if (it != sub_objects.end())
      return it->second.get();
    return default_executer.get();
  }

  std::shared_ptr<cmd_context>                                       default_executer;
  std::unordered_map<std::string_view, std::shared_ptr<cmd_context>> sub_objects;
};

template <typename CmdClass>
struct classic_command
{
  using classic_command_type = CmdClass;
  classic_param_data data;

  classic_command() noexcept
  {
    data.current = &data.main;
  }

  inline bool execute(scli& scli)
  {
    if constexpr (detail::SimpleCommand<CmdClass>)
      return CmdClass().execute(scli, *data.current);
    return true;
  }

  inline bool enter(scli& scli)
  {
    if constexpr (detail::HasEntry<CmdClass>)
      return CmdClass().enter(scli);
    return true;
  }

  inline void exit(scli& scli)
  {
    if constexpr (detail::HasExit<CmdClass>)
      CmdClass().exit(scli);
  }
};

template <typename CmdClass, typename Base = cmd_context>
struct cmd_proxy : Base
{
  struct state
  {
    CmdClass                      data;
    stack_allocator::rewind_point rewind;
  };

  param_context* value_ctx = nullptr;

  cmd_proxy() noexcept
  {
    value_ctx = param_context_impl<CmdClass>::get_instance();
  }

  cmd_state* construct(scli& scli) override
  {
    return reinterpret_cast<cmd_state*>(scli.create_cmd_state<state>());
  }

  void destroy(scli& scli, cmd_state* cstate) override
  {
    return scli.destroy_cmd_state(reinterpret_cast<state*>(cstate));
  }

  bool execute(scli& scli, cmd_state* cstate) override
  {
    auto cs = reinterpret_cast<state*>(cstate);
    if constexpr (detail::AutoCommand<CmdClass>)
      return cs->data.execute(scli);
    return true;
  }

  bool enter(scli& scli, cmd_state* cstate) override
  {
    return false;
  }

  void exit(scli& scli, cmd_state* cstate) override {}

  std::pair<param_context*, cmd_state*> enter_param_context(scli& scli, int param_pos, std::string_view param_name,
                                                            cmd_state* cstate) override
  {
    return value_ctx->enter_param_context(scli, param_pos, param_name, cstate);
  }

  void exit_param_context(scli& scli, int param_pos, cmd_state* cstate_inner, cmd_state* cstate_cur) override
  {
    return value_ctx->exit_param_context(scli, param_pos, cstate_inner, cstate_cur);
  }

  void parse_param(scli& scli, std::string_view value, cmd_state* cstate) override
  {
    return value_ctx->parse_param(scli, value, cstate);
  }

  void parse_param(scli& scli, int param_pos, std::string_view param_name, std::string_view value,
                   cmd_state* cstate) override
  {
    return value_ctx->parse_param(scli, param_pos, param_name, value, cstate);
  }
};

template <typename CmdClass>
struct cmd_group_proxy : cmd_proxy<CmdClass, cmd_group>
{
  using typename cmd_proxy<CmdClass, cmd_group>::state;
  bool enter(scli& scli, cmd_state* cstate) override
  {
    auto cs = reinterpret_cast<state*>(cstate);
    if constexpr (detail::HasEntry<CmdClass>)
      return cs->data.enter(scli);
    return true;
  }

  void exit(scli& scli, cmd_state* cstate) override
  {
    auto cs = reinterpret_cast<state*>(cstate);
    if constexpr (detail::HasExit<CmdClass>)
      cs->data.exit(scli);
  }
};

template <typename T, typename Base = cmd_group>
struct reg_proxy;

template <detail::CodeRegionHandler RegClass, typename Base>
struct reg_proxy<RegClass, Base> : Base
{
  void enter_region(scli& s, std::string_view id, std::string_view name) override
  {
    RegClass::enter(s, id, name);
  }

  inline virtual bool is_text_context() const noexcept override
  {
    return false;
  }
};

template <detail::TextRegionHandler RegClass, typename Base>
struct reg_proxy<RegClass, Base> : Base
{
  using txt_region_t = void;
  void enter_region(scli& s, std::string_view id, std::string_view name, text_content&& content) override
  {
    RegClass::enter(s, id, name, std::move(content));
  }

  inline virtual bool is_text_context() const noexcept override
  {
    return true;
  }
};

template <string_literal Name, typename Type>
struct command
{
  using command_type = std::conditional_t<detail::SimpleCommand<Type>, classic_command<Type>, Type>;
  inline static constexpr std::string_view name() noexcept
  {
    return (std::string_view)Name;
  }
  constexpr command() noexcept = default;
};

template <typename C>
concept RegionType = requires(C c) {
  typename C::region_handler_type;
  {
    c.name()
  } -> std::same_as<std::string_view>;
};

template <typename C>
concept CommandOrRegionType = CommandType<C> || RegionType<C>;

template <typename C>
concept AliasType = requires(C c) {
  typename C::is_alias;
  {
    c.name()
  } -> std::same_as<std::string_view>;
  {
    c.source()
  } -> std::same_as<std::string_view>;
};

template <string_literal Name, RegionHandler Type>
struct region
{
  using region_handler_type = Type;
  inline static constexpr std::string_view name() noexcept
  {
    return (std::string_view)Name;
  }
  constexpr region() noexcept = default;
};

template <string_literal Name, string_literal Source>
struct alias
{
  using is_alias = void;
  inline static constexpr std::string_view name() noexcept
  {
    return (std::string_view)Name;
  }
  inline static constexpr std::string_view source() noexcept
  {
    return (std::string_view)Source;
  }

  constexpr alias() noexcept = default;
};

} // namespace detail

template <string_literal Name, typename CmdType>
constexpr auto cmd = detail::command<Name, CmdType>();

template <string_literal Name, typename RegType>
constexpr auto reg = detail::region<Name, RegType>();

template <string_literal Name, string_literal Source>
constexpr auto alias = detail::alias<Name, Source>();

class scli::builder
{
public:
  scli::builder() noexcept
  {
    stack.push_back(&region_map);
    current_ctx = &region_map;
  }

  template <detail::AliasType C>
  scli::builder& operator-(C other) noexcept
  {
    std::string_view src   = other.source();
    cmd_context*     start = &region_map;
    while (!src.empty() && start)
    {
      auto id  = src;
      auto pos = src.find_first_of('.');
      if (pos != src.npos)
      {
        id  = src.substr(0, pos);
        src = src.substr(pos + 1);
      }
      else
        src = {};

      start = start->get_sub_command(id);
    }
    if (start && current_ctx)
      current_ctx->add_sub_command(other.name(), std::static_pointer_cast<cmd_context>(start->shared_from_this()));
    return *this;
  }

  template <detail::CommandType C>
  scli::builder& operator-(C other) noexcept
  {
    current_ctx->add_sub_command(other.name(), std::make_shared<detail::cmd_proxy<typename C::command_type>>());
    return *this;
  }

  template <detail::CommandOrRegionType C>
  scli::builder& operator+(C other) noexcept
  {
    std::shared_ptr<cmd_context> proxy;
    if constexpr (detail::CommandType<C>)
    {
      proxy = std::make_shared<detail::cmd_group_proxy<typename C::command_type>>();
    }
    else
    {
      proxy = std::make_shared<detail::reg_proxy<typename C::region_handler_type, detail::cmd_group>>();
    }
    auto cmd = proxy.get();
    current_ctx->add_sub_command(other.name(), std::move(proxy));
    stack.emplace_back(current_ctx);
    current_ctx = cmd;
    return *this;
  }

  inline scli::builder& operator-(acl::endl_type) noexcept
  {
    current_ctx = stack.back();
    stack.pop_back();
    return *this;
  }

  std::shared_ptr<scli::context> build();

private:
  cmd_context*              current_ctx;
  std::vector<cmd_context*> stack;
  detail::cmd_group         region_map;
};

using scli_source = scli::location;

} // namespace acl
