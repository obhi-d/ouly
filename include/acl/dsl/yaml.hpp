
#pragma once
#include <acl/containers/small_vector.hpp>
#include <cassert>
#include <compare>
#include <cstdint>
#include <format>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace acl::yaml
{

struct string_slice
{
  uint32_t start = 0;
  uint32_t count = 0;

  auto operator<=>(string_slice const&) const = default;
};

using string_slice_array = acl::small_vector<string_slice, 8>;

class context
{
public:
  virtual ~context() noexcept                    = default;
  virtual void begin_array()                     = 0;
  virtual void end_array()                       = 0;
  virtual void begin_key(std::string_view slice) = 0;
  virtual void end_key()                         = 0;
  virtual void set_value(std::string_view slice) = 0;
};

class istream
{
public:
  inline explicit istream(std::string_view content) : content_(content) {}

  // Main parse function that processes the YAML content
  void parse();

  void set_handler(context* ctx)
  {
    ctx_ = ctx;
  }

private:
  enum class token_type : uint8_t
  {
    indent,   // Whitespace at start of line
    key,      // Key followed by colon
    value,    // Simple scalar value
    dash,     // Array item marker
    pipe,     // | for literal block scalar
    gt,       // > for folded block scalar
    newline,  // Line ending
    lbracket, // [
    rbracket, // ]
    comma,    // ,
    eof       // End of input
  };

  enum class parse_state : uint8_t
  {
    none,
    in_object,
    in_key,
    in_value,
    in_block_scalar,
    in_compact_mapping,
    in_array
  };

  enum class container_type : uint8_t
  {
    none,
    array,
    object
  };

  struct token
  {
    token_type   type = token_type::eof;
    string_slice content;

    inline operator bool() const noexcept
    {
      return type != token_type::eof;
    }
  };

  // Token processing
  token next_token();
  void  process_token(token tok);
  // Context management
  void handle_indent(uint16_t new_indent);
  void handle_key(string_slice key);
  void handle_value(string_slice value);
  void handle_dash(uint16_t extra_indent);
  void handle_block_scalar(token_type type);
  void collect_block_scalar();
  void close_context(uint16_t new_indent);

  // Utility functions
  std::string_view get_view(string_slice slice) const
  {
    return content_.substr(slice.start, slice.count);
  }

  string_slice count_indent()
  {
    auto start = current_pos_;
    while (current_pos_ < content_.length() && (content_[current_pos_] == ' ' || content_[current_pos_] == '\t'))
    {
      current_pos_++;
    }
    return {start, static_cast<uint32_t>(current_pos_ - start)};
  }

  void skip_whitespace()
  {
    while (current_pos_ < content_.length() && (content_[current_pos_] == ' ' || content_[current_pos_] == '\t'))
    {
      current_pos_++;
    }
  }

  char peek(uint32_t offset) const
  {
    return (current_pos_ + offset < content_.length()) ? content_[current_pos_ + offset] : '\0';
  }

  string_slice get_current_line()
  {
    auto start = current_pos_;
    while (current_pos_ < content_.length() && content_[current_pos_] != '\n')
    {
      current_pos_++;
    }
    return {start, static_cast<uint32_t>(current_pos_ - start)};
  }

  container_type pop_indent()
  {
    if (indent_stack_.empty())
      return container_type::none;
    auto top = indent_stack_.back();
    indent_stack_.pop_back();
    return top.type;
  }

  void close_last_context();

private:
  inline void throw_error(token token, std::string_view error) const
  {
    throw std::runtime_error(
      std::format("parse-error @{} : (around {}) - {}", token.content.start, get_view(token.content), error));
  }

  struct indent_entry
  {
    uint16_t       indent = 0;
    container_type type   = container_type::none;
  };

  std::string_view              content_;
  small_vector<indent_entry, 8> indent_stack_;
  small_vector<string_slice, 8> block_lines_;

  context*    ctx_                 = nullptr;
  parse_state state_               = parse_state::none;
  uint32_t    current_pos_         = 0;
  uint16_t    indent_level_        = 0;
  token_type  block_style_         = token_type::eof;
  bool        at_line_start_ : 1   = true;
  bool        can_be_sequence_ : 1 = false;
};
} // namespace acl::yaml
