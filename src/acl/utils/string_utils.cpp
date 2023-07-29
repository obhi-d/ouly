
#include <acl/utils/string_utils.hpp>
#include <stdexcept>

namespace acl
{

std::string format_snake_case(const std::string& str)
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
      ret += upper ? ::toupper(str[i]) : ::tolower(str[i]);
      upper = false;
    }
    i++;
  }
  return ret;
}

std::string format_camel_case(const std::string& str)
{
  std::string ret;
  ret.reserve(str.length() + 4);
  size_t i = 1;
  ret += ::toupper(str[0]);
  while (i < str.length())
  {
    if (::isupper(str[i]))
    {
      ret += ' ';
    }
    ret += str[i];
    i++;
  }
  return ret;
}

std::string format_name(const std::string& str)
{
  if (!str.length())
    return {};
  if (str.find('_') != std::string::npos)
    return format_snake_case(str);
  else
    return format_camel_case(str);
}

inline void encode(std::string& enc, utf32 c) {}

} // namespace acl