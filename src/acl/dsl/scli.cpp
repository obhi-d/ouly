
#include <acl/dsl/scli.hpp>
#include <cassert>
#include <fstream>

namespace acl
{
class scli::context : public detail::cmd_group
{
public:
  friend class scli;
private:
};

struct classic_param_context_impl : param_context
{
  using param_data = detail::classic_param_data;
  std::pair<param_context*, cmd_state*> enter_param_context(scli& scli_inst, int, std::string_view name,
                                                            cmd_state* cstate) final
  {
    auto data = reinterpret_cast<param_data*>(cstate);
    if (!data)
    {
      scli_inst.error(scli_inst.source, "Missing param context data", "param");
      return {this, cstate};
    }
    data->stack.push_back(data->current);
    auto current = data->current;
    if (!current)
    {
      scli_inst.error(scli_inst.source, "Null scope", "param");
      return {this, cstate};
    }
    auto newl     = std::make_unique<parameter_list>(name);
    data->current = newl.get();
    current->add(std::move(newl));
    return {this, cstate};
  }

  void exit_param_context(scli& scli_inst, int, cmd_state*, cmd_state* cstate) final
  {
    auto data = reinterpret_cast<param_data*>(cstate);
    if (data->stack.empty())
    {
      scli_inst.error(scli_inst.source, "Invalid param context pop", "param");
      return;
    }
    data->current = data->stack.back();
    data->stack.pop_back();
  }

  void parse_param(scli&, std::string_view value, cmd_state* cstate) final {}

