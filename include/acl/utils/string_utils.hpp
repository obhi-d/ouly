#pragma once

#include "common.hpp"
#include "word_list.hpp"
#include <acl/utils/wyhash.hpp>
#include <ctime>
#include <regex>
#include <sstream>
#include <vector>

namespace acl
{

using string_view_pair = std::pair<std::string, std::string>;

static inline int32_t                          k_default_uchar = 0xFFFD;
static inline int32_t                          k_wrong_uchar   = 0xFFFF;
static inline int32_t                          k_last_uchar    = 0x10FFFF;
static inline constexpr std::string_view       k_default       = "default";
static inline constexpr std::string_view const k_default_sym   = "*";

using utf8  = char8_t;
using utf32 = char32_t;
using utf16 = char16_t;

/// @brief Index of string
/// @param to_find The string to find
/// @param in The word list
///
inline uint32_t index_of(std::string const& in, std::string_view to_find)
{
  return word_list<>::index_of(in, to_find);
}

/// @brief Check if string is present
/// @param value The string to find
/// @param in The word list
///
inline bool contains(std::string const& in, std::string const& to_find)
{
  return in.find(to_find) != std::string::npos;
}

/// @brief Push the string at the very end of the buffer
/// @param in The string array container
/// @param word The string to push
///
inline void word_push_back(std::string& in, const std::string_view& word)
{
  word_list<>::push_back(in, word);
}

inline std::string time_stamp()
{
  std::time_t t = std::time(nullptr);
  char        mbstr[32];
#ifdef _MSC_VER
  struct tm buf;
  localtime_s(&buf, &t);
  std::strftime(mbstr, sizeof(mbstr), "%m-%d-%y_%H-%M-%S", &buf);
#else
  struct tm buf;
  std::strftime(mbstr, sizeof(mbstr), "%m-%d-%y_%H-%M-%S", localtime_r(&t, &buf));
#endif
  return std::string(mbstr);
}

inline std::string time_string()
{
  std::time_t t = std::time(nullptr);
  char        mbstr[32];
#ifdef _MSC_VER
  struct tm buf;
  localtime_s(&buf, &t);
  std::strftime(mbstr, sizeof(mbstr), "%H-%M-%S", &buf);
#else
  struct tm buf;
  std::strftime(mbstr, sizeof(mbstr), "%H-%M-%S", localtime_r(&t, &buf));
#endif
  return std::string(mbstr);
}

template <class OutputIt, class BidirIt, class Traits, class CharT, class UnaryFunction>
OutputIt regex_replace(OutputIt out, BidirIt first, BidirIt last, const std::basic_regex<CharT, Traits>& re,
                       UnaryFunction f)
{
  typename std::match_results<BidirIt>::difference_type pos_of_last_match = 0;
  auto                                                  end_of_last_match = first;

  auto callback = [&](const std::match_results<BidirIt>& match)
  {
    auto pos_of_this_match = match.position(0);
    auto diff              = pos_of_this_match - pos_of_last_match;

    auto start_of_this_match = end_of_last_match;
    std::advance(start_of_this_match, diff);

    std::copy(end_of_last_match, start_of_this_match, out);
    auto m = f(match);
    std::copy(std::begin(m), std::end(m), out);

    auto len_of_match = match.length(0);

    pos_of_last_match = pos_of_this_match + len_of_match;

    end_of_last_match = start_of_this_match;
    std::advance(end_of_last_match, len_of_match);
  };

  std::regex_iterator<BidirIt> begin(first, last, re), end;
  std::for_each(begin, end, callback);
  std::copy(end_of_last_match, last, out);
  return out;
}

template <class CharT, class UnaryFunction>
auto regex_replace(std::basic_string<CharT> const& s, const std::basic_regex<CharT>& re, UnaryFunction f)
{
  std::basic_string<CharT> out;
  regex_replace(std::back_inserter(out), s.cbegin(), s.cend(), re, f);
  return out;
}

///  @brief	Returns the pointer where no whitespace [' ','\n','\t','\r'] is encountered
///
///  @return	null if it fails, else pointer where no whitespace [' ', '\n','\t','\r'] is encountered.
ACL_API const char* eat_white(const char* str);

/// @brief	Eat white utf 8.
///
/// @return	null if it fails, else.
ACL_API const utf8* eat_white_utf8(const utf8*);

/// @brief	Eat white utf 32.
///
/// @return	null if it fails, else.
ACL_API const utf32* eat_white_utf32(const utf32*);

/// @brief	Gets an indexof utf8.
///
/// @return	returns at the encounter of 'indexofchar' or null
ACL_API const utf8* locate_utf8(const utf8*, const utf8);

/// @brief	Gets an indexof utf32.
///
/// @return	returns at the encounter of 'indexofchar' or null
ACL_API const utf32* locate_utf32(const utf32*, const utf32 indexofchar);

/// @brief	Returns at the encounter of first whitespace character the returned poiner either points to the whitespace
/// or Null
/// @return	null if it fails, else the word endptr utf 8.
ACL_API const utf8* get_word_end_utf8(const utf8*);

/// @brief   Returns at the encounter of first whitespace character
///          the returned poiner either points to the whitespace or
///          Null
/// @return	null if it fails, else the word endptr utf 32.
ACL_API const utf32* get_word_end_utf32(const utf32*);

/// @brief	First eats white then starts a scan. Returns at the encounter of first whitespace character. The returned
/// poiner either points to the whitespace or to null. Also accepts a ptr to recieve the start of word.
/// @return	null if it fails, else the nextword endptr utf 8.
ACL_API const utf8* get_next_word_end_utf8(const utf8*, const utf8** start);

/// @brief	First eats white then starts a scan. Returns at the encounter of first whitespace character. The returned
/// poiner either points to the whitespace or to null. Also accepts a ptr to recieve the start of word.
/// @return	null if it fails, else the nextword endptr utf 32.
ACL_API const utf32* get_next_word_end_utf32(const utf32*, const utf32** start);

inline std::string indent(int32_t amt)
{
  std::string s;
  s.resize(amt, ' ');
  return s;
}

/// @brief	Replace all occurences of a substring in a string with another string in-place.
///
/// @param [in,out]	s	std::string to replace in. Will be modified.
/// @param	sub			Substring to replace.
/// @param	other		std::string to replace substring with.
///
/// @return	The string after replacements.
///
ACL_API std::string& replace(std::string& s, std::string const& sub, std::string const& other);

/// @brief	Matches string pattern based on wildcards[*,?,etc].
///
/// @param [in,out]	pattern	If non-null, the pattern.
/// @param [in,out]	str		If non-null, the string.
/// @param	checkcase		true to checkcase.
///
/// @return	true if it succeeds, false if it fails.
ACL_API bool pattern_match(const char* pattern, const char* str, bool checkcase = true);

/// Formats a camel or upper case name into a formatted string.
/// @example
/// "madeInChina" to "Made In China" or
/// MADE_IN_CHINA to "Made In China"
ACL_API std::string format_name(std::string const& str);

/// @brief	Converts string to lower case.
/// @param [in,out]	str	The string to convert.
template <typename StringType>
inline void to_lower(StringType& str)
{
  std::transform(std::begin(str), std::end(str), std::begin(str), ::tolower);
}

/// @brief	Converts string to upper case.
/// @param [in,out]	str	The string to convert.
template <typename StringType>
void to_upper(StringType& str)
{
  std::transform(std::begin(str), std::end(str), std::begin(str), ::toupper);
}

/// Use the string hasher to find hash
inline auto hash(const char* v, uint32_t length, uint32_t seed = 0)
{
  return acl::wyhash32()(v, length);
}

/// Use the string hasher to find hash
inline auto hash(std::string_view v, uint32_t seed = 0)
{
  return hash(v.data(), (uint32_t)v.length(), seed);
}

/// @brief Separates a string pair of the format Abc:efg into 'Abc' and 'efg', if the separator is not present the
/// returned pair has the first element filled with empty string if is_prefix is true, if is_prefix is false, the second
/// string is set to empty
inline string_view_pair split(std::string_view name, char by = ':', bool is_prefix = true) noexcept
{
  size_t seperator = name.find_first_of(by);
  if (seperator < name.size())
  {
    return string_view_pair(name.substr(0, seperator), name.substr(seperator + 1));
  }
  else if (is_prefix)
  {
    return string_view_pair(std::string_view(), name);
  }
  else
  {
    return string_view_pair(name, std::string_view());
  }
}

/// @brief Separates a string pair of the format Abc:efg into 'Abc' and 'efg', if the separator is not present the
/// returned pair has the first element filled with empty string if is_prefix is true, if is_prefix is false, the second
/// string is set to empty. The search begins from the end unlike in acl::split
inline string_view_pair split_last(const std::string_view& name, char by = ':', bool is_prefix = true)
{
  size_t seperator = name.find_last_of(by);
  if (seperator < name.size())
  {
    return string_view_pair(name.substr(0, seperator), name.substr(seperator + 1));
  }
  else if (is_prefix)
  {
    return string_view_pair(std::string_view(), name);
  }
  else
  {
    return string_view_pair(name, std::string_view());
  }
}

inline auto tokenize(std::string const& value, std::string const& seperators) -> std::vector<std::string_view>
{
  std::vector<std::string_view> ret;
  size_t                        what = std::numeric_limits<size_t>::max();
  do
  {
    size_t start = what + 1;
    what         = value.find_first_of(seperators, start);
    ret.push_back(std::string_view(value.c_str() + start, what));
  }
  while (what != std::string::npos);
  return ret;
}

inline std::string tokenize_word_list(std::string const& value, std::string const& seperators)
{
  std::string ret;
  size_t      what = std::numeric_limits<size_t>::max();
  do
  {
    size_t start = what + 1;
    what         = value.find_first_of(seperators, start);
    word_push_back(ret, (value.substr(start, what)));
  }
  while (what != std::string::npos);
  return ret;
}

/// @brief trim leading white space
inline void trim_leading(std::string& str)
{
  size_t endpos = str.find_first_not_of(" \t\n\r");
  if (endpos != 0)
    str.erase(0, endpos);
}

/// @brief trim trailing white space
inline void trim_trailing(std::string& str)
{
  size_t endpos = str.find_last_not_of(" \t\n\r");
  if (endpos != std::string::npos)
    str.erase(endpos + 1);
}

inline void trim(std::string& str)
{
  trim_leading(str);
  trim_trailing(str);
}

/// @brief trim leading white space
inline auto trim_leading(std::string_view str)
{
  size_t endpos = str.find_first_not_of(" \t\n\r");
  if (endpos != 0)
    str = str.substr(endpos);
  return str;
}

/// @brief trim trailing white space
inline auto trim_trailing(std::string_view str)
{
  size_t endpos = str.find_last_not_of(" \t\n\r");
  if (endpos != std::string::npos)
    str = str.substr(0, endpos + 1);
  return str;
}

inline std::string_view trim(std::string_view str)
{
  str = trim_leading(str);
  str = trim_trailing(str);
  return str;
}

/// Unicode conversion UTF-8 to UTF-16
ACL_API void        to_utf16(std::wstring& oString, std::string_view utf8_str);
ACL_API void        to_utf8(std::string& oString, std::wstring_view utf16_str);
inline std::wstring to_utf16(std::string_view iutf8)
{
  std::wstring value;
  to_utf16(value, iutf8);
  return value;
}
inline std::string to_utf8(std::wstring_view iutf16)
{
  std::string value;
  to_utf8(value, iutf16);
  return value;
}
inline std::string_view to_utf8(std::string_view iutf8)
{
  return iutf8;
}

inline bool is_ascii(std::string const& utf8_str)
{
  return std::find_if(utf8_str.begin(), utf8_str.end(),
                      [](char a) -> bool
                      {
                        return !isascii(a);
                      }) == utf8_str.end();
}

inline bool is_tagged(std::string const& value)
{
  return value.find_first_of(':') != std::string::npos;
}

inline bool is_tagged(std::string const& value, std::string const& tag, std::string& outStr)
{
  string_view_pair tagVal = split(value, ':');
  if (tagVal.first.compare(tag.c_str()) == 0)
  {
    outStr = tagVal.second;
    return true;
  }
  return false;
}

inline std::string get_tagged_val(std::string const& tag, word_list<>::iterator it)
{
  std::string_view value;
  while (it.has_next(value))
  {
    if (value.length())
    {
      string_view_pair tagVal = split(value, ':');
      if (tagVal.first.compare(tag.c_str()) == 0)
      {
        return std::string(tagVal.second);
      }
    }
  }
  return {};
}

template <typename StringType>
inline bool ends_with(const StringType& str, const StringType& suffix)
{
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

template <typename StringType>
inline bool starts_with(const StringType& str, const StringType& prefix)
{
  return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline int32_t length_utf32(const utf32* b)
{
  const utf32* s = b;
  while (*b)
    ++b;
  return (int32_t)(b - s);
}

// returns the advance amount in the buffer,
// invalid codepoints will be replaced by default uchar
inline int32_t decode_utf8(const utf8* buffer, int32_t length, utf32& c)
{
  if (buffer == 0)
  {
    return 0;
  }
  utf8 scan = buffer[0];
  if (scan < 0x80)
  {
    c = scan;
    return 1;
    ;
  }
  else if (scan < 0xE0 && length >= 2)
  {
    c = ((scan & 0x1F) << 6);
    c |= (buffer[1] & 0x3F);
    return 2;
  }
  else if (scan < 0xF0 && length >= 3)
  {
    c = ((scan & 0x0F) << 12);
    c |= ((buffer[1] & 0x3F) << 6);
    c |= (buffer[2] & 0x3F);
    return 3;
  }
  else if (length >= 4)
  {
    c = ((scan & 0x07) << 18);
    c |= ((buffer[1] & 0x3F) << 12);
    c |= ((buffer[2] & 0x3F) << 6);
    c |= (buffer[3] & 0x3F);
    return 4;
  }
  c = k_default_uchar;
  return -1;
}

inline int32_t fast_decode_utf8(const utf8* src, int32_t srcLen, utf32* dest, int32_t destLen)
{
  int32_t dc = destLen;

  for (int32_t idx = 0; ((idx < srcLen) && (dc > 0));)
  {
    utf32 cp;
    utf8  cu = src[idx++];

    if (cu < 0x80)
    {
      cp = (utf32)(cu);
    }
    else if (cu < 0xE0)
    {
      cp = ((cu & 0x1F) << 6);
      cp |= (src[idx++] & 0x3F);
    }
    else if (cu < 0xF0)
    {
      cp = ((cu & 0x0F) << 12);
      cp |= ((src[idx++] & 0x3F) << 6);
      cp |= (src[idx++] & 0x3F);
    }
    else
    {
      cp = ((cu & 0x07) << 18);
      cp |= ((src[idx++] & 0x3F) << 12);
      cp |= ((src[idx++] & 0x3F) << 6);
      cp |= (src[idx++] & 0x3F);
    }

    *dest++ = cp;
    --dc;
  }

  return destLen - dc;
}

inline int32_t decode_length_utf8(const utf8* buf, int32_t len)
{
  utf8    tcp;
  int32_t count = 0;
  while (len-- > 0)
  {
    tcp = *buf++;
    ++count;
    if (tcp < 0x80)
    {
    }
    else if (tcp < 0xE0)
    {
      --len;
      ++buf;
    }
    else if (tcp < 0xF0)
    {
      len -= 2;
      buf += 2;
    }
    else
    {
      len -= 2;
      buf += 3;
    }
  }
  return count;
}

template <typename NameValueMap>
inline std::string const& find(const NameValueMap& nvm, std::string const& name, std::string const& default_value = {})
{
  auto it = nvm.find(name);
  if (it != nvm.end())
  {
    return (*it).second;
  }
  return default_value;
}

template <typename HashValueMap>
inline std::string const& find(const HashValueMap& nvm, uint32_t name, std::string const& default_value = {})
{
  auto it = nvm.find(name);
  if (it != nvm.end())
  {
    return (*it).second;
  }
  return default_value;
}

template <typename HashValueMap>
inline std::string const& find(const HashValueMap& nvm, std::string const& name, std::string const& default_value = {})
  requires(std::same_as<typename HashValueMap::key_type, uint32_t>)
{
  auto it = nvm.find(hash(name));
  if (it != nvm.end())
  {
    return (*it).second;
  }
  return default_value;
}

inline uint32_t word_wrap(std::ostream& out, uint32_t indent, uint32_t width, std::string const& line)
{
  size_t             formatted = 0;
  std::string        word;
  std::istringstream words(line);
  while (words >> word)
  {
    if (word.size() + formatted > width)
    {
      out << '\n' << std::setw(indent) << ' ';
      formatted = 0;
    }
    out << word << ' ';
    formatted += word.size() + 1;
  }
  return static_cast<uint32_t>(formatted);
}
inline uint32_t word_wrap_multiline(std::ostream& out, uint32_t indent, uint32_t width, std::string const& input)
{
  std::stringstream s(input);
  std::string       line;
  std::getline(s, line);
  uint32_t last = word_wrap(out, indent, width, line);
  while (std::getline(s, line))
  {
    out << '\n' << std::setw(indent) << ' ';
    last = word_wrap(out, indent, width, line);
  }
  return std::min(width, last);
}
template <typename StringType>
bool is_number(const StringType& s)
{
  return !s.empty() && std::find_if(s[0] == '-' ? s.begin() + 1 : s.begin(), s.end(),
                                    [](char c)
                                    {
                                      return !std::isdigit(c);
                                    }) == s.end();
}
template <typename StringType>
std::string as_indexed_name(const StringType& s, uint32_t i)
{
  std::string name = s;
  name += '[';
  name += std::to_string(i);
  name += ']';
  return name;
}
template <typename StringType>
std::string join(StringType first, StringType second)
{
  std::string name = first;
  name += '.';
  name += second;
  return name;
}

template <typename StringType>
bool has_space(StringType first)
{
  auto end = std::end(first);
  for (auto it = std::begin(first); it != end; ++it)
    if (std::isspace(*it))
      return true;
  return false;
}

} // namespace acl
