// SPDX-License-Identifier: MIT

#pragma once

namespace ouly
{

template <typename I>
class subrange
{
public:
  using difference_type         = I;
  constexpr subrange() noexcept = default;
  constexpr subrange(I vbegin, I vend) noexcept : begin_(vbegin), end_(vend) {}

  constexpr void begin(I v) noexcept
  {
    begin_ = v;
  }

  constexpr void end(I v) noexcept
  {
    end_ = v;
  }

  constexpr auto begin() const noexcept -> I
  {
    return begin_;
  }

  constexpr auto end() const noexcept -> I
  {
    return end_;
  }

  constexpr auto size() const noexcept
  {
    return end_ - begin_;
  }

  [[nodiscard]] constexpr auto empty() const noexcept -> bool
  {
    return begin_ == end_;
  }

  auto split() -> subrange<I>
  {
    auto mid   = begin_ + (size() >> 1);
    auto right = subrange(mid, end_);
    end_       = mid; // Fix: truncate original range to left half
    return right;
  }

  [[nodiscard]] constexpr auto is_divisible() const noexcept -> bool
  {
    return size() > 1; // Arbitrary condition for divisibility
  }

  [[nodiscard]] constexpr auto front() const noexcept -> I
  {
    return begin_;
  }

  [[nodiscard]] constexpr auto back() const noexcept -> I
  {
    return end_ - 1;
  }

private:
  I begin_ = {};
  I end_   = {};
};

} // namespace ouly
