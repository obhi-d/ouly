// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/containers/detail/indirection.hpp"
#include "ouly/ecs/entity.hpp"
#include "ouly/utility/detail/vector_abstraction.hpp"
#include "ouly/utility/optional_ref.hpp"
#include <bit>
#include <cstdint>
#include <iterator>
#include <vector>

namespace ouly::ecs
{
/**
 * @brief A container for managing components in an Entity Component System (ECS).
 *
 * This class provides efficient storage and management of components, supporting
 * various storage strategies and custom configurations.
 *
 * @tparam Ty The type of component being stored.
 * @tparam EntityTy The entity type used for identification.
 * @tparam Config Configuration options for the component storage.
 *
 * This class provides a efficient storage and management system for components in an ECS.
 * It supports both sparse and dense storage strategies, direct mapping, and self-indexing
 * capabilities based on the provided Config.
 *
 * Key features:
 * - Flexible storage strategies (sparse or dense)
 * - Optional direct mapping for faster access
 * - Self-indexing capabilities for reverse lookups
 * - Support for custom allocators
 * - Iteration over components with entity information
 *
 * Requirements:
 * - Ty must be default constructible
 * - Ty must be move assignable
 *
 * Example usage:
 * @code
 * components<Position, Entity> positions;
 * positions.emplace_at(entity, x, y, z);
 * positions.for_each([](Entity e, Position& p) {
 *     // Process each position component
 * });
 * @endcode
 *
 * @note The storage strategy and behavior can be customized through the Config template parameter
 * @see EntityTy
 * Config:
 *  @par ouly::cfg::custom_vector<std::vector<Ty>>
 *  Use custom vector as the container
 *  @par ouly::cfg::use_direct_mapping
 *  Use direct non continuous data block
 *  @par ouly::cfg::use_sparse
 *  Use sparse vector for data storage
 *  @par ouly::cfg::pool_size<V>
 *  Sparse vector pool size
 */
template <typename Ty, typename EntityTy = ouly::ecs::entity<>, typename Config = ouly::default_config<Ty>>
class components
{

  static_assert(std::is_default_constructible_v<Ty>, "Type must be default constructible");
  static_assert(std::is_move_assignable_v<Ty>, "Type must be move assignable");

public:
  using entity_type     = EntityTy;
  using config          = Config;
  using value_type      = Ty;
  using vector_type     = typename ouly::detail::custom_vector_type<config, Ty>::type;
  using reference       = typename vector_type::reference;
  using const_reference = typename vector_type::const_reference;
  using pointer         = typename vector_type::pointer;
  using const_pointer   = typename vector_type::const_pointer;
  using size_type       = typename entity_type::size_type;
  using ssize_type      = std::make_signed_t<size_type>;
  using revision_type   = typename EntityTy::revision_type;
  using allocator_type  = ouly::detail::custom_allocator_t<Config>;

  static_assert(std::is_same_v<typename vector_type::value_type, value_type>,
                "Custom vector must have same value_type as Ty");

private:
  static constexpr bool      has_self_index     = ouly::detail::HasSelfIndexValue<config>;
  static constexpr bool      has_direct_mapping = ouly::detail::HasDirectMapping<config>;
  static constexpr bool      has_sparse_storage = ouly::detail::HasUseSparseAttrib<config>;
  static constexpr size_type tombstone          = std::numeric_limits<size_type>::max();

  using this_type     = components<value_type, entity_type, config>;
  using optional_val  = ouly::optional_ref<reference>;
  using optional_cval = ouly::optional_ref<const_reference>;

  struct default_index_pool_size
  {
    static constexpr uint32_t self_index_pool_size_v  = 1024;
    static constexpr bool     self_use_sparse_index_v = true;
    static constexpr uint32_t keys_index_pool_size_v  = 4096;
    static constexpr bool     keys_use_sparse_index_v = false;
  };

  struct self_index_traits_base
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

  template <typename TrTy>
  struct self_index_traits : self_index_traits_base
  {};

