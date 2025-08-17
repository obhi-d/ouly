// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/containers/detail/indirection.hpp"
#include "ouly/ecs/entity.hpp"
#include "ouly/utility/detail/vector_abstraction.hpp"
#include "ouly/utility/optional_ref.hpp"

namespace ouly::ecs
{

/**
 * @brief A high-performance map that maps sparse entity indices to dense continuous indices
 *
 * This class is designed for Entity Component Systems (ECS) where you need to map
 * discontinuous entity IDs to continuous array indices for efficient iteration and
 * cache-friendly access patterns. It maintains two internal containers:
 * - A sparse mapping from entity IDs to dense indices
 * - A dense array of entity values for efficient iteration
 *
 * The map supports efficient insertion, lookup, and removal operations while
 * maintaining dense packing of the stored values.
 *
 * @tparam EntityTy The entity type to use as keys (default: ouly::ecs::entity<>)
 * @tparam Config Configuration type that controls allocation and indexing behavior
 *
 * ## Usage Examples:
 *
 * ### Basic Operations:
 * @code
 * ouly::ecs::map<> entity_map;
 * ouly::ecs::entity<> e1{42};
 * ouly::ecs::entity<> e2{100};
 *
 * // Insert entities and get their dense indices
 * auto idx1 = entity_map.emplace(e1);  // Returns dense index (e.g., 0)
 * auto idx2 = entity_map.emplace(e2);  // Returns dense index (e.g., 1)
 *
 * // Look up dense indices
 * auto lookup_idx = entity_map.key(e1);  // Returns idx1
 * bool exists = entity_map.contains(e2); // Returns true
 *
 * // Access using operator[]
 * auto idx = entity_map[e1];  // Same as entity_map.key(e1)
 * @endcode
 *
 * ### Working with External Value Arrays:
 * @code
 * ouly::ecs::map<> entity_map;
 * std::vector<ComponentData> components;
 *
 * // Insert entities and components in parallel
 * ouly::ecs::entity<> e1{42};
 * auto idx = entity_map.emplace(e1);
 * components.resize(entity_map.size());
 * components[idx] = ComponentData{...};
 *
 * // Efficient iteration over all components
 * for (size_t i = 0; i < entity_map.size(); ++i) {
 *     auto entity_value = entity_map.get_entity_at(i);
 *     auto& component = components[i];
 *     // Process component...
 * }
 * @endcode
 *
 * ### Removal with External Value Management:
 * @code
 * // Method 1: Manual swap (performance-critical paths)
 * auto swap_idx = entity_map.erase_and_get_swap_index(entity_to_remove);
 * if (swap_idx < components.size() - 1) {
 *     components[swap_idx] = std::move(components.back());
 * }
 * components.pop_back();
 *
 * // Method 2: Automatic handling (convenience)
 * entity_map.erase_and_swap_values(entity_to_remove, components);
 * @endcode
 *
 * ## Performance Characteristics:
 * - Insertion: O(1) average, O(n) worst case (sparse array growth)
 * - Lookup: O(1) average
 * - Removal: O(1)
 * - Iteration: O(1) per element (dense array)
 * - Memory: Sparse for entity mapping, dense for value storage
 *
 * ## Thread Safety:
 * This class is not thread-safe. External synchronization is required for
 * concurrent access.
 */
template <typename EntityTy = ouly::ecs::entity<>, typename Config = ouly::default_config<EntityTy>>
class map
{
public:
  using entity_type    = EntityTy;
  using config         = Config;
  using size_type      = typename entity_type::size_type;
  using ssize_type     = std::make_signed_t<size_type>;
  using revision_type  = typename EntityTy::revision_type;
  using allocator_type = ouly::detail::custom_allocator_t<Config>;

private:
  static constexpr size_type tombstone = std::numeric_limits<size_type>::max();

  using this_type = map<entity_type, config>;

  struct default_index_pool_size
  {
    static constexpr uint32_t self_index_pool_size_v  = 1024;
    static constexpr bool     self_use_sparse_index_v = true;
    static constexpr uint32_t keys_index_pool_size_v  = 4096;
    static constexpr bool     keys_use_sparse_index_v = false;
  };

  struct self_index_traits
  {
    using size_type                        = ouly::detail::choose_size_t<uint32_t, Config>;
    static constexpr size_type pool_size_v = std::conditional_t<ouly::detail::HasSelfIndexPoolSize<config>, config,
                                                                default_index_pool_size>::self_index_pool_size_v;
    static constexpr bool      use_sparse_index_v =
     std::conditional_t<ouly::detail::HasSelfUseSparseIndexAttrib<config>, config,
                        default_index_pool_size>::self_use_sparse_index_v;
    static constexpr size_type null_v       = std::numeric_limits<size_type>::max();
    static constexpr bool      assume_pod_v = true;
  };

