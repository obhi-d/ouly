// SPDX-License-Identifier: MIT
#pragma once
#include <tuple>
#include <utility>

namespace ouly
{

/**
 * @brief A zip iterator that combines multiple iterators into a single iterator.
 *
 * This iterator allows iterating over multiple collections simultaneously by
 * combining their iterators and returning a tuple of references to the current elements.
 *
 * @tparam Iters The types of the iterators being combined
 */
template <typename... Iters>
class zip_iterator
{
  template <typename I>
  using reference_type = decltype(*std::declval<I>());

public:
  /**
   * @brief The value type returned when dereferencing the iterator
   *
   * This is a tuple containing references to the elements from each of the iterators.
   */
  using value_type = std::tuple<reference_type<Iters>...>;

  /**
   * @brief Default constructor is deleted
   */
  zip_iterator() = delete;

  /**
   * @brief Constructs a zip_iterator from a set of iterators
   *
   * @param iters The iterators to combine into this zip_iterator
   */
  zip_iterator(Iters&&... iters) : it_{std::forward<Iters>(iters)...} {}

  /**
   * @brief Pre-increment operator that advances all iterators
   *
   * @return Reference to this iterator after incrementing
   */
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

  /**
   * @brief Post-increment operator that advances all iterators
   *
   * @return A copy of this iterator before incrementing
   */
  auto operator++(int) -> zip_iterator
  {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  /**
   * @brief Inequality comparison operator
   *
   * @note If the iterator lengths don't match, this will cause undefined behavior
   */
  auto operator!=(zip_iterator const& other) const noexcept -> bool = default;

  /**
   * @brief Equality comparison operator
   *
   * @note If the iterator lengths don't match, this will cause undefined behavior
   */
  auto operator==(zip_iterator const& other) const noexcept -> bool = default;

  /**
   * @brief Dereference operator that returns a tuple of references to current elements
   *
   * @return A tuple containing references to the elements of each iterator
   */
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
  std::tuple<Iters...> it_; ///< Tuple of iterators
};

/**
 * @brief A view that combines multiple collections for simultaneous iteration
 *
 * This class provides a unified interface for iterating over multiple collections
 * at the same time, with each iteration yielding a tuple of references to elements
 * from each collection.
 *
 * @tparam T The types of the collections to be zipped together
 */
template <typename... T>
class zip_view
{
  template <typename I>
  using select_iterator_for = std::decay_t<decltype(std::begin(std::declval<I>()))>;

public:
  /**
   * @brief The type of iterator used to traverse the zipped collections
   */
  using zip_type = zip_iterator<select_iterator_for<T>...>;

  /**
   * @brief Constructs a zip_view from multiple collections
   *
   * @param spans The collections to be zipped together
   */
  template <typename... Args>
  zip_view(Args&&... spans) : spans_{std::forward<Args>(spans)...}
  {}

  /**
   * @brief Returns an iterator to the beginning of the zipped collections
   *
   * @return A zip iterator pointing to the first elements of all collections
   */
  auto begin() -> zip_type
  {
    return std::apply(
     [](auto&&... args)
     {
       return zip_type(std::begin(args)...);
     },
     spans_);
  }

  /**
   * @brief Returns an iterator to the end of the zipped collections
   *
   * @return A zip iterator pointing to the end of all collections
   */
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
  std::tuple<T...> spans_; ///< Tuple of the collections being zipped
};

/**
 * @brief A convenience function to create a zip_view from multiple collections
 *
 * @tparam T The types of the collections to be zipped
 * @param spans The collections to zip together
 * @return A zip_view that combines the provided collections
 */
template <typename... T>
auto zip(T&&... spans)
{
  return zip_view<T...>{std::forward<T>(spans)...};
}

} // namespace ouly
