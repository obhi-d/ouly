// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
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

/**
 * @brief An enumerate iterator that pairs a running index with combined iterators.
 *
 * This behaves like @ref zip_iterator, but additionally tracks a monotonically
 * increasing index that is yielded as the first element of the dereferenced tuple.
 *
 * @tparam IntType The integer type used for the index
 * @tparam Iters The types of the iterators being combined
 */
template <typename IntType, typename... Iters>
class enumerate_iterator
{
  template <typename I>
  using reference_type = decltype(*std::declval<I>());

public:
  /**
   * @brief The value type returned when dereferencing the iterator
   *
   * This is a tuple whose first element is the current index and whose remaining
   * elements are references to the elements from each of the iterators.
   */
  using value_type = std::tuple<IntType, reference_type<Iters>...>;

  /**
   * @brief Default constructor is deleted
   */
  enumerate_iterator() = delete;

  /**
   * @brief Constructs an enumerate_iterator from a starting index and a set of iterators
   *
   * @param index The initial index value
   * @param iters The iterators to combine into this enumerate_iterator
   */
  enumerate_iterator(IntType index, Iters&&... iters) : it_{std::forward<Iters>(iters)...}, index_{index} {}

  /**
   * @brief Pre-increment operator that advances all iterators and the index
   *
   * @return Reference to this iterator after incrementing
   */
  auto operator++() -> enumerate_iterator&
  {
    std::apply(
     [](auto&&... args)
     {
       ((args++), ...);
     },
     it_);
    ++index_;
    return *this;
  }

  /**
   * @brief Post-increment operator that advances all iterators and the index
   *
   * @return A copy of this iterator before incrementing
   */
  auto operator++(int) -> enumerate_iterator
  {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  /**
   * @brief Inequality comparison operator
   *
   * @note Only the underlying iterators participate in the comparison so that the
   *       index does not affect when iteration terminates.
   */
  auto operator!=(enumerate_iterator const& other) const noexcept -> bool
  {
    return it_ != other.it_;
  }

  /**
   * @brief Equality comparison operator
   *
   * @note Only the underlying iterators participate in the comparison.
   */
  auto operator==(enumerate_iterator const& other) const noexcept -> bool
  {
    return it_ == other.it_;
  }

  /**
   * @brief Dereference operator that returns a tuple of the index and element references
   *
   * @return A tuple whose first element is the current index followed by references to
   *         the elements of each iterator
   */
  auto operator*() -> value_type
  {
    return std::apply(
     [this](auto&&... args)
     {
       return value_type(index_, *args...);
     },
     it_);
  }

private:
  std::tuple<Iters...> it_;        ///< Tuple of iterators
  IntType              index_ = 0; ///< Current index
};

/**
 * @brief A view that combines multiple collections for simultaneous indexed iteration
 *
 * This behaves like @ref zip_view, but each iteration additionally yields the current
 * index as the first element of the tuple.
 *
 * @tparam IntType The integer type used for the index
 * @tparam T The types of the collections to be enumerated together
 */
template <typename IntType, typename... T>
class enumerate_view
{
  template <typename I>
  using select_iterator_for = std::decay_t<decltype(std::begin(std::declval<I>()))>;

public:
  /**
   * @brief The type of iterator used to traverse the enumerated collections
   */
  using enumerate_type = enumerate_iterator<IntType, select_iterator_for<T>...>;

  /**
   * @brief Constructs an enumerate_view from multiple collections
   *
   * @param spans The collections to be enumerated together
   */
  template <typename... Args>
  enumerate_view(Args&&... spans) : spans_{std::forward<Args>(spans)...}
  {}

  /**
   * @brief Returns an iterator to the beginning of the enumerated collections
   *
   * @return An enumerate iterator pointing to the first elements of all collections
   */
  auto begin() -> enumerate_type
  {
    return std::apply(
     [](auto&&... args)
     {
       return enumerate_type(IntType{0}, std::begin(args)...);
     },
     spans_);
  }

  /**
   * @brief Returns an iterator to the end of the enumerated collections
   *
   * @return An enumerate iterator pointing to the end of all collections
   */
  auto end() -> enumerate_type
  {
    return std::apply(
     [](auto&&... args)
     {
       return enumerate_type(IntType{0}, std::end(args)...);
     },
     spans_);
  }

private:
  std::tuple<T...> spans_; ///< Tuple of the collections being enumerated
};

/**
 * @brief A convenience function to create an enumerate_view from multiple collections
 *
 * Behaves like @ref zip, but the yielded tuple additionally includes the current index
 * as its first element.
 *
 * @tparam IntType The integer type used for the index (defaults to uint32_t)
 * @tparam T The types of the collections to be enumerated
 * @param spans The collections to enumerate together
 * @return An enumerate_view that combines the provided collections
 */
template <typename IntType = uint32_t, typename... T>
auto enumerate(T&&... spans)
{
  return enumerate_view<IntType, T...>{std::forward<T>(spans)...};
}

} // namespace ouly
