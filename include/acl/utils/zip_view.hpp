
#pragma once
#include <tuple>
#include <utility>

namespace acl
{

template <typename... Iters>
class zip_iterator
{
  template <typename I>
  using reference_type = decltype(*std::declval<I>());

public:
  using value_type = std::tuple<reference_type<Iters>...>;

  zip_iterator() = delete;
  zip_iterator(Iters&&... iters) : it_{std::forward<Iters>(iters)...} {}

  auto operator++() -> zip_iterator&
  {
    std::apply(
     [](auto&&... args)
     {
       ((args++), ...);
     },
     it_);
    return *this;
  }

  auto operator++(int) -> zip_iterator
  {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  // If the iterator lengths don't match we will have an error
  auto operator!=(zip_iterator const& other) const noexcept -> bool = default;
  auto operator==(zip_iterator const& other) const noexcept -> bool = default;

  auto operator*() -> value_type
  {
    return std::apply(
     [](auto&&... args)
     {
       return value_type(*args...);
     },
     it_);
  }

private:
  std::tuple<Iters...> it_;
};

template <typename... T>
class zip_view
{
  template <typename I>
  using select_iterator_for = std::decay_t<decltype(std::begin(std::declval<I>()))>;

public:
  using zip_type = zip_iterator<select_iterator_for<T>...>;

  template <typename... Args>
  zip_view(Args&&... spans) : spans_{std::forward<Args>(spans)...}
  {}

  auto begin() -> zip_type
  {
    return std::apply(
     [](auto&&... args)
     {
       return zip_type(std::begin(args)...);
     },
     spans_);
  }

  auto end() -> zip_type
  {
    return std::apply(
     [](auto&&... args)
     {
       return zip_type(std::end(args)...);
     },
     spans_);
  }

private:
  std::tuple<T...> spans_;
};

template <typename... T>
auto zip(T&&... spans)
{
  return zip_view<T...>{std::forward<T>(spans)...};
}

} // namespace acl
