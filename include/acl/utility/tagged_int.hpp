
#pragma once

#include <compare>

namespace acl
{

template <typename Tag, typename Int = int, Int Null = Int()>
class tagged_int
{
public:
  static constexpr Int null = Null;

  constexpr tagged_int() noexcept = default;
  explicit constexpr tagged_int(Int value_) noexcept : value_(value_) {}

  constexpr auto value() const noexcept -> Int
  {
    return value_;
  }

  constexpr explicit operator Int() const noexcept
  {
    return value_;
  }

  constexpr explicit operator bool() const noexcept
  {
    return value_ != null;
  }

  constexpr auto operator=(Int value_) noexcept -> tagged_int&
  {
    this->value_ = value_;
    return *this;
  }

  constexpr auto operator<=>(tagged_int const&) const noexcept = default;

private:
  Int value_ = null;
};

} // namespace acl
