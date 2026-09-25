// SPDX-License-Identifier: MIT
#pragma once
/**
 * @file lite_yml_writer_context.hpp
 * @brief Provides utilities for writing YAML content in a lightweight manner.
 *
 * This file defines classes and methods for managing YAML writer state and
 * generating YAML content with proper indentation and formatting.
 */

#include "ouly/reflection/reflection.hpp"
#include "ouly/utility/to_chars.hpp"
#include "ouly/utility/type_traits.hpp"
#include <algorithm>
#include <string>
#include <string_view>

namespace ouly::detail
{

/**
 * @brief Manages the state of a YAML writer.
 *
 * This class provides methods for handling indentation, array entries, and
 * map entries while writing YAML content.
 */
class writer_state
{
  std::string stream_;

  int  indent_level_ = -1;
  bool skip_indent_  = false;

public:
  auto get() -> std::string
  {
    return std::move(stream_);
  }

  void begin_array()
  {
    indent_level_++;
    indent();
    stream_.push_back('-');
    stream_.push_back(' ');
    indent_level_++;
  }

  void end_array()
  {
    indent_level_ -= 2;
    skip_indent_ = false;
  }

  void begin_object()
  {
    indent_level_++;
    indent();
  }

  void end_object()
  {
    indent_level_--;
    skip_indent_ = false;
  }

  void key(std::string_view slice)
  {
    if (needs_quotes(slice))
    {
      append_quoted(slice);
    }
    else
    {
      stream_.append(slice);
    }
    stream_.push_back(':');
    stream_.push_back(' ');
    skip_indent_ = false;
  }

  void as_string(std::string_view slice)
  {
    // An empty scalar written bare leaves `key:` with nothing after it, and the parser then reads
    // whatever follows -- the next list entry -- as this key's value. A scalar opening with `#` reads
    // back as a comment, and one opening with a quote, bracket or block indicator changes token
    // type. Quoting keeps every such string the same string on the way back in.
    if (needs_quotes(slice))
    {
      append_quoted(slice);
    }
    else
    {
      stream_.append(slice);
    }
    skip_indent_ = false;
  }

  void as_uint64(uint64_t value)
  {
    append_chars(value);
  }

  void as_int64(int64_t value)
  {
    append_chars(value);
  }

  void as_double(double value)
  {
    // std::to_chars emits the shortest representation that round-trips, unlike
    // std::to_string which truncates to 6 decimal places.
    append_chars(value);
  }

  void as_bool(bool value)
  {
    stream_.append(value ? "true" : "false");
    skip_indent_ = false;
  }

  void as_null()
  {
    stream_.append("null");
    skip_indent_ = false;
  }

  void next_map_entry()
  {
    indent();
  }

  void next_array_entry()
  {
    indent_level_--;
    indent();
    stream_.push_back('-');
    stream_.push_back(' ');
    indent_level_++;
  }

private:
  static auto needs_quotes(std::string_view slice) noexcept -> bool
  {
    if (slice.empty() || slice.front() == ' ' || slice.back() == ' ' || slice.back() == ':')
    {
      return true;
    }

    switch (slice.front())
    {
    case '#':
    case '"':
    case '\'':
    case '[':
    case ']':
    case '{':
    case '}':
    case '|':
    case '>':
    case ',':
    case '&':
    case '*':
    case '!':
    case '%':
    case '@':
    case '`':
      return true;
    case '-':
      if (slice.size() == 1 || slice[1] == ' ')
      {
        return true;
      }
      break;
    default:
      break;
    }

    if (slice.find(": ") != std::string_view::npos || slice.find(" #") != std::string_view::npos)
    {
      return true;
    }

    return std::ranges::any_of(slice,
                               [](char c) -> bool
                               {
                                 return static_cast<unsigned char>(c) < static_cast<unsigned char>(' ') || c == ',' ||
                                        c == ']';
                               });
  }

  void append_quoted(std::string_view slice)
  {
    stream_.push_back('"');
    for (char c : slice)
    {
      switch (c)
      {
      case '"':
      case '\\':
        stream_.push_back('\\');
        stream_.push_back(c);
        break;
      case '\0':
        stream_.append("\\0");
        break;
      case '\a':
        stream_.append("\\a");
        break;
      case '\b':
        stream_.append("\\b");
        break;
      case '\t':
        stream_.append("\\t");
        break;
      case '\n':
        stream_.append("\\n");
        break;
      case '\v':
        stream_.append("\\v");
        break;
      case '\f':
        stream_.append("\\f");
        break;
      case '\r':
        stream_.append("\\r");
        break;
      case '\x1b':
        stream_.append("\\e");
        break;
      default:
        stream_.push_back(c);
        break;
      }
    }
    stream_.push_back('"');
  }

  template <typename V>
  void append_chars(V value)
  {
    ouly::to_chars(stream_, value);
    skip_indent_ = false;
  }

  void indent()
  {
    if (!skip_indent_)
    {
      stream_.push_back('\n');
      stream_.append(static_cast<size_t>(indent_level_), ' ');
    }
    skip_indent_ = true;
  }
};
} // namespace ouly::detail
