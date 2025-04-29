// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/utility/config.hpp"
#include "ouly/utility/type_traits.hpp"
#include <compare>
#include <ranges>

namespace ouly
{

/**
 * @brief A view that projects a specific member from each element in a container.
 *
 * This class takes a container of objects and provides a view that lets you iterate
 * over a specific member variable of each object, as if it were a container of just those members.
 *
 * @tparam M The member pointer to project (e.g., &Class::member)
 * @tparam C The type of the container elements, deduced from the member pointer
 */
template <auto M, typename C = typename ouly::cfg::member<M>::class_type>
class projected_view : public std::ranges::view_interface<projected_view<M, C>>
{
public:
  /**
   * @brief Indicates whether the view is projecting from const objects
   */
  static constexpr bool is_const = std::is_const_v<C>;

  /**
   * @brief Iterator wrapper that provides access to projected members
   *
   * This iterator delegates most operations to the underlying container's iterator
   * but dereferences to the specified member instead of the whole object.
   */
  struct iterator_wrapper
  {
  public:
    /**
     * @brief Default constructor
     */
    iterator_wrapper() noexcept = default;

    /**
     * @brief Constructs an iterator wrapper around a pointer to the container element
     *
     * @param d Pointer to a container element
     */
    iterator_wrapper(C* d) noexcept : data_(d) {}

    /**
     * @brief Iterator category tag
     */
    using iterator_category = std::random_access_iterator_tag;

    /**
     * @brief The value type of the projected member
     */
    using value_type = std::remove_reference_t<decltype(std::declval<C&>().*M)>;

    /**
     * @brief The difference type used for iterator arithmetic
     */
    using difference_type = std::ptrdiff_t;

    /**
     * @brief Dereferences the iterator to access the projected member
     *
     * @return Reference to the member of the current object
     */
    auto operator*() const& noexcept -> value_type&
    {
      return data_->*M;
    }

    /**
     * @brief Arrow operator to access the projected member
     *
     * @return Pointer to the member of the current object
     */
    auto operator->() const& noexcept -> value_type*
    {
      return &(data_->*M);
    }

    /**
     * @brief Pre-increment operator that advances to the next element
     *
     * @return Reference to this iterator after advancing
     */
    auto operator++() & noexcept -> iterator_wrapper&
    {
      ++data_;
      return *this;
    }

    /**
     * @brief Post-increment operator that advances to the next element
     *
     * @return A copy of this iterator before advancing
     */
    auto operator++(int) & noexcept -> iterator_wrapper
    {
      return iterator_wrapper(data_++);
    }

    /**
     * @brief Pre-decrement operator that moves to the previous element
     *
     * @return Reference to this iterator after moving back
     */
    auto operator--() & noexcept -> iterator_wrapper&
    {
      --data_;
      return *this;
    }

    /**
     * @brief Post-decrement operator that moves to the previous element
     *
     * @return A copy of this iterator before moving back
     */
    auto operator--(int) & noexcept -> iterator_wrapper
    {
      return iterator_wrapper(data_--);
    }

    /**
     * @brief Compound addition operator that advances the iterator by n positions
     *
     * @param n Number of positions to advance
     * @return Reference to this iterator after advancing
     */
    auto operator+=(difference_type n) & noexcept -> iterator_wrapper&
    {
      data_ += n;
      return *this;
    }

    /**
     * @brief Addition operator that creates a new iterator advanced by n positions
     *
     * @param n Number of positions to advance
     * @return New iterator positioned n elements ahead
     */
    auto operator+(difference_type n) const& noexcept -> iterator_wrapper
    {
      return iterator_wrapper(data_ + n);
    }

    /**
     * @brief Addition operator with reversed operands
     *
     * @param n Number of positions to advance
     * @param other The iterator to advance
     * @return New iterator positioned n elements ahead of other
     */
    friend auto operator+(difference_type n, iterator_wrapper other) noexcept -> iterator_wrapper
    {
      return iterator_wrapper(other.data_ + n);
    }

    /**
     * @brief Compound subtraction operator that moves the iterator back by n positions
     *
     * @param n Number of positions to move back
     * @return Reference to this iterator after moving back
     */
    auto operator-=(difference_type n) & noexcept -> iterator_wrapper&
    {
      data_ -= n;
      return *this;
    }

    /**
     * @brief Subtraction operator that creates a new iterator moved back by n positions
     *
     * @param n Number of positions to move back
     * @return New iterator positioned n elements behind
     */
    auto operator-(difference_type n) const& noexcept -> iterator_wrapper
    {
      return iterator_wrapper(data_ - n);
    }

    /**
     * @brief Subtraction operator between iterators that calculates the distance
     *
     * @param other Another iterator
     * @return The number of elements between this iterator and other
     */
    auto operator-(iterator_wrapper other) const& noexcept -> difference_type
    {
      return static_cast<difference_type>(data_ - other.data_);
    }

    /**
     * @brief Three-way comparison operator for comparing iterators
     *
     * @param other Another iterator to compare with
     * @return A comparison result (less, equal, greater)
     */
    auto operator<=>(iterator_wrapper const& other) const& noexcept = default;

    /**
     * @brief Subscript operator for random access to projected members
     *
     * @param n The offset from the current position
     * @return Reference to the member of the element at position data_ + n
     */
    auto operator[](difference_type n) const& noexcept -> value_type&
    {
      return data_[n].*M;
    }

  private:
    C* data_ = nullptr; ///< Pointer to the current element in the container
  };

  /**
   * @brief Returns an iterator to the beginning of the projected view
   *
   * @return An iterator pointing to the first element's projected member
   */
  auto begin() const noexcept
  {
    return iterator_wrapper(begin_);
  }

  /**
   * @brief Returns an iterator to the end of the projected view
   *
   * @return An iterator pointing past the last element's projected member
   */
  auto end() const noexcept
  {
    return iterator_wrapper(end_);
  }

  /**
   * @brief Constructs a projected view from a range of elements
   *
   * @param first Pointer to the first element
   * @param count Number of elements in the range
   */
  projected_view(C* first, size_t count) noexcept : begin_(first), end_(first + count) {}

  /**
   * @brief Default constructor
   */
  projected_view() noexcept = default;

private:
  C* begin_ = nullptr; ///< Pointer to the first element
  C* end_   = nullptr; ///< Pointer past the last element
};

/**
 * @brief Type alias for a projected view of const elements
 *
 * This is a convenience alias for creating a projected view where the container
 * elements are const, ensuring the members can't be modified through the view.
 *
 * @tparam M The member pointer to project
 */
template <auto M>
using projected_cview = projected_view<M, typename ouly::cfg::member<M>::class_type const>;

} // namespace ouly