  void parse_param(scli& scli_inst, int param_pos, std::string_view param_name, std::string_view value,
                   cmd_state* cstate) final
  {
    auto data = reinterpret_cast<param_data*>(cstate);
    data->current->add(std::make_unique<parameter_value>(param_name, value));
  }
};

param_context* detail::classic_param_context::get_instance()
{
  static classic_param_context_impl impl;
  return &impl;
}

std::shared_ptr<scli::context> scli::builder::build()
{
  auto r         = std::make_shared<scli::context>();
  r->sub_objects = std::move(region_map);
  return r;
}

bool scli::is_code_region(std::string_view reg) const noexcept
{
  auto ctx = sstate.ctx.get_context(*this, reg);
  return ctx && !ctx->is_text_context();
}

void scli::set_next_command(std::string_view name) noexcept
{
  parameter   = {};
  param_pos   = 0;
  current_cmd = nullptr;
  if (current_cmd_ctx)
  {
    current_cmd = current_cmd_ctx->get_context(*this, name);
    param_ctx   = current_cmd;
    command     = name;
  }

  if (!current_cmd)
  {
    error(source, "Command not found.", name);
    return;
  }

  current_cmd_state = current_cmd->construct(*this);
}

void scli::execute_command()
{
  if (current_cmd && !skip_depth)
  {
    if (!current_cmd->execute(*this, current_cmd_state))
      error(source, "Command execution failed.", command);
  }
}

void scli::enter_command_scope()
{
  if (current_cmd)
  {
    if (!skip_depth)
    {
      bool entered = current_cmd->enter(*this, current_cmd_state);

      cmd_ctx_stack.emplace_back(current_cmd_ctx, current_cmd_state);

      current_cmd_ctx   = current_cmd;
      current_cmd       = nullptr;
      current_cmd_state = nullptr;

      if (!entered)
        skip_depth++;
    }
    else
      skip_depth++;
  }
}

void scli::exit_command_scope()
{
  if (current_cmd)
  {
    if (skip_depth)
      --skip_depth;
    if (!skip_depth)
    {
      if (cmd_ctx_stack.empty())
      {
        error(source, "Unexpected command end }", command);
      }
      auto old_ctx                                 = current_cmd_ctx;
      std::tie(current_cmd_ctx, current_cmd_state) = cmd_ctx_stack.back();
      cmd_ctx_stack.pop_back();
      old_ctx->exit(*this, current_cmd_state);
    }
  }
}

void scli::set_next_param_name(std::string_view param) noexcept
{
  parameter = param;
}

void scli::set_param(std::string_view value)
{
  if (param_ctx)
  {
    param_ctx->parse_param(*this, param_pos, parameter, value, current_cmd_state);
    param_pos++;
    parameter = {};
  }
}

void scli::set_param(text_content&& tc)
{
  set_param(std::holds_alternative<std::string_view>(tc) ? std::get<std::string_view>(tc) : std::get<std::string>(tc));
}

void scli::enter_param_scope()
{
  if (param_ctx)
  {
    auto [ctx, pstate] = param_ctx->enter_param_context(*this, param_pos, parameter, current_cmd_state);
    if (!ctx)
    {
      std::string pos = std::to_string(param_pos) + "@" + std::string(parameter);
      error(source, "Parameter cannot be a list.", parameter);
      param_ctx = parent_param_ctx = nullptr;
    }
    else
    {
      param_ctx_stack.emplace_back(parent_param_ctx, current_cmd_state, param_pos);
      current_cmd_state = pstate;
      parent_param_ctx  = param_ctx;
      param_ctx         = ctx;
      param_pos         = 0;
    }
  }
}

void scli::exit_param_scope()
{
  if (param_ctx_stack.empty())
  {
    error(source, "Invalid parameter stack pop.", parameter);
    return;
  }

  auto save                                                = current_cmd_state;
  param_ctx                                                = parent_param_ctx;
  std::tie(parent_param_ctx, current_cmd_state, param_pos) = param_ctx_stack.back();
  param_ctx_stack.pop_back();
  if (param_ctx)
    param_ctx->exit_param_context(*this, param_pos, save, current_cmd_state);
  param_pos++;
}

void scli::enter_region(std::string_view reg)
{
  region_id       = reg;
  current_cmd_ctx = sstate.ctx.get_context(*this, reg);
  if (current_cmd_ctx)
    current_cmd_ctx->enter_region(*this, reg);
}

void scli::enter_text_region(std::string_view name, text_content&& content)
{
  current_cmd_ctx = sstate.ctx.get_context(*this, name);
  if (current_cmd_ctx)
    current_cmd_ctx->enter_region(*this, name, std::move(content));
}

void scli::import_script(text_content&& tc)
{
  scli import_ctx(sstate);
  auto src_name =
    std::holds_alternative<std::string_view>(tc) ? std::get<std::string_view>(tc) : std::get<std::string>(tc);
  auto content = sstate.import_handler(std::holds_alternative<std::string_view>(tc) ? std::get<std::string_view>(tc)
                                                                                    : std::get<std::string>(tc));
  import_ctx.parse(src_name, content);
}

void scli::put(int32_t len) noexcept
{
  len_reading += len;
  assert(contents[pos_commit + len_reading - 1] != 0);
}

void scli::skip_len(std::int32_t len) noexcept
{
  assert(len_reading == 0);
  pos_commit += len;
}

std::string_view scli::trim(std::string_view str, std::string_view whitespace)
{
  const auto str_begin = str.find_first_not_of(whitespace);
  if (str_begin == std::string::npos)
    return str;

  const auto str_end   = str.find_last_not_of(whitespace);
  const auto str_range = str_end - str_begin + 1;

  return str.substr(str_begin, str_range);
}

std::string_view scli::make_token() noexcept
{
  auto m = contents.substr(pos_commit, len_reading);
  pos_commit += len_reading;
  len_reading = 0;
  return m;
}

void scli::escape_sequence(std::string_view ss) noexcept
{
  token += ss;
}

text_content scli::make_text() noexcept
{
  text_content tc = std::string_view();
  if (token.empty())
    tc = make_token();
  else
  {
    token += make_token();
    tc = token;
    token.clear();
  }
  return tc;
}

// region id
void scli::set_current_reg_id(std::string_view name) noexcept
{
  region_id = name;
}

std::string_view scli::get() const noexcept
{
  return contents.substr(pos_commit, len_reading);
}

int scli::read(char* data, int size) noexcept
{
  auto min = std::min<std::int32_t>(static_cast<std::int32_t>(contents.size() - pos), size);
  if (min)
  {
    std::memcpy(data, contents.data() + pos, static_cast<std::size_t>(min));
    pos += min;
    if (min < size)
      data[min] = 0;

    assert(min <= size);
    return min;
  }
  data[0] = 0;
  return 0;
}

void scli::destroy_comamnd_state()
{
  if (current_cmd)
  {
    current_cmd->destroy(*this, current_cmd_state);
    current_cmd_state = nullptr;
  }
}

std::string_view scli::default_import_handler(shared_state& sstate, std::string_view file) noexcept
{
  auto it = sstate.imports.find(file);
  if (it != sstate.imports.end())
  {
    return std::string_view(it->second);
  }

  std::ifstream t;
  uint32_t      include = 0;
  std::string   name    = std::string(file);

  for (auto& ip : sstate.include_paths)
  {
    t = std::ifstream(ip + name);
    if (t.is_open())
      break;
  }

  t.seekg(0, std::ios::end);
  size_t      size = t.tellg();
  std::string buffer(size, '\0');
  t.seekg(0);
  t.read(&buffer[0], size);

  auto r = sstate.imports.emplace(file, std::move(buffer));
  return r.first->second;
}

} // namespace acl