  template <ouly::detail::HasSelfIndexValue TrTy>
  struct self_index_traits<TrTy> : self_index_traits_base
  {
    using self_index = typename TrTy::self_index;
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

  using self_index =
   std::conditional_t<has_direct_mapping, std::monostate, ouly::detail::self_index_type<self_index_traits<config>>>;
  using key_index =
   std::conditional_t<has_direct_mapping, std::monostate, ouly::detail::indirection_type<key_index_traits>>;

public:
  components() noexcept = default;
  components(components&& other) noexcept
  {
    *this = std::move(other);
  }
  components(components const& other) noexcept
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  ~components()
  {
    clear();
    shrink_to_fit();
  }

  auto operator=(components&& other) noexcept -> components&
  {
    if (this == &other)
    {
      return *this;
    }
    clear();
    shrink_to_fit();

    values_  = std::move(other.values_);
    keys_    = std::move(other.keys_);
    self_    = std::move(other.self_);
    present_ = std::move(other.present_);
    // leave other in valid empty state
    other.present_ = {};
    return *this;
  }

  auto operator=(components const& other) noexcept -> components& requires(std::is_copy_constructible_v<value_type>) {
    if (this == &other)
    {
      return *this;
    }
    clear();
    shrink_to_fit();

    values_  = other.values_;
    keys_    = other.keys_;
    self_    = other.self_;
    present_ = other.present_;
    return *this;
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept an entity and value_type& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda>(std::forward<Lambda>(lambda));
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept an entity and value_type const& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for_each_l<Lambda>(std::forward<Lambda>(lambda));
  }

  /**
   * @brief Lambda called for each element in range
   * @tparam Lambda Lambda should accept an entity and value_type& parameter
   * @param first First index in the range (0-based, inclusive)
   * @param last Last index in the range (exclusive)
   */
  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) noexcept
  {
    for_each_l<Lambda>(first, last, std::forward<Lambda>(lambda));
  }

  /**
   * @copydoc for_each
   */
  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) const noexcept
  {
    for_each_l<Lambda>(first, last, std::forward<Lambda>(lambda));
  }

  /**
   * @brief Returns size of packed array
   */
  auto size() const noexcept -> size_type
  {
    if constexpr (has_direct_mapping)
    {
      // Effective element count when using direct mapping
      return present_.count_;
    }
    else
    {
      return static_cast<size_type>(values_.size());
    }
  }

  /**
   * @brief Valid range that can be iterated over
   */
  auto range() const noexcept -> size_type
  {
    return static_cast<size_type>(values_.size());
  }

  auto data() -> vector_type&
  {
    return values_;
  }

  auto data() const -> vector_type const&
  {
    return values_;
  }

  // Helper: check if index has a valid value (used by iterators)
  auto is_present_at_index(size_type idx) const noexcept -> bool
  {
    if constexpr (has_direct_mapping)
    {
      return present_.test(idx);
    }
    else
    {
      // For non-direct mapping, iterators traverse packed values_ without holes
      // so presence-by-index is equivalent to bound check on packed storage.
      return idx < static_cast<size_type>(values_.size());
    }
  }

  // Helper: first present index at or after idx, clamped to range() (used by iterators)
  auto next_present_index(size_type idx) const noexcept -> size_type
  {
    return present_.next_present(idx, range());
  }

  // Helper: map packed index to entity
  auto entity_at_index(size_type idx) const noexcept -> entity_type
  {
    if constexpr (has_direct_mapping)
    {
      return entity_type(static_cast<typename entity_type::size_type>(idx));
    }
    else if constexpr (has_self_index)
    {
      return entity_type(self_.get(values_[idx]));
    }
    else
    {
      return entity_type(self_.get(idx));
    }
  }

  // Helper: access value at packed index
  auto value_at_index(size_type idx) noexcept -> reference
  {
    return values_[idx];
  }
  auto value_at_index(size_type idx) const noexcept -> const_reference
  {
    return values_[idx];
  }

