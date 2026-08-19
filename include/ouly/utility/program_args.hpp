// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/utility/from_chars.hpp"
#include <algorithm>
#include <any>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

/**
 * @file program_args.hpp
 * @brief A flexible command-line argument parsing utility.
 *
 * This header provides the program_args class template for parsing and managing
 * command-line arguments in C++ applications. It supports various argument types
 * including scalars, booleans, arrays, and strings with features such as:
 *
 * - Support for both long (--argument) and short (-a) argument formats
 * - Support for equals, space-separated, grouped-short, and attached-short values
 * - Positional arguments and the `--` end-of-options marker
 * - Type-safe argument parsing and conversion
 * - Documentation generation
 * - Automatic help message generation
 * - Flexible argument declaration and value retrieval
 *
 * Example usage:
 * @code
 * ouly::program_args args;
 * args.parse_args(argc, argv);
 * auto arg = args.decl<int>("number", "n").doc("A number argument");
 * if (auto value = arg.value()) {
 *     // use *value
 * }
 * @endcode
 *
 * The utility supports several argument type concepts:
 * - ProgramDocFormatter: For formatting documentation output
 * - ProgramArgScalarType: For integral and floating-point types
 * - ProgramArgBoolType: For boolean flags
 * - ProgramArgArrayType: For container types
 *
 * @tparam StringType The string type used for storing names and documentation
 *                    (defaults to std::string_view)
 */

namespace ouly
{
enum class program_document_type : uint8_t
{
  brief_doc,
  full_doc,
  arg_doc
};

template <typename T>
concept ProgramDocFormatter = requires(T a, program_document_type f) { a(f, "", "", ""); };
template <typename V>
concept ProgramArgScalarType = (std::integral<V> || std::floating_point<V>) && !std::same_as<V, bool>;
template <typename V>
concept ProgramArgBoolType = std::is_same_v<bool, V>;
template <typename V, typename S>
concept ProgramArgArrayType = requires(V a) {
  typename V::value_type;
  a.push_back(typename V::value_type());
} && !std::same_as<V, std::basic_string<typename S::value_type>>;

template <typename StringType = std::string_view>
/**
 * @brief A utility class for parsing and managing program arguments.
 *
 * This class provides methods to parse command-line arguments, define argument declarations,
 * and retrieve argument values with optional type casting.
 *
 * @tparam StringType The string type used for argument names and values (default is std::string_view).
 */
class program_args
{
  using value_type             = std::any;
  static constexpr int is_flag = -1;
  static constexpr int no_flag = -2; // doesnt have a flag
  struct arg
  {
    value_type value_;
    StringType doc_;
    StringType name_;
    int        flag_ = no_flag; // is_flag (-1): this argument is a flag, no_flag (-2): no associated flag,
                                // otherwise: index of the associated flag argument

    constexpr arg() noexcept = default;
    constexpr arg(StringType name) noexcept : name_(name) {}
  };

  struct positional_arg
  {
    StringType value_;
    bool       consumed_ = false;
  };

  struct option_occurrence
  {
    StringType                name_;
    std::optional<StringType> value_;
    std::optional<StringType> attached_value_;
    std::optional<size_t>     following_value_;
  };

public:
  template <typename V>
  class arg_decl
  {
  public:
    auto doc(StringType h) noexcept -> auto&
    {
      if (std::empty(h))
      {
        return *this;
      }

      args()[arg_].doc_ = h;
      return *this;
    }

    operator bool() const noexcept
    {
      if (args()[arg_].value_.has_value())
      {
        auto outp = std::any_cast<bool>(&args()[arg_].value_);
        return outp && *outp;
      }
      return false;
    }

    auto value() const noexcept -> std::optional<V>
    {
      if (args()[arg_].value_.has_value())
      {
        auto outp = std::any_cast<V>(&args()[arg_].value_);
        if (outp)
        {
          return *outp;
        }
      }
      return {};
    }

    template <typename T>
    auto sink(T& value) const noexcept -> bool
    {
      if constexpr (std::is_pointer_v<T>)
      {
        return sink_ref(value);
      }
      else
      {
        return sink_copy(value);
      }
    }

  private:
    auto sink_copy(V& store) const noexcept -> bool
    {
      if (args()[arg_].value_.has_value())
      {
        auto outp = std::any_cast<V>(&args()[arg_].value_);
        if (outp)
        {
          store = *outp;
          return true;
        }
      }
      return false;
    }

