
#pragma once

namespace ouly
{

template <typename I>
class integer_range
{
public:
  using difference_type              = I;
  constexpr integer_range() noexcept = default;
  constexpr integer_range(I vbegin, I vend) noexcept : begin_(vbegin), end_(vend) {}

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

  constexpr auto size() const noexcept -> I
  {
    return end_ - begin_;
  }

  [[nodiscard]] constexpr auto empty() const noexcept -> bool
  {
    return begin_ == end_;
  }

private:
  I begin_ = {};
  I end_   = {};
};

} // namespace ouly
