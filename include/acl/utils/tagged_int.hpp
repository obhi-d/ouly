
#pragma once

#include <compare>

namespace acl
{

template <typename Tag, typename Int = int, Int null_ = Int()>
class tagged_int
{
public:
  static constexpr Int null = null_;

  constexpr tagged_int() noexcept = default;
  explicit constexpr tagged_int(Int value_) noexcept : value_(value_) {}

  constexpr Int value() const noexcept
  {
    return value_;
  }

  inline constexpr explicit operator Int() const noexcept
  {
    return value_();
  }

  inline constexpr explicit operator bool() const noexcept
  {
    return value_() != null;
  }

  inline constexpr tagged_int& operator=(Int value_) noexcept
  {
    this->value_ = value_;
    return *this;
  }

  inline constexpr auto operator<=>(tagged_int const&) const noexcept = default;

private:
  Int value_ = null;
};

} // namespace acl