    auto sink_ref(V*& store) const noexcept -> bool
    {
      if (args()[arg_].value_.has_value())
      {
        auto outp = std::any_cast<V>(&args()[arg_].value_);
        if (outp)
        {
          store = outp;
          return true;
        }
      }
      return false;
    }

    friend class program_args;

    arg_decl(std::vector<arg>& a, size_t i) noexcept : p_args_(&a), arg_(i) {}

    auto args() -> std::vector<arg>&
    {
      return *p_args_;
    }

    auto args() const -> std::vector<arg> const&
    {
      return *p_args_;
    }

    std::vector<arg>* p_args_;
    size_t            arg_;
  };

  program_args() noexcept = default;

  /**
   * @brief Parse C main command line arguments.
   *
   * The first argument is treated as the executable name, as required by the
   * C and C++ conventions for argc/argv. Both `--name=value` and
   * `--name value` forms are accepted, as are their short-option equivalents.
   * `--` stops option parsing.
   */
  void parse_args(int argc, char const* const* argv) noexcept
  {
    if (argc <= 1 || argv == nullptr)
    {
      return;
    }

    bool parse_options = true;
    for (int i = 1; i < argc; ++i)
    {
      if (argv[i] == nullptr)
      {
        continue;
      }

      StringType token(argv[i]);
      if (parse_options && token == "--")
      {
        parse_options = false;
        continue;
      }

      if (!parse_options || !is_option(token))
      {
        if (parse_options && token.find('=') != token.npos)
        {
          // Preserve the legacy unprefixed name=value spelling.
          parse_option(token, 0, {});
        }
        else
        {
          add_positional(token);
        }
        continue;
      }

      auto following = std::optional<size_t>();
      if (!has_inline_value(token) && i + 1 < argc && argv[i + 1] != nullptr)
      {
        StringType next(argv[i + 1]);
        if (is_value(next))
        {
          following = add_positional(next);
          ++i;
        }
      }
      parse_option(token, option_prefix_size(token), following);
    }
  }

  /**
   * @brief Parse a single arg
   */
  void parse_arg(StringType asv) noexcept
  {
    if (asv == "--")
    {
      return;
    }
    if (is_option(asv))
    {
      parse_option(asv, option_prefix_size(asv), {});
    }
    else if (asv.find('=') != asv.npos)
    {
      parse_option(asv, 0, {});
    }
    else
    {
      // parse_arg historically accepts a normalized, unprefixed option name.
      add_occurrence(asv, {}, {}, {});
    }
  }

  /**
   * @brief Return positional arguments that were not consumed as option values.
   *
   * Values after `--` are always positional. Since declarations provide the
   * option types, call this after declaring or sinking options.
   */
  [[nodiscard]] auto get_positional_args() const -> std::vector<StringType>
  {
    std::vector<StringType> result;
    result.reserve(positional_args_.size());
    for (auto const& positional : positional_args_)
    {
      if (!positional.consumed_)
      {
        result.push_back(positional.value_);
      }
    }
    return result;
  }

  void brief(StringType h) noexcept
  {
    brief_ = h;
  }

  void doc(StringType h) noexcept
  {
    docs_.push_back(h);
  }

  template <typename V = StringType>
  auto decl(StringType name, StringType flag = StringType()) noexcept -> arg_decl<V>
  {
    // Resolve arg
    auto decl_arg = add(name);
    int  flag_arg = no_flag;
    auto length   = static_cast<uint32_t>(name.size());
    if (!flag.empty())
    {
      flag_arg                   = (int)add(flag);
      arguments_[flag_arg].flag_ = is_flag;
      length += static_cast<uint32_t>(flag.size()) + 2;
    }
    arguments_[decl_arg].flag_ = flag_arg;
    auto occurrence            = find_occurrence(name, flag);
    if (occurrence != nullptr)
    {
      auto raw_value = occurrence->value_;
      if constexpr (ProgramArgBoolType<V>)
      {
        if (raw_value)
        {
          auto value = convert_to<V>(*raw_value);
          if (value)
          {
            arguments_[decl_arg].value_ = *value;
          }
          else
          {
            arguments_[decl_arg].value_.reset();
          }
        }
        else
        {
          arguments_[decl_arg].value_ = true;
        }
      }
      else
      {
        consume_following_values(name, flag);
        if (!raw_value && occurrence->attached_value_)
        {
          raw_value = occurrence->attached_value_;
        }
        if (!raw_value && occurrence->following_value_)
        {
          auto const positional_index = *occurrence->following_value_;
          if (positional_index < positional_args_.size())
          {
            positional_args_[positional_index].consumed_ = true;
            raw_value                                    = positional_args_[positional_index].value_;
          }
        }

        if (raw_value)
        {
          if constexpr (std::same_as<StringType, V>)
          {
            arguments_[decl_arg].value_ = *raw_value;
          }
          else
          {
            auto value = convert_to<V>(*raw_value);
            if (value)
            {
              arguments_[decl_arg].value_ = *value;
            }
            else
            {
              arguments_[decl_arg].value_.reset();
            }
          }
        }
        else
        {
          arguments_[decl_arg].value_.reset();
        }
      }
    }
    max_arg_length_ = std::max(length, max_arg_length_);
    return arg_decl<V>(arguments_, decl_arg);
  }