  struct key_index_traits
  {
    using size_type                       = ouly::detail::choose_size_t<uint32_t, Config>;
    static constexpr uint32_t pool_size_v = std::conditional_t<ouly::detail::HasKeysIndexPoolSize<config>, config,
                                                               default_index_pool_size>::keys_index_pool_size_v;
    static constexpr bool     use_sparse_index_v =
     std::conditional_t<ouly::detail::HasKeysUseSparseIndexAttrib<config>, config,
                        default_index_pool_size>::keys_use_sparse_index_v;
    static constexpr size_type null_v       = tombstone;
    static constexpr bool      assume_pod_v = true;
  };

  using self_index = ouly::detail::self_index_type<self_index_traits>;
  using key_index  = ouly::detail::indirection_type<key_index_traits>;

public:
  map() noexcept = default;
  map(allocator_type&& alloc) noexcept : allocator_type(std::move<allocator_type>(alloc)) {}
  map(allocator_type const& alloc) noexcept : allocator_type(alloc) {}
  map(map&& other) noexcept
  {
    *this = std::move(other);
  }
  map(map const& other) noexcept
  {
    *this = other;
  }
  ~map()
  {
    clear();
    shrink_to_fit();
  }

  auto operator=(map&& other) noexcept -> map&
  {
    if (this == &other)
    {
      return *this;
    }
    clear();
    shrink_to_fit();

    keys_ = std::move(other.keys_);
    self_ = std::move(other.self_);
    return *this;
  }

  auto operator=(map const& other) noexcept -> map&
  {
    if (this == &other)
    {
      return *this;
    }
    clear();
    shrink_to_fit();

    keys_ = other.keys_;
    self_ = other.self_;
    return *this;
  }

  /**
   * @brief Returns size of packed array
   */
  auto size() const noexcept -> size_type
  {
    return self_.size();
  }

  auto emplace(entity_type point) noexcept -> size_type
  {
    auto& k = keys_.ensure_at(point.get());
    k       = self_.size();
    self_.push_back(point.value());
    return k;
  }

  /**
   * @brief Erase a single element at `l` (legacy API)
   * @param l The entity to remove
   * @return The index which needs to be swapped with the back of the values
   * @note The final operation would require values to move the last element to returned index followed by a pop_back
   * @deprecated Use erase_and_get_swap_index() for better API clarity
   */
  auto erase(entity_type l) noexcept -> size_type
  {
    if constexpr (ouly::debug)
    {
      validate(l);
    }
    return erase_at(l);
  }

  /**
   * @brief Removes an entity from the map using swap-and-pop strategy
   * @param entity The entity to remove
   * @return The dense index that was swapped with the last element
   *
   * This is the low-level erase operation that returns the index that needs
   * to be handled externally. After calling this function, external value
   * arrays need to:
   * 1. Move the last element to the returned index position
   * 2. Pop the last element
   *
   * @note This API is designed for performance-critical code where you want
   * full control over the value array management. For convenience, consider
   * using erase_and_swap_values() instead.
   *
   * Example:
   * @code
   * auto swap_idx = map.erase_and_get_swap_index(entity);
   * if (swap_idx < values.size() - 1) {
   *     values[swap_idx] = std::move(values.back());
   * }
   * values.pop_back();
   * @endcode
   */
  auto erase_and_get_swap_index(entity_type entity) noexcept -> size_type
  {
    if constexpr (ouly::debug)
    {
      validate(entity);
    }
    return erase_at(entity);
  }

  /**
   * @brief Removes an entity and automatically handles value array swapping
   * @tparam ValueContainer... Container types that support operator[], size(), and pop_back()
   * @param entity The entity to remove
   * @param values The external value containers to update
   *
   * This convenience function handles the common pattern of removing an entity
   * and updating an external value array. It performs the swap-and-pop operation
   * on the value container automatically.
   *
   * Example:
   * @code
   * std::vector<ComponentData> components;
   * // ... fill components parallel to map ...
   * map.erase_and_swap_values(entity, components);
   * @endcode
   */
  template <typename... ValueContainer>
  void erase_and_swap_values(entity_type entity, ValueContainer&... values) noexcept
  {
    auto swap_idx = erase_and_get_swap_index(entity);
    (swap_value(swap_idx, values), ...);
  }

