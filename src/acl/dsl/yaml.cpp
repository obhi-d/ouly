
#include <acl/dsl/yaml.hpp>

namespace acl::yaml
{
void istream::parse()
{
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
    close_context(0);
  }
}

auto istream::next_token() -> istream::token
{

  // Start of line - check indentation

  if (at_line_start_)
  {
    at_line_start_   = false;
    can_be_sequence_ = true;
    auto indent      = count_indent();
    if (peek(1) == '\n')
    {
      return token{.type_ = token_type::newline, .content_ = indent};
    }
    return token{.type_ = token_type::indent, .content_ = indent};
  }

  skip_whitespace();

  if (current_pos_ >= content_.length())
  {
    return token{
     .type_ = token_type::eof, .content_ = {.start_ = current_pos_, .count_ = 0}
    };
  }

  char c = content_[current_pos_];
  switch (c)
  {
  case '-':
    if (can_be_sequence_ && (std::isspace(peek(1)) != 0))
    {
      current_pos_++;
      auto indent = count_indent();
      auto tok    = token{
          .type_ = token_type::dash, .content_ = {.start_ = current_pos_, .count_ = 1 + indent.count_}
      };
      return tok;
    }
    break;
  case '|':
  {
    current_pos_++;
    return token{
     .type_ = token_type::pipe, .content_ = {.start_ = current_pos_ - 1, .count_ = 1}
    };
  }
  case '>':
  {
    current_pos_++;
    return token{
     .type_ = token_type::gt, .content_ = {.start_ = current_pos_ - 1, .count_ = 1}
    };
  }
  case '[':
  {
    current_pos_++;
    return token{
     .type_ = token_type::lbracket, .content_ = {.start_ = current_pos_ - 1, .count_ = 1}
    };
  }
  case ']':
  {
    current_pos_++;
    return token{
     .type_ = token_type::rbracket, .content_ = {.start_ = current_pos_ - 1, .count_ = 1}
    };
  }
  case ',':
  {
    current_pos_++;
    return token{
     .type_ = token_type::comma, .content_ = {.start_ = current_pos_ - 1, .count_ = 1}
    };
  }
  case '\n':
  {
    current_pos_++;
    at_line_start_ = true;
    return token{
     .type_ = token_type::newline, .content_ = {.start_ = current_pos_ - 1, .count_ = 1}
    };
  }
  case '"':
  {
    // Quoted string
    auto start = current_pos_;
    current_pos_++;
    while (current_pos_ < content_.length())
    {
      c = content_[current_pos_];
      if (c == '"')
      {
        current_pos_++;
        break;
      }
      current_pos_++;
    }
    return token{
     .type_    = token_type::value,
     .content_ = {.start_ = start + 1, .count_ = static_cast<uint32_t>(current_pos_ - start - 2)}
    };
  }
  default:
    break;
  }

  can_be_sequence_ = false;
  // Handle key or value
  auto start = current_pos_;
  while (current_pos_ < content_.length())
  {
    c = content_[current_pos_];
    if (c == ':' && (std::isspace(peek(1)) != 0))
    {
      auto slice = string_slice{.start_ = start, .count_ = (current_pos_ - start)};
      current_pos_++;
      return token{.type_ = token_type::key, .content_ = slice};
    }
    if (((c == ',' || (std::isspace(c) != 0)) && state_ == parse_state::in_compact_mapping) || c == '\n')
    {
      break;
    }
    current_pos_++;
  }

  // If we got here, it's a value
  return token{
   .type_ = token_type::value, .content_ = string_slice{.start_ = start, .count_ = (current_pos_ - start)}
  };
}

void istream::process_token(token tok)
{
  if (!tok)
  {
    return;
  }

  switch (tok.type_)
  {
  case token_type::lbracket:
    if (state_ == parse_state::none || state_ == parse_state::in_value)
    {
      state_ = parse_state::in_compact_mapping;
    }
    break;

  case token_type::comma:
    if (state_ != parse_state::in_compact_mapping)
    {
      throw_error(tok, "Unexpected ','");
    }
    break;

  case token_type::rbracket:
    if (state_ != parse_state::in_compact_mapping)
    {
      throw_error(tok, "Unexpected ']'");
    }
    state_ = parse_state::none;
    break;

  case token_type::indent:
    handle_indent(static_cast<uint16_t>(tok.content_.count_));
    break;

  case token_type::key:
    if (state_ == parse_state::in_compact_mapping)
    {
      throw_error(tok, "Unexpected key, ']' expected");
    }
    handle_key(tok.content_);
    break;

  case token_type::value:
    handle_value(tok.content_);
    break;

  case token_type::dash:
    handle_dash(static_cast<uint16_t>(tok.content_.count_));
    break;

  case token_type::pipe:
  case token_type::gt:
    handle_block_scalar(tok.type_);
    break;

  case token_type::newline:
    if (state_ == parse_state::in_block_scalar)
    {
      collect_block_scalar();
    }
    break;
  default:
    break;
  }
}

void istream::handle_indent(uint16_t new_indent)
{
  if (new_indent < indent_level_)
  {
    close_context(new_indent);
  }
  indent_level_ = new_indent;
}

void istream::handle_key(string_slice key)
{
  ctx_->begin_key(get_view(key));
  indent_stack_.emplace_back(indent_level_, container_type::object);
  state_ = parse_state::in_value;
}

void istream::handle_value(string_slice value)
{
  if (value.count_ == 0U)
  {
    return;
  }

  if (state_ == parse_state::in_compact_mapping)
  {
    ctx_->begin_array();
    indent_stack_.emplace_back(indent_level_, container_type::array);
  }

  ctx_->set_value(get_view(value));
  close_last_context();

  if (state_ != parse_state::in_compact_mapping)
  {
    state_ = parse_state::none;
  }
}

//
//  arr:
//    - key: x
//         - g: a
//    - key: a
//         - g: a

void istream::handle_dash(uint16_t extra_indent)
{
  indent_level_ += extra_indent;
  ctx_->begin_array();
  indent_stack_.emplace_back(indent_level_, container_type::array);
  state_ = parse_state::in_array;
}

void istream::handle_block_scalar(token_type type)
{
  state_       = parse_state::in_block_scalar;
  block_style_ = type;
  block_lines_.clear();
}

void istream::collect_block_scalar()
{
  auto indent = count_indent();
  auto line   = get_current_line();
  if ((line.count_ != 0U) && line.count_ > indent.count_)
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
    close_last_context();
    block_lines_.clear();
    state_ = parse_state::none;
  }
}

void istream::close_context(uint16_t new_indent)
{
  while (!indent_stack_.empty() && indent_stack_.back().indent_ >= new_indent)
  {
    auto current = indent_stack_.back();
    if (current.type_ == container_type::array)
    {
      ctx_->end_array();
    }
    else
    {
      ctx_->end_key();
    }
    indent_stack_.pop_back();
  }
}

void istream::close_last_context()
{
  if (!indent_stack_.empty())
  {
    auto current = indent_stack_.back();
    if (current.type_ == container_type::array)
    {
      ctx_->end_array();
    }
    else
    {
      ctx_->end_key();
    }
    indent_stack_.pop_back();
  }
}

} // namespace acl::yaml
