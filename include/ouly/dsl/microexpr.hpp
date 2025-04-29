// SPDX-License-Identifier: MIT

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

namespace ouly
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

  microexpr(macro_context&& ctx) noexcept : ctx_(std::move(ctx)) {}

  [[nodiscard]] auto evaluate(std::string_view expr) const -> bool;

private:
  macro_context ctx_;
};

} // namespace ouly