  // -------- Iterators over component values (skipping holes for direct mapping) --------
  template <bool IsConst>
  class value_iterator_base
  {
  public:
    using parent_type       = std::conditional_t<IsConst, const components, components>;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using value_type        = Ty;
    using reference_t       = std::conditional_t<IsConst, const_reference, typename components::reference>;
    using pointer_t         = std::conditional_t<IsConst, const_pointer, typename components::pointer>;
    using reference         = reference_t;
    using pointer           = pointer_t;

    value_iterator_base() noexcept = default;
    value_iterator_base(parent_type* p, size_type idx) noexcept : p_(p), idx_(idx)
    {
      satisfy();
    }

    auto operator*() const noexcept -> reference_t
    {
      return deref();
    }
    auto operator->() const noexcept -> pointer_t
    {
      return std::addressof(deref());
    }

    auto operator++() noexcept -> value_iterator_base&
    {
      ++idx_;
      satisfy();
      return *this;
    }
    auto operator++(int) noexcept -> value_iterator_base
    {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    friend auto operator==(value_iterator_base const& a, value_iterator_base const& b) noexcept -> bool
    {
      return a.p_ == b.p_ && a.idx_ == b.idx_;
    }
    friend auto operator!=(value_iterator_base const& a, value_iterator_base const& b) noexcept -> bool
    {
      return !(a == b);
    }

  private:
    auto deref() const noexcept -> reference_t
    {
      return const_cast<parent_type*>(p_)->value_at_index(idx_);
    }

    void satisfy() noexcept
    {
      // Packed storage has no holes; only direct mapping needs to skip absent slots
      if constexpr (has_direct_mapping)
      {
        if (p_ != nullptr)
        {
          idx_ = p_->next_present_index(idx_);
        }
      }
    }

    parent_type* p_   = nullptr;
    size_type    idx_ = 0;
  };

  using iterator       = value_iterator_base<false>;
  using const_iterator = value_iterator_base<true>;

  auto begin() noexcept -> iterator
  {
    return iterator(this, 0);
  }
  auto end() noexcept -> iterator
  {
    return iterator(this, range());
  }
  auto begin() const noexcept -> const_iterator
  {
    return const_iterator(this, 0);
  }
  auto end() const noexcept -> const_iterator
  {
    return const_iterator(this, range());
  }
  auto cbegin() const noexcept -> const_iterator
  {
    return const_iterator(this, 0);
  }
  auto cend() const noexcept -> const_iterator
  {
    return const_iterator(this, range());
  }

  // -------- Iteration yielding (entity, component&) pairs --------
  template <bool IsConst>
  class entity_iterator_base
  {
  public:
    using parent_type       = std::conditional_t<IsConst, const components, components>;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using ref_t             = std::conditional_t<IsConst, const_reference, typename components::reference>;

    struct pair_proxy
    {
      entity_type e_;
      ref_t       v_;
    };

    // Proxy iterator: dereference yields a value, not a reference
    using value_type = pair_proxy;
    using reference  = pair_proxy;
    using pointer    = void;

    entity_iterator_base() noexcept = default;
    entity_iterator_base(parent_type* p, size_type idx) noexcept : p_(p), idx_(idx)
    {
      satisfy();
    }

    auto operator*() const noexcept -> pair_proxy
    {
      return {entity_at(), value_at()};
    }
    auto operator++() noexcept -> entity_iterator_base&
    {
      ++idx_;
      satisfy();
      return *this;
    }
    auto operator++(int) noexcept -> entity_iterator_base
    {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }
    friend auto operator==(entity_iterator_base const& a, entity_iterator_base const& b) noexcept -> bool
    {
      return a.p_ == b.p_ && a.idx_ == b.idx_;
    }
    friend auto operator!=(entity_iterator_base const& a, entity_iterator_base const& b) noexcept -> bool
    {
      return !(a == b);
    }

  private:
    auto entity_at() const noexcept -> entity_type
    {
      return p_->entity_at_index(idx_);
    }

    auto value_at() const noexcept -> ref_t
    {
      return const_cast<parent_type*>(p_)->value_at_index(idx_);
    }

    void satisfy() noexcept
    {
      // Packed storage has no holes; only direct mapping needs to skip absent slots
      if constexpr (has_direct_mapping)
      {
        if (p_ != nullptr)
        {
          idx_ = p_->next_present_index(idx_);
        }
      }
    }

    parent_type* p_   = nullptr;
    size_type    idx_ = 0;
  };

  using entity_iterator       = entity_iterator_base<false>;
  using const_entity_iterator = entity_iterator_base<true>;

  auto begin_entities() noexcept -> entity_iterator
  {
    return entity_iterator(this, 0);
  }
  auto end_entities() noexcept -> entity_iterator
  {
    return entity_iterator(this, range());
  }
  auto begin_entities() const noexcept -> const_entity_iterator
  {
    return const_entity_iterator(this, 0);
  }
  auto end_entities() const noexcept -> const_entity_iterator
  {
    return const_entity_iterator(this, range());
  }
  auto cbegin_entities() const noexcept -> const_entity_iterator
  {
    return const_entity_iterator(this, 0);
  }
  auto cend_entities() const noexcept -> const_entity_iterator
  {
    return const_entity_iterator(this, range());
  }

  /**
   * @brief Construct an item in a given location; if the location is occupied, the item is replaced
   */
  template <typename... Args>
  auto emplace_at(entity_type point, Args&&... args) noexcept -> reference
  {
    if constexpr (has_direct_mapping)
    {
      auto  idx = point.get();
      auto& ref = ouly::detail::emplace_at(values_, idx, std::forward<Args>(args)...);
      present_.mark(idx);
      return ref;
    }
    else
    {
      auto  ent_idx = point.get();
      auto& k       = keys_.ensure_at(ent_idx);
      if (k != tombstone)
      {
        reference ref = values_[k];
        ref           = value_type{std::forward<Args>(args)...};
        if constexpr (has_self_index)
        {
          self_.get(ref) = point.value();
        }
        else
        {
          self_.get(k) = point.value();
        }
        return ref;
      }
      k = static_cast<size_type>(values_.size());

      values_.emplace_back(std::forward<Args>(args)...);
      if constexpr (has_self_index)
      {
        self_.get(values_.back()) = point.value();
      }
      else
      {
        self_.ensure_at(k) = point.value();
      }

      return values_.back();
    }
  }

  /**
   * @brief For indirectly mapped components, returns the key (index) associated with the entity
   * @return The key associated with the entity or `tombstone` if not found (which is
   * std::numeric_limits<uint32_t>::max())
   */
  auto key(entity_type point) const noexcept -> size_type
    requires(!has_direct_mapping)
  {
    auto item_id = keys_.get_if(point.get());
    if (item_id == tombstone || item_id >= static_cast<size_type>(values_.size()))
    {
      return tombstone;
    }
    return get_ref_at_idx(item_id) == point.value() ? item_id : tombstone;
  }

  /**
   * @brief Returns a const reference to the internal keys container
   * @details This function is only available when the class does not use direct mapping
   * @return Const reference to the container holding the keys
   * @requires !has_direct_mapping
   */
  auto keys() const noexcept -> auto const&
    requires(!has_direct_mapping)
  {
    return keys_;
  }

