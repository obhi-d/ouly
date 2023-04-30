
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

namespace acl
{
enum class program_document_type
{
  brief_doc,
  full_doc,
  arg_doc
};

template <typename T>
concept ProgramDocFormatter = requires(T a, program_document_type f) { a(f, ""); };
template <typename V>
concept ProgramArgScalarType = std::is_same_v<uint32_t, V> || std::is_same_v<int32_t, V> || std::is_same_v<float, V>;
template <typename V>
concept ProgramArgBoolType = std::is_same_v<bool, V>;
template <typename V, typename S>
concept ProgramArgArrayType = !std::same_as<V, S> && requires(V a) {
                                typename V::value_type;
                                a.push_back(typename V::value_type());
};

template <typename string_type = std::string_view>
class program_args
{
  using value_type = std::any;

  struct arg
  {
    value_type  value_;
    string_type doc_;
    string_type name_;

    constexpr arg() noexcept = default;
    constexpr inline arg(string_type name) : name_(name) {}
  };

public:

  template <typename V>
  class arg_decl
  {
  public:
    inline auto& doc(string_type h)
    {
      arg_.doc_ = h;
      return *this;
    }

    inline operator bool() const noexcept
    {
      if (arg_.value_.has_value())
      {
        auto outp = std::any_cast<bool>(&arg_.value_);
        return outp && *outp;
      }
      return false;
    }

    std::optional<V> value() const
    {
      if (arg_.value_.has_value())
      {
        auto outp = std::any_cast<V>(&arg_.value_);
        if (outp)
          return *outp;
      }
      return {};
    }

    template <typename T>
    inline bool sink(T& value) const noexcept
    {
      if constexpr (std::is_pointer_v<T>)
        return sink_ref(value);
      else
        return sink_copy(value);
    }

  private:

    inline bool sink_copy(V& store) const noexcept
    {
      if (arg_.value_.has_value())
      {
        auto outp = std::any_cast<V>(&arg_.value_);
        if (outp)
        {
          store = *outp;
          return true;
        }
      }
      return false;
    }

    inline bool sink_ref(V*& store) const noexcept
    {
      if (arg_.value_.has_value())
      {
        auto outp = std::any_cast<V>(&arg_.value_);
        if (outp)
        {
          store = outp;
          return true;
        }
      }
      return false;
    }

    friend class program_args;

    inline arg_decl(arg& a) noexcept : arg_(a) {}

    arg& arg_;
  };

  program_args() noexcept = default;

  /// @brief Parse C main command line args
  inline void parse_args(int argc, char const* const* argv)
  {
    for (int i = 0; i < argc; ++i)
      parse_arg(argv[i]);
  }

  /// @brief Parse a single arg
  inline void parse_arg(char const* arg)
  {
    string_type asv = arg;

    if (asv.starts_with("--"))
      asv = asv.substr(2);
    else if (asv.starts_with("-"))
      asv = asv.substr(1);
    auto has_val = asv.find_first_of('=');
    if (has_val != asv.npos)
      add(asv.substr(0, has_val)).value_ = asv.substr(has_val + 1);
    else
      add(asv.substr(0, has_val)).value_ = true;
  }

  inline void brief(string_type h)
  {
    brief_ = h;
  }

  inline void doc(string_type h)
  {
    docs_.push_back(h);
  }

  template <typename V = string_type>
  inline arg_decl<V> decl(string_type name, string_type flag = string_type())
  {
    // Resolve arg
    auto& decl_arg = add(name);
    if (!decl_arg.value_.has_value() && !flag.empty())
    {
      auto& flag_arg = add(flag);
      if (flag_arg.value_.has_value())
        decl_arg.value_ = flag_arg.value_;
    }
    if constexpr (!std::is_same_v<string_type, V>)
    {
      auto svalue = std::any_cast<string_type>(&decl_arg.value_);
      if (svalue)
      {
        auto value = convert_to<V>(*svalue);
        if (value)
          decl_arg.value_ = *value;
      }
    }
    max_arg_length_ = std::max(name.size(), max_arg_length_);
    return arg_decl<V>(decl_arg);
  }
  
  template <typename T>
  inline bool sink(T& value, string_type name, string_type flag = string_type(), string_type docu = string_type())    
  {
    // Resolve arg
    return decl<T>(name, flag).doc(docu).sink(value);
  }

  template <ProgramDocFormatter formatter>
  inline auto& doc(formatter& f) const noexcept
  {
    if (!brief_.empty())
      f(program_document_type::brief_doc, brief_);
    for (auto d : docs_)
      f(program_document_type::full_doc, d);
    for (auto const& a : arguments_)
      f(program_document_type::arg_doc, a.doc_);
    return f;
  }

private:

  template <ProgramArgArrayType<string_type> V>
  static std::optional<V> convert_to(string_type const& sv)
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
          break;
        auto val = convert_to<value_type>(sv.substr(start, word_end - start));
        if (!val)
          return {};
        vector.push_back(*val);
        start = word_end;
      }
      return vector;
    }
    return {};
  }

  template <typename V>
  static std::optional<V> convert_to(string_type const& sv)
    requires(std::same_as<V, string_type>)
  {
    return sv;
  }

  template <ProgramArgScalarType V>
  static std::optional<V> convert_to(string_type const& sv)
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
  static std::optional<V> convert_to(string_type const& sv)
  {
    return (bool)(!sv.empty() && (sv[0] == 'Y' || sv[0] == 'y' || sv[0] == 't' || sv[0] == '1'));
  }

  std::optional<arg> find(string_type name) const
  {
    auto it = std::ranges::find(arguments_, name, &arg::name_);
    return it != arguments_.end() ? std::optional<arg>((*it)) : std::optional<arg>();
  }

  arg& add(string_type name)
  {
    auto it = std::ranges::find(arguments_, name, &arg::name_);
    if (it == arguments_.end())
    {
      arguments_.emplace_back(name);
      return arguments_.back();
    }
    else
      return *it;
  }

  std::vector<arg>         arguments_;
  string_type              brief_;
  std::vector<string_type> docs_;
  size_t                   max_arg_length_ = 0;
};

} // namespace acl
