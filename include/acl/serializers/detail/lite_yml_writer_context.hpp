
#pragma once
#include <acl/reflection/reflection.hpp>
#include <acl/utility/type_traits.hpp>

namespace acl::detail
{

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
  }

  void end_array()
  {
    indent_level_--;
  }

  void begin_object()
  {
    indent_level_++;
    indent();
  }

  void end_object()
  {
    indent_level_--;
  }

  void key(std::string_view slice)
  {
    stream_.append(slice);
    stream_.push_back(':');
    stream_.push_back(' ');
    skip_indent_ = false;
  }

  void as_string(std::string_view slice)
  {
    stream_.append(slice);
    skip_indent_ = false;
  }

  void as_uint64(uint64_t value)
  {
    stream_.append(std::to_string(value));
    skip_indent_ = false;
  }

  void as_int64(int64_t value)
  {
    stream_.append(std::to_string(value));
    skip_indent_ = false;
  }

  void as_double(double value)
  {
    stream_.append(std::to_string(value));
    skip_indent_ = false;
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
    indent();
    stream_.push_back('-');
    stream_.push_back(' ');
  }

private:
  void indent()
  {
    stream_.push_back('\n');
    if (!skip_indent_)
    {
      std::fill_n(std::back_inserter(stream_), static_cast<size_t>(indent_level_), ' ');
    }
    skip_indent_ = true;
  }
};
} // namespace acl::detail