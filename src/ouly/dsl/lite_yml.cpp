// SPDX-License-Identifier: MIT

#include "ouly/dsl/lite_yml.hpp"
#include "ouly/utility/user_config.hpp"

namespace ouly::yml
{
namespace
{
auto is_space(char value) -> bool
{
  return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}
} // namespace

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
  state_           = parse_state::none;
  indent_level_    = 0;
  current_pos_     = 0;
  line_start_      = 0;
  at_line_start_   = true;
  can_be_sequence_ = false;
  value_finished_  = false;
  indent_stack_.clear();
  block_lines_.clear();

  while (auto nxt_tok = next_token())
  {
    process_token(nxt_tok);
  }

  if (state_ == parse_state::in_block_scalar)
  {
    collect_block_scalar();
  }

  for (auto const& entry : indent_stack_)
  {
    if (entry.type_ == container_type::compact_array)
    {
      throw_error({.content_ = {.start_ = current_pos_}}, "Unterminated array, ']' expected");
    }
  }

  // Close any open structures
  while (!indent_stack_.empty())
  {
    close_context(0);
  }
}

auto lite_stream::next_line_start_token() -> lite_stream::token
{
  line_start_      = current_pos_;
  can_be_sequence_ = !is_scope_of_type(container_type::compact_array);
  auto indent      = count_indent();
  if (peek(0) == '\r' || peek(0) == '\n')
  {
    if (peek(0) == '\r')
    {
      current_pos_++;
    }
    if (peek(0) == '\n')
    {
      current_pos_++;
    }
    return token{.type_ = token_type::newline, .content_ = indent};
  }

  // Full-line comment: '#' (optionally after indentation) runs to end of line.
  // Treat it like a blank line so it does not affect the document structure.
  if (peek(0) == '#')
  {
    skip_to_line_end();
    return token{.type_ = token_type::newline, .content_ = indent};
  }

  at_line_start_ = false;
  if (can_be_sequence_ && peek(0) == '-' && (is_space(peek(1)) || peek(1) == '\0'))
  {
    current_pos_++;
    count_indent();
    return token{
     .type_ = token_type::dash, .content_ = {.start_ = indent.start_, .count_ = indent.count_ + 1}
    };
  }
  return token{.type_ = token_type::indent, .content_ = indent};
}

auto lite_stream::next_token() -> lite_stream::token
{
  // Start of line - check indentation
  if (at_line_start_)
  {
    return next_line_start_token();
  }

  skip_whitespace();

  if (current_pos_ >= content_.length())
  {
    return token{
     .type_ = token_type::eof, .content_ = {.start_ = current_pos_, .count_ = 0}
    };
  }

  char c = ouly::detail::vector_access(content_, current_pos_);
  if (value_finished_ && c != ',' && c != ']' && c != '\n' && c != '#')
  {
    throw_error(
     {
      .content_ = {.start_ = current_pos_, .count_ = 1}
    },
     "Unexpected content after value");
  }
  switch (c)
  {
  case '-':
    if (can_be_sequence_ && (is_space(peek(1)) || peek(1) == '\0'))
    {
      auto start = current_pos_++;
      count_indent();
      auto tok = token{
       .type_ = token_type::dash, .content_ = {.start_ = start, .count_ = start - line_start_ + 1}
      };
      return tok;
    }
    break;
  case '#':
    skip_to_line_end();
    at_line_start_ = true;
    return {.type_ = token_type::newline, .content_ = {}};
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
  case '\0':
  {
    auto pos     = current_pos_;
    current_pos_ = static_cast<uint32_t>(content_.length());
    return token{
     .type_ = token_type::eof, .content_ = {.start_ = pos, .count_ = 0}
    };
  }
  case '"':
  case '\'':
    return quoted_token();
  default:
    break;
  }

  can_be_sequence_ = false;
  // Handle key or value
  auto start = current_pos_;
  while (current_pos_ < content_.length())
  {
    c = ouly::detail::vector_access(content_, current_pos_);
    if (c == ':' && (is_space(peek(1)) || peek(1) == '\0'))
    {
      auto end = current_pos_;
      while (end > start && is_space(ouly::detail::vector_access(content_, end - 1)))
      {
        --end;
      }
      auto slice = string_slice{.start_ = start, .count_ = end - start};
      current_pos_++;
      return token{.type_ = token_type::key, .content_ = slice};
    }
    if (((c == ',' || c == ']') && is_scope_of_type(container_type::compact_array)) || c == '\n' || c == '\r' ||
        (c == '#' && (current_pos_ == start || is_space(ouly::detail::vector_access(content_, current_pos_ - 1)))))
    {
      break;
    }
    current_pos_++;
  }

  // Whitespace separating a scalar from a comment or line ending is not part of its value.
  auto end = current_pos_;
  while (end > start && is_space(ouly::detail::vector_access(content_, end - 1)))
  {
    --end;
  }
  return token{
   .type_ = token_type::value, .content_ = string_slice{.start_ = start, .count_ = end - start}
  };
}

