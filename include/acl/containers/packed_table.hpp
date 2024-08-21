#pragma once

#include "sparse_vector.hpp"
#include <acl/containers/indirection.hpp>
#include <acl/utils/link.hpp>
#include <acl/utils/type_traits.hpp>

#include <memory>
#include <tuple>

namespace acl
{

/**
 * @brief Contents are packed in vector or sparse vector
 * @tparam Ty Type of object
 * @tparam Options Controls the parameters for class instantiation
 *   Valid options:
 *         - use_sparse : bool - true to use sparse vector
 *         - pool_size : integer - Pool size of the sparse vector when sparse vector is used to store data
 *         - self_index : typename acl::opt::self_index<&type::offset> self backref member
 *         - self_index_pool_size : integer - when offset is missing, indicates the pool size of self references if
 *         they use sparse vector
 *         - keys_index_pool_size : integer - indicates the pool size of keys if they use sparse vector
 *         - self_use_sparse_index : bool - use sparse vector for self reference
 *         - keys_use_sparse_index : bool - use sparse vector for key index
 *         - size_type : typename - uint32_t to reduce footprint
 */
template <typename Ty, typename Options = acl::default_options<Ty>>
class packed_table : public detail::custom_allocator_t<Options>
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
  using link            = acl::link<value_type, size_type>;
  using allocator_type  = detail::custom_allocator_t<Options>;

private:
  static constexpr bool has_self_index = detail::HasSelfIndexValue<options>;

  using this_type     = packed_table<value_type, options>;
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
    using size_type                          = uint32_t;
    static constexpr uint32_t pool_size_v    = std::conditional_t<detail::HasSelfIndexPoolSize<options>, options,
                                                               default_index_pool_size>::self_index_pool_size_v;
    static constexpr bool use_sparse_index_v = std::conditional_t<detail::HasSelfUseSparseIndexAttrib<options>, options,
                                                                  default_index_pool_size>::self_use_sparse_index_v;
    static constexpr uint32_t null_v         = 0;
    static constexpr bool     no_fill_v      = true;
    static constexpr bool     zero_out_memory_v = true;
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
    using size_type                          = uint32_t;
    static constexpr uint32_t pool_size_v    = std::conditional_t<detail::HasKeysIndexPoolSize<options>, options,
                                                               default_index_pool_size>::keys_index_pool_size_v;
    static constexpr bool use_sparse_index_v = std::conditional_t<detail::HasKeysUseSparseIndexAttrib<options>, options,
                                                                  default_index_pool_size>::keys_use_sparse_index_v;
    static constexpr uint32_t null_v         = 0;
    static constexpr bool     no_fill_v      = true;
    static constexpr bool     zero_out_memory_v = true;
  };

  using self_index = detail::self_index_type<self_index_traits<options>>;
  using key_index  = detail::indirection_type<key_index_traits>;

