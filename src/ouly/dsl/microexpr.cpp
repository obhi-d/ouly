// SPDX-License-Identifier: MIT

#include "ouly/dsl/microexpr.hpp"
#include "ouly/utility/from_chars.hpp"
#include "ouly/utility/user_config.hpp"
#include <cctype>
#include <cstdint>

namespace ouly
{

struct microexpr_state
{
  microexpr::macro_context const* ctx_ = nullptr;
  std::string_view                content_;
  uint32_t                        read_ = 0;

  microexpr_state(microexpr::macro_context const& ctx, std::string_view txt) noexcept : ctx_(&ctx), content_(txt) {}

  [[nodiscard]] auto get() const noexcept -> char
  {
    return ouly::detail::vector_access(content_, read_);
  }

  [[nodiscard]] auto read_token() const -> std::string_view;

  void skip_white() noexcept;
  auto conditional() -> int64_t;
  auto comparison() -> int64_t;
  auto binary() -> int64_t;
  auto unary() -> int64_t;
  auto group() -> int64_t;
  auto defined() -> int64_t;
  auto expect(char c) -> bool;

  auto exec_binary(int64_t& left) -> bool;
};

auto microexpr::evaluate(std::string_view expr) const -> bool
{
  auto state = microexpr_state(ctx_, expr);
  return state.conditional() != 0;
}

void microexpr_state::skip_white() noexcept
{
  auto tok_pos = content_.find_first_not_of(" \t\r\n", read_);
  if (tok_pos != std::string_view::npos)
  {
    read_ = static_cast<uint32_t>(tok_pos);
  }
}

auto microexpr_state::conditional() -> int64_t
{
  int64_t left = comparison();
  skip_white();
  if (read_ >= content_.length() || ouly::detail::vector_access(content_, read_) != '?')
  {
    return left;
  }
  read_++;
  int64_t op_a = comparison();
  skip_white();
  if (read_ >= content_.length() || ouly::detail::vector_access(content_, read_) != ':')
  {
    return left;
  }
  read_++;
  int64_t op_b = comparison();
  return (left != 0) ? op_a : op_b;
}

auto microexpr_state::comparison() -> int64_t
{
  int64_t left = binary();
  skip_white();
  if (read_ >= content_.length())
  {
    return left;
  }

  char sview[2] = {ouly::detail::vector_access(content_, read_), 0};
  if (read_ + 1 < content_.length())
  {
    sview[1] = ouly::detail::vector_access(content_, read_ + 1);
  }

  if (sview[0] == '=' && sview[1] == '=')
  {
    read_ += 2;
    return static_cast<int64_t>(left == binary());
  }
  if (sview[0] == '!' && sview[1] == '=')
  {
    read_ += 2;
    return static_cast<int64_t>(left != binary());
  }
  if (sview[0] == '<' && sview[1] == '=')
  {
    read_ += 2;
    return static_cast<int64_t>(left <= binary());
  }
  if (sview[0] == '>' && sview[1] == '=')
  {
    read_ += 2;
    return static_cast<int64_t>(left >= binary());
  }
  if (sview[0] == '>')
  {
    read_++;
    return static_cast<int64_t>(left > binary());
  }
  if (sview[0] == '<')
  {
    read_++;
    return static_cast<int64_t>(left < binary());
  }

  return left;
}

auto microexpr_state::exec_binary(int64_t& left) -> bool
{
  skip_white();

  if (read_ >= content_.length())
  {
    return false;
  }

  char sview[2] = {ouly::detail::vector_access(content_, read_), 0};
  if (read_ + 1 < content_.length())
  {
    sview[1] = ouly::detail::vector_access(content_, read_ + 1);
  }

  if (sview[0] == '&' && sview[1] == '&')
  {
    read_ += 2;
    // the right operand is always parsed, short-circuiting it would leave it unread
    auto const right = unary();
    left             = static_cast<int64_t>((left != 0) && (right != 0));
  }
  else if (sview[0] == '|' && sview[1] == '|')
  {
    read_ += 2;
    auto const right = unary();
    left             = static_cast<int64_t>((left != 0) || (right != 0));
  }
  else if (sview[0] == '&')
  {
    read_++;
    left = left & unary();
  }
  else if (sview[0] == '|')
  {
    read_++;
    left = left | unary();
  }
  else if (sview[0] == '^')
  {
    read_++;
    left = left ^ unary();
  }
  else if (sview[0] == '+')
  {
    read_++;
    left = left + unary();
  }
  else if (sview[0] == '-')
  {
    read_++;
    left = left - unary();
  }
  else if (sview[0] == '*')
  {
    read_++;
    left = left * unary();
  }
  else if (sview[0] == '/')
  {
    read_++;
    auto value = unary();
    left       = (value != 0) ? left / value : value;
  }
  else if (sview[0] == '%')
  {
    read_++;
    auto value = unary();
    left       = (value != 0) ? left % value : value;
  }
  else
  {
    return false;
  }
  return true;
}

auto microexpr_state::binary() -> int64_t
{
  int64_t left = unary();
  while (exec_binary(left))
  {
  }
  return left;
}

auto microexpr_state::read_token() const -> std::string_view
{
  uint32_t token = read_;
  while (token < content_.size())
  {
    auto const c = static_cast<unsigned char>(ouly::detail::vector_access(content_, token));
    if (std::isalnum(c) == 0 && c != '_')
    {
      break;
    }
    token++;
  }

  return content_.substr(read_, token - read_);
}

auto microexpr_state::unary() -> int64_t
{
  skip_white();
  if (read_ < content_.length())
  {
    char oper = get();
    if (oper == '(')
    {
      return group();
    }
    if (oper == '!')
    {
      read_++;
      return static_cast<int64_t>(unary() == 0);
    }
    if (oper == '-')
    {
      read_++;
      return -unary();
    }
    if (oper == '~')
    {
      read_++;
      return ~unary();
    }
    if (std::isdigit(static_cast<unsigned char>(oper)) != 0)
    {
      auto token = read_token();
      read_ += static_cast<uint32_t>(token.length());
      uint64_t value = 0;

      from_chars(token, value);

      return static_cast<int64_t>(value);
    }
    if (oper == '$')
    {
      return defined();
    }
    if ((std::isalpha(static_cast<unsigned char>(oper)) != 0) || oper == '_')
    {
      auto token = read_token();
      read_ += static_cast<uint32_t>(token.length());
      auto value = (*ctx_)(token);
      return value.has_value() ? value.value() : 0;
    }
  }
  return 0;
}

auto microexpr_state::expect(char c) -> bool
{
  skip_white();
  if (read_ >= content_.length() || get() != c)
  {
    return false;
  }
  read_++;
  return true;
}

auto microexpr_state::group() -> int64_t
{
  read_++; // '('
  int64_t result = conditional();
  return expect(')') ? result : 0;
}

auto microexpr_state::defined() -> int64_t
{
  read_++; // '$'
  // both $NAME and $(NAME) test whether NAME is defined
  bool const parenthesized = expect('(');
  skip_white();
  auto token = read_token();
  read_ += static_cast<uint32_t>(token.length());
  if (parenthesized && !expect(')'))
  {
    return 0;
  }
  return static_cast<int64_t>((*ctx_)(token).has_value());
}

} // namespace ouly
