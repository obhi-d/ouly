
#pragma once
#include <acl/containers/small_vector.hpp>
#include <cassert>
#include <compare>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace acl::yaml
{

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
  location() noexcept = default;
  location(std::string_view source_name) noexcept : source_name(source_name) {}
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
    std::string value = std::string(source_name.empty() ? "istream" : source_name);
    value += "(" + std::to_string(begin.line) + ":" + std::to_string(begin.character) + "-" + std::to_string(end.line) +
             ":" + std::to_string(end.character) + "): ";
    return value;
  }

  inline friend std::ostream& operator<<(std::ostream& yyo, location const& l) noexcept

  {
    std::string value = std::string(l.source_name.empty() ? "istream" : l.source_name);
    if (l.begin == l.end)
      yyo << "<" << value << '-' << l.begin << ">";
    else
      yyo << "<" << value << '-' << l.begin << "-" << l.end << ">";
    return yyo;
  }

  position next_line() const noexcept
  {
    return position{begin.line + 1, 0};
  }

  inline auto operator<=>(location const&) const noexcept = default;

  std::string_view source_name;
  position         begin;
  position         end;
};

struct string_slice
{
  uint32_t start = 0;
  uint32_t count = 0;
};

class context
{
public:
  virtual ~context() noexcept                            = default;
  virtual void start_mapping(std::string_view slice)     = 0;
  virtual void add_mapping_value(std::string_view slice) = 0;
  virtual void end_mapping()                             = 0;
  virtual void start_sequence()                          = 0;
  virtual void end_sequence()                            = 0;
  virtual void start_sequence_item()                     = 0;
  virtual void add_sequence_item(std::string_view slice) = 0;
  virtual void end_sequence_item()                       = 0;
};

class istream
{
public:
  inline void start_mapping(string_slice slice) const
  {
    if (handler_)
      handler_->start_mapping(get_view(slice));
  }

  inline void add_mapping_value(string_slice slice) const
  {
    if (handler_)
      handler_->add_mapping_value(get_view(slice));
  }

  inline void end_mapping() const
  {
    if (handler_)
      handler_->end_mapping();
  }

  inline void start_sequence() const
  {
    if (handler_)
      handler_->start_sequence();
  }

  inline void end_sequence() const
  {
    if (handler_)
      handler_->end_sequence();
  }

  inline void start_sequence_item() const
  {
    if (handler_)
      handler_->start_sequence_item();
  }

  inline void add_sequence_item(string_slice slice) const
  {
    if (handler_)
      handler_->add_sequence_item(get_view(slice));
  }

  inline void end_sequence_item() const
  {
    if (handler_)
      handler_->end_sequence_item();
  }

  inline std::string_view get_file_name() const noexcept
  {
    return source_.source_name;
  }

  inline void throw_error(location const& loc, std::string_view error, std::string_view context) const
  {
    auto str = (std::string)loc;
    str += '[';
    str += context;
    str += ']';
    str += error;

    throw std::runtime_error(str);
  }

  inline auto const& location() const noexcept
  {
    return source_;
  }

  inline uint32_t peek_indent() const noexcept
  {
    return indent_stack.empty() ? 0 : indent_stack.back();
  }

  inline void push_indent(uint32_t indent) noexcept
  {
    indent_stack.push_back(indent);
  }

  inline void pop_indent() noexcept
  {
    indent_stack.pop_back();
  }

  inline string_slice get_view(uint32_t length) const
  {
    return {cursor_, length};
  }

  inline void move_ahead(uint32_t length) noexcept
  {
    cursor_ += length;
  }

  inline int read(char* istream, int siz) noexcept
  {
    if (cursor_ >= contents_.size())
      return 0;

    siz = std::min(siz, static_cast<int>(contents_.size() - cursor_));
    contents_.copy(istream, siz, cursor_);
    cursor_ += siz;
    return siz;
  }

  inline auto const& get_source() const
  {
    return source_;
  }

  inline void* get_scanner() const noexcept
  {
    return scanner;
  }

  inline void set_current_key(string_slice slice) noexcept
  {
    current_key_ = slice;
  }

  inline string_slice get_current_key() const noexcept
  {
    return current_key_;
  }

  inline void set_handler(context* handler) noexcept
  {
    handler_ = handler;
  }

  istream(std::string_view source, std::string_view content) noexcept : source_(source), contents_(content) {}

  void parse(context& handler_);
  void begin_scan();
  void end_scan();

private:
  inline std::string_view get_view(string_slice slice) const noexcept
  {
    return contents_.substr(slice.start, slice.count);
  }

  void*                          scanner = nullptr;
  acl::small_vector<uint32_t, 8> indent_stack;

  string_slice     current_key_;
  uint32_t         cursor_  = 0;
  context*         handler_ = nullptr;
  yaml::location   source_;
  std::string_view contents_;
};

} // namespace acl::yaml