  /**
   * @brief Construct/Replace an item in a given location, depending upon if the location was empty or not.
   */
  auto replace(entity_type point, value_type&& args) noexcept -> reference
  {
    if constexpr (has_direct_mapping)
    {
      auto  idx = point.get();
      auto& ref = ouly::detail::replace_at(values_, idx, std::move(args));
      present_.mark(idx);
      return ref;
    }
    else
    {
      auto idx = point.get();
      auto k   = keys_.get_if(idx);
      if (k == tombstone)
      {
        return emplace_at(point, std::move(args));
      }
      reference val = values_[k];
      val           = std::move(args);
      if constexpr (has_self_index)
      {
        self_.get(val) = point.value();
      }
      else
      {
        self_.get(k) = point.value();
      }
      return val;
    }
  }
  /**
   * @brief Construct/Retrieve reference to an item, if the location is empty, an item is default constructed
   */
  auto get_ref(entity_type point) noexcept -> reference
  {
    if constexpr (has_direct_mapping)
    {
      auto  idx = point.get();
      auto& ref = ouly::detail::ensure_at(values_, idx);
      present_.mark(idx);
      return ref;
    }
    else
    {
      auto idx = point.get();
      auto k   = keys_.get_if(idx);
      if (k == tombstone)
      {
        return emplace_at(point);
      }
      if (get_ref_at_idx(k) != point.value())
      {
        reference ref = values_[k];
        ref           = value_type{};
        if constexpr (has_self_index)
        {
          self_.get(ref) = point.value();
        }
        else
        {
          self_.get(k) = point.value();
        }
        return ref;
      }

      return values_[k];
    }
  }

  /**
   * @brief Erase a single element.
   */

  void erase(entity_type l) noexcept
  {
    if constexpr (ouly::debug)
    {
      validate(l);
    }
    erase_at(l);
  }

  /**
   * @brief Erase a single element by object when a self index is available.
   * @remarks Only available if the container is configured with a self index
   */
  void erase(value_type const& obj) noexcept
    requires(has_self_index)
  {
    erase_at(entity_type(self_.get(obj)));
  }

  /**
   * @brief Find an object associated with a entity
   * @return optional value of the object
   */

  auto find(entity_type lnk) noexcept -> optional_val
  {
    return optional_val(sfind(*this, lnk));
  }

  /**
   * @brief Find an object associated with a entity
   * @return optional value of the object
   */

  auto find(entity_type lnk) const noexcept -> optional_cval
  {
    return optional_cval(sfind(*this, lnk));
  }

  /**
   * @brief Find an object associated with a entity, provided a default value to return if it is not found
   */

  auto find(entity_type lnk, value_type def) const noexcept -> value_type
  {
    return sfind(*this, lnk, def);
  }

  /**
   * @brief Drop unused pages
   */
  void shrink_to_fit() noexcept
  {
    values_.shrink_to_fit();
    if constexpr (has_direct_mapping)
    {
      present_.shrink_to_fit();
    }
    else
    {
      keys_.shrink_to_fit();
      self_.shrink_to_fit();
    }
  }

  /**
   * @brief Set size to 0, memory is not released, objects are destroyed
   */
  void clear() noexcept
  {
    values_.clear();
    if constexpr (has_direct_mapping)
    {
      present_.clear();
    }
    else
    {
      keys_.clear();
      self_.clear();
    }
  }

  auto at(entity_type l) noexcept -> reference
  {
    if constexpr (ouly::debug)
    {
      validate(l);
    }
    return item_at(l.get());
  }

  auto at(entity_type l) const noexcept -> const_reference
  {
    if constexpr (ouly::debug)
    {
      validate(l);
    }
    return item_at(l.get());
  }

  auto operator[](entity_type l) noexcept -> reference
  {
    return at(l);
  }

  auto operator[](entity_type l) const noexcept -> const_reference
  {
    return at(l);
  }

  auto contains(entity_type l) const noexcept -> bool
  {
    if constexpr (has_direct_mapping)
    {
      return present_.test(l.get());
    }
    else
    {
      return key(l) != tombstone;
    }
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    if constexpr (has_direct_mapping)
    {
      return present_.count_ == 0;
    }
    else
    {
      return values_.empty();
    }
  }