  /**
   * @brief For indirectly mapped map, returns the key (index) associated with the entity
   * @return The key associated with the entity or `tombstone` if not found (which is
   * std::numeric_limits<uint32_t>::max())
   */
  auto key(entity_type point) const noexcept -> size_type
  {
    return keys_.get_if(point.get());
  }

  /**
   * @brief Gets the entity value stored at a specific dense index
   * @param dense_index The dense array index (must be < size())
   * @return The entity value at the specified dense index
   *
   * This allows iteration over the dense array to access all stored entities
   * in order. Useful for efficient processing of all entities.
   *
   * Example:
   * @code
   * for (size_t i = 0; i < map.size(); ++i) {
   *     auto entity_value = map.get_entity_at(i);
   *     auto entity = entity_type(entity_value);
   *     // Process entity...
   * }
   * @endcode
   *
   * @note No bounds checking is performed. Ensure dense_index < size().
   */
  auto get_entity_at(size_type dense_index) const noexcept -> typename entity_type::size_type
  {
    return self_.get(dense_index);
  }

  /**
   * @brief Returns a const reference to the internal keys container
   * @details This function is only available when the class does not use direct mapping
   * @return Const reference to the container holding the keys
   * @requires !has_direct_mapping
   */
  auto keys() const noexcept -> auto const&
  {
    return keys_;
  }

  /**
   * @brief Drop unused pages
   */
  void shrink_to_fit() noexcept
  {
    keys_.shrink_to_fit();
    self_.shrink_to_fit();
  }

  /**
   * @brief Set size to 0, memory is not released, objects are destroyed
   */
  void clear() noexcept
  {
    keys_.clear();
    self_.clear();
  }

  auto at(entity_type l) const noexcept -> size_type
  {
    if constexpr (ouly::debug)
    {
      validate(l);
    }
    return key(l);
  }

  auto operator[](entity_type l) const noexcept -> size_type
  {
    return at(l);
  }

  auto contains(entity_type l) const noexcept -> bool
  {
    auto idx = l.get();
    if (keys_.contains(idx))
    {
      auto val = keys_.get(idx);
      return self_.get(val) == l.value();
    }
    return false;
  }

  /**
   * @brief Checks if the map contains no entities
   * @return true if the map is empty, false otherwise
   */
  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return self_.size() == 0;
  }

  /**
   * @brief Validates the internal consistency of the map data structures
   *
   * This method performs extensive validation to ensure the mapping between
   * sparse keys and dense indices is consistent. It checks:
   * - All dense indices have valid mappings back to sparse keys
   * - All sparse keys point to valid dense indices
   *
   * @note This method uses OULY_ASSERT internally and is typically used
   * for debugging and testing. In release builds, assertions may be disabled.
   */
  void validate_integrity() const
  {
    for (size_type first = 0, last = size(); first < last; ++first)
    {
      OULY_ASSERT(keys_.get(entity_type(get_entity_at(first)).get()) == first);
    }

    for (size_type i = 0; i < keys_.size(); ++i)
    {
      if (keys_.contains(i))
      {
        OULY_ASSERT(keys_.get(i) < self_.size());
      }
    }
  }

  /**
   * @brief Pre-allocates space in the sparse mapping for efficient insertion
   * @param size The maximum entity index to accommodate
   *
   * This method pre-allocates space in the sparse key mapping to accommodate
   * entities up to the specified index. This can improve performance when
   * you know the range of entity IDs that will be used, as it avoids
   * repeated allocations during insertion.
   *
   * Example:
   * @code
   * // If you know entities will have IDs from 0 to 999
   * map.set_max(1000);
   * // Now insertions for entities 0-999 will be more efficient
   * @endcode
   *
   * @param size The maximum number of elements to accommodate
   */
  void set_max(size_type size)
  {
    if (size)
    {
      keys_.resize(size, tombstone);
    }
  }

private:
  template <typename ValueContainer>
  void swap_value(size_type swap_idx, ValueContainer& values) noexcept
  {
    if (swap_idx < values.size() - 1)
    {
      values[swap_idx] = std::move(values.back());
    }
    values.pop_back();
  }

  void validate(entity_type l) const noexcept
  {
    auto lnk  = l.get();
    auto idx  = keys_.get(lnk);
    auto self = self_.get(idx);
    OULY_ASSERT(self == l.value());
  }

  auto erase_at(entity_type l) noexcept -> size_type
  {
    auto lnk                                                = l.get();
    auto item_id                                            = keys_.get(lnk);
    keys_.get(lnk)                                          = tombstone;
    keys_.get(entity_type(self_.best_erase(item_id)).get()) = item_id;
    return item_id;
  }

  key_index  keys_;
  self_index self_;
};

} // namespace ouly::ecs
