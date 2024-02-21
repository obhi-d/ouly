#pragma once

#include "sparse_vector.hpp"
#include <acl/containers/indirection.hpp>
#include <acl/utils/rlink.hpp>
#include <acl/utils/type_traits.hpp>

#include <memory>
#include <tuple>

namespace acl
{

namespace detail
{
template <typename T, typename V>
concept IsVoidOrRLink =
  std::same_as<T, V> || std::same_as<T, acl::rlink<void, typename V::size_type, V::num_revision_bits>>;
}

/**
 * @brief Contents are packed in vector or sparse vector, meant to compliment rlink_registry. Object revisions
 * @tparam Ty Type of object
 * @tparam Options Controls the parameters for class instantiation
 *   At a minimum expected options must include:
 *         - use_sparse : bool - true to use sparse vector
 *         - pool_size : integer - Pool size of the sparse vector when sparse vector is used to store data
 *         - offset : typename - acl::offset<&type::offset> self backref member
 *         - self_index_pool_size : integer - when offset is missing, indicates the pool size of self references if
 *         they use sparse vector
 *         - keys_index_pool_size : integer - indicates the pool size of keys if they use sparse vector
 *         - self_use_sparse_index : bool - use sparse vector for self reference
 *         - keys_use_sparse_index : bool - use sparse vector for key index
 *         - size_type : typename - uint32_t to reduce footprint
 */
template <typename Ty, typename Options = acl::default_options<Ty>>
class rlink_object_table : public detail::custom_allocator_t<Options>
{

  static_assert(std::is_move_assignable_v<Ty>, "Type must be move assignable");
  static_assert(std::is_default_constructible_v<Ty>, "Type must be default constructibe");

public:
  using options         = Options;
  using vector_type     = typename detail::custom_vector_type<options, Ty>::type;
  using value_type      = typename vector_type::value_type;
  using reference       = typename vector_type::reference;
  using const_reference = typename vector_type::const_reference;
  using pointer         = typename vector_type::pointer;
  using const_pointer   = typename vector_type::const_pointer;
  using size_type       = detail::choose_size_t<uint32_t, Options>;

  static constexpr size_type revision_bytes_v = sizeof(size_type) > 4 ? 2 : 1;
  using link_value_type                       = detail::link_value_t<Options>;
  using link                                  = acl::rlink<link_value_type, size_type, revision_bytes_v * 8>;
  using allocator_type                        = detail::custom_allocator_t<Options>;

private:
  static constexpr bool has_backref = detail::HasBackrefValue<options>;

  using this_type     = rlink_object_table<Ty, options>;
  using storage       = detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
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

  template <detail::HasBackrefValue TrTy>
  struct self_index_traits<TrTy> : self_index_traits_base
  {
    using offset = typename TrTy::offset;
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

  using self_index = detail::backref_type<self_index_traits<options>>;
  using key_index  = detail::indirection_type<key_index_traits>;

public:
  inline rlink_object_table() noexcept {}
  inline rlink_object_table(allocator_type&& alloc) noexcept : allocator_type(std::move<allocator_type>(alloc)) {}
  inline rlink_object_table(allocator_type const& alloc) noexcept : allocator_type(alloc) {}
  inline rlink_object_table(rlink_object_table&& other) noexcept
  {
    *this = std::move(other);
  }
  inline rlink_object_table(rlink_object_table const& other) noexcept
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  inline ~rlink_object_table()
  {
    clear();
    shrink_to_fit();
  }

  rlink_object_table& operator=(rlink_object_table&& other) noexcept
  {
    clear();
    shrink_to_fit();

    values_ = std::move(other.values_);
    keys_   = std::move(other.keys_);
    self_   = std::move(other.self_);
    return *this;
  }

  rlink_object_table& operator=(rlink_object_table const& other) noexcept
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
  template <detail::IsVoidOrRLink<link> ulink, typename... Args>
  reference emplace_at(ulink point, Args&&... args) noexcept
  {
    auto& k = keys_.ensure_at(point.as_index());
    k       = static_cast<size_type>(values_.size());

    values_.emplace_back(std::forward<Args>(args)...);
    if constexpr (has_backref)
    {
      self_.get(values_.back()) = point.value();
    }
    else
      self_.ensure_at(k) = point.value();

    return values_.back();
  }

  /**
   * @brief Construct/Replace an item in a given location, depending upon if the location was empty or not. 
   */
  template <detail::IsVoidOrRLink<link> ulink>
  reference replace(ulink point, value_type&& args) noexcept
  {    
    auto  k   = keys_.get_if(point.as_index());
    if (k == std::numeric_limits<size_type>::max())
    {
      return emplace_at(point, std::forward<value_type>(args));
    }
    auto& val = values_[k];
    val       = std::move(args);
    if constexpr (has_backref)
    {
      self_.get(val) = point.value();
    }
    else
      self_.get(k) = point.value();
    return val;
  }
  /**
   * @brief Construct/Retrieve reference to an item, if the location is empty, an item is default constructed
   */
  template <detail::IsVoidOrRLink<link> ulink>
  reference get_ref(ulink point) noexcept
  {    
    auto k = keys_.get_if(point.as_index());
    if (k == std::numeric_limits<size_type>::max())
    {      
      return emplace_at(point);
    }

    auto& val = values_[k];
    return val;
  }