  void validate_integrity() const
  {
    if constexpr (has_direct_mapping)
    {
      // Nothing to validate for direct mapping beyond bounds
    }
    else
    {
      for (size_type first = 0, last = size(); first < last; ++first)
      {
        OULY_ASSERT(keys_.get(entity_type(get_ref_at_idx(first)).get()) == first);
      }

      for (size_type i = 0; i < keys_.size(); ++i)
      {
        if (keys_.contains(i))
        {
          OULY_ASSERT(keys_.get(i) < values_.size());
        }
      }
    }
  }

  auto item_at(size_type l) noexcept -> reference
  {
    if constexpr (has_direct_mapping)
    {
      return values_[l];
    }
    else
    {
      return item_at_idx(keys_.get(l));
    }
  }

  auto item_at(size_type l) const noexcept -> const_reference
  {
    if constexpr (has_direct_mapping)
    {
      return values_[l];
    }
    else
    {
      return item_at_idx(keys_.get(l));
    }
  }

  /**
   * @brief Sets the maximum size for the component storage
   *
   * If the storage has direct mapping, ensures the internal storage
   * can accommodate at least (size-1) elements. This operation is
   * a no-op if the storage doesn't use direct mapping.
   *
   * @param size The maximum number of elements to accommodate
   */
  void set_max(size_type size)
  {
    if constexpr (has_direct_mapping)
    {
      if (size)
      {
        ouly::detail::ensure_at(values_, size - 1);
        present_.ensure_capacity(size - 1);
      }
    }
  }

private:
  template <typename T>
  static auto sfind(T& cont, entity_type lnk) noexcept
   -> std::conditional_t<std::is_const_v<T>, value_type const*, value_type*>
  {
    if constexpr (has_direct_mapping)
    {
      auto idx = lnk.get();
      if (!cont.present_.test(idx))
      {
        return nullptr;
      }
      return ouly::detail::get_if(cont.values_, idx);
    }
    else
    {
      auto k = cont.key(lnk);
      if (k == tombstone)
      {
        return nullptr;
      }
      return &cont.values_[k];
    }
  }

  template <typename T>
  static auto sfind(T& cont, entity_type lnk, value_type def) noexcept -> value_type
  {
    if constexpr (has_direct_mapping)
    {
      auto idx = lnk.get();
      if (!cont.present_.test(idx))
      {
        return def;
      }
      return ouly::detail::get_or(cont.values_, idx, def);
    }
    else
    {
      auto k = cont.key(lnk);
      if (k == tombstone)
      {
        return def;
      }
      return cont.values_[k];
    }
  }

  void validate([[maybe_unused]] entity_type l) const noexcept
  {
    OULY_ASSERT(contains(l));
  }

  auto get_ref_at_idx(size_type idx) const noexcept
    requires(has_direct_mapping && !has_self_index)
  {
    return idx;
  }

  auto get_ref_at_idx(size_type idx) const noexcept
    requires(!has_self_index && !has_direct_mapping)
  {
    return self_.get(idx);
  }

  auto get_ref_at_idx(size_type idx) const noexcept
    requires(has_self_index)
  {
    return self_.get(item_at_idx(idx));
  }

  auto item_at_idx(size_type item_id) noexcept -> reference
  {
    return values_[item_id];
  }

  auto item_at_idx(size_type item_id) const noexcept -> const_reference
  {
    return values_[item_id];
  }

  static void move_value(reference dst, reference src) noexcept
  {
    if constexpr (ouly::detail::is_tuple<value_type>::value)
    {
      [&]<std::size_t... I>(std::index_sequence<I...>) -> void
      {
        (ouly::detail::move(std::get<I>(dst), std::get<I>(src)), ...);
      }(std::make_index_sequence<std::tuple_size_v<value_type>>());
    }
    else
    {
      if constexpr (std::is_reference_v<reference>)
      {
        dst = std::move(src);
      }
      else
      {
        dst = static_cast<value_type>(src);
      }
    }
  }

