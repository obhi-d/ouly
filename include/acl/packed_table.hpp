#pragma once

#include "default_allocator.hpp"
#include "detail/indirection.hpp"
#include "link.hpp"
#include "sparse_vector.hpp"
#include "type_traits.hpp"

#include <memory>
#include <tuple>

namespace acl
{
/// @brief Contenst are packed in vector or sparse vector
/// @tparam Ty Type of object
/// @tparam Allocator Underlying allocator
/// @tparam Traits Controls the parameters for class instantiation
///   At a minimum expected traits must include:
///         - use_sparse : bool - true to use sparse vector
///         - pool_size : integer - Pool size of the sparse vector when sparse vector is used to store data
///         - offset : typename - acl::offset<&type::offset> self backref member
///         - self_index_pool_size : integer - when offset is missing, indicates the pool size of self references if
///         they use sparse vector
///         - keys_index_pool_size : integer - indicates the pool size of keys if they use sparse vector
///         - self_use_sparse_index : bool - use sparse vector for self reference
///         - keys_use_sparse_index : bool - use sparse vector for key index
///         - size_type : typename - uint32_t to reduce footprint
template <typename Ty, typename Allocator = default_allocator<>, typename Traits = acl::traits<Ty>>
class packed_table : public Allocator
{

  static_assert(std::is_move_assignable_v<Ty>, "Type must be move assignable");
  static_assert(std::is_default_constructible_v<Ty>, "Type must be default constructibe");

public:
  using value_type     = Ty;
  using size_type      = detail::choose_size_t<uint32_t, Traits>;
  using link           = acl::link<value_type, size_type>;
  using allocator_type = Allocator;

private:
  static constexpr bool has_backref = detail::has_backref_v<Traits>;
  static constexpr bool has_sparse  = detail::has_use_sparse_attrib<Traits>;

  using this_type    = packed_table<value_type, Allocator, Traits>;
  using vector_type  = std::conditional_t<has_sparse, sparse_vector<Ty, Allocator, Traits>, vector<Ty, Allocator>>;
  using storage      = detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
  using allocator    = Allocator;
  using optional_ptr = acl::detail::optional_ptr<Ty>;

  struct default_index_pool_size
  {
    static constexpr uint32_t self_index_pool_size  = 4096;
    static constexpr bool     self_use_sparse_index = false;
    static constexpr uint32_t keys_index_pool_size  = 4096;
    static constexpr bool     keys_use_sparse_index = false;
  };

  struct self_index_traits_base
  {
    using size_type                     = uint32_t;
    static constexpr uint32_t pool_size = std::conditional_t<detail::has_self_index_pool_size<Traits>, Traits,
                                                             default_index_pool_size>::self_index_pool_size;
    static constexpr bool     use_sparse_index =
      std::conditional_t<detail::has_self_use_sparse_index_attrib<Traits>, Traits,
                         default_index_pool_size>::self_use_sparse_index;
    static constexpr uint32_t null_v      = 0;
    static constexpr bool     zero_memory = true;
  };

  template <typename Ty>
  struct self_index_traits : self_index_traits_base
  {};

  template <detail::has_backref_v Ty>
  struct self_index_traits<Ty> : self_index_traits_base
  {
    using offset = typename Ty::offset;
  };

  struct key_index_traits
  {
    using size_type                            = uint32_t;
    static constexpr uint32_t pool_size        = std::conditional_t<detail::has_keys_index_pool_size<Traits>, Traits,
                                                             default_index_pool_size>::keys_index_pool_size;
    static constexpr bool     use_sparse_index = std::conditional_t<detail::has_keys_use_sparse_index_attrib<Traits>, Traits,
                         default_index_pool_size>::keys_use_sparse_index;
    static constexpr uint32_t null_v           = 0;
    static constexpr bool     zero_memory      = true;
  };

  using self_index = detail::backref_type<Allocator, self_index_traits<Traits>>;
  using key_index  = detail::indirection_type<Allocator, key_index_traits>;

public:
  inline packed_table() noexcept
  {
  }
  inline packed_table(Allocator&& alloc) noexcept : Allocator(std::move<Allocator>(alloc))
  {
  }
  inline packed_table(Allocator const& alloc) noexcept : Allocator(alloc)
  {
  }
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

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, value_type>(std::forward<Lambda>(lambda));
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    const_cast<this_type*>(this)->for_each_l<Lambda, value_type const>(std::forward<Lambda>(lambda));
  }

