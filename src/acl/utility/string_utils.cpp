
#include <acl/utility/string_utils.hpp>
#include <stdexcept>

namespace acl
{

auto format_snake_case(const std::string& str) -> std::string
{
  std::string ret;
  ret.reserve(str.length() + 1);
  bool   upper = true;
  size_t i     = 0;
  while (i < str.length())
  {
    if (str[i] == '_')
    {
      if (!upper)
      {
        ret += ' ';
        upper = true;
      }
    }
    else
    {
      ret += static_cast<char>(upper ? ::toupper(str[i]) : ::tolower(str[i]));
      upper = false;
    }
    i++;
  }
  return ret;
}

auto format_camel_case(const std::string& str) -> std::string
{
  std::string ret;
  ret.reserve(str.length() + 4);
  size_t i = 1;
  ret += static_cast<char>(::toupper(str[0]));
  while (i < str.length())
  {
    if (::isupper(str[i]) != 0)
    {
      ret += ' ';
    }
    ret += str[i];
    i++;
  }
  return ret;
}

auto format_name(const std::string& str) -> std::string
{
  if (str.empty())
  {
    return {};
  }
  if (str.find('_') != std::string::npos)
  {
    return format_snake_case(str);
  }
  return format_camel_case(str);
}

inline void encode(std::string& enc, utf32 c) {}

} // namespace acl