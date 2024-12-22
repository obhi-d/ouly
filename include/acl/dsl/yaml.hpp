
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
  virtual void begin_object()                    = 0;
  virtual void end_object()                      = 0;
  virtual void begin_array()                     = 0;
  virtual void end_array()                       = 0;
  virtual void set_key(std::string_view slice)   = 0;
  virtual void set_value(std::string_view slice) = 0;
};

class istream
{
public:
  inline explicit istream(std::string_view content) : content_(content) {}

  // Main parse function that processes the YAML content
  void parse(context& ctx);

  void set_handler(context* ctx)
  {
    ctx_ = ctx;
  }

private:
  enum class token_type : uint8_t
  {
    indent,  // Whitespace at start of line
    key,     // Key followed by colon
    value,   // Simple scalar value
    dash,    // Array item marker
    pipe,    // | for literal block scalar
    gt,      // > for folded block scalar
    newline, // Line ending
    eof      // End of input
  };

  enum class parse_state : uint8_t
  {
    none,
    in_key,
    in_value,
    in_block_scalar,
    in_array
  };

  struct token
  {
    token_type   type;
    string_slice content;
  };

  // Token processing
  std::optional<token> next_token();
  void                 process_token(const std::optional<token>& tok);
  // Context management
  void handle_indent(uint32_t new_indent);
  void handle_key(string_slice key);
  void handle_value(string_slice value);
  void handle_dash();
  void handle_block_scalar(token_type type);
  void collect_block_scalar();
  void close_context(uint32_t new_indent);

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

private:
  enum class container_type
  {
    object,
    array
  };
  struct indent_entry
  {
    uint32_t       indent;
    container_type type;
  };

  std::string_view content_;
  context*         ctx_           = nullptr;
  parse_state      state_         = parse_state::none;
  token_type       block_style_   = token_type::eof;
  uint32_t         indent_level_  = 0;
  uint32_t         current_pos_   = 0;
  bool             at_line_start_ = true;

  small_vector<indent_entry, 8> indent_stack_;
  small_vector<string_slice, 8> block_lines_;
};
} // namespace acl::yaml
