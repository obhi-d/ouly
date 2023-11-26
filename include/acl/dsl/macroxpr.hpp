
#pragma once
#include <concepts>
#include <cstdint>
#include <string_view>

namespace acl
{

template <typename T>
concept MacroContext = requires(T a, std::string_view macro) {
  {
    a.defined(macro)
  } -> std::same_as<bool>;

  {
    a.lookup(macro)
  } -> std::same_as<int>;
};

/**
 * @brief Class is a macro expression evaluator that evaluates boolean macro expressions to true or false.
 * All valid macro expressions are supported.
 * @example
 *     defined(DEFINE_A) && !defined(DEFINE_B)
 * A macro context is used to communicate between the evaluator and expression, the context will store information
 * regarding what macros are defined and what values are set.
 *
 */
template <MacroContext Context>
class macroxpr
{
public:
  macro_expr(MacroContext const& ctx) noexcept : ctx_(ctx) {}

  bool evalute(std::string_view expr);

private:
  template <typename ReTy>
  ReTy lex(void* yyscanner);

  friend class macroxpr_parser;
  MacroContext& ctx_;
};

} // namespace acl