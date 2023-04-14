
#pragma once
#include "link.hpp"
#include "sparse_vector.hpp"
#include "detail/indirection.hpp"

#include <optional>

namespace acl
{

/// @brief This is an associated container where the 'index' is a key to an element in the table
///   The Key is a link<Ty> type of object.
/// @tparam Ty Type of object contained in the array
/// @tparam Allocator Underlying allocator
/// @tparam Traits Traits controlling the contianer feature:
///   At a minimum traits are expected to contain :
///   [optional] - index_pool_size Power of 2, count of elements in a single index pool (defaults to: 32)
/// References are invalidated on insert and erase
template <typename Ty, typename Allocator = default_allocator<>, typename Traits = acl::traits<Ty>>
class indexed_table : public detail::index_table_base<Ty, Allocator, Traits>
{
  struct default_index_pool_size
  {
    static constexpr uint32_t index_pool_size = 128;
  };

  struct index_traits
  {
    static constexpr uint32_t pool_size =
      std::conditional_t<traits_has_index_pool_size<Traits>, Traits, default_index_pool_size>::index_pool_size;
    static constexpr uint32_t null_v = 0;
  };

  using base = detail::index_table_base<Ty, Allocator, Traits>;
  using base::set_ref;
  using base::get_ref;
  using base::clear;
public:
  indexed_table() noexcept = default;

  using optional_ptr = acl::detail::optional_ptr<Ty>;
  using link = acl::link<Ty>;
  using index = uint32_t;

  /// @brief Test if a certain link was added to the map
  bool contains(link l) noexcept
  {
    return keys_.contains(l.value());
  }

  /// @brief Find an object associated with a link
  /// @return optional value of the object
  optional_ptr find(link lnk) noexcept
  {
    auto idx = keys_.get_value(lnk.value());
    if (idx)
      return values_[idx];
    return {};
  }

  /// @brief Add a link to the map, and construct an object to assoicate with the link
  template <typename... Args>
  void emplace(link key, Args&&... args)
  {
    auto idx = (uint32_t)values_.size();
    keys_.emplace_at(key.value(), idx);
    values_.emplace_back(std::forward<Args>(args)...);
    set_ref_at_idx(idx, key);
  }

  void erase(link key)
  {
    assert(contains(key));
    auto index     = keys_.get_unsafe(key.value());
    auto lnk = get_ref(values_.back());
    values_[index] = std::move(values_.back());
    set_ref_at_idx(index, key);
    values_.pop_back();
  }

  void clear()
  {
    base::clear();
    values_.clear();
    keys_.clear();
  }

  
  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for (size_type i = 0, end = (size_type)values_.size(); i < end; ++i)
      lambda(values_[i], link(get_ref_at_idx(i)));
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for (size_type i = 0, end = (size_type)values_.size(); i < end; ++i)
      lambda(values_[i], link(get_ref_at_idx(i)));
  }

  Ty& operator[](link key) noexcept
  {
    auto& k = keys_.ensure(key.value());
    if (!k)
    {
      k = (uint32_t)values_.size();
      values_.emplace_back();
      return values_.back();
    }
      
    auto values_[k];
  }

  Ty const& operator[](link lnk) const noexcept
  {
    assert(contains(lnk));
    auto index = keys_.get_unsafe(key.value());
    return values_[index]
  }

private:
  static constexpr bool has_backref = detail::has_backref_v<Traits>;
  
  inline auto get_ref_at_idx(size_type idx) const noexcept
    requires(!has_backref)
  {
    return get_ref(idx);
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept
    requires(has_backref)
  {
    return get_ref(item_at_idx(idx));
  }

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept
    requires(!has_backref)
  {
    return set_ref(idx, lnk);
  }

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept
    requires(has_backref)
  {
    return set_ref(item_at_idx(idx), lnk);
  }

  using indices = acl::sparse_vector<uint32_t, Allocator, index_traits>;

  vector<Ty, Allocator> values_ = vector<Ty, Allocator>(1);
  indices               keys_;
};
} // namespace acl
