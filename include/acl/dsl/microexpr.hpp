
#pragma once
#include <charconv>
#include <compare>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace acl
{

/**
 * @brief Class is a macro expression evaluator that evaluates boolean macro expressions to true or false.
 * All valid macro expressions are supported.
 * @example
 *     $(DEFINE_A) && !$(DEFINE_B)
 * A macro context lambda is used to communicate between the evaluator and expression, the context will store
 * information regarding what macros are defined and what values are set.
 *
 */
class microexpr
{
public:
  /** Evaluator must return undefined if macro is not found */
  using macro_context = std::function<std::optional<int>(std::string_view)>;

  inline microexpr(macro_context&& ctx) noexcept : ctx_(std::move(ctx)) {}

  bool evaluate(std::string_view expr);

private:
  inline char get() const noexcept
  {
    return content_[read_];
  }

  std::string_view read_token() noexcept;

  void    skip_white() noexcept;
  int64_t conditional();
  int64_t comparison();
  int64_t binary();
  int64_t unary();

private:
  macro_context    ctx_;
  std::string_view content_;
  uint32_t         read_ = 0;
};

} // namespace acl