
#pragma once

#include <algorithm>
#include <any>
#include <charconv>
#include <cstdint>
#include <optional>
#include <ranges>
#include <sstream>
#include <string_view>
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
 * - Type-safe argument parsing and conversion
 * - Documentation generation
 * - Automatic help message generation
 * - Flexible argument declaration and value retrieval
 *
 * Example usage:
 * @code
 * acl::program_args args;
 * args.parse_args(argc, argv);
 * auto arg = args.decl<int>("number", "n").doc("A number argument");
 * if (auto value = arg.value()) {
 *     // use *value
 * }
 * @endcode
 *
 * The utility supports several argument type concepts:
 * - ProgramDocFormatter: For formatting documentation output
 * - ProgramArgScalarType: For numeric types (int32_t, uint32_t, float)
 * - ProgramArgBoolType: For boolean flags
 * - ProgramArgArrayType: For container types
 *
 * @tparam StringType The string type used for storing names and documentation
 *                    (defaults to std::string_view)
 */

namespace acl
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
concept ProgramArgScalarType = std::is_same_v<uint32_t, V> || std::is_same_v<int32_t, V> || std::is_same_v<float, V>;
template <typename V>
concept ProgramArgBoolType = std::is_same_v<bool, V>;
template <typename V, typename S>
concept ProgramArgArrayType = requires(V a) {
  typename V::value_type;
  a.push_back(typename V::value_type());
} && !std::same_as<V, std::basic_string<typename S::value_type>>;

template <typename StringType = std::string_view>
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
    int        flag_ = no_flag; // -1 means its a flag, -999 means

    constexpr arg() noexcept = default;
    constexpr arg(StringType name) noexcept : name_(name) {}
  };

public:
  template <typename V>
  class arg_decl
  {
  public:
    auto doc(StringType h) noexcept -> auto&
    {
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
   * @brief Parse C main command line args
   */
  void parse_args(int argc, char const* const* argv) noexcept
  {
    for (int i = 0; i < argc; ++i)
    {
      parse_arg(argv[i]);
    }
  }

  /**
   * @brief Parse a single arg
   */
  void parse_arg(StringType asv) noexcept
  {
    if (asv == "--help" || asv == "-h")
    {
      print_help_ = true;
    }

    if (asv.starts_with("--"))
    {
      asv = asv.substr(2);
    }
    else if (asv.starts_with("-"))
    {
      asv = asv.substr(1);
    }
    auto has_val  = asv.find_first_of('=');
    auto arg_name = asv.substr(0, has_val);
    if (has_val != asv.npos)
    {
      arguments_[add(arg_name)].value_ = asv.substr(has_val + 1);
    }
    else
    {
      arguments_[add(arg_name)].value_ = true;
    }
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
    if (!arguments_[decl_arg].value_.has_value() && !flag.empty())
    {
      if (arguments_[flag_arg].value_.has_value())
      {
        arguments_[decl_arg].value_ = arguments_[flag_arg].value_;
      }
    }
    if constexpr (!std::is_same_v<StringType, V>)
    {
      auto svalue = std::any_cast<StringType>(&arguments_[decl_arg].value_);
      if (svalue)
      {
        auto value = convert_to<V>(*svalue);
        if (value)
        {
          arguments_[decl_arg].value_ = *value;
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
    using value_type = typename V::value_type;
    auto start       = sv.find_first_of('[');
    auto end         = sv.find_first_of(']');
    if (start != sv.npos && end != sv.npos)
    {
      while (start < end)
      {
        start         = sv.find_first_not_of(' ', ++start);
        auto word_end = sv.find_first_of(", ]", start);
        if (start == word_end)
        {
          break;
        }
        auto val = convert_to<value_type>(sv.substr(start, word_end - start));
        if (!val)
        {
          return {};
        }
        vector.push_back(*val);
        start = word_end;
      }
      return vector;
    }
    return {};
  }

  template <typename V>
  static auto convert_to(StringType const& sv) noexcept -> std::optional<V>
  {
    return V(sv);
  }

  template <ProgramArgScalarType V>
  static auto convert_to(StringType const& sv) noexcept -> std::optional<V>
  {
    V numeric;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), numeric);
    if (ec == std::errc())
    {
      return numeric;
    }
    return {};
  }

  template <ProgramArgBoolType V>
  static auto convert_to(StringType const& sv) noexcept -> std::optional<V>
  {
    return (bool)(!sv.empty() && (sv[0] == 'Y' || sv[0] == 'y' || sv[0] == 't' || sv[0] == '1'));
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

  std::vector<arg>        arguments_{};
  StringType              brief_;
  std::vector<StringType> docs_{};
  uint32_t                max_arg_length_ = 0;
  bool                    print_help_     = false;
};

} // namespace acl
