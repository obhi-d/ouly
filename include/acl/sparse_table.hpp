#pragma once

#include "allocator.hpp"
#include "default_allocator.hpp"
#include "detail/indirection.hpp"
#include "detail/utils.hpp"
#include "link.hpp"
#include "podvector.hpp"
#include "type_traits.hpp"
#include <memory>

namespace acl
{

template <typename Ty, typename Allocator = default_allocator<>, typename Traits = acl::traits<Ty>>
class sparse_table : public acl::detail::sparse_table_base<Ty, Allocator, Traits>
{
  static_assert(sizeof(Ty) >= sizeof(typename Traits::size_type), "Type must big enough to hold a link");

public:
  using value_type     = Ty;
  using size_type      = detail::choose_size_t<Traits, Allocator>;
  using link           = acl::link<value_type, size_type>;
  using allocator_type = Allocator;

private:
  static constexpr auto pool_div    = detail::log2(Traits::pool_size);
  static constexpr auto pool_size   = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_mod    = pool_size - 1;
  static constexpr bool has_backref = detail::has_backref_v<Traits>;
  using this_type                   = sparse_table<value_type, Allocator, Traits>;
  using base_type                   = detail::sparse_table_base<value_type, Allocator, Traits>;
  using storage                     = detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
  using allocator                   = Allocator;

public:
  inline sparse_table() noexcept {}
  inline sparse_table(Allocator&& alloc) noexcept : base_type(std::move<Allocator>(alloc)) {}
  inline sparse_table(Allocator const& alloc) noexcept : base_type(alloc) {}
  inline sparse_table(sparse_table&& other) noexcept
  {
    *this = std::move(other);
  }
  inline sparse_table(sparse_table const& other) noexcept requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  inline ~sparse_table()
  {
    clear();
    shrink_to_fit();
  }

  sparse_table& operator=(sparse_table&& other) noexcept
  {
    clear();
    shrink_to_fit();

    (base_type&)* this     = std::move((base_type&)other);
    items                  = std::move(other.items);
    length                 = other.length;
    extend                 = other.extend;
    first_free_index       = other.first_free_index;
    other.length           = 0;
    other.extend           = 0;
    other.first_free_index = link::null_v;
    return *this;
  }

  sparse_table& operator=(sparse_table const& other) noexcept requires(std::is_copy_constructible_v<value_type>)
  {
    clear();
    shrink_to_fit();

    items.resize(other.items.size());
    for (auto& data : items)
      data = acl::allocate<storage>(*this, sizeof(storage) * pool_size);

    for (size_type first = 0; first != other.extend; ++first)
    {
      auto const& src = reinterpret_cast<value_type const&>(other.items[first >> pool_div][first & pool_mod]);
      auto&       dst = reinterpret_cast<value_type&>(items[first >> pool_div][first & pool_mod]);

      auto ref = other.get_ref_at_idx(first);
      if (detail::is_valid(ref))
        std::construct_at(&dst, src);

      set_ref_at_idx(first, ref);
    }

    static_cast<base_type&>(*this) = static_cast<base_type const&>(other);
    extend                         = other.extend;
    length                         = other.length;
    first_free_index               = other.first_free_index;
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
  /// @param first first index in range. This should be between 0 and size()
  /// @param last last index in range. This should be between 0 and size()
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
    return length;
  }

  /// @brief Returns capacity of packed array
  size_type capacity() const noexcept
  {
    return static_cast<size_type>(items.size()) * pool_size;
  }

  /// @brief Returns the maximum entry slot currently already reserved for the table.
  /// @remarks This value is more than item capacity, and is the current max link value.
  size_type max_size() const noexcept
  {
    return capacity();
  }

  /// @brief Valid range that can be iterated over
  size_type range() const noexcept
  {
    return extend;
  }

  /// @brief packed_table has active pool count depending upon number of elements it contains
  /// @return active pool count
  size_type active_pools() const noexcept
  {
    return extend >> pool_div;
  }

  /// @brief Get item pool and number of items give the pool number
  /// @param i Must be between [0, active_pools())
  /// @return Item pool raw array and array size
  auto get_pool(size_type i) const noexcept -> std::tuple<value_type const*, size_type>
  {
    return {reinterpret_cast<value_type const*>(items[i]), items[i] == items.back() ? extend & pool_mod : pool_size};
  }

  auto get_pool(size_type i) noexcept -> std::tuple<value_type*, size_type>
  {
    return {reinterpret_cast<value_type*>(items[i]), items[i] == items.back() ? extend & pool_mod : pool_size};
  }

