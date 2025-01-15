
#pragma once

#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/podvector.hpp>
#include <acl/ecs/entity.hpp>
#include <acl/utility/config.hpp>
#include <acl/utility/type_traits.hpp>
#include <acl/utility/utils.hpp>
#include <limits>

namespace acl::ecs
{

/**
 * @brief A collection class for managing entities with optional revision tracking
 *
 * @tparam EntityTy The entity type to be stored
 * @tparam Config Configuration config for the collection
 *
 * Collection provides efficient storage and management of entities using a bitset-based
 * approach with optional revision tracking. It organizes entities in fixed-size pools
 * for memory efficiency.
 *
 * Features:
 * - Efficient entity insertion and removal
 * - Optional revision tracking for entity validation
 * - Memory-efficient bitset-based storage
 * - Custom allocator support
 * - Range-based iteration capabilities
 *
 * The collection uses a pool-based memory allocation strategy where entities are stored
 * in fixed-size pools. When revision tracking is enabled, it maintains both entity
 * existence bits and revision numbers.
 *
 * Memory Layout:
 * - Without revision: Single bitset per pool
 * - With revision: Bitset and hazard (revision) array per pool
 *
 * Usage:
 * ```
 * collection<Entity> entities;
 * entities.emplace(entity);  // Add entity
 * entities.erase(entity);    // Remove entity
 * entities.contains(entity); // Check if entity exists
 * ```
 *
 * @note The collection automatically manages memory allocation and deallocation of pools
 * @note Revision tracking is only enabled in debug mode and when entity type uses uint8_t revision
 */
template <typename EntityTy, typename Config = acl::default_config<EntityTy>>
class collection : public acl::detail::custom_allocator_t<Config>
{
public:
  using size_type      = acl::detail::choose_size_t<uint32_t, Config>;
  using entity_type    = EntityTy;
  using allocator_type = acl::detail::custom_allocator_t<Config>;

private:
  static constexpr auto pool_mul     = acl::detail::log2(acl::detail::pool_size_v<Config>);
  static constexpr auto pool_size    = static_cast<size_type>(1) << pool_mul;
  static constexpr auto pool_mod     = pool_size - 1;
  using this_type                    = collection<EntityTy, Config>;
  using base_type                    = allocator_type;
  using storage                      = uint8_t;
  static constexpr bool has_revision = std::same_as<typename EntityTy::revision_type, uint8_t> && acl::debug;

public:
  collection() noexcept = default;
  collection(allocator_type&& alloc) noexcept : base_type(std::move<allocator_type>(alloc)) {}
  collection(allocator_type const& alloc) noexcept : base_type(alloc) {}
  collection(collection&& other) noexcept = default;
  collection(collection const& other) noexcept
  {
    *this = other;
  }

  ~collection() noexcept
  {
    clear();
    shrink_to_fit();
  }

  auto operator=(collection&& other) noexcept -> collection& = default;

  auto operator=(collection const& other) noexcept -> collection&
  {
    constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
    constexpr auto haz_page_size = sizeof(storage) * pool_size;

    items_.resize(other.items_.size());
    if constexpr (has_revision)
    {
      for (std::size_t i = 0, end = items_.size() / 2; i < end; ++i)
      {
        items_[(i * 2) + 0] = acl::allocate<storage>(*this, bit_page_size);
        items_[(i * 2) + 1] = acl::allocate<storage>(*this, haz_page_size);
        std::memcpy(items_[(i * 2) + 0], other.items_[(i * 2) + 0], bit_page_size);
        std::memcpy(items_[(i * 2) + 1], other.items_[(i * 2) + 1], haz_page_size);
      }
    }
    else
    {
      for (std::size_t i = 0, end = items_.size(); i < end; ++i)
      {
        items_[i] = acl::allocate<storage>(*this, bit_page_size);
        std::memcpy(items_[i], other.items_[i], bit_page_size);
      }
    }

    length_ = other.length_;
    return *this;
  }

  /**
   * @brief Applies a lambda function to each element in the container within the entire range.
   *
   * This is a convenience overload that processes the entire container range.
   *
   * @tparam Cont The container type
   * @tparam Lambda The lambda function type
   * @param cont The container to process
   * @param lambda The lambda function to apply to each element
   *
   * @note This operation is noexcept
   */
  template <typename Cont, typename Lambda>
  void for_each(Cont& cont, Lambda&& lambda) noexcept
  {
    for_each_l<Cont, Lambda>(cont, 0, range(), std::forward<Lambda>(lambda));
  }

  /**
   * @brief Applies a lambda function to each element in the container within the entire range.
   *
   * This is a const-qualified convenience overload that processes the entire container range.
   *
   * @tparam Cont The container type
   * @tparam Lambda The lambda function type
   * @param cont The container to process
   * @param lambda The lambda function to apply to each element
   *
   * @note This operation is noexcept
   */
  template <typename Cont, typename Lambda>
  void for_each(Cont const& cont, Lambda&& lambda) const noexcept
  {
    // NOLINTNEXTLINE
    const_cast<this_type*>(this)->for_each_l(cont, 0, range(), std::forward<Lambda>(lambda));
  }

  /**
   * @brief Applies a lambda function to each element in the container within the specified range.
   *
   * This is a convenience overload that processes the container range from first to last.
   *
   * @tparam Cont The container type
   * @tparam Lambda The lambda function type
   * @param cont The container to process
   * @param first The first element index
   * @param last The last element index
   * @param lambda The lambda function to apply to each element
   *
   * @note This operation is noexcept
   */
  template <typename Cont, typename Lambda>
  void for_each(Cont& cont, size_type first, size_type last, Lambda&& lambda) noexcept
  {
    for_each_l<Cont, Lambda>(cont, first, last, std::forward<Lambda>(lambda));
  }

  /**
   * @brief Applies a lambda function to each element in the container within the specified range.
   */
  template <typename Cont, typename Lambda>
  void for_each(Cont const& cont, size_type first, size_type last, Lambda&& lambda) const noexcept
  {
    // NOLINTNEXTLINE
    const_cast<this_type*>(this)->for_each_l(cont, first, last, std::forward<Lambda>(lambda));
  }

  /**
   * @brief Adds an entity to the collection.
   *
   * This function inserts an entity into the collection by setting the appropriate bit
   * and updating related metadata. If revision tracking is enabled, it also stores
   * the entity's revision information.
   *
   * @param l The entity to be emplaced into the collection.
   *
   * @note This operation is noexcept and will not throw exceptions.
   *
   * @details The function:
   * - Updates the maximum linked index if necessary
   * - Sets the corresponding bit for the entity
   * - Updates revision information if revision tracking is enabled
   * - Increments the collection length
   */
  void emplace(entity_type l) noexcept
  {
    auto idx = l.get();
    max_lnk_ = std::max(idx, max_lnk_);
    set_bit(idx);
    if constexpr (has_revision)
    {
      set_hazard(idx, static_cast<uint8_t>(l.revision()));
    }
    length_++;
  }

  /**
   * @brief Removes an entity from the collection
   *
   * Erases the specified entity from the collection by unsetting its corresponding bit
   * and decrements the collection length. If revision checking is enabled, validates
   * the entity's hazard revision before erasing.
   *
   * @param l The entity to erase from the collection
   *
   * @note This operation is noexcept and will not throw exceptions
   *
   * @pre If has_revision is true, the entity's revision must match the stored revision
   * @post The entity will be removed and the collection length decremented
   */
  void erase(entity_type l) noexcept
  {
    auto idx = l.get();
    if constexpr (has_revision)
    {
      validate_hazard(idx, static_cast<uint8_t>(l.revision()));
    }
    unset_bit(idx);
    length_--;
  }

  /**
   * @brief Checks if the entity exists in the collection
   * @param l The entity to check
   * @return true if the entity exists in the collection, false otherwise
   * @note This operation is noexcept and will not throw exceptions
   */
  auto contains(entity_type l) const noexcept -> bool
  {
    return is_bit_set(l.get());
  }

  auto size() const noexcept -> size_type
  {
    return length_;
  }

  auto capacity() const noexcept -> size_type
  {
    return static_cast<size_type>(items_.size()) * pool_size;
  }