  /// @brief Lambda called for each element in range
  /// @tparam Lambda Lambda Lambda should accept A link and value_type& parameter
  /// @param first first index in range. This should be between 1 and size()
  /// @param last last index in range. This should be between 1 and size()
  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, value_type>(first, last, std::forward<Lambda>(lambda));
  }

  /// @copydoc for_each
  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) const noexcept
  {
    const_cast<this_type*>(this)->for_each_l<Lambda, value_type const>(first, last, std::forward<Lambda>(lambda));
  }

  /// @brief Returns size of packed array
  size_type size() const noexcept
  {
    return (size_type)values_.size();
  }

  /// @brief Valid range that can be iterated over
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

  /// @brief Emplace back an element. Order is not guranteed.
  /// @tparam ...Args Constructor args for value_type
  /// @return Returns link to the element pushed. link can be used to destroy entry.
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
      l              = detail::revise(detail::validate(free_key_slot_));
      auto& k        = keys_.get(detail::index_val(free_key_slot_));
      free_key_slot_ = k;
      k              = key;
    }
    else
    {
      l = keys_.size();
      keys_.push_back(key);
    }

    if constexpr (has_backref)
    {
      self_.get(values_.back()) = l;
    }
    else
      self_.ensure_at(key) = l;
    return link(l);
  }

  /// @brief Construct an item in a given location, assuming the location was empty
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
    if constexpr (has_backref)
    {
      self_.get(values_.back()) = point.value();
    }
    else
      self_.ensure_at(key) = point.value();
  }

  /// @brief Construct an item in a given location, assuming the location was empty
  void replace(link point, value_type&& args) noexcept
  {
    if constexpr (detail::debug)
      assert(contains(point));

    auto  k   = keys_.get(point.as_index());
    auto& val = values_[k];
    val       = std::move(args);
    if constexpr (has_backref)
    {
      self_.get(val) = point.value();
    }
    else
      self_.get(k) = point.value();
  }

  /// @brief Erase a single element.
  void erase(link l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    erase_at(l);
  }

  /// @brief Erase a single element by object when backref is available.
  /// @remarks Only available if backref is available
  void erase(value_type const& obj) noexcept
    requires(has_backref)
  {
    erase_at(link(self_.get(obj)));
  }

  /// @brief Find an object associated with a link
  /// @return optional value of the object
  optional_ptr find(link lnk) noexcept
  {
    if (keys_.contains(lnk))
    {
      auto idx = keys_.get(lnk.as_index());
      return values_[idx];
    }
    return {};
  }

  /// @brief Drop unused pages
  inline void shrink_to_fit() noexcept
  {
    keys_.shrink_to_fit();
    values_.shrink_to_fit();
    self_.shrink_to_fit();
  }

  /// @brief Set size to 0, memory is not released, objects are destroyed
  inline void clear() noexcept
  {
    keys_.clear();
    values_.clear();
    self_.clear();
    free_key_slot_ = 0;
  }

  value_type& at(link l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l.as_index());
  }

  value_type const& at(link l) const noexcept
  {
    return const_cast<this_type*>(this)->at(l);
  }

  value_type& operator[](link l) noexcept
  {
    return at(l);
  }

  value_type const& operator[](link l) const noexcept
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
    for (uint32_t first = 1, last = size(); first != last; ++first)
    {
      assert(keys_.get(detail::index_val(get_ref_at_idx(first))) == first);
    }

    std::vector<uint32_t> frees;
    auto fi = free_key_slot_;
    while (fi)
    {
      auto idx = detail::index_val(fi);
      assert(keys_.get(idx) != idx);
      assert(std::ranges::find(frees, idx) == frees.end());
      fi = keys_.get(idx);
      frees.emplace_back(idx);
    }
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
    assert(self == l.value());
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

  inline auto& item_at(size_type l) noexcept
  {
    return item_at_idx(detail::index_val(keys_.get(l)));
  }

  inline auto& item_at_idx(size_type item_id) noexcept
  {
    return values_[item_id];
  }

  inline auto const& item_at_idx(size_type item_id) const noexcept
  {
    return values_[item_id];
  }

  inline void erase_at(link l) noexcept
  {
    auto lnk       = l.as_index();
    auto item_id   = keys_.get(lnk);
    keys_.get(lnk) = free_key_slot_;
    free_key_slot_ = detail::invalidate(l.value());
    auto& lb       = values_[item_id];
    auto& back     = values_.back();
    if (&back != &lb)
    {

      if constexpr (has_backref)
        keys_.get(detail::index_val(self_.get(back))) = item_id;
      else
        keys_.get(detail::index_val(self_.best_erase(item_id))) = item_id;

      lb = std::move(back);
    }
    else
    {
      if constexpr (!has_backref)
        self_.pop_back();
    }

    values_.pop_back();

  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept value_type& parameter
  template <typename Lambda, typename Cast>
  inline void for_each_l(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, Cast>(1, static_cast<size_type>(values_.size()), std::forward<Lambda>(lambda));
  }

  template <typename Lambda, typename Cast>
  inline void for_each_l(size_type first, size_type last, Lambda&& lambda) noexcept
  {
    constexpr auto arity = detail::function_traits<Lambda>::arity;
    for (; first != last; ++first)
    {
      if constexpr (arity == 2)
        lambda(link(get_ref_at_idx(first)), static_cast<Cast&>(values_[first]));
      else
        lambda(static_cast<Cast&>(values_[first]));
    }
  }

  vector_type values_;
  key_index   keys_;
  uint32_t    free_key_slot_ = 0;
  self_index  self_;
};

} // namespace acl