auto lite_stream::quoted_token() -> lite_stream::token
{
  auto const start = current_pos_++;
  auto const quote = ouly::detail::vector_access(content_, start);
  decoded_.clear();
  bool escaped = false;
  while (current_pos_ < content_.size())
  {
    auto c = ouly::detail::vector_access(content_, current_pos_++);
    if (c == quote)
    {
      if (quote == '\'' && peek(0) == '\'')
      {
        ++current_pos_;
        decoded_ += '\'';
        escaped = true;
        continue;
      }
      token result{
       .type_    = token_type::value,
       .content_ = {.start_ = start + 1, .count_ = current_pos_ - start - 2},
       .decoded_ = escaped
      };
      skip_whitespace();
      if (peek(0) == ':' && (is_space(peek(1)) || peek(1) == '\0'))
      {
        ++current_pos_;
        result.type_ = token_type::key;
      }
      can_be_sequence_ = false;
      return result;
    }
    if (c == '\n' || c == '\r')
    {
      throw_error(
       {
        .content_ = {.start_ = start, .count_ = current_pos_ - start}
      },
       "Multiline quoted scalars are not supported");
    }
    if (quote == '"' && c == '\\')
    {
      escaped = true;
      c       = peek(0);
      if (current_pos_ < content_.size())
      {
        ++current_pos_;
      }
      switch (c)
      {
      case '0':
        c = '\0';
        break;
      case 'a':
        c = '\a';
        break;
      case 'b':
        c = '\b';
        break;
      case 't':
        c = '\t';
        break;
      case 'n':
        c = '\n';
        break;
      case 'v':
        c = '\v';
        break;
      case 'f':
        c = '\f';
        break;
      case 'r':
        c = '\r';
        break;
      case 'e':
        c = '\x1b';
        break;
      case ' ':
      case '/':
      case '\\':
      case '"':
        break;
      default:
        throw_error(
         {
          .content_ = {.start_ = start, .count_ = current_pos_ - start}
        },
         "Unsupported quoted scalar escape");
      }
    }
    decoded_ += c;
  }
  throw_error(
   {
    .content_ = {.start_ = start, .count_ = current_pos_ - start}
  },
   "Unterminated quoted scalar");
  return {};
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
    if (!value_finished_)
    {
      throw_error(tok, "Expected an array value before ','");
    }
    ctx_->begin_new_array_item();
    value_finished_ = false;
    state_          = parse_state::in_new_context;
    break;

  case token_type::rbracket:
    if (!is_scope_of_type(container_type::compact_array))
    {
      throw_error(tok, "Unexpected ']'");
    }
    close_last_context();
    state_          = parse_state::none;
    value_finished_ = true;
    break;

  case token_type::indent:
    if (!is_scope_of_type(container_type::compact_array))
    {
      handle_indent(static_cast<uint16_t>(tok.content_.count_));
    }
    break;

  case token_type::key:
    if (is_scope_of_type(container_type::compact_array))
    {
      throw_error(tok, "Unexpected key, ']' expected");
    }
    handle_key(tok.decoded_ ? std::string_view(decoded_) : get_view(tok.content_));
    break;

  case token_type::value:
    handle_value(tok.decoded_ ? std::string_view(decoded_) : get_view(tok.content_));
    break;

  case token_type::dash:
    handle_dash(static_cast<uint16_t>(tok.content_.count_), false);
    indent_level_ = static_cast<uint16_t>(current_pos_ - line_start_);
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
    if (!is_scope_of_type(container_type::compact_array))
    {
      value_finished_ = false;
    }
    break;
  case token_type::eof:
  default:
    break;
  }
}