  template <typename T>
  auto sink(T& value, StringType name, StringType flag = StringType(), StringType docu = StringType()) -> bool
  {
    // Resolve arg
    return decl<T>(name, flag).doc(docu).sink(value);
  }

  auto get(StringType name, StringType flag = StringType(), StringType docu = StringType()) -> bool
  {
    // Resolve arg
    bool value  = false;
    bool result = decl<bool>(name, flag).doc(docu).sink(value);
    return result && value;
  }

  template <typename T>
  auto as(StringType name, StringType flag = StringType(), StringType docu = StringType()) -> std::optional<T>
  {
    // Resolve arg
    T value = {};
    if (decl<bool>(name, flag).doc(docu).sink(value))
    {
      return std::optional<T>{value};
    }
    return std::nullopt;
  }

  template <ProgramDocFormatter Formatter>
  void doc(Formatter formatter) const noexcept
  {
    if (!brief_.empty())
    {
      formatter(program_document_type::brief_doc, "Usage", "", brief_);
    }
    for (auto d : docs_)
    {
      formatter(program_document_type::full_doc, "Description", "", d);
    }
    for (auto const& a : arguments_)
    {
      if (a.flag_ != is_flag)
      {
        if (a.flag_ != no_flag && (size_t)a.flag_ < arguments_.size())
        {
          formatter(program_document_type::arg_doc, a.name_, arguments_[(size_t)a.flag_].name_, a.doc_);
        }
        else
        {
          formatter(program_document_type::arg_doc, a.name_, "", a.doc_);
        }
      }
    }
  }

  [[nodiscard]] auto get_max_arg_length() const noexcept -> std::size_t
  {
    return max_arg_length_;
  }

  [[nodiscard]] auto must_print_help() const noexcept -> bool
  {
    return print_help_;
  }

private:
  template <ProgramArgArrayType<StringType> V>
  static auto convert_to(StringType const& sv) noexcept -> std::optional<V>
  {
    V vector;
    using v_value_type = typename V::value_type;
    if (sv.size() < 2 || sv.front() != '[' || sv.back() != ']')
    {
      return {};
    }

    auto contents = sv.substr(1, sv.size() - 2);
    while (!contents.empty())
    {
      auto comma = contents.find(',');
      auto value = trim(contents.substr(0, comma));
      if (value.empty())
      {
        return {};
      }
      auto converted = convert_to<v_value_type>(value);
      if (!converted)
      {
        return {};
      }
      vector.push_back(*converted);
      if (comma == contents.npos)
      {
        break;
      }
      contents = contents.substr(comma + 1);
    }
    return vector;
  }

  template <typename V>
  static auto convert_to(StringType const& sv) noexcept -> std::optional<V>
  {
    return V(sv);
  }

  template <ProgramArgScalarType V>
  static auto convert_to(StringType const& sv) noexcept -> std::optional<V>
  {
    V numeric{};
    try
    {
      from_chars(sv, numeric);
    }
    catch (...)
    {
      return {};
    }
    return numeric;
  }

  template <ProgramArgBoolType V>
  static auto convert_to(StringType const& sv) noexcept -> std::optional<V>
  {
    if (equals_ignore_case(sv, "true") || equals_ignore_case(sv, "yes") || equals_ignore_case(sv, "on") || sv == "1" ||
        equals_ignore_case(sv, "y"))
    {
      return true;
    }
    if (equals_ignore_case(sv, "false") || equals_ignore_case(sv, "no") || equals_ignore_case(sv, "off") || sv == "0" ||
        equals_ignore_case(sv, "n"))
    {
      return false;
    }
    return {};
  }

  static auto trim(StringType value) noexcept -> StringType
  {
    constexpr std::string_view whitespace = " \t\n\r\f\v";
    auto const                 first      = value.find_first_not_of(whitespace);
    if (first == value.npos)
    {
      return {};
    }
    auto const last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
  }

