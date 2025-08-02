// SPDX-License-Identifier: MIT

#pragma once
#include <concepts>
#include <iterator>
#include <type_traits>

namespace ouly
{

template <typename I>
class subrange
{
public:
  using iterator         = I;
  using const_iterator   = I;
  using sentinel         = I;
  using value_type       = I;
  using difference_type  = std::iter_difference_t<I>; // e.g. ptrdiff_t
  using size_type        = std::make_unsigned_t<difference_type>;
  using iterator_concept = std::random_access_iterator_tag;

  constexpr subrange() noexcept = default;
  constexpr subrange(I first, I last) noexcept : first_(first), last_(last) {}

  // setters (optional)
  constexpr void begin(I v) noexcept
  {
    first_ = v;
  }
  constexpr void end(I v) noexcept
  {
    last_ = v;
  }

  // iterators
  [[nodiscard]] constexpr auto begin() noexcept -> I
  {
    return first_;
  }
  [[nodiscard]] constexpr auto end() noexcept -> I
  {
    return last_;
  }
  [[nodiscard]] constexpr auto begin() const noexcept -> I
  {
    return first_;
  }
  [[nodiscard]] constexpr auto end() const noexcept -> I
  {
    return last_;
  }

  // capacity
  [[nodiscard]] constexpr auto size() const noexcept -> size_type
  {
    return static_cast<size_type>(last_ - first_);
  }

  [[nodiscard]] constexpr auto empty() const noexcept -> bool
  {
    return first_ == last_;
  }

  // helpers
  [[nodiscard]] constexpr auto front() const noexcept -> I
  {
    return first_;
  }

  // iterator to first
  [[nodiscard]] constexpr auto back() const noexcept -> I
  {
    return last_ - 1;
  } // iterator to last

  // split into [first_, mid) and [mid, last_)
  [[nodiscard]] auto split() -> subrange
  {
    I        mid = first_ + static_cast<difference_type>(size() / 2);
    subrange right(mid, last_);
    last_ = mid;
    return right;
  }

private:
  I first_{};
  I last_{};
};

} // namespace ouly
