
#pragma once

#include <acl/containers/indirection.hpp>
#include <acl/utils/vector_abstraction.hpp>

namespace acl::ecs
{

/**
 * Options:
 *  @par acl::opt::custom_vector<std::vector<Ty>>
 *  Use custom vector as the container
 *  @par acl::opt::use_direct_mapping
 *  Use direct non continuous data block
 *  @par acl::opt::use_sparse
 *  Use sparse vector for data storage
 *  @par acl::opt::pool_size<V>
 *  Sparse vector pool size
 */
template <typename Ty, typename EntityTy, typename Options = acl::default_options<Ty>>
class components
{

  static_assert(std::is_default_constructible_v<Ty>, "Type must be default constructible");
  static_assert(std::is_move_assignable_v<Ty>, "Type must be move assignable");

public:
  using entity     = EntityTy;
  using options    = Options;
  using value_type = Ty;
  using vector_type =
    std::conditional_t<detail::HasUseSparseAttrib<options>, sparse_vector<Ty, acl::options<options, acl::opt::no_fill>>,
                       vector<Ty, detail::custom_allocator_t<options>>>;
  using reference       = typename vector_type::reference;
  using const_reference = typename vector_type::const_reference;
  using pointer         = typename vector_type::pointer;
  using const_pointer   = typename vector_type::const_pointer;
  using size_type       = typename entity::size_type;
  using ssize_type      = std::make_signed_t<size_type>;
  using revision_type   = typename EntityTy::revision_type;
  using allocator_type  = detail::custom_allocator_t<Options>;

  static_assert(std::is_same_v<typename vector_type::value_type, value_type>,
                "Custom vector must have same value_type as Ty");

private:
  static constexpr bool has_self_index     = detail::HasSelfIndexValue<options>;
  static constexpr bool has_direct_mapping = detail::HasDirectMapping<options>;
  static constexpr bool has_sparse_storage = detail::HasUseSparseAttrib<options>;

  using this_type     = components<value_type, entity, options>;
  using optional_val  = acl::detail::optional_ref<reference>;
  using optional_cval = acl::detail::optional_ref<const_reference>;

  struct default_index_pool_size
  {
    static constexpr uint32_t self_index_pool_size_v  = 4096;
    static constexpr bool     self_use_sparse_index_v = false;
    static constexpr uint32_t keys_index_pool_size_v  = 4096;
    static constexpr bool     keys_use_sparse_index_v = false;
  };

  struct self_index_traits_base
  {
    using size_type                          = detail::choose_size_t<uint32_t, Options>;
    static constexpr size_type pool_size_v   = std::conditional_t<detail::HasSelfIndexPoolSize<options>, options,
                                                                  default_index_pool_size>::self_index_pool_size_v;
    static constexpr bool use_sparse_index_v = std::conditional_t<detail::HasSelfUseSparseIndexAttrib<options>, options,
                                                                  default_index_pool_size>::self_use_sparse_index_v;
    static constexpr size_type null_v        = std::numeric_limits<size_type>::max();
    static constexpr bool      assume_pod_v  = true;
  };

  template <typename TrTy>
  struct self_index_traits : self_index_traits_base
  {};

  template <detail::HasSelfIndexValue TrTy>
  struct self_index_traits<TrTy> : self_index_traits_base
  {
    using self_index = typename TrTy::self_index;
  };

  struct key_index_traits
  {
    using size_type                          = detail::choose_size_t<uint32_t, Options>;
    static constexpr uint32_t pool_size_v    = std::conditional_t<detail::HasKeysIndexPoolSize<options>, options,
                                                                  default_index_pool_size>::keys_index_pool_size_v;
    static constexpr bool use_sparse_index_v = std::conditional_t<detail::HasKeysUseSparseIndexAttrib<options>, options,
                                                                  default_index_pool_size>::keys_use_sparse_index_v;
    static constexpr uint32_t null_v         = std::numeric_limits<size_type>::max();
    static constexpr bool     assume_pod_v   = true;
  };

  using self_index =
    std::conditional_t<has_direct_mapping, std::monostate, detail::self_index_type<self_index_traits<options>>>;
  using key_index = std::conditional_t<has_direct_mapping, std::monostate, detail::indirection_type<key_index_traits>>;

public:
  inline components() noexcept {}
  inline components(allocator_type&& alloc) noexcept : allocator_type(std::move<allocator_type>(alloc)) {}
  inline components(allocator_type const& alloc) noexcept : allocator_type(alloc) {}
  inline components(components&& other) noexcept
  {
    *this = std::move(other);
  }
  inline components(components const& other) noexcept
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  inline ~components()
  {
    clear();
    shrink_to_fit();
  }

  components& operator=(components&& other) noexcept
  {
    clear();
    shrink_to_fit();

    values_ = std::move(other.values_);
    keys_   = std::move(other.keys_);
    self_   = std::move(other.self_);
    return *this;
  }

