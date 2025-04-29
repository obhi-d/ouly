// SPDX-License-Identifier: MIT
#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>

namespace ouly::ecs
{
/**
 * @brief A basic entity class that manages entity identifiers with optional revision tracking
 *
 * @tparam SizeType The underlying type used for storing entity identifiers
 * @tparam RevisionBits The number of bits reserved for revision tracking
 * @tparam NullValue The value representing a null/invalid entity
 * @tparam min_revision_bit_count Minimum number of bits required for revision tracking
 *
 * This class provides a type-safe wrapper around entity identifiers with built-in
 * support for optional revision tracking. The identifier is split into two parts:
 * - Index bits: Used to store the actual entity identifier
 * - Revision bits: Used to track entity reuse and detect stale references
 *
 * Key features:
 * - Configurable size type and null value
 * - Optional revision tracking through template parameters
 * - Efficient bit manipulation for index and revision management
 * - Comparison operators
 * - Explicit conversion to underlying type and boolean
 *
 * The revision system helps detect use-after-free scenarios in entity management
 * by incrementing a revision counter when entities are recycled.
 *
 * Example usage:
 * @code
 * basic_entity<uint32_t, 8> entity; // 8 bits for revision, 24 for index
 * @endcode
 *
 * @note When RevisionBits is 0, revision tracking is disabled and the entire
 * size_type is used for the entity index.
 */
constexpr uint8_t min_revision_bit_count = 8;
template <typename Ty, typename SizeType = uint32_t, uint32_t RevisionBits = 0,
          SizeType NullValue = static_cast<SizeType>(0)>
class basic_entity
{
public:
  static_assert(RevisionBits < sizeof(SizeType) * min_revision_bit_count,
                "Revision bits must be less than the SizeType");

  using revision_type =
   std::conditional_t<RevisionBits == 0, void,
                      std::conditional_t<(RevisionBits > min_revision_bit_count), uint16_t, uint8_t>>;

  using size_type                             = SizeType;
  static constexpr size_type null_v           = NullValue;
  static constexpr size_type nb_revision_bits = RevisionBits;
  static constexpr size_type nb_usable_bits   = ((sizeof(size_type) * 8) - nb_revision_bits);
  static constexpr size_type index_mask_v     = std::numeric_limits<size_type>::max() >> nb_revision_bits;
  static constexpr size_type revision_mask_v  = []() -> size_type
  {
    if constexpr (nb_revision_bits > 0)
    {
      return std::numeric_limits<size_type>::max() << nb_usable_bits;
    }
    return 0;
  }();
  static constexpr size_type version_inc_v = []() -> size_type
  {
    constexpr size_type one = 1;
    if constexpr (nb_revision_bits > 0)
    {
      return one << nb_usable_bits;
    }
    return 0;
  }();

  /**
   * @brief Default constructor that initializes the entity to null
   */
  constexpr basic_entity() noexcept                    = default;
  basic_entity(basic_entity&&) noexcept                = default;
  constexpr basic_entity(basic_entity const&) noexcept = default;
  /**
   * @brief Constructs an entity with the specified ID value
   *
   * @param id The raw ID value for the entity
   */
  constexpr explicit basic_entity(size_type id) noexcept : i_(id) {}

  /**
   * @brief Constructs an entity from an index and optional revision
   *
   * @param index The entity index
   * @param revision The entity revision (ignored if RevisionBits is 0)
   */
  constexpr basic_entity(size_type index, size_type revision) noexcept
    requires(nb_revision_bits > 0)
      : i_(revision << nb_usable_bits | index)
  {}
  ~basic_entity() noexcept = default;

  auto operator=(basic_entity&&) noexcept -> basic_entity& = default;
  auto operator=(basic_entity const&) -> basic_entity&     = default;

  /**
   * @brief Gets the raw entity ID value
   *
   * @return The raw entity ID containing both index and revision (if enabled)
   */
  [[nodiscard]] constexpr auto value() const noexcept -> size_type
  {
    return i_;
  }

  /**
   * @brief Gets the entity index part of the ID
   *
   * @return The index part of the entity ID (without revision bits)
   */

  constexpr auto get() const noexcept -> size_type
  {
    if constexpr (nb_revision_bits > 0)
    {
      return (i_ & index_mask_v);
    }
    else
    {
      return i_;
    }
  }

  /**
   * @brief Checks if this entity is valid (not null)
   *
   * @return true if the entity is valid, false if it's null
   */
  [[nodiscard]] constexpr explicit operator bool() const noexcept
  {
    return i_ != null_v;
  }

  /**
   * @brief Checks if this entity is null
   *
   * @return true if the entity is null, false if it's valid
   */
  [[nodiscard]] constexpr auto is_null() const noexcept -> bool
  {
    return i_ == null_v;
  }

  /**
   * @brief Gets the revision part of the ID
   *
   * @return The revision part of the entity ID (if RevisionBits > 0)
   */
  constexpr auto revision() const noexcept -> size_type
    requires(nb_revision_bits > 0)
  {
    return i_ >> nb_usable_bits;
  }

  /**
   * @brief Gets the revised entity with incremented revision
   *
   * @return A new entity with the incremented revision
   */
  constexpr auto revised() const noexcept -> basic_entity
    requires(nb_revision_bits > 0)
  {
    return basic_entity(i_ + version_inc_v);
  }

  constexpr auto revision() const noexcept -> size_type
    requires(nb_revision_bits <= 0)
  {
    return 0;
  }

  constexpr auto revised() const noexcept -> basic_entity
    requires(nb_revision_bits <= 0)
  {
    return basic_entity(i_);
  }

  /**
   * @brief Sets the revision of this entity to a specific value
   *
   * @param rev The new revision value to set
   * @note Only available if RevisionBits > 0
   */
  constexpr void set_revision(size_type rev) noexcept
    requires(nb_revision_bits > 0)
  {
    i_ = (i_ & index_mask_v) | ((rev << nb_usable_bits) & revision_mask_v);
  }

  /**
   * @brief Increments the entity's revision counter
   *
   * Increases the revision by 1, wrapping around if it exceeds the maximum
   * value that can be stored in the revision bits.
   *
   * @note Only available if RevisionBits > 0
   */
  constexpr void bump_revision() noexcept
    requires(nb_revision_bits > 0)
  {
    i_ += version_inc_v;
    if ((i_ & revision_mask_v) == 0)
    {
      i_ |= version_inc_v;
    }
  }

  /**
   * @brief Creates a null entity
   *
   * @return A basic_entity with the null value
   */
  [[nodiscard]] static constexpr auto null() noexcept -> basic_entity
  {
    return basic_entity{null_v};
  }

  constexpr explicit operator size_type() const noexcept
  {
    return value();
  }

  /**
   * @brief Comparison operators for entity objects
   */
  constexpr auto operator<=>(basic_entity const&) const noexcept = default;

private:
  size_type i_ = null_v;
};

template <typename T = std::true_type>
using entity = basic_entity<T, uint32_t>;

template <typename T = std::true_type>
using rxentity = basic_entity<T, uint32_t, min_revision_bit_count>;

} // namespace ouly::ecs

template <typename Ty, typename SizeType, uint32_t RevisionBits>
struct std::hash<ouly::ecs::basic_entity<Ty, SizeType, RevisionBits>>
{
  auto operator()(ouly::ecs::basic_entity<Ty, SizeType, RevisionBits> const& s) const noexcept -> std::size_t
  {
    return std::hash<SizeType>{}(s.value());
  }
};