  // Direct-mapped erase; storage is not shrunk, only the presence bit is cleared
  void erase_at(entity_type l) noexcept
    requires(has_direct_mapping)
  {
    auto idx = l.get();
    if (present_.test(idx))
    {
      // Reset underlying storage to a default/null value for determinism
      values_[idx] = value_type();
      present_.unmark(idx);
    }
  }

  // Indirect mapping with self backref
  void erase_at(entity_type l) noexcept
    requires(!has_direct_mapping && has_self_index)
  {
    auto      lnk     = l.get();
    size_type item_id = keys_.get(lnk);
    keys_.get(lnk)    = tombstone;

    auto last_id = static_cast<size_type>(values_.size() - 1);
    if (item_id != last_id)
    {
      reference back = values_.back();
      // Update key of moved-back element using self index on value
      keys_.get(entity_type(self_.get(back)).get()) = item_id;
      move_value(values_[item_id], back);
    }

    values_.pop_back();
  }

  // Indirect mapping without self backref
  void erase_at(entity_type l) noexcept
    requires(!has_direct_mapping && !has_self_index)
  {
    auto      lnk     = l.get();
    size_type item_id = keys_.get(lnk);
    keys_.get(lnk)    = tombstone;

    auto last_id = static_cast<size_type>(values_.size() - 1);
    if (item_id != last_id)
    {
      // best_erase returns the original entity id stored at item_id (before erase)
      keys_.get(entity_type(self_.best_erase(item_id)).get()) = item_id;
      move_value(values_[item_id], values_[last_id]);
    }
    else
    {
      // If erasing the last, drop the parallel self_ entry
      self_.pop_back();
    }

    values_.pop_back();
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept value_type& parameter
   */
  template <typename Lambda>
  void for_each_l(Lambda&& lambda) noexcept
  {
    if constexpr (has_direct_mapping)
    {
      for_each_l<Lambda>(0, range(), std::forward<Lambda>(lambda));
    }
    else
    {
      for_each_l<Lambda>(0, static_cast<size_type>(values_.size()), std::forward<Lambda>(lambda));
    }
  }

  template <typename Lambda>
  void for_each_l(Lambda&& lambda) const noexcept
  {
    if constexpr (has_direct_mapping)
    {
      for_each_l<Lambda>(0, range(), std::forward<Lambda>(lambda));
    }
    else
    {
      for_each_l<Lambda>(0, static_cast<size_type>(values_.size()), std::forward<Lambda>(lambda));
    }
  }

  template <typename Lambda, typename Self>
    requires(function_traits<Lambda>::arity == 3)
  static void invoke(Self& self, Lambda& lambda, size_type base)
  {
    lambda(entity_type(self.get_ref_at_idx(base)), base, self.values_[base]);
  }

  template <typename Lambda, typename Self>
    requires(function_traits<Lambda>::arity == 2)
  static void invoke(Self& self, Lambda& lambda, size_type base)
  {
    lambda(entity_type(self.get_ref_at_idx(base)), self.values_[base]);
  }

  template <typename Lambda, typename Self>
    requires(function_traits<Lambda>::arity == 1)
  static void invoke(Self& self, Lambda& lambda, size_type base)
  {
    lambda(self.values_[base]);
  }

  template <typename Lambda, typename Self>
  static void for_each_impl(Self& self, size_type first, size_type last, Lambda& lambda) noexcept
  {
    if constexpr (has_direct_mapping)
    {
      self.present_.for_each_set(first, last,
                                 [&](size_type base) -> void
                                 {
                                   invoke(self, lambda, base);
                                 });
    }
    else
    {
      for (; first != last; ++first)
      {
        invoke(self, lambda, first);
      }
    }
  }

  // Lambda is deliberately invoked as an lvalue (possibly many times), never forwarded onward
  template <typename Lambda>
  void for_each_l(size_type first, size_type last,
                  Lambda&& lambda) noexcept // NOLINT(cppcoreguidelines-missing-std-forward)
  {
    for_each_impl(*this, first, last, lambda);
  }

  template <typename Lambda>
  void for_each_l(size_type first, size_type last,
                  Lambda&& lambda) const noexcept // NOLINT(cppcoreguidelines-missing-std-forward)
  {
    for_each_impl(*this, first, last, lambda);
  }

  /**
   * @brief Presence bitfield for direct-mapping storage; tracks which slots hold a live value
   * and how many are set. Only instantiated when `has_direct_mapping` (std::monostate otherwise).
   */
  struct presence_bits
  {
    static constexpr size_type word_mask  = 63;
    static constexpr size_type word_shift = 6;

    static constexpr auto word_index(size_type bit) noexcept -> size_type
    {
      return bit >> word_shift;
    }

    static constexpr auto bit_mask(size_type bit) noexcept -> std::uint64_t
    {
      return std::uint64_t{1} << (bit & word_mask);
    }

    void ensure_capacity(size_type bit)
    {
      auto need = static_cast<size_type>(word_index(bit) + 1);
      if (words_.size() < need)
      {
        words_.resize(need, 0);
      }
    }

    /** @brief Set the bit and maintain the count of set bits */
    void mark(size_type bit)
    {
      ensure_capacity(bit);
      auto&      word = words_[word_index(bit)];
      auto const mask = bit_mask(bit);
      count_ += static_cast<size_type>((word & mask) == 0);
      word |= mask;
    }

    /** @brief Clear the bit and maintain the count of set bits */
    void unmark(size_type bit) noexcept
    {
      auto wi = word_index(bit);
      if (wi < words_.size())
      {
        auto&      word = words_[wi];
        auto const mask = bit_mask(bit);
        count_ -= static_cast<size_type>((word & mask) != 0);
        word &= ~mask;
      }
    }

    auto test(size_type bit) const noexcept -> bool
    {
      auto wi = word_index(bit);
      return wi < words_.size() && ((words_[wi] & bit_mask(bit)) != 0);
    }

    /** @brief First set bit at or after `bit`, or `bound` if there is none below it */
    auto next_present(size_type bit, size_type bound) const noexcept -> size_type
    {
      auto const nwords = static_cast<size_type>(words_.size());
      auto       wi     = word_index(bit);
      if (wi >= nwords)
      {
        return bound;
      }
      auto word = words_[wi] >> (bit & word_mask);
      while (word == 0)
      {
        if (++wi >= nwords)
        {
          return bound;
        }
        word = words_[wi];
        bit  = static_cast<size_type>(wi << word_shift);
      }
      bit += static_cast<size_type>(std::countr_zero(word));
      return bit < bound ? bit : bound;
    }

    /**
     * @brief Invoke `fn(index)` for every set bit in [first, last). Empty 64-bit words are
     * skipped wholesale and countr_zero jumps between set bits within a word.
     */
    template <typename Fn>
    void for_each_set(size_type first, size_type last, Fn fn) const noexcept
    {
      auto const nwords = static_cast<size_type>(words_.size());
      while (first < last)
      {
        auto const wi = word_index(first);
        if (wi >= nwords)
        {
          break;
        }
        auto word = words_[wi] >> (first & word_mask);
        auto base = first;
        first     = (first | word_mask) + 1;
        while (word != 0)
        {
          auto const skip = static_cast<size_type>(std::countr_zero(word));
          base += skip;
          if (base >= last)
          {
            return;
          }
          fn(base);
          word >>= skip;
          word >>= 1U;
          base += 1;
        }
      }
    }

    void clear() noexcept
    {
      words_.clear();
      count_ = 0;
    }

    void shrink_to_fit() noexcept
    {
      words_.shrink_to_fit();
    }

    std::vector<std::uint64_t> words_;
    size_type                  count_ = 0;
  };

  vector_type values_;
  key_index   keys_;
  self_index  self_;
  // Presence tracking, only used for direct mapping (empty placeholder otherwise)
  std::conditional_t<has_direct_mapping, presence_bits, std::monostate> present_;
};

} // namespace ouly::ecs