  static auto equals_ignore_case(StringType lhs, std::string_view rhs) noexcept -> bool
  {
    if (lhs.size() != rhs.size())
    {
      return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i)
    {
      auto const left  = lhs[i] >= 'A' && lhs[i] <= 'Z' ? static_cast<char>(lhs[i] - 'A' + 'a') : lhs[i];
      auto const right = rhs[i] >= 'A' && rhs[i] <= 'Z' ? static_cast<char>(rhs[i] - 'A' + 'a') : rhs[i];
      if (left != right)
      {
        return false;
      }
    }
    return true;
  }

  static auto is_negative_number(StringType value) noexcept -> bool
  {
    return value.size() > 1 && value[0] == '-' && ((value[1] >= '0' && value[1] <= '9') || value[1] == '.');
  }

  static auto is_option(StringType value) noexcept -> bool
  {
    return value.size() > 1 && value[0] == '-' && (value.find('=') != value.npos || !is_negative_number(value));
  }

  static auto is_value(StringType value) noexcept -> bool
  {
    return value != "--" && (!is_option(value) || is_negative_number(value));
  }

  static auto has_inline_value(StringType value) noexcept -> bool
  {
    return value.find('=') != value.npos;
  }

  static auto option_prefix_size(StringType value) noexcept -> size_t
  {
    return value.starts_with("--") ? 2 : 1;
  }

  auto add_positional(StringType value) noexcept -> size_t
  {
    positional_args_.push_back({value, false});
    return positional_args_.size() - 1;
  }

  void add_occurrence(StringType name, std::optional<StringType> value, std::optional<StringType> attached,
                      std::optional<size_t> following) noexcept
  {
    if (name.empty())
    {
      return;
    }
    option_occurrences_.push_back({name, value, attached, following});
    arguments_[add(name)].value_ = value ? value_type(*value) : value_type(true);
    if (name == "help" || name == "h")
    {
      print_help_ = true;
    }
  }

  void parse_option(StringType token, size_t prefix_size, std::optional<size_t> following) noexcept
  {
    auto body  = token.substr(prefix_size);
    auto equal = body.find('=');
    if (equal != body.npos)
    {
      add_occurrence(body.substr(0, equal), body.substr(equal + 1), {}, {});
      return;
    }

    if (prefix_size == 1 && body.size() > 1)
    {
      // POSIX short options may be grouped (-abc), and the remainder may also
      // be an attached value once the option is declared (-j8).
      for (size_t index = 0; index < body.size(); ++index)
      {
        auto attached = index + 1 < body.size() ? std::optional<StringType>(body.substr(index + 1)) : std::nullopt;
        add_occurrence(body.substr(index, 1), {}, attached, index + 1 == body.size() ? following : std::nullopt);
      }
      return;
    }

    add_occurrence(body, {}, {}, following);
  }

  auto find_occurrence(StringType name, StringType flag) noexcept -> option_occurrence*
  {
    for (auto occurrence = option_occurrences_.rbegin(); occurrence != option_occurrences_.rend(); ++occurrence)
    {
      if (occurrence->name_ == name || (!flag.empty() && occurrence->name_ == flag))
      {
        return &*occurrence;
      }
    }
    return nullptr;
  }

  void consume_following_values(StringType name, StringType flag) noexcept
  {
    for (auto const& occurrence : option_occurrences_)
    {
      auto const matches = occurrence.name_ == name || (!flag.empty() && occurrence.name_ == flag);
      if (matches && !occurrence.value_ && !occurrence.attached_value_ && occurrence.following_value_)
      {
        auto const positional_index = *occurrence.following_value_;
        if (positional_index < positional_args_.size())
        {
          positional_args_[positional_index].consumed_ = true;
        }
      }
    }
  }

  auto find(StringType name) const -> std::optional<arg>
  {
    auto it = std::ranges::find(arguments_, name, &arg::name_);
    return it != arguments_.end() ? std::optional<arg>((*it)) : std::optional<arg>();
  }

  auto add(StringType name) noexcept -> size_t
  {
    auto it = std::ranges::find(arguments_, name, &arg::name_);
    if (it == arguments_.end())
    {
      arguments_.emplace_back(name);
      return arguments_.size() - 1;
    }
    return std::distance(arguments_.begin(), it);
  }

  std::vector<arg>               arguments_{};
  std::vector<option_occurrence> option_occurrences_{};
  std::vector<positional_arg>    positional_args_{};
  StringType                     brief_;
  std::vector<StringType>        docs_{};
  uint32_t                       max_arg_length_ = 0;
  bool                           print_help_     = false;
};

} // namespace ouly
