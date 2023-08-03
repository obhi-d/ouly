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

/**
 * @brief Index of string
 * @param to_find The string to find
 * @param in The word list
 *
 */
inline uint32_t index_of(std::string const& in, std::string_view to_find)
{
  return word_list<>::index_of(in, to_find);
}

/**
 * @brief Check if string is present
 * @param value The string to find
 * @param in The word list
 *
 */
inline bool contains(std::string const& in, std::string const& to_find)
{
  return in.find(to_find) != std::string::npos;
}

/**
 * @brief Push the string at the very end of the buffer
 * @param in The string array container
 * @param word The string to push
 *
 */
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

inline std::string indent(int32_t amt)
{
  std::string s;
  s.resize(amt, ' ');
  return s;
}

/**
 * @brief	Replace first occurence of a substring in a string with another string in-place.
 * @return	true if string was replaced.
 *
 */
inline bool replace_first(std::string& source, std::string_view search, std::string_view replace)
{
  assert(!search.empty());
  size_t b = 0;
  if (((b = source.find(search, b)) != std::string::npos))
  {
    source.replace(b, search.size(), replace);
    return true;
  }

  return false;
}

/**
 * @brief	Replace all occurences of a substring in a string with another string in-place.
 * @return	Number of replacements
 *
 */
inline uint32_t replace(std::string& source, std::string_view search, std::string_view replace)

{
  if (search.empty())
    return 0;
  uint32_t count = 0;

  size_t start_pos = 0;
  while ((start_pos = source.find(search, start_pos)) != std::string::npos)
  {
    source.replace(start_pos, search.length(), replace);
    start_pos += replace.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    count++;
  }
  return count;
}

/**
 * Formats a camel or upper case name into a formatted string.
 * @example
 * "madeInChina" to "Made In China" or
 * MADE_IN_CHINA to "Made In China"
 */
ACL_API std::string format_name(std::string const& str);

/**
 * @brief	Converts string to lower case.
 * @param [in,out]	str	The string to convert.
 */
template <typename StringType>
inline StringType& to_lower(StringType& str)
{
  std::transform(std::begin(str), std::end(str), std::begin(str), ::tolower);
  return str;
}

/**
 * @brief	Converts string to upper case.
 * @param [in,out]	str	The string to convert.
 */
template <typename StringType>
inline StringType& to_upper(StringType& str)
{
  std::transform(std::begin(str), std::end(str), std::begin(str), ::toupper);
  return str;
}

/**
 * Use the string hasher to find hash
 */
inline auto hash(std::string_view v, uint32_t seed = 1337)
{
  return acl::wyhash32(seed)(v.data(), (uint32_t)v.length());
}

/**
 * @brief Separates a string pair of the format Abc:efg into 'Abc' and 'efg', if the separator is not present the
 * returned pair has the second element filled with empty string if is_prefix is true, if is_prefix is false, the
 * first string is set to empty
 */
inline string_view_pair split(std::string_view name, char by = ':', bool is_prefix = true) noexcept
{
  size_t seperator = name.find_first_of(by);
  if (seperator < name.size())
  {
    return string_view_pair(name.substr(0, seperator), name.substr(seperator + 1));
  }
  else if (is_prefix)
  {
    return string_view_pair(name, std::string_view());
  }
  else
  {
    return string_view_pair(std::string_view(), name);
  }
}

/**
 * @brief Separates a string pair of the format Abc:efg into 'Abc' and 'efg', if the separator is not present the
 * returned pair has the second element filled with empty string if is_prefix is true, if is_prefix is false, the
 * first string is set to empty. The search begins from the end unlike in acl::split
 */
inline string_view_pair split_last(const std::string_view& name, char by = ':', bool is_prefix = true)
{
  size_t seperator = name.find_last_of(by);
  if (seperator < name.size())
  {
    return string_view_pair(name.substr(0, seperator), name.substr(seperator + 1));
  }
  else if (is_prefix)
  {
    return string_view_pair(name, std::string_view());
  }
  else
  {
    return string_view_pair(std::string_view(), name);
  }
}

template <typename A>
inline void tokenize(A&& acceptor, std::string_view value, std::string_view seperators)
{
  size_t what = std::numeric_limits<size_t>::max();
  do
  {
    size_t start = what + 1;
    what         = value.find_first_of(seperators, start);
    auto end     = what == value.npos ? value.length() : what;
    if (end > start)
      acceptor(start, end, what == value.npos ? 0 : value[what]);
  }
  while (what != std::string::npos);
}

/**
 * @brief trim leading white space
 */
inline auto trim_leading(std::string_view str)
{
  size_t endpos = str.find_first_not_of(" \t\n\r");
  if (endpos != 0)
    str = str.substr(endpos);
  return str;
}

/**
 * @brief trim trailing white space
 */
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

inline bool is_ascii(std::string_view utf8_str)
{
  return std::find_if(utf8_str.begin(), utf8_str.end(),
                      [](char a) -> bool
                      {
                        return !isascii(a);
                      }) == utf8_str.end();
}

template <typename L>
inline void word_wrap(L&& line_accept, uint32_t width, std::string_view line, uint32_t tab_width = 2)
{
  size_t   line_start = 0;
  size_t   line_end   = 0;
  uint32_t nb_tabs    = 0;

  tokenize(
    [&](std::size_t token_start, std::size_t token_end, char token)
    {
      if (token == '\t')
        nb_tabs++;
      auto line_width = (token_end - line_start) + nb_tabs * tab_width;
      if (line_width > width)
      {
        line_accept(line_start, line_end);
        line_start = line_end;
        nb_tabs    = 0;
      }
      line_end = token_end;
    },
    line, " \t");

  line_accept(line_start, line.length());
}

template <typename L>
inline void word_wrap_multiline(L&& line_accept, uint32_t width, std::string_view input, uint32_t tab_width = 2)
{
  tokenize(
    [&](std::size_t token_start, std::size_t token_end, char)
    {
      word_wrap(
        [&line_accept, &token_start](std::size_t line_start, std::size_t line_end)
        {
          line_accept(line_start + token_start, line_end + token_start);
        },
        width, input.substr(token_start, token_end - token_start), tab_width);
    },
    input, "\n");
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

} // namespace acl
