
#include <acl/dsl/yaml.hpp>

namespace acl::yaml
{
void istream::parse(context& ctx)
{
  ctx_          = &ctx;
  state_        = parse_state::none;
  indent_level_ = 0;
  current_pos_  = 0;

  while (auto token = next_token())
  {
    process_token(token);
  }

  // Close any open structures
  while (!indent_stack_.empty())
  {
    close_context(-1);
  }
}

std::optional<istream::token> istream::next_token()
{
  skip_whitespace();
  if (current_pos_ >= content_.length())
    return std::nullopt;

  // Start of line - check indentation
  if (at_line_start_)
  {
    at_line_start_ = false;
    auto indent    = count_indent();
    if (indent.count > 0)
      return token{token_type::indent, indent};
  }

  char c = content_[current_pos_];
  switch (c)
  {
  case '-':
  {
    if (peek(1) == ' ')
    {
      current_pos_ += 2;
      return token{
        token_type::dash, {current_pos_ - 2, 1}
      };
    }
    break;
  }
  case '|':
  {
    current_pos_++;
    return token{
      token_type::pipe, {current_pos_ - 1, 1}
    };
  }
  case '>':
  {
    current_pos_++;
    return token{
      token_type::gt, {current_pos_ - 1, 1}
    };
  }
  case '\n':
  {
    current_pos_++;
    at_line_start_ = true;
    return token{
      token_type::newline, {current_pos_ - 1, 1}
    };
  }
  }

  // Handle key or value
  auto start = current_pos_;
  while (current_pos_ < content_.length())
  {
    c = content_[current_pos_];
    if (c == ':' && peek(1) == ' ')
    {
      auto slice = string_slice{start, static_cast<uint32_t>(current_pos_ - start)};
      current_pos_ += 2;
      return token{token_type::key, slice};
    }
    if (c == '\n')
      break;
    current_pos_++;
  }

  // If we got here, it's a value
  return token{
    token_type::value, string_slice{start, static_cast<uint32_t>(current_pos_ - start)}
  };
}

void istream::process_token(const std::optional<token>& tok)
{
  if (!tok)
    return;

  switch (tok->type)
  {
  case token_type::indent:
    handle_indent(static_cast<int>(tok->content.count));
    break;

  case token_type::key:
    handle_key(tok->content);
    break;

  case token_type::value:
    handle_value(tok->content);
    break;

  case token_type::dash:
    handle_dash();
    break;

  case token_type::pipe:
  case token_type::gt:
    handle_block_scalar(tok->type);
    break;

  case token_type::newline:
    if (state_ == parse_state::in_block_scalar)
    {
      collect_block_scalar();
    }
    break;
  }
}

void istream::handle_indent(int32_t new_indent)
{
  if (new_indent < indent_level_)
  {
    close_context(new_indent);
  }
  indent_level_ = new_indent;
}

void istream::handle_key(string_slice key)
{
  handle_indent(indent_level_);

  if (state_ != parse_state::in_key)
  {
    ctx_->begin_object();
    indent_stack_.push_back({indent_level_, container_type::object});
  }

  ctx_->set_key(get_view(key));
  state_ = parse_state::in_value;
}

void istream::handle_value(string_slice value)
{
  if (!value.count)
    return;
  ctx_->set_value(get_view(value));
  state_ = parse_state::none;
}

void istream::handle_dash()
{
  if (state_ != parse_state::in_array)
  {
    ctx_->begin_array();
    indent_stack_.push_back({indent_level_, container_type::array});
    state_ = parse_state::in_array;
  }
}

void istream::handle_block_scalar(token_type type)
{
  state_       = parse_state::in_block_scalar;
  block_style_ = type;
  block_lines_.clear();
}

void istream::collect_block_scalar()
{
  auto line = get_current_line();
  if (line.count && line.count > indent_level_)
  {
    block_lines_.push_back(line);
  }
  else
  {
    // End of block scalar
    std::string result;
    for (uint32_t i = 0; i < block_lines_.size(); ++i)
    {
      if (i > 0)
      {
        result += (block_style_ == token_type::pipe) ? '\n' : ' ';
      }
      result += get_view(block_lines_[i]);
    }
    ctx_->set_value(result);
    block_lines_.clear();
    state_ = parse_state::none;
  }
}

void istream::close_context(int32_t new_indent)
{
  while (!indent_stack_.empty() && indent_stack_.back().indent > new_indent)
  {
    auto& current = indent_stack_.back();
    if (current.type == container_type::array)
    {
      ctx_->end_array();
    }
    else
    {
      ctx_->end_object();
    }
    indent_stack_.pop_back();
  }
}
} // namespace acl::yaml
