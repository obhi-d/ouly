
#include <acl/dsl/lite_yml.hpp>

namespace acl::yml
{

// Example YAML:
//
// # Simple key-value pairs
// name: John
// age: 30
//
// # Nested objects
// person:
//   name: Jane
//   address:
//     street: 123 Main St
//     city: Anytown
//
// # Arrays
// colors:
//   - red
//   - blue
//   - green
//
// # Array of objects
// users:
//   - name: Alice
//     role: admin
//   - name: Bob
//     role: user
//
// # Compact arrays
// numbers: [1, 2, 3, 4]
//
// # Block scalars
// description: |
//   This is a multi-line
//   description that preserves
//   line breaks
//
// comment: >
//   This is a multi-line
//   comment that folds
//   into a single line
//
void lite_stream::parse()
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

auto lite_stream::next_token() -> lite_stream::token
{

  // Start of line - check indentation

  if (at_line_start_)
  {
    at_line_start_   = false;
    can_be_sequence_ = true;
    auto indent      = count_indent();
    if (peek(0) == '\n')
    {
      current_pos_++;
      return token{.type_ = token_type::newline, .content_ = indent};
    }
    if (peek(0) == '-')
    {
      current_pos_++;
      auto after_dash_indent = count_indent();
      return token{
       .type_    = token_type::dash,
       .content_ = {.start_ = indent.start_, .count_ = indent.count_ + 1 + after_dash_indent.count_}
      };
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
          .type_ = token_type::dash, .content_ = {.start_ = current_pos_ - 1, .count_ = 1 + indent.count_}
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
    if (((c == ',' || (std::isspace(c) != 0)) && is_scope_of_type(container_type::compact_array)) || c == '\n')
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

void lite_stream::process_token(token tok)
{
  if (!tok)
  {
    return;
  }

  switch (tok.type_)
  {
  case token_type::lbracket:
    handle_dash(indent_level_, true);
    break;

  case token_type::comma:
    if (!is_scope_of_type(container_type::compact_array))
    {
      throw_error(tok, "Unexpected ','");
    }
    ctx_->begin_new_array_item();
    break;

  case token_type::rbracket:
    if (!is_scope_of_type(container_type::compact_array))
    {
      throw_error(tok, "Unexpected ']'");
    }
    close_last_context();
    break;

  case token_type::indent:
    handle_indent(static_cast<uint16_t>(tok.content_.count_));
    break;

  case token_type::key:
    if (is_scope_of_type(container_type::compact_array))
    {
      throw_error(tok, "Unexpected key, ']' expected");
    }
    handle_key(tok.content_);
    break;

  case token_type::value:
    handle_value(tok.content_);
    break;

  case token_type::dash:
    handle_dash(static_cast<uint16_t>(tok.content_.count_), false);
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

void lite_stream::handle_indent(uint16_t new_indent)
{
  if (new_indent < indent_level_)
  {
    close_context(new_indent);
  }
  else if (new_indent > indent_level_)
  {
    state_ = parse_state::in_new_context;
  }

  indent_level_ = new_indent;
}

void lite_stream::handle_key(string_slice key)
{
  if (state_ == parse_state::in_new_context)
  {
    ctx_->begin_object();
    indent_stack_.emplace_back(indent_level_, container_type::object);
  }
  ctx_->set_key(get_view(key));
  state_ = parse_state::in_key;
}

void lite_stream::handle_value(string_slice value)
{
  if (value.count_ == 0U)
  {
    return;
  }

  ctx_->set_value(get_view(value));
  state_ = parse_state::none;
}

//
//  arr:
//    - key: x
//         - g: a
//    - key: a
//         - g: a

void lite_stream::handle_dash(uint16_t new_indent, bool compact)
{
  handle_indent(new_indent);
  if (state_ == parse_state::in_new_context || compact)
  {
    ctx_->begin_array();
    indent_stack_.emplace_back(indent_level_, compact ? container_type::compact_array : container_type::array);
  }
  if (is_scope_of_type(container_type::object, indent_level_))
  {
    ctx_->end_object();
  }
  ctx_->begin_new_array_item();
  state_ = parse_state::in_new_context;
}

void lite_stream::handle_block_scalar(token_type type)
{
  state_       = parse_state::in_block_scalar;
  block_style_ = type;
  block_lines_.clear();
}

void lite_stream::collect_block_scalar()
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
    block_lines_.clear();
    state_ = parse_state::none;
  }
}

void lite_stream::close_context(uint16_t new_indent)
{
  while (!indent_stack_.empty() && indent_stack_.back().indent_ >= new_indent)
  {
    auto current = indent_stack_.back();
    if (current.type_ == container_type::array || current.type_ == container_type::compact_array)
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

void lite_stream::close_last_context()
{
  if (!indent_stack_.empty())
  {
    auto current = indent_stack_.back();
    if (current.type_ == container_type::array || current.type_ == container_type::compact_array)
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

} // namespace acl::yml