void lite_stream::handle_indent(uint16_t new_indent)
{
  if (new_indent < indent_level_)
  {
    close_context(new_indent + 1);
  }
  else if (new_indent > indent_level_)
  {
    state_ = parse_state::in_new_context;
  }

  indent_level_ = new_indent;
}

void lite_stream::handle_key(std::string_view key)
{
  if (state_ == parse_state::in_new_context)
  {
    ctx_->begin_object();
    indent_stack_.emplace_back(indent_level_, container_type::object);
  }
  ctx_->set_key(key);
  state_ = parse_state::in_key;
}

void lite_stream::handle_value(std::string_view value)
{
  ctx_->set_value(value);
  state_          = parse_state::none;
  value_finished_ = true;
}

void lite_stream::handle_dash(uint16_t new_indent, bool compact)
{
  handle_indent(new_indent);
  if (state_ == parse_state::in_new_context || compact)
  {
    ctx_->begin_array();
    indent_stack_.emplace_back(indent_level_, compact ? container_type::compact_array : container_type::array);
  }
  else
  {
    close_until(indent_level_, container_type::array);
  }
  ctx_->begin_new_array_item();
  state_          = parse_state::in_new_context;
  value_finished_ = false;
}

void lite_stream::handle_block_scalar(token_type type)
{
  if (is_scope_of_type(container_type::compact_array))
  {
    throw_error(
     {
      .content_ = {.start_ = current_pos_ - 1, .count_ = 1}
    },
     "Block scalars are not allowed in flow arrays");
  }
  block_parent_indent_ = indent_level_;
  if (state_ == parse_state::in_new_context && is_scope_of_type(container_type::array))
  {
    block_parent_indent_ = static_cast<uint16_t>(indent_stack_.back().indent_ - 1);
  }
  state_          = parse_state::in_block_scalar;
  block_style_    = type;
  value_finished_ = true;
  block_lines_.clear();
}

void lite_stream::collect_block_scalar()
{
  // Read the whole block here; leave the first dedented line for the tokenizer.
  uint32_t block_indent = 0;
  while (current_pos_ < content_.size())
  {
    auto const start  = current_pos_;
    auto const indent = count_indent();
    auto       line   = get_current_line();
    if (line.count_ != 0 && ouly::detail::vector_access(content_, current_pos_ - 1) == '\r')
    {
      --line.count_;
    }
    if (line.count_ != 0)
    {
      if (indent.count_ <= block_parent_indent_ || (block_indent != 0 && indent.count_ < block_indent))
      {
        current_pos_ = start;
        break;
      }
      if (block_indent == 0)
      {
        block_indent = indent.count_;
      }
      // Preserve indentation beyond the block's base indentation.
      line.start_ = start + block_indent;
      line.count_ += indent.count_ - block_indent;
    }
    block_lines_.push_back(line);
    if (peek(0) == '\n')
    {
      ++current_pos_;
    }
  }

  // Keep lite_yml's existing convention of stripping trailing block newlines.
  while (!block_lines_.empty() && block_lines_.back().count_ == 0)
  {
    block_lines_.pop_back();
  }
  std::string result;
  for (uint32_t i = 0; i < block_lines_.size(); ++i)
  {
    auto line = get_view(ouly::detail::vector_access(block_lines_, i));
    if (i > 0)
    {
      auto previous = get_view(ouly::detail::vector_access(block_lines_, i - 1));
      if (block_style_ == token_type::pipe || line.empty() || (!line.empty() && is_space(line.front())) ||
          (!previous.empty() && is_space(previous.front())))
      {
        result += '\n';
      }
      else if (!previous.empty())
      {
        result += ' ';
      }
    }
    result += line;
  }
  ctx_->set_value(result);
  block_lines_.clear();
  state_         = parse_state::none;
  at_line_start_ = true;
}

void lite_stream::close_until(uint16_t new_indent, container_type type)
{
  while (!indent_stack_.empty() && indent_stack_.back().indent_ >= new_indent)
  {
    auto current = indent_stack_.back();
    if (current.type_ == type)
    {
      return;
    }
    if (current.type_ == container_type::array || current.type_ == container_type::compact_array)
    {
      ctx_->end_array();
    }
    else
    {
      ctx_->end_object();
    }
    indent_stack_.pop_back();
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
    state_ = parse_state::none;
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

} // namespace ouly::yml
