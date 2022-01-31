#pragma once

#include "detail/indirection.hpp"
#include "detail/utils.hpp"
#include "link.hpp"
#include "podvector.hpp"
#include "table_traits.hpp"
#include <memory>

namespace acl
{

template <typename Ty, typename Allocator = std::allocator<Ty>>
class sparse_table : public acl::detail::sparse_table_base<Ty, Allocator>
{
  static_assert(sizeof(Ty) >= sizeof(acl::size_type<Ty>), "Type must big enough to hold a link");

public:
  using value_type       = Ty;
  using size_type        = acl::size_type<value_type>;
  using link             = acl::link<value_type>;
  using allocator_type   = Allocator;
  using allocator_traits = std::allocator_traits<Allocator>;

private:
  static constexpr auto pool_div  = detail::log2(acl::pool_size_v<value_type>);
  static constexpr auto pool_size = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_mod  = pool_size - 1;
  using this_type                 = sparse_table<value_type, Allocator>;
  using base_type                 = detail::sparse_table_base<value_type, Allocator>;
  using storage                   = detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
  using allocator                 = typename allocator_traits::template rebind_alloc<storage*>;

public:
  inline sparse_table() noexcept {}
  inline sparse_table(Allocator&& alloc) noexcept : base_type(std::move<Allocator>(alloc)) {}
  inline sparse_table(Allocator const& alloc) noexcept : base_type(alloc) {}
  inline sparse_table(sparse_table&& other) noexcept = default;
  inline sparse_table(sparse_table const& other) noexcept requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  inline ~sparse_table()
  {
    clear();
    shrink_to_fit();
  }

  sparse_table& operator=(sparse_table&& other) noexcept = default;

  sparse_table& operator=(sparse_table const& other) noexcept requires(std::is_copy_constructible_v<value_type>)
  {
    items.resize(other.items.size());
    for (auto& data : items)
      data = reinterpret_cast<storage*>(allocator_traits::allocate(*this, sizeof(storage) * pool_size));

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

  /// @brief Erase a single element.
  void remove(link l) noexcept
  {
    if (detail::debug)
      validate(l);
    erase_at(l.value());
  }

  /// @brief Erase a single element by object when backref is available.
  /// @remarks Only available if backref is available
  void remove(value_type const& obj) noexcept requires(detail::has_backref_v<value_type>)
  {
    erase_at(base_type::get_ref(obj));
  }

  /// @brief Drop unused pages
  void shrink_to_fit() noexcept
  {
    auto block = (extend + pool_size - 1) >> pool_div;
    for (auto i = block, end = static_cast<size_type>(items.size()); i < end; ++i)
      allocator_traits::deallocate(static_cast<Allocator&>(*this),
                                   reinterpret_cast<allocator_traits::pointer>(items[i]), sizeof(storage) * pool_size);
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
    first_free_index = 0;
    base_type::clear();
  }

  value_type& at(link l) noexcept
  {
    if (detail::debug)
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

private:
  inline void validate(link l) const noexcept
  {
    auto idx  = detail::index_val(l.value());
    auto self = get_ref_at_idx(idx);
    assert(self == l.value());
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept requires(!detail::has_backref_v<value_type>)
  {
    return base_type::get_ref(idx);
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept requires(detail::has_backref_v<value_type>)
  {
    return base_type::get_ref(item_at_idx(idx));
  }

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept requires(!detail::has_backref_v<value_type>)
  {
    return base_type::set_ref(idx, lnk);
  }

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept requires(detail::has_backref_v<value_type>)
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

    if constexpr (detail::has_backref_v<value_type>)
      base_type::set_ref(item, first_free_index);
    else
      set_ref_at_idx(lnk, first_free_index);

    first_free_index = newlnk;
  }

  inline auto ensure_slot() noexcept
  {
    length++;
    size_type lnk;
    if (first_free_index == link::null)
    {
      auto block = extend >> pool_div;
      if (block >= items.size())
        items.emplace_back(allocator_traits::allocate(*this, sizeof(storage) * pool_size));

      lnk = extend++;
    }
    else
    {
      lnk              = first_free_index;
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
  size_type                      first_free_index = link::null;
};

} // namespace acl