  /// @brief Emplace back an element. Order is not guranteed.
  /// @tparam ...Args Constructor args for value_type
  /// @return Returns link to the element pushed. link can be used to destroy entry.
  template <typename... Args>
  link emplace(Args&&... args) noexcept
  {
    auto lnk = ensure_slot();
    auto idx = detail::index_val(lnk);

    auto block = idx >> pool_div;
    auto index = idx & pool_mod;

    std::construct_at(reinterpret_cast<value_type*>(items[block] + index), std::forward<Args>(args)...);
    set_ref_at_idx(idx, lnk);

    return link(lnk);
  }

  /// @brief Construct an item in a given location, assuming the location was empty
  void replace(link point, value_type&& args) noexcept
  {
    if constexpr (detail::debug)
      assert(contains(point));

    at(point) = std::move(args);
  }

  /// @brief Erase a single element.
  void erase(link l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    erase_at(l.value());
  }

  /// @brief Erase a single element by object when backref is available.
  /// @remarks Only available if backref is available
  void erase(value_type const& obj) noexcept requires(has_backref)
  {
    erase_at(base_type::get_ref(obj));
  }

  /// @brief Drop unused pages
  void shrink_to_fit() noexcept
  {
    auto block = (extend + pool_size - 1) >> pool_div;
    for (auto i = block, end = static_cast<size_type>(items.size()); i < end; ++i)
      acl::deallocate(*this, items[i], sizeof(storage) * pool_size);
    items.resize(block);
    items.shrink_to_fit();
    base_type::shrink_to_fit(extend);
  }

  /// @brief Set size to 0, memory is not released, objects are destroyed
  void clear() noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<value_type>)
      for_each(
        [](auto, auto& v)
        {
          std::destroy_at(std::addressof(v));
        });
    extend           = 0;
    length           = 0;
    first_free_index = link::null_v;
    base_type::clear();
  }

  value_type& at(link l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(detail::index_val(l.value()));
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

  bool contains(link l) const noexcept
  {
    auto idx = detail::index_val(l.value());
    return idx < extend && detail::is_valid(get_ref_at_idx(idx));
  }

  bool empty() const noexcept
  {
    return length == 0;
  }

private:
  inline void validate(link l) const noexcept
  {
    auto idx  = detail::index_val(l.value());
    auto self = get_ref_at_idx(idx);
    assert(self == l.value());
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept requires(!has_backref)
  {
    return base_type::get_ref(idx);
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept requires(has_backref)
  {
    return base_type::get_ref(item_at_idx(idx));
  }

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept requires(!has_backref)
  {
    return base_type::set_ref(idx, lnk);
  }

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept requires(has_backref)
  {
    return base_type::set_ref(item_at_idx(idx), lnk);
  }

  inline auto& item_at(size_type l) noexcept
  {
    return item_at_idx(l);
  }

  inline auto& item_at_idx(size_type item_id) noexcept
  {
    return reinterpret_cast<value_type&>(items[item_id >> pool_div][item_id & pool_mod]);
  }

  inline auto const& item_at_idx(size_type item_id) const noexcept
  {
    return reinterpret_cast<value_type const&>(items[item_id >> pool_div][item_id & pool_mod]);
  }

  inline void erase_at(size_type l) noexcept
  {
    length--;

    auto lnk = detail::index_val(l);

    auto& item = reinterpret_cast<value_type&>(items[lnk >> pool_div][lnk & pool_mod]);

    if constexpr (!std::is_trivially_destructible_v<value_type>)
      std::destroy_at(&item);

    auto newlnk = detail::revise_invalidate(l);

    if constexpr (has_backref)
      base_type::set_ref(item, first_free_index);
    else
      set_ref_at_idx(lnk, first_free_index);

    first_free_index = newlnk;
  }

  inline auto ensure_slot() noexcept
  {
    length++;
    size_type lnk;
    if (first_free_index == link::null_v)
    {
      auto block = extend >> pool_div;
      if (block >= items.size())
        items.emplace_back(acl::allocate<storage>(*this, sizeof(storage) * pool_size));

      lnk = extend++;
    }
    else
    {
      lnk              = detail::validate(first_free_index);
      first_free_index = get_ref_at_idx(detail::index_val(lnk));
    }
    return lnk;
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept value_type& parameter
  template <typename Lambda, typename Cast>
  void for_each_l(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, Cast>(0, extend, std::forward<Lambda>(lambda));
  }

  template <typename Lambda, typename Cast>
  void for_each_l(size_type first, size_type last, Lambda&& lambda) noexcept
  {
    for (; first != last; ++first)
    {
      auto ref = get_ref_at_idx(first);
      if (detail::is_valid(ref))
        lambda(link(get_ref_at_idx(first)), reinterpret_cast<Cast&>(items[first >> pool_div][first & pool_mod]));
    }
  }

  podvector<storage*, allocator> items;
  size_type                      length           = 0;
  size_type                      extend           = 0;
  size_type                      first_free_index = link::null_v;
};

} // namespace acl