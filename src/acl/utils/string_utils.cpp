
#include <acl/utils/string_utils.hpp>
#include <stdexcept>

namespace acl
{
#define utf32isspace(p) (((p) >= 0x09 && (p) <= 0x0D) || (p) == 0x20)
#define utf8isspace(p)  (((p) >= 0x09 && (p) <= 0x0D) || (p) == 0x20)

const utf8* get_next_word_end_utf8(const utf8* p, const utf8** start)
{
  std::locale l;
  while (*p && std::isspace((char)*p, l))
    ++p;
  if (start)
    *start = p;
  while (*p)
  {
    if (std::isspace((char)*p, l))
      break;
    ++p;
  }
  return p;
}

const utf32* get_next_word_end_utf32(const utf32* p, const utf32** start)
{
  while (*p && utf32isspace(*p))
    ++p;
  if (start)
    *start = p;
  while (*p)
  {
    if (utf32isspace(*p))
      break;
    ++p;
  }
  return p;
}

const utf8* get_word_end_utf8(const utf8* p)
{
  while (*p)
  {
    if (isspace(*p))
      break;
    ++p;
  }
  return p;
}

const utf32* get_word_end_utf32(const utf32* p)
{
  while (*p)
  {
    if (utf32isspace(*p))
      break;
    ++p;
  }
  return p;
}

/* returns the pointer where no whitespace [' ','\n','\t'] is encountered */
const utf32* eat_white_utf32(const utf32* p)
{
  if (!p)
    return 0;
  while (*p && utf32isspace(*p))
    ++p;
  return p;
}

/* returns the pointer where no whitespace [' ','\n','\t'] is encountered */
const utf8* eat_white_utf8(const utf8* p)
{
  if (!p)
    return 0;
  while (*p && utf8isspace(*p))
    ++p;
  return p;
}

//@ returns at the encounter of 'indexofchar' or null

const utf8* locate_utf8(const utf8* p, const utf8 c)
{
  while (*p && *p != c)
    ++p;
  return p;
}

//@ returns at the encounter of 'indexofchar' or null

const utf32* locate_utf32(const utf32* p, const utf32 c)
{
  while (*p && *p != c)
    ++p;
  return p;
}

/* returns the pointer where no whitespace [' ','\n','\t'] is encountered */
const char* eat_white(const char* p)
{
  if (!p)
    return 0;
  std::locale l;
  while (*p && std::isspace(*p, l))
    ++p;
  return p;
}

const char* match_tag(const char* text, const char* tag)
{
  const char* mtext = text;
  mtext             = eat_white(mtext);
  while (*mtext && *tag)
  {
    if (*(mtext++) != *(tag++))
      return 0;
  }
  if (!(*tag))
  {
    return mtext;
  }
  return 0;
}

std::string& replace(std::string& s, const std::string& sub, const std::string& other)
{
  assert(!sub.empty());
  size_t b = 0;
  while (((b = s.find(sub, b)) != std::string::npos))
  {
    s.replace(b, sub.size(), other);
    b += other.size();
  }
  return s;
}

bool pattern_match(const char* pat, const char* str, bool checkcase)
{
  while (*str)
  {
    switch (*pat)
    {
    case '?':
      if (*str == '.')
        return false;
      break;
    case '*':
      do
      {
        ++pat;
      }
      while (*pat == '*');
      if (!*pat)
        return true;
      while (*str)
      {
        if (pattern_match(pat, str++))
          return true;
      }
      return false;
    default:
    {
      char c1 = *str;
      char c2 = *pat;
      // doing here to avoid extra allocations
      if (!checkcase)
      {
        c1 = tolower(c1);
        c2 = tolower(c2);
      }
      if (c1 != c2)
        return false;
    }
    break;
    }
    ++pat, ++str;
  }
  while (*pat == '*')
    ++pat;
  return (*pat) == 0;
}

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

void to_utf16(std::wstring& output, std::string_view utf8_str)
{
  output.reserve(utf8_str.length() + 1);
  output.clear();
  const utf8* buffer = (const utf8*)utf8_str.data();
  size_t      length = utf8_str.length();
  utf32       c;
  while (*buffer && length > 0)
  {
    utf8 scan = buffer[0];
    if (scan < 0x80)
    {
      output += scan;
      ++buffer;
      --length;
    }
    else if (scan < 0xE0 && length >= 2)
    {
      c = ((scan & 0x1F) << 6);
      c |= (buffer[1] & 0x3F);
      // L_ASSERT (c <= 0xD800 && c >= 0xDFFF);
      output += (utf16)(c);
      buffer += 2;
      length -= 2;
    }
    else if (scan < 0xF0 && length >= 3)
    {
      // decode
      c = ((scan & 0x0F) << 12);
      c |= ((buffer[1] & 0x3F) << 6);
      c |= (buffer[2] & 0x3F);
      // L_ASSERT (c <= 0xD800 && c >= 0xDFFF);
      output += (utf16)(c);
      buffer += 3;
      length -= 3;
    }
    else if (length >= 4)
    {
      c = ((scan & 0x07) << 18);
      c |= ((buffer[1] & 0x3F) << 12);
      c |= ((buffer[2] & 0x3F) << 6);
      c |= (buffer[3] & 0x3F);
      // encode
      uint32_t v = c - 0x10000;
      output += (utf16)((v >> 10) + 0xD800);
      output += (utf16)((v & 0x3FF) + 0xDC00);
      buffer += 4;
      length -= 4;
    }
    else
    {
      throw std::invalid_argument("Malformed expression");
    }
  }
}

void to_utf8(std::string& output, std::wstring_view utf16_str)
{
  output.reserve(utf16_str.length() + 1);
  output.clear();
  const utf16* buffer = (const utf16*)utf16_str.data();
  size_t       length = utf16_str.length();
  while (*buffer && length > 0)
  {
    utf16 scan = buffer[0];
    if (scan < 0x80)
    {
      output += (utf8)scan;
      --length;
      ++buffer;
    }
    else if (scan >= 0xD800 && scan <= 0xDFFF && length >= 2)
    {
      // higher 10 bits
      scan = scan & 0x3FF;
      // lower 10 bits
      utf16    scan2 = (buffer[1] & 0x3FF);
      uint32_t c     = ((scan << 10) | scan2) + 0x10000;
      length -= 2;
      buffer += 2;
      // U+10000 to U+1FFFFF
      output += (utf8)((char)(c >> 18) & 0x07) | 0xF0;
      output += (utf8)((char)(c >> 12) & 0x3F) | 0x80;
      output += (utf8)((char)(c >> 6) & 0x3F) | 0x80;
      output += (utf8)(((char)c) & 0x3F) | 0x80;
    }
    else
    {
      // U+0080 - U+07FF
      if (scan < 0x07FF)
      {
        output += (((char)(scan >> 6) & 0x1F) | 0xC0);
        output += (((char)scan) & 0x3F) | 0x80;
      }
      // U+0800 - U-FFFF
      else
      {
        output += ((char)(scan >> 12) & 0x0F) | 0xE0;
        output += ((char)(scan >> 6) & 0x3F) | 0x80;
        output += (((char)scan) & 0x3F) | 0x80;
      }
      --length;
      ++buffer;
    }
  }
}
} // namespace acl