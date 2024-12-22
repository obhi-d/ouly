
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
struct location_type
{
  uint32_t                    begin = 0;
  uint32_t                    end   = 0;
  inline friend std::ostream& operator<<(std::ostream& yyo, location_type const& l) noexcept
  {
    return yyo << l.begin << ':' << l.end;
  }
};

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

;
class istream
{
public:
  using indent = uint32_t;

  void set_idention_level(uint16_t level);
  void set_idention_level(string_slice level);
  void add_dash_indention(string_slice dash_level);

  void add_new_mapping(string_slice key);
  void add_new_mapped_sequence(string_slice key);
  void add_mapping_value(string_slice value);
  void add_mapping_value(string_slice_array value);
  void begin_array();
  void end_array();
  void add_new_sequence_value(string_slice value);
  void add_new_sequence_value(string_slice_array value);

  void close_all_mappings();

  inline void throw_error(location_type const& loc, std::string_view error, std::string_view context) const
  {
    throw std::runtime_error(std::format("[{}] token@{}: \"{}\" - {}", context, loc.begin,
                                         contents.substr(loc.begin, loc.end - loc.begin), error));
  }

  inline auto location() const noexcept
  {
    return location_type{current_token.start, current_token.start + current_token.count};
  }

  inline string_slice get_view() const
  {
    return current_token;
  }

  inline string_slice get_quoted_string() const
  {
    return {current_token.start + 1, current_token.count - 2};
  }

  inline void set_token(uint32_t length) noexcept
  {
    current_token.start = token_cursor;
    current_token.count = length;
    token_cursor += length;
  }

  inline int read(char* istream, int siz) noexcept
  {
    if (read_cursor >= contents.size())
      return 0;

    siz = std::min(siz - 1, static_cast<int>(contents.size() - read_cursor));
    contents.copy(istream, siz, read_cursor);
    istream[siz] = 0;
    read_cursor += siz;
    return siz;
  }

  inline void* get_scanner() const noexcept
  {
    return scanner;
  }

  inline void set_handler(context* handler) noexcept
  {
    handler = handler;
  }

  istream(std::string_view content) noexcept : contents(content) {}

  void parse(context& handler);
  void begin_scan();
  void end_scan();

private:
  inline std::string_view get_view(string_slice slice) const noexcept
  {
    return contents.substr(slice.start, slice.count);
  }

  enum class indent_type : uint8_t
  {
    none,
    object,
    array,
  };

  void*  scanner              = nullptr;
  indent current_indent_level = {};
  indent test_indent_level    = {};

  acl::small_vector<indent_type, 16> indent_stack;

  string_slice     current_key;
  string_slice     current_token;
  uint32_t         read_cursor  = 0;
  uint32_t         token_cursor = 0;
  context*         handler      = nullptr;
  std::string_view contents;
};

} // namespace acl::yaml
