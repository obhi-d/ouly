// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/containers/detail/indirection.hpp"
#include "ouly/ecs/entity.hpp"
#include "ouly/utility/detail/vector_abstraction.hpp"
#include "ouly/utility/optional_ref.hpp"

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
  using vector_type     = std::conditional_t<ouly::detail::HasUseSparseAttrib<config>, sparse_vector<Ty, config>,
                                             vector<Ty, ouly::detail::custom_allocator_t<config>>>;
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
  components(allocator_type&& alloc) noexcept : allocator_type(std::move<allocator_type>(alloc)) {}
  components(allocator_type const& alloc) noexcept : allocator_type(alloc) {}
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

    values_        = std::move(other.values_);
    keys_          = std::move(other.keys_);
    self_          = std::move(other.self_);
    count_present_ = other.count_present_;
    max_index_     = other.max_index_;
    present_       = std::move(other.present_);
    // leave other in valid empty state
    other.count_present_ = 0;
    other.max_index_     = 0;
    other.present_.clear();
    return *this;
  }

  auto operator=(components const& other) noexcept -> components& requires(std::is_copy_constructible_v<value_type>) {
    if (this == &other)
    {
      return *this;
    }
    clear();
    shrink_to_fit();

    values_        = other.values_;
    keys_          = other.keys_;
    self_          = other.self_;
    count_present_ = other.count_present_;
    max_index_     = other.max_index_;
    present_       = other.present_;
    return *this;
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A entity and value_type& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda>(std::forward<Lambda>(lambda));
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A entity and value_type const& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for_each_l<Lambda>(std::forward<Lambda>(lambda));
  }

  /**
   * @brief Lambda called for each element in range
   * @tparam Lambda Lambda Lambda should accept A entity and value_type& parameter
   * @param first first index in range. This should be between 1 and size()
   * @param last last index in range. This should be between 1 and size()
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
      return count_present_;
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
    if constexpr (has_direct_mapping)
    {
      return max_index_ + 1;
    }
    else
    {
      return static_cast<size_type>(values_.size());
    }
  }

  auto data() -> vector_type&
  {
    return values_;
  }

  auto data() const -> vector_type const&
  {
    return values_;
  }

  /**
   * @brief Construct an item in a given location, assuming the location was empty
   */
  template <typename... Args>
  auto emplace_at(entity_type point, Args&&... args) noexcept -> reference
  {
    if constexpr (has_direct_mapping)
    {
      auto  idx         = point.get();
      bool  was_present = test_present(idx);
      auto& ref         = ouly::detail::emplace_at(values_, idx, std::forward<Args>(args)...);
      set_present(idx);
      if (!was_present)
      {
        count_present_++;
        max_index_ = std::max(max_index_, idx);
      }
      return ref;
    }
    else
    {
      auto& k = keys_.ensure_at(point.get());
      k       = static_cast<size_type>(values_.size());

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
    return keys_.get_if(point.get());
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
      auto  idx         = point.get();
      bool  was_present = test_present(idx);
      auto& ref         = ouly::detail::replace_at(values_, idx, std::move(args));
      set_present(idx);
      if (!was_present)
      {
        count_present_++;
        max_index_ = std::max(max_index_, idx);
      }
      return ref;
    }
    else
    {
      auto k = keys_.get_if(point.get());
      if (k == tombstone)
      {
        return emplace_at(point, std::forward<value_type>(args));
      }
      auto& val = values_[k];
      val       = std::move(args);
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
      auto  idx         = point.get();
      bool  was_present = test_present(idx);
      auto& ref         = ouly::detail::ensure_at(values_, idx);
      set_present(idx);
      if (!was_present)
      {
        count_present_++;
        max_index_ = std::max(max_index_, idx);
      }
      return ref;
    }
    else
    {
      auto k = keys_.get_if(point.get());
      if (k == tombstone)
      {
        return emplace_at(point);
      }

      auto& val = values_[k];
      return val;
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
   * @brief Erase a single element by object when backref is available.
   * @remarks Only available if backref is available
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
    if constexpr (!has_direct_mapping)
    {
      keys_.shrink_to_fit();
      self_.shrink_to_fit();
    }
    else
    {
      present_.shrink_to_fit();
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
      count_present_ = 0;
      max_index_     = 0;
      present_.clear();
    }
    if constexpr (!has_direct_mapping)
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
    auto idx = l.get();
    if constexpr (has_direct_mapping)
    {
      return test_present(idx);
    }
    else
    {
      if (keys_.contains(idx))
      {
        auto val = keys_.get(idx);
        if constexpr (has_self_index)
        {
          return self_.get(values_[val]) == l.value();
        }
        else
        {
          return self_.get(val) == l.value();
        }
      }
    }
    return false;
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    if constexpr (has_direct_mapping)
    {
      return count_present_ == 0;
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
      return values_.at(l);
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
      return values_.at(l);
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
        max_index_ = std::max<size_type>(max_index_, size ? size - 1 : 0);
        ensure_bit_capacity(size - 1);
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
      if (!cont.test_present(idx))
      {
        return nullptr;
      }
      return ouly::detail::get_if(cont, idx);
    }
    else
    {
      auto idx = lnk.get();
      if (cont.keys_.contains(idx))
      {
        if constexpr (has_self_index)
        {
          auto& val = cont.values_[cont.keys_.get(idx)];
          if (cont.self_.get(val) == lnk.value())
          {
            return &val;
          }
        }
        else
        {
          auto val_idx = cont.keys_.get(idx);
          if (cont.self_.get(val_idx) == lnk.value())
          {
            return &cont.values_[val_idx];
          }
        }
      }
    }
    return {};
  }

  template <typename T>
  static auto sfind(T& cont, entity_type lnk, value_type def) noexcept -> value_type
  {
    if constexpr (has_direct_mapping)
    {
      auto idx = lnk.get();
      if (!cont.test_present(idx))
      {
        return def;
      }
      return ouly::detail::get_or(cont, idx, def);
    }
    else
    {
      auto idx = lnk.get();
      if (cont.keys_.contains(idx))
      {
        if constexpr (has_self_index)
        {
          auto& val = cont.values_[cont.keys_.get(idx)];
          if (cont.self_.get(val) == lnk.value())
          {
            return val;
          }
        }
        else
        {
          auto val_idx = cont.keys_.get(idx);
          if (cont.self_.get(val_idx) == lnk.value())
          {
            return cont.values_[val_idx];
          }
        }
      }
      return def;
    }
  }

  void validate(entity_type l) const noexcept
  {
    if constexpr (has_direct_mapping)
    {
      OULY_ASSERT(test_present(l.get()));
    }
    else
    {
      auto lnk  = l.get();
      auto idx  = keys_.get(lnk);
      auto self = get_ref_at_idx(idx);
      OULY_ASSERT(self == l.value());
    }
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

  void erase_at(entity_type l) noexcept
  {
    if constexpr (has_direct_mapping)
    {
      auto idx = l.get();
      if (test_present(idx))
      {
        // Reset underlying storage to a default/null value for determinism
        values_[idx] = value_type();
        clear_present(idx);
        // Do not shrink container for direct mapping, only adjust count
        if (count_present_ > 0)
        {
          count_present_--;
        }
        // max_index_ is not decreased here to keep range monotonic; optional optimization could scan backwards
      }
    }
    else
    {
      auto lnk       = l.get();
      auto item_id   = keys_.get(lnk);
      keys_.get(lnk) = tombstone;
      reference lb   = values_[item_id];
      reference back = values_.back();
      if (&back != &lb)
      {
        if constexpr (has_self_index)
        {
          keys_.get(entity_type(self_.get(back)).get()) = item_id;
        }
        else
        {
          keys_.get(entity_type(self_.best_erase(item_id)).get()) = item_id;
        }

        if constexpr (ouly::detail::is_tuple<value_type>::value)
        {
          // move each tuple element reference
          [&]<std::size_t... I>(std::index_sequence<I...>)
          {
            (ouly::detail::move(std::get<I>(lb), std::get<I>(back)), ...);
          }(std::make_index_sequence<std::tuple_size_v<value_type>>());
        }
        else
        {
          lb = std::move(back);
        }
      }
      else
      {
        if constexpr (!has_self_index)
        {
          self_.pop_back();
        }
      }

      values_.pop_back();
    }
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

  template <typename Lambda>
  void for_each_l(size_type first, size_type last, Lambda lambda) noexcept
  {
    constexpr auto arity = function_traits<Lambda>::arity;
    if constexpr (has_direct_mapping)
    {
      for (; first != last; ++first)
      {
        if (!test_present(first))
        {
          continue;
        }
        if constexpr (arity == 2)
        {
          lambda(entity_type(get_ref_at_idx(first)), values_[first]);
        }
        else
        {
          lambda(values_[first]);
        }
      }
    }
    else
    {
      for (; first != last; ++first)
      {
        if constexpr (arity == 2)
        {
          lambda(entity_type(get_ref_at_idx(first)), values_[first]);
        }
        else
        {
          lambda(values_[first]);
        }
      }
    }
  }

  template <typename Lambda>
  void for_each_l(size_type first, size_type last, Lambda lambda) const noexcept
  {
    constexpr auto arity = function_traits<Lambda>::arity;
    if constexpr (has_direct_mapping)
    {
      for (; first != last; ++first)
      {
        if (!test_present(first))
        {
          continue;
        }
        if constexpr (arity == 2)
        {
          lambda(entity_type(get_ref_at_idx(first)), values_[first]);
        }
        else
        {
          lambda(values_[first]);
        }
      }
    }
    else
    {
      for (; first != last; ++first)
      {
        if constexpr (arity == 2)
        {
          lambda(entity_type(get_ref_at_idx(first)), values_[first]);
        }
        else
        {
          lambda(values_[first]);
        }
      }
    }
  }

  vector_type values_;
  key_index   keys_;
  self_index  self_;
  // Tracking for direct mapping mode
  size_type count_present_ = 0; // number of valid entries when using direct mapping
  size_type max_index_     = 0; // highest seen index when using direct mapping
  // Presence bitfield for direct mapping storage (used for both sparse and normal vectors)
  std::vector<std::uint64_t> present_;

  // --- bitfield helpers (only meaningful for has_direct_mapping) ---
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

  void ensure_bit_capacity(size_type bit)
  {
    auto need = static_cast<size_type>(word_index(bit) + 1);
    if (present_.size() < need)
    {
      present_.resize(need, 0);
    }
  }

  void set_present(size_type bit)
  {
    ensure_bit_capacity(bit);
    present_[word_index(bit)] |= bit_mask(bit);
  }
  void clear_present(size_type bit) noexcept
  {
    auto wi = word_index(bit);
    if (wi < present_.size())
    {
      present_[wi] &= ~bit_mask(bit);
    }
  }
  auto test_present(size_type bit) const noexcept -> bool
  {
    auto wi = word_index(bit);
    return wi < present_.size() && ((present_[wi] & bit_mask(bit)) != 0);
  }
};

} // namespace ouly::ecs