  /**
   * @brief Returns the size of the range covering all valid indices
   *
   * The range includes all indices from 0 to the maximum link value plus 1,
   * representing the complete span of possible indices in the collection.
   *
   * @return size_type The number of elements in the range (max_link + 1)
   */
  auto range() const noexcept -> size_type
  {
    return max_lnk_ + 1;
  }

  void shrink_to_fit() noexcept
  {
    if (!length_)
    {
      constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
      constexpr auto haz_page_size = sizeof(storage) * pool_size;

      if constexpr (has_revision)
      {
        for (size_type i = 0, end = static_cast<size_type>(items_.size()) / 2; i < end; ++i)
        {
          acl::deallocate(static_cast<allocator_type&>(*this), items_[(i * 2) + 0], bit_page_size);
          acl::deallocate(static_cast<allocator_type&>(*this), items_[(i * 2) + 1], haz_page_size);
        }
      }
      else
      {
        for (auto i : items_)
        {
          acl::deallocate(static_cast<allocator_type&>(*this), i, bit_page_size);
        }
      }
    }
  }

  void clear() noexcept
  {
    length_  = 0;
    max_lnk_ = 0;
  }

private:
  void validate_hazard(size_type nb, std::uint8_t hz) const noexcept
  {
    auto block = hazard_page(nb >> pool_mul);
    auto index = nb & pool_mod;

    assert(items_[block][index] == hz);
  }

  auto bit_page(size_type p) const noexcept -> size_type
  {
    if constexpr (has_revision)
    {
      return p * 2;
    }
    else
    {
      return p;
    }
  }

  auto hazard_page(size_type p) const noexcept -> size_type
  {
    if constexpr (has_revision)
    {
      return (p * 2) + 1;
    }
    else
    {
      return std::numeric_limits<uint32_t>::max();
    }
  }

  auto is_bit_set(size_type nb) const noexcept -> bool
  {
    auto                   block = bit_page(nb >> pool_mul);
    auto                   index = nb & pool_mod;
    constexpr std::uint8_t one   = 1;
    constexpr uint8_t      mask  = 0x7;

    return (block < items_.size()) && (items_[block][index >> 3] & (one << static_cast<std::uint8_t>(index & mask)));
  }

  void unset_bit(size_type nb) noexcept
  {
    auto              block = bit_page(nb >> pool_mul);
    auto              index = nb & pool_mod;
    constexpr uint8_t mask  = 0x7;

    constexpr std::uint8_t one = 1;
    items_[block][index >> 3] &= ~(one << static_cast<std::uint8_t>(index & mask));
  }

  void set_bit(size_type nb) noexcept
  {
    auto block = bit_page(nb >> pool_mul);
    auto index = nb & pool_mod;

    if (block >= items_.size())
    {
      constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
      constexpr auto haz_page_size = sizeof(storage) * pool_size;

      items_.emplace_back(acl::allocate<storage>(*this, bit_page_size));
      std::memset(items_.back(), 0, bit_page_size);
      if constexpr (has_revision)
      {
        items_.emplace_back(acl::allocate<storage>(*this, haz_page_size));
        std::memset(items_.back(), 0, haz_page_size);
      }
    }

    constexpr std::uint8_t one  = 1;
    constexpr uint8_t      mask = 0x7;
    items_[block][index >> 3] |= one << static_cast<std::uint8_t>(index & mask);
  }

  void set_hazard(size_type nb, std::uint8_t hz) noexcept
  {
    auto block           = hazard_page(nb >> pool_mul);
    auto index           = nb & pool_mod;
    items_[block][index] = hz;
  }

  auto get_hazard(size_type nb) noexcept -> std::uint8_t
  {
    auto block = hazard_page(nb >> pool_mul);
    auto index = nb & pool_mod;
    return items_[block][index];
  }

  template <typename ContT, typename Lambda>
  void for_each_l(ContT& cont, size_type first, size_type last, Lambda lambda) noexcept
  {
    for (; first != last; ++first)
    {
      if (is_bit_set(first))
      {
        entity_type l = has_revision ? entity_type(first, get_hazard(first)) : entity_type(first);
        lambda(l, cont.at(l));
      }
    }
  }

  podvector<storage*, allocator_type> items_;
  size_type                           length_  = 0;
  size_type                           max_lnk_ = 0;
};

} // namespace acl::ecs
