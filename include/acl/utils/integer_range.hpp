
#pragma once

namespace acl
{

template <typename I>
class integer_range
{
public:
  using difference_type    = I;
  integer_range() noexcept = default;
  integer_range(I vbegin, I vend) noexcept : begin_(vbegin), end_(vend) {}

  void begin(I v) noexcept
  {
    begin_ = v;
  }

  void end(I v) noexcept
  {
    end_ = v;
  }

  auto begin() const noexcept -> I
  {
    return begin_;
  }

  auto end() const noexcept -> I
  {
    return end_;
  }

  auto size() const noexcept -> I
  {
    return end_ - begin_;
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return begin_ == end_;
  }

private:
  I begin_ = {};
  I end_   = {};
};

} // namespace acl
