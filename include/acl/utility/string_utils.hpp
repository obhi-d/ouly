#pragma once

#include "common.hpp"
#include "word_list.hpp"
#include <acl/utility/wyhash.hpp>
#include <algorithm>
#include <cstddef>
#include <ctime>
#include <regex>
#include <sstream>
#include <vector>

/**
 * @file string_utils.hpp
 * @brief Utility functions for string manipulation and processing
 *
 * This header provides a comprehensive set of string manipulation utilities including:
 * - String searching and modification (index_of, contains, replace)
 * - Case conversion (to_lower, to_upper)
 * - String splitting and tokenization (split, split_last, tokenize)
 * - Time string formatting (time_stamp, time_string)
 * - String trimming and whitespace handling (trim, trim_leading, trim_trailing)
 * - Word wrapping utilities (word_wrap, word_wrap_multiline)
 * - Unicode and ASCII handling
 * - Regular expression utilities
 *
 * The utilities support various string types including std::string, std::string_view,
 * and Unicode encodings (UTF-8, UTF-16, UTF-32).
 *
 * Constants:
 * - k_default_uchar: Default Unicode character (0xFFFD)
 * - k_wrong_uchar: Invalid Unicode character marker (0xFFFF)
 * - k_last_uchar: Maximum Unicode codepoint (0x10FFFF)
 * - k_default: Default string value
 * - k_default_sym: Default symbol (*)
 *
 * @note This utility header is part of the ACL (Abstract Core Library) namespace
 * @note All functions are designed to be exception-safe and performance-oriented
 */
namespace acl
{

using string_view_pair = std::pair<std::string, std::string>;

static constexpr inline int32_t                k_default_uchar = 0xFFFD;
static constexpr inline int32_t                k_wrong_uchar   = 0xFFFF;
static constexpr inline int32_t                k_last_uchar    = 0x10FFFF;
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
inline auto index_of(std::string const& in, std::string_view to_find) -> uint32_t
{
  return word_list<>::index_of(in, to_find);
}

/**
 * @brief Check if string is present
 * @param value The string to find
 * @param in The word list
 *
 */
inline auto contains(std::string const& in, std::string const& to_find) -> bool
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

inline auto time_stamp() -> std::string
{
  std::time_t      t               = std::time(nullptr);
  constexpr size_t max_size        = 32;
  char             mbstr[max_size] = {};
#ifdef _MSC_VER
  struct tm buf;
  localtime_s(&buf, &t);
  std::strftime(mbstr, sizeof(mbstr), "%m-%d-%y_%H-%M-%S", &buf);
#else
  struct tm buf{};
  std::strftime(static_cast<char*>(mbstr), sizeof(mbstr), "%m-%d-%y_%H-%M-%S", localtime_r(&t, &buf));
#endif
  return {static_cast<char const*>(mbstr)};
}

inline auto time_string() -> std::string
{
  std::time_t      t               = std::time(nullptr);
  constexpr size_t max_size        = 32;
  char             mbstr[max_size] = {};
#ifdef _MSC_VER
  struct tm buf;
  localtime_s(&buf, &t);
  std::strftime(mbstr, sizeof(mbstr), "%H-%M-%S", &buf);
#else
  struct tm buf{};
  std::strftime(static_cast<char*>(mbstr), sizeof(mbstr), "%H-%M-%S", localtime_r(&t, &buf));
#endif
  return {static_cast<char const*>(mbstr)};
}

template <class OutputIt, class BidirIt, class Traits, class CharT, class UnaryFunction>
auto regex_replace(OutputIt out, BidirIt first, BidirIt last, const std::basic_regex<CharT, Traits>& re,
                   UnaryFunction f) -> OutputIt
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

