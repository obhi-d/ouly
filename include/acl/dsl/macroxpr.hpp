
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <charconv>
#include <cstring>
#include <optional>
#include <functional>
#include <compare>
#include <ostream>

namespace acl
{

/**
 * @brief Class is a macro expression evaluator that evaluates boolean macro expressions to true or false.
 * All valid macro expressions are supported.
 * @example
 *     defined(DEFINE_A) && !defined(DEFINE_B)
 * A macro context lambda is used to communicate between the evaluator and expression, the context will store information
 * regarding what macros are defined and what values are set.
 *
 */
class macroxpr
{
public:
  /** Evaluator must return undefined if macro is not found */
  using macro_context = std::function<std::optional<int>(std::string_view)>;

  struct location_type
  {
    int begin = 0;
    int end   = 0;


    inline operator std::string() const noexcept

    {
      return std::to_string(begin);
    }

    inline friend std::ostream& operator<<(std::ostream& yyo, location_type const& l) noexcept

    {
      yyo << '(' << l.begin << ')';
      return yyo;
    }

    inline auto operator<=>(location_type const&) const noexcept = default;
  };
  inline macroxpr(macro_context const& ctx) noexcept : ctx_(ctx) {}

  bool evaluate(std::string_view expr);

  /// Parser API
  /// -----------------------------------------------
  void* get_scanner() const { return scanner_; }

  auto location() const noexcept { return cursor_; }

  int read(char* buff, int max_size) const noexcept 
  {
    int length = (int)content_.length() - cursor_.begin;
    if (length > 0)
    {
      int cpy = std::max(max_size, length);
      std::memcpy(buff, content_.data() + (size_t)cursor_.begin, (size_t)cpy);
      return cpy;
    }
    return 0;
  }

  void error(std::string_view e)
  { 
    error_ = e; 
    result_ = 0; 
  }

  void move(int len) noexcept { cursor_.begin += len; }
  
  inline std::string_view accept_str(int l) noexcept 
  {
    auto ret = std::string_view(content_.substr((size_t)cursor_.begin, (size_t)(cursor_.begin + l))); 
    cursor_.begin += l; 
    return ret;
  }

  inline int accept_int(int l) noexcept
  {
    auto ret = std::string_view(content_.substr((size_t)cursor_.begin, (size_t)(cursor_.begin + l))); 
    cursor_.begin += l; 
    int r = 0;
    std::from_chars(ret.data(), ret.data() + ret.size(), r);
    return r;
  }

  inline int lookup(std::string_view view) const noexcept 
  {
    auto o = ctx_(view);
    if (o)
      return o.value();
    return 0;
  }

  inline bool defined(std::string_view view) const noexcept 
  {
    return ctx_(view).has_value();
  }

private:

  void begin_scan() noexcept;
  void end_scan() noexcept;
  
  macro_context const& ctx_;
  std::string_view     content_;
  location_type        cursor_;
  std::string          error_;
  void*                scanner_ = nullptr;
  int                  result_  = 0;
};

} // namespace acl