  components& operator=(components const& other) noexcept
    requires(std::is_copy_constructible_v<value_type>)
  {
    clear();
    shrink_to_fit();

    values_ = other.values_;
    keys_   = other.keys_;
    self_   = other.self_;
    return *this;
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, value_type>(std::forward<Lambda>(lambda));
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type const& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for_each_l<Lambda, value_type const>(std::forward<Lambda>(lambda));
  }

  /**
   * @brief Lambda called for each element in range
   * @tparam Lambda Lambda Lambda should accept A link and value_type& parameter
   * @param first first index in range. This should be between 1 and size()
   * @param last last index in range. This should be between 1 and size()
   */
  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, value_type>(first, last, std::forward<Lambda>(lambda));
  }

  /**
   * @copydoc for_each
   */
  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) const noexcept
  {
    for_each_l<Lambda, value_type const>(first, last, std::forward<Lambda>(lambda));
  }

  /**
   * @brief Returns size of packed array
   */
  size_type size() const noexcept
  {
    return (size_type)values_.size();
  }

  /**
   * @brief Valid range that can be iterated over
   */
  size_type range() const noexcept
  {
    return (size_type)values_.size();
  }

  vector_type& data()
  {
    return values_;
  }

  vector_type const& data() const
  {
    return values_;
  }

  /**
   * @brief Construct an item in a given location, assuming the location was empty
   */
  template <typename... Args>
  reference emplace_at(entity point, Args&&... args) noexcept
  {
    if constexpr (has_direct_mapping)
    {
      return detail::emplace_at(values_, point.get(), std::forward<Args>(args)...);
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
        self_.ensure_at(k) = point.value();

      return values_.back();
    }
  }

  size_type key(entity point) const noexcept
    requires(!has_direct_mapping)
  {
    return keys_.get_if(point.get());
  }

  auto const& keys() const noexcept
    requires(!has_direct_mapping)
  {
    return keys_;
  }

  /**
   * @brief Construct/Replace an item in a given location, depending upon if the location was empty or not.
   */
  reference replace(entity point, value_type&& args) noexcept
  {
    if constexpr (has_direct_mapping)
    {
      return detail::replace_at(values_, point.get(), std::move(args));
    }
    else
    {
      auto k = keys_.get_if(point.get());
      if (k == std::numeric_limits<size_type>::max())
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
        self_.get(k) = point.value();
      return val;
    }
  }
  /**
   * @brief Construct/Retrieve reference to an item, if the location is empty, an item is default constructed
   */
  reference get_ref(entity point) noexcept
  {
    if constexpr (has_direct_mapping)
    {
      return detail::ensure_at(point.get());
    }
    else
    {
      auto k = keys_.get_if(point.get());
      if (k == std::numeric_limits<size_type>::max())
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

  void erase(entity l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    erase_at(l);
  }

  /**
   * @brief Erase a single element by object when backref is available.
   * @remarks Only available if backref is available
   */
  void erase(value_type const& obj) noexcept
    requires(has_self_index)
  {
    erase_at(entity(self_.get(obj)));
  }

  /**
   * @brief Find an object associated with a link
   * @return optional value of the object
   */

  optional_val find(entity lnk) noexcept
  {
    return optional_val(sfind(*this, lnk));
  }

  /**
   * @brief Find an object associated with a link
   * @return optional value of the object
   */

  optional_cval find(entity lnk) const noexcept
  {
    return optional_val(sfind(*this, lnk));
  }

  /**
   * @brief Find an object associated with a link, provided a default value to return if it is not found
   */

  value_type find(entity lnk, value_type def) const noexcept
  {
    return sfind(*this, lnk, def);
  }

  /**
   * @brief Drop unused pages
   */
  inline void shrink_to_fit() noexcept
  {
    values_.shrink_to_fit();
    if constexpr (!has_direct_mapping)
    {
      keys_.shrink_to_fit();
      self_.shrink_to_fit();
    }
  }

  /**
   * @brief Set size to 0, memory is not released, objects are destroyed
   */
  inline void clear() noexcept
  {
    values_.clear();
    if constexpr (!has_direct_mapping)
    {
      keys_.clear();
      self_.clear();
    }
  }

  inline reference at(entity l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l.get());
  }

  inline const_reference at(entity l) const noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l.get());
  }

  inline reference operator[](entity l) noexcept
  {
    return at(l);
  }

  inline const_reference operator[](entity l) const noexcept
  {
    return at(l);
  }

  inline bool contains(entity l) const noexcept
  {
    auto idx = l.get();
    if constexpr (has_direct_mapping)
    {
      if constexpr (has_sparse_storage)
        return values_.contains(idx);
      else
        return idx < values_.size();
    }
    else
    {
      if (keys_.contains(idx))
      {
        auto val = keys_.get(idx);
        if constexpr (has_self_index)
          return self_.get(values_[val]) == l.value();
        else
          return self_.get(val) == l.value();
      }
    }
    return false;
  }

  inline bool empty() const noexcept
  {
    return values_.empty();
  }

  inline void validate_integrity() const
  {
    if constexpr (has_direct_mapping)
    {
    }
    else
    {
      for (size_type first = 0, last = size(); first < last; ++first)
      {
        ACL_ASSERT(keys_.get(link(get_ref_at_idx(first)).get()) == first);
      }

      for (size_type i = 0; i < keys_.size(); ++i)
      {
        if (keys_.contains(i))
        {
          ACL_ASSERT(keys_.get(i) < values_.size());
        }
      }
    }
  }

  inline reference item_at(size_type l) noexcept
  {
    if constexpr (has_direct_mapping)
      return values_.at(l);
    else
      return item_at_idx(keys_.get(l));
  }

  inline const_reference item_at(size_type l) const noexcept
  {
    if constexpr (has_direct_mapping)
      return values_.at(l);
    else
      return item_at_idx(keys_.get(l));
  }