  std::regex_iterator<BidirIt> begin(first, last, re);
  std::regex_iterator<BidirIt> end;
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

inline auto indent(int32_t amt) -> std::string
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
inline auto replace_first(std::string& source, std::string_view search, std::string_view replace, size_t start_pos = 0)
 -> bool
{
  assert(!search.empty());

  if (size_t b = source.find(search, start_pos); b != std::string::npos)
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
inline auto replace(std::string& source, std::string_view search, std::string_view replace, size_t start_pos = 0)
 -> uint32_t

{
  if (search.empty())
  {
    return 0;
  }
  uint32_t count = 0;

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
ACL_API auto format_name(std::string const& str) -> std::string;

/**
 * @brief	Converts string to lower case.
 * @param [in,out]	str	The string to convert.
 */
template <typename StringType>
inline auto to_lower(StringType& str) -> StringType&
{
  std::transform(std::begin(str), std::end(str), std::begin(str), ::tolower);
  return str;
}

/**
 * @brief	Converts string to upper case.
 * @param [in,out]	str	The string to convert.
 */
template <typename StringType>
inline auto to_upper(StringType& str) -> StringType&
{
  std::transform(std::begin(str), std::end(str), std::begin(str), ::toupper);
  return str;
}

/**
 * Use the string hasher to find hash
 */
inline auto hash(std::string_view v, uint32_t seed = acl::wyhash32_default_prime_seed)
{
  return acl::wyhash32(seed)(v.data(), (uint32_t)v.length());
}

/**
 * @brief Separates a string pair of the format Abc:efg into 'Abc' and 'efg', if the separator is not present the
 * returned pair has the second element filled with empty string if is_prefix is true, if is_prefix is false, the
 * first string is set to empty
 */
inline auto split(std::string_view name, char by = ':', bool is_prefix = true) noexcept -> string_view_pair
{
  size_t seperator = name.find_first_of(by);
  if (seperator < name.size())
  {
    return string_view_pair(name.substr(0, seperator), name.substr(seperator + 1));
  }
  if (is_prefix)
  {
    return string_view_pair(name, std::string_view());
  }

  return string_view_pair(std::string_view(), name);
}

/**
 * @brief Separates a string pair of the format Abc:efg into 'Abc' and 'efg', if the separator is not present the
 * returned pair has the second element filled with empty string if is_prefix is true, if is_prefix is false, the
 * first string is set to empty. The search begins from the end unlike in acl::split
 */
inline auto split_last(const std::string_view& name, char by = ':', bool is_prefix = true) -> string_view_pair
{
  size_t seperator = name.find_last_of(by);
  if (seperator < name.size())
  {
    return string_view_pair(name.substr(0, seperator), name.substr(seperator + 1));
  }
  if (is_prefix)
  {
    return string_view_pair(name, std::string_view());
  }

  return string_view_pair(std::string_view(), name);
}

enum class response : uint8_t
{
  e_ok,
  e_cancel,
  e_continue
};

template <typename A>
inline auto tokenize(A acceptor, std::string_view value, std::string_view seperators) -> response
{
  response r    = response::e_ok;
  size_t   what = std::numeric_limits<size_t>::max();
  while (true)
  {
    size_t start = what + 1;
    what         = value.find_first_of(seperators, start);
    auto end     = what == std::string_view::npos ? value.length() : what;
    if (end > start)
    {
      r = acceptor(start, end, what == std::string_view::npos ? 0 : value[what]);
      if (r != response::e_continue)
      {
        return r;
      }
    }
    if (what == std::string_view::npos)
    {
      break;
    }
  }

  return r;
}

/**
 * @brief trim leading white space
 */
inline auto trim_leading(std::string_view str)
{
  size_t endpos = str.find_first_not_of(" \t\n\r");
  if (endpos != 0)
  {
    str = str.substr(endpos);
  }
  return str;
}

/**
 * @brief trim trailing white space
 */
inline auto trim_trailing(std::string_view str)
{
  size_t endpos = str.find_last_not_of(" \t\n\r");
  if (endpos != std::string::npos)
  {
    str = str.substr(0, endpos + 1);
  }
  return str;
}

/**
 * @brief Removes leading and trailing whitespace from a string.
 *
 * This function trims both leading and trailing whitespace characters from the input string
 * by calling trim_leading and trim_trailing in sequence.
 *
 * @param str The string_view to be trimmed
 * @return std::string_view The trimmed string
 */
inline auto trim(std::string_view str) -> std::string_view
{
  str = trim_leading(str);
  str = trim_trailing(str);
  return str;
}

/**
 * @brief Checks if a string contains only ASCII characters
 *
 * Iterates through each character in the string and verifies if it falls within
 * the ASCII character set (0-127).
 *
 * @param utf8_str The string view to check for ASCII characters
 * @return true if all characters in the string are ASCII
 * @return false if any character in the string is not ASCII
 */
inline auto is_ascii(std::string_view utf8_str) -> bool
{
  return std::ranges::find_if(utf8_str,
                              [](char a) -> bool
                              {
                                return isascii(a) == 0;
                              }) == utf8_str.end();
}

/**
 * @brief Word wraps text to a specified width, handling tabs
 *
 * Takes a line of text and wraps it at word boundaries to ensure each line is no longer
 * than the specified width. Tabs are converted to spaces according to tab_width.
 *
 * @tparam L Callable type that accepts two size_t parameters (line start and end positions)
 * @param line_accept Callback function called for each wrapped line with (start, end) positions
 * @param width Maximum width in characters for each line
 * @param line Input text to be word-wrapped
 * @param tab_width Number of spaces to substitute for each tab character (default: 2)
 *
 * The function processes the input text token by token, keeping track of tabs and line width.
 * When a line exceeds the specified width, it calls the line_accept callback with the
 * current line's start and end positions, then starts a new line.
 */
template <typename L>
inline void word_wrap(L line_accept, uint32_t width, std::string_view line, uint32_t tab_width = 2)
{
  size_t   line_start = 0;
  size_t   line_end   = 0;
  uint32_t nb_tabs    = 0;

  tokenize(
   [&](std::size_t token_start, std::size_t token_end, char token) -> response
   {
     if (token == '\t')
     {
       nb_tabs++;
     }
     auto line_width = (token_end - line_start) + (static_cast<unsigned long>(nb_tabs * tab_width));
     if (line_width >= width)
     {
       line_accept(line_start, line_end);
       line_start = line_end;
       nb_tabs    = 0;
     }
     line_end = token_end;
     return response::e_continue;
   },
   line, " \t");

  line_accept(line_start, line.length());
}

/**
 * @brief Wraps text into multiple lines respecting both line width and existing line breaks
 *
 * Takes a multiline text input and performs word wrapping on each line individually,
 * preserving original line breaks while ensuring no line exceeds the specified width.
 *
 * @tparam L Callable type that accepts line start and end positions
 * @param line_accept Callback function called for each wrapped line with (start, end) positions
 * @param width Maximum width of each line in characters
 * @param input Input text to be wrapped
 * @param tab_width Number of spaces a tab character represents (default: 2)
 *
 * The line_accept callback receives:
 * - start position of the line in the original input
 * - end position of the line in the original input
 */
template <typename L>
inline void word_wrap_multiline(L line_accept, uint32_t width, std::string_view input, uint32_t tab_width = 2)
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
     return response::e_continue;
   },
   input, "\n");
}

template <typename StringType>
auto is_number(const StringType& s) -> bool
{
  return !s.empty() && std::find_if(s[0] == '-' ? s.begin() + 1 : s.begin(), s.end(),
                                    [](char c)
                                    {
                                      return !std::isdigit(c);
                                    }) == s.end();
}

} // namespace acl
