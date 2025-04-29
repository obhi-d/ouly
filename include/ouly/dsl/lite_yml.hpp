// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/containers/small_vector.hpp"
#include "ouly/utility/user_config.hpp"
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string_view>

namespace ouly::yml
{

struct string_slice
{
  uint32_t start_ = 0;
  uint32_t count_ = 0;

  auto operator<=>(string_slice const&) const = default;
};

constexpr uint32_t small_buffer_size = 8;
using string_slice_array             = ouly::small_vector<string_slice, small_buffer_size>;

/**
 * @brief Base class for all parser contexts in the YAML parsing system
 *
 * This abstract class defines the interface for context objects used during YAML parsing.
 * Each derived context handles different parsing states and element types.
 */
class context
{
public:
  context() noexcept                             = default;
  context(const context&)                        = default;
  context(context&&) noexcept                    = delete;
  auto operator=(const context&) -> context&     = default;
  auto operator=(context&&) noexcept -> context& = delete;
  virtual ~context() noexcept                    = default;

  /**
   * @brief Called when a new array begins in the YAML document
   */
  virtual void begin_array() = 0;

  /**
   * @brief Called when an array ends in the YAML document
   */
  virtual void end_array() = 0;

  /**
   * @brief Called when a new object begins in the YAML document
   */
  virtual void begin_object() = 0;

  /**
   * @brief Called when an object ends in the YAML document
   */
  virtual void end_object() = 0;

  /**
   * @brief Called when a new array item is encountered
   */
  virtual void begin_new_array_item() = 0;

  /**
   * @brief Called when a key is encountered in a mapping
   *
   * @param slice The key as a string view
   */
  virtual void set_key(std::string_view slice) = 0;

  /**
   * @brief Called when a scalar value is encountered
   *
   * @param slice The value as a string view
   */
  virtual void set_value(std::string_view slice) = 0;
};

class lite_stream
{
public:
  explicit lite_stream(std::string_view content, context* ctx) : content_(content), ctx_(ctx) {}

  // Main parse function that processes the yml content
  void parse();

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
    in_new_context,
    in_key,
    in_block_scalar,
    in_array
  };

  enum class container_type : uint8_t
  {
    none,
    array,
    compact_array,
    object
  };

  struct token
  {
    token_type   type_ = token_type::eof;
    string_slice content_;

    operator bool() const noexcept
    {
      return type_ != token_type::eof;
    }
  };

  // Token processing
  auto next_token() -> token;
  void process_token(token tok);
  // Context management
  void handle_indent(uint16_t new_indent);
  void handle_key(string_slice key);
  void handle_value(string_slice value);
  void handle_dash(uint16_t new_indent, bool compact);
  void handle_block_scalar(token_type type);
  void collect_block_scalar();
  void close_context(uint16_t new_indent);
  void close_until(uint16_t new_indent, container_type type);

  // Utility functions
  [[nodiscard]] auto get_view(string_slice slice) const -> std::string_view
  {
    return content_.substr(slice.start_, slice.count_);
  }

  auto count_indent() -> string_slice
  {
    auto start = current_pos_;
    while (current_pos_ < content_.length() && (content_[current_pos_] == ' ' || content_[current_pos_] == '\t'))
    {
      current_pos_++;
    }
    return {.start_ = start, .count_ = (current_pos_ - start)};
  }

  void skip_whitespace()
  {
    while (current_pos_ < content_.length() && (content_[current_pos_] == ' ' || content_[current_pos_] == '\t'))
    {
      current_pos_++;
    }
  }

  [[nodiscard]] auto peek(uint32_t offset) const -> char
  {
    return (current_pos_ + offset < content_.length()) ? content_[current_pos_ + offset] : '\0';
  }

  auto get_current_line() -> string_slice
  {
    auto start = current_pos_;
    while (current_pos_ < content_.length() && content_[current_pos_] != '\n')
    {
      current_pos_++;
    }
    return {.start_ = start, .count_ = (current_pos_ - start)};
  }

  auto pop_indent() -> container_type
  {
    if (indent_stack_.empty())
    {
      return container_type::none;
    }
    auto top = indent_stack_.back();
    indent_stack_.pop_back();
    return top.type_;
  }

  void close_last_context();

  void throw_error(token tok, std::string_view error) const
  {
#ifdef __clang__
    // Write error handlign without using std::format
    std::string error_str = "parse-error - ";
    error_str += std::to_string(tok.content_.start_);
    error_str += " : (around ";
    error_str += get_view(tok.content_);
    error_str += ") - ";
    error_str += error;
    throw std::runtime_error(error_str);
#else
    throw std::runtime_error(
     std::format("parse-error - {} : (around {}) - {}", tok.content_.start_, get_view(tok.content_), error));
#endif
  }

  [[nodiscard]] auto is_scope_of_type(container_type type) const -> bool
  {
    return !indent_stack_.empty() && indent_stack_.back().type_ == type;
  }

  [[nodiscard]] auto is_scope_of_type(container_type type, uint16_t indent) const -> bool
  {
    return !indent_stack_.empty() && indent_stack_.back().indent_ == indent && indent_stack_.back().type_ == type;
  }

  struct indent_entry
  {
    uint16_t       indent_ = 0;
    container_type type_   = container_type::none;
  };

  std::string_view content_;

  small_vector<indent_entry, small_buffer_size> indent_stack_;
  small_vector<string_slice, small_buffer_size> block_lines_;

  context*    ctx_                 = nullptr;
  parse_state state_               = parse_state::in_new_context;
  uint32_t    current_pos_         = 0;
  uint16_t    indent_level_        = 0;
  token_type  block_style_         = token_type::eof;
  bool        at_line_start_ : 1   = true;
  bool        can_be_sequence_ : 1 = false;
};
} // namespace ouly::yml