private:
  template <typename T>
  static auto sfind(T&     cont,
                    entity lnk) noexcept -> std::conditional_t<std::is_const_v<T>, value_type const*, value_type*>
  {
    if constexpr (has_direct_mapping)
    {
      return detail::get_if(cont, lnk.get());
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
            return &val;
        }
        else
        {
          auto val_idx = cont.keys_.get(idx);
          if (cont.self_.get(val_idx) == lnk.value())
            return &cont.values_[val_idx];
        }
      }
    }
    return {};
  }

  template <typename T>
  static auto sfind(T& cont, entity lnk, value_type def) noexcept -> value_type
  {
    if constexpr (has_direct_mapping)
    {
      return detail::get_or(cont, lnk.get(), def);
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
            return val;
        }
        else
        {
          auto val_idx = cont.keys_.get(idx);
          if (cont.self_.get(val_idx) == lnk.value())
            return cont.values_[val_idx];
        }
      }
      return def;
    }
  }

  inline void validate(entity l) const noexcept
  {
    if constexpr (has_direct_mapping)
    {
      ACL_ASSERT(get_if(values_, l.get()));
    }
    else
    {
      auto lnk  = l.get();
      auto idx  = keys_.get(lnk);
      auto self = get_ref_at_idx(idx);
      ACL_ASSERT(self == l.value());
    }
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept
    requires(has_direct_mapping && !has_self_index)
  {
    return idx;
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept
    requires(!has_self_index && !has_direct_mapping)
  {
    return self_.get(idx);
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept
    requires(has_self_index)
  {
    return self_.get(item_at_idx(idx));
  }

  inline reference item_at_idx(size_type item_id) noexcept
  {
    return values_[item_id];
  }

  inline const_reference item_at_idx(size_type item_id) const noexcept
  {
    return values_[item_id];
  }

  inline void erase_at(entity l) noexcept
  {
    if constexpr (has_direct_mapping)
    {
      values_[l.get()] = value_type();
    }
    else
    {
      auto lnk       = l.get();
      auto item_id   = keys_.get(lnk);
      keys_.get(lnk) = std::numeric_limits<size_type>::max();
      reference lb   = values_[item_id];
      reference back = values_.back();
      if (&back != &lb)
      {
        if constexpr (has_self_index)
          keys_.get(link(self_.get(back)).get()) = item_id;
        else
          keys_.get(link(self_.best_erase(item_id)).get()) = item_id;

        if constexpr (acl::detail::is_tuple<value_type>::value)
        {
          // move each tuple element reference
          [&]<std::size_t... I>(std::index_sequence<I...>)
          {
            (detail::move(std::get<I>(lb), std::get<I>(back)), ...);
          }(std::make_index_sequence<std::tuple_size_v<value_type>>());
        }
        else
          lb = std::move(back);
      }
      else
      {
        if constexpr (!has_self_index)
          self_.pop_back();
      }

      values_.pop_back();
    }
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept value_type& parameter
   */
  template <typename Lambda, typename Cast>
  inline void for_each_l(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, Cast>(0, static_cast<size_type>(values_.size()), std::forward<Lambda>(lambda));
  }

  template <typename Lambda, typename Cast>
  inline void for_each_l(Lambda&& lambda) const noexcept
  {
    for_each_l<Lambda, Cast>(0, static_cast<size_type>(values_.size()), std::forward<Lambda>(lambda));
  }

  template <typename Lambda, typename Cast>
  inline void for_each_l(size_type first, size_type last, Lambda&& lambda) noexcept
  {
    constexpr auto arity = function_traits<Lambda>::arity;
    for (; first != last; ++first)
    {
      if constexpr (arity == 2)
        lambda(link(get_ref_at_idx(first)), static_cast<Cast&>(values_[first]));
      else
        lambda(values_[first]);
    }
  }

  template <typename Lambda, typename Cast>
  inline void for_each_l(size_type first, size_type last, Lambda&& lambda) const noexcept
  {
    constexpr auto arity = function_traits<Lambda>::arity;
    for (; first != last; ++first)
    {
      if constexpr (arity == 2)
        lambda(link(get_ref_at_idx(first)), static_cast<Cast&>(values_[first]));
      else
        lambda(values_[first]);
    }
  }

  vector_type values_;
  key_index   keys_;
  self_index  self_;
};

} // namespace acl::ecs