public:
  inline packed_table() noexcept {}
  inline packed_table(allocator_type&& alloc) noexcept : allocator_type(std::move<allocator_type>(alloc)) {}
  inline packed_table(allocator_type const& alloc) noexcept : allocator_type(alloc) {}
  inline packed_table(packed_table&& other) noexcept
  {
    *this = std::move(other);
  }
  inline packed_table(packed_table const& other) noexcept
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  inline ~packed_table()
  {
    clear();
    shrink_to_fit();
  }

  packed_table& operator=(packed_table&& other) noexcept
  {
    clear();
    shrink_to_fit();

    values_              = std::move(other.values_);
    keys_                = std::move(other.keys_);
    free_key_slot_       = other.free_key_slot_;
    self_                = std::move(other.self_);
    other.free_key_slot_ = 0;
    return *this;
  }

  packed_table& operator=(packed_table const& other) noexcept
    requires(std::is_copy_constructible_v<value_type>)
  {
    clear();
    shrink_to_fit();

    values_        = other.values_;
    keys_          = other.keys_;
    free_key_slot_ = other.free_key_slot_;
    self_          = other.self_;
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
   * @brief Emplace back an element. Order is not guranteed.
   * @tparam ...Args Constructor args for value_type
   * @return Returns link to the element pushed. link can be used to destroy entry.
   */
  template <typename... Args>
  link emplace(Args&&... args) noexcept
  {
    if (values_.empty()) [[unlikely]]
    {
      keys_.push_back(0);
      values_.emplace_back();
    }

    auto key = (uint32_t)values_.size();
    values_.emplace_back(std::forward<Args>(args)...);
    size_type l;

    if (free_key_slot_)
    {
      auto idx       = detail::validate(free_key_slot_);
      l              = detail::revise(idx);
      auto& k        = keys_.get(detail::index_val(idx));
      free_key_slot_ = k;
      k              = key;
    }
    else
    {
      l = detail::validate(keys_.size());
      keys_.push_back(key);
    }

    if constexpr (has_self_index)
    {
      self_.get(values_.back()) = l;
    }
    else
      self_.ensure_at(key) = l;
    return link(l);
  }

  /**
   * @brief Construct an item in a given location, assuming the location was empty
   */
  template <typename... Args>
  void emplace_at(link point, Args&&... args) noexcept
  {
    if (values_.empty()) [[unlikely]]
    {
      keys_.push_back(0);
      values_.emplace_back();
    }

    auto key = (uint32_t)values_.size();
    values_.emplace_back(std::forward<Args>(args)...);
    auto& k = keys_.ensure_at(point.as_index());
    if (k)
      disconnect_free(k);
    k = key;
    if constexpr (has_self_index)
    {
      self_.get(values_.back()) = point.value();
    }
    else
      self_.ensure_at(key) = point.value();
  }

  /**
   * @brief Construct an item in a given location, assuming the location was empty
   */
  void replace(link point, value_type&& args) noexcept
  {
    if constexpr (detail::debug)
      ACL_ASSERT(contains(point));

    auto  k   = keys_.get(point.as_index());
    auto& val = values_[k];
    val       = std::move(args);
    if constexpr (has_self_index)
    {
      self_.get(val) = point.value();
    }
    else
      self_.get(k) = point.value();
  }

  /**
   * @brief Erase a single element.
   */
  void erase(link l) noexcept
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
    erase_at(link(self_.get(obj)));
  }

  /**
   * @brief Find an object associated with a link
   * @return optional value of the object
   */
  optional_val find(link lnk) noexcept
  {
    if (keys_.contains_valid(lnk.as_index()))
    {
      auto idx = detail::index_val(keys_.get(lnk.as_index()));
      return optional_val(values_[idx]);
    }
    return {};
  }

  /**
   * @brief Find an object associated with a link
   * @return optional value of the object
   */
  optional_cval find(link lnk) const noexcept
  {
    if (keys_.contains_valid(lnk.as_index()))
    {
      auto idx = detail::index_val(keys_.get(lnk.as_index()));
      return optional_cval(values_[idx]);
    }
    return {};
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
    free_key_slot_ = 0;
  }

  inline reference at(link l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l.as_index());
  }

  inline const_reference at(link l) const noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l.as_index());
  }

  inline reference operator[](link l) noexcept
  {
    return at(l);
  }

  inline const_reference operator[](link l) const noexcept
  {
    return at(l);
  }

  inline bool contains(link l) const noexcept
  {
    return keys_.contains_valid(l.as_index());
  }

  inline bool empty() const noexcept
  {
    return values_.size() <= 1;
  }

  inline void validate_integrity() const
  {
    for (uint32_t first = 1, last = size(); first < last; ++first)
    {
      ACL_ASSERT(keys_.get(detail::index_val(get_ref_at_idx(first))) == first);
    }

    std::vector<uint32_t> frees;
    auto                  fi = free_key_slot_;
    while (fi)
    {
      auto idx = detail::index_val(detail::validate(fi));
      ACL_ASSERT(keys_.get(idx) != idx);
      ACL_ASSERT(std::ranges::find(frees, idx) == frees.end());
      fi = keys_.get(idx);
      frees.emplace_back(idx);
    }
  }

  inline reference item_at(size_type l) noexcept
  {
    return item_at_idx(detail::index_val(keys_.get(l)));
  }

  inline const_reference item_at(size_type l) const noexcept
  {
    return item_at_idx(detail::index_val(keys_.get(l)));
  }

private:
  inline void disconnect_free(uint32_t inode)
  {
    auto     node = detail::index_val(detail::validate(inode));
    auto     fi   = detail::index_val(detail::validate(free_key_slot_));
    uint32_t prev = 0;
    while (fi != node)
    {
      prev = fi;
      fi   = detail::index_val(detail::validate(keys_.get(fi)));
    }
    if (!prev)
      free_key_slot_ = keys_.get(fi);
    else
      keys_.get(prev) = keys_.get(fi);
  }

  inline void validate(link l) const noexcept
  {
    auto lnk  = l.as_index();
    auto idx  = keys_.get(lnk);
    auto self = get_ref_at_idx(idx);
    ACL_ASSERT(self == l.value());
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept
    requires(!has_self_index)
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

  inline void erase_at(link l) noexcept
  {
    auto lnk       = l.as_index();
    auto item_id   = keys_.get(lnk);
    keys_.get(lnk) = free_key_slot_;
    free_key_slot_ = detail::invalidate(l.value());
    reference lb   = values_[item_id];
    reference back = values_.back();
    if (&back != &lb)
    {

      if constexpr (has_self_index)
        keys_.get(detail::index_val(self_.get(back))) = item_id;
      else
        keys_.get(detail::index_val(self_.best_erase(item_id))) = item_id;

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

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept value_type& parameter
   */
  template <typename Lambda, typename Cast>
  inline void for_each_l(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, Cast>(1, static_cast<size_type>(values_.size()), std::forward<Lambda>(lambda));
  }

  template <typename Lambda, typename Cast>
  inline void for_each_l(Lambda&& lambda) const noexcept
  {
    for_each_l<Lambda, Cast>(1, static_cast<size_type>(values_.size()), std::forward<Lambda>(lambda));
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
  uint32_t    free_key_slot_ = 0;
  self_index  self_;
};

} // namespace acl