  /**
   * @brief Erase a single element.
   */
  template <detail::IsVoidOrRLink<link> ulink>
  void erase(ulink l) noexcept
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
    requires(has_backref)
  {
    erase_at(link(self_.get(obj)));
  }

  /**
   * @brief Find an object associated with a link
   * @return optional value of the object
   */
  template <detail::IsVoidOrRLink<link> ulink>
  optional_val find(ulink lnk) noexcept
  {
    return optional_val(sfind(*this, lnk));
  }

  /**
   * @brief Find an object associated with a link
   * @return optional value of the object
   */
  template <detail::IsVoidOrRLink<link> ulink>
  optional_cval find(ulink lnk) const noexcept
  {
    return optional_val(sfind(*this, lnk));
  }

  /**
   * @brief Find an object associated with a link, provided a default value to return if it is not found
   */
  template <detail::IsVoidOrRLink<link> ulink>
  value_type find(ulink lnk, value_type def) const noexcept
  {
    return sfind(*this, lnk, def);
  }

  /**
   * @brief Drop unused pages
   */
  inline void shrink_to_fit() noexcept
  {
    keys_.shrink_to_fit();
    values_.shrink_to_fit();
    self_.shrink_to_fit();
  }

  /**
   * @brief Set size to 0, memory is not released, objects are destroyed
   */
  inline void clear() noexcept
  {
    keys_.clear();
    values_.clear();
    self_.clear();
  }

  template <detail::IsVoidOrRLink<link> ulink>
  inline reference at(ulink l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l.as_index());
  }

  template <detail::IsVoidOrRLink<link> ulink>
  inline const_reference at(ulink l) const noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l.as_index());
  }

  template <detail::IsVoidOrRLink<link> ulink>
  inline reference operator[](ulink l) noexcept
  {
    return at(l);
  }

  template <detail::IsVoidOrRLink<link> ulink>
  inline const_reference operator[](ulink l) const noexcept
  {
    return at(l);
  }

  template <detail::IsVoidOrRLink<link> ulink>
  inline bool contains(ulink l) const noexcept
  {
    auto idx = l.as_index();
    if (keys_.contains(idx))
    {
      auto val = keys_.get(idx);
      if constexpr (has_backref)
        return self_.get(values_[val]) == l.value();
      else
        return self_.get(val) == l.value();
    }
    return false;
  }

  inline bool empty() const noexcept
  {
    return values_.empty();
  }

  inline void validate_integrity() const
  {
    for (size_type first = 0, last = size(); first < last; ++first)
    {
      ACL_ASSERT(keys_.get(link(get_ref_at_idx(first)).as_index()) == first);
    }

    for (size_type i = 0; i < keys_.size(); ++i)
    {
      if (keys_.contains(i))
      {
        ACL_ASSERT(keys_.get(i) < values_.size());
      }
    }
  }

private:
  template <typename T, detail::IsVoidOrRLink<link> ulink>
  static auto sfind(T& cont, ulink lnk) noexcept
    -> std::conditional_t<std::is_const_v<T>, value_type const*, value_type*>
  {
    auto idx = lnk.as_index();
    if (cont.keys_.contains(idx))
    {
      if constexpr (has_backref)
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
    return {};
  }


  template <typename T, detail::IsVoidOrRLink<link> ulink>
  static auto sfind(T& cont, ulink lnk, value_type def) noexcept -> value_type
  {
    auto idx = lnk.as_index();
    if (cont.keys_.contains(idx))
    {
      if constexpr (has_backref)
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


  template <detail::IsVoidOrRLink<link> ulink>
  inline void validate(ulink l) const noexcept
  {
    auto lnk  = l.as_index();
    auto idx  = keys_.get(lnk);
    auto self = get_ref_at_idx(idx);
    ACL_ASSERT(self == l.value());
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept
    requires(!has_backref)
  {
    return self_.get(idx);
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept
    requires(has_backref)
  {
    return self_.get(item_at_idx(idx));
  }

  inline reference item_at(size_type l) noexcept
  {
    return item_at_idx(keys_.get(l));
  }

  inline const_reference item_at(size_type l) const noexcept 
  {
    return item_at_idx(keys_.get(l));
  }

  inline reference item_at_idx(size_type item_id) noexcept
  {
    return values_[item_id];
  }

  inline const_reference item_at_idx(size_type item_id) const noexcept
  {
    return values_[item_id];
  }

  template <detail::IsVoidOrRLink<link> ulink>
  inline void erase_at(ulink l) noexcept
  {
    auto lnk       = l.as_index();
    auto item_id   = keys_.get(lnk);
    keys_.get(lnk) = std::numeric_limits<size_type>::max();
    reference lb   = values_[item_id];
    reference back = values_.back();
    if (&back != &lb)
    {

      if constexpr (has_backref)
        keys_.get(link(self_.get(back)).as_index()) = item_id;
      else
        keys_.get(link(self_.best_erase(item_id)).as_index()) = item_id;

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
      if constexpr (!has_backref)
        self_.pop_back();
    }

    values_.pop_back();
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

} // namespace acl
