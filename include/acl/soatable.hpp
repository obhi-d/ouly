#pragma once

#include "default_allocator.hpp"
#include "detail/indirection.hpp"
#include "link.hpp"
#include "podvector.hpp"
#include "type_traits.hpp"
#include <memory>
#include <tuple>

namespace acl
{

// Use this for a packed table of SOA (tuple of arrays)
template <typename Tuple, typename Allocator = default_allocator<>, typename Traits = acl::traits<Tuple>>
class soatable
    : public detail::ref_indirection<Allocator, Traits, detail::base_indirection<Allocator, Traits, Allocator>>
{

  using TupleType = std::conditional_t<detail::is_tuple<Tuple>::value, Tuple, std::tuple<Tuple>>;
  template <std::size_t I>
  using constant = std::integral_constant<std::size_t, I>;

public:
  static_assert(std::is_move_assignable_v<TupleType>, "Type must be move assignable");
  using value_type                  = TupleType;
  using size_type                   = detail::choose_size_t<Traits, Allocator>;
  using link                        = acl::link<value_type, size_type>;
  using allocator_type              = Allocator;
  static constexpr auto pool_div    = detail::log2(Traits::pool_size);
  static constexpr auto pool_size   = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_mod    = pool_size - 1;
  static constexpr bool has_backref = detail::has_backref_v<Traits>;
  static constexpr auto tuple_size  = std::tuple_size_v<value_type>;
  static auto constexpr index_seq   = std::make_index_sequence<tuple_size>();
  using this_type                   = soatable<Tuple, Allocator, Traits>;
  using array_type                  = detail::tuple_of_ptrs<value_type>;
  using carray_type                 = detail::tuple_of_cptrs<value_type>;
  using ref_type                    = detail::tuple_of_refs<value_type>;
  using cref_type                   = detail::tuple_of_crefs<value_type>;
  using allocator                   = Allocator;

  using base_type = detail::ref_indirection<Allocator, Traits, detail::base_indirection<Allocator, Traits, Allocator>>;

  inline soatable() noexcept {}
  inline soatable(allocator&& alloc) noexcept : base_type(std::move<Allocator>(alloc)) {}
  inline soatable(allocator const& alloc) noexcept : base_type(alloc) {}
  inline soatable(soatable&& other) noexcept
  {
    *this = std::move(other);
  }
  inline soatable(soatable const& other) noexcept requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  inline ~soatable()
  {
    clear();
    shrink_to_fit();
  }
  soatable& operator=(soatable&& other) noexcept
  {
    (base_type&)* this     = std::move((base_type&)other);
    items                  = std::move(other.items);
    length                 = other.length;
    first_free_index       = other.first_free_index;
    other.length           = 0;
    other.first_free_index = link::null_v;
    return *this;
  }

  soatable& operator=(soatable const& other) noexcept requires(std::is_copy_constructible_v<value_type>)
  {
    items.resize(other.items.size());
    unpack(
      [&other, this]<typename I>(I)
      {
        for (auto& data : items)
          std::get<I::value>(data) = acl::allocate_count<std::tuple_element_t<I::value, value_type>>(*this, pool_size);
        for (size_type first = 0; first != other.length; ++first)
        {
          auto const& src = std::get<I::value>(other.items[first >> pool_div])[first & pool_mod];
          auto&       dst = std::get<I::value>(items[first >> pool_div])[first & pool_mod];
          std::construct_at(&dst, src);
        }
      },
      index_seq);

    static_cast<base_type&>(*this) = static_cast<base_type const&>(other);
    length                         = other.length;
    first_free_index               = other.first_free_index;
    return *this;
  }

  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for_each(0, length, std::forward<Lambda>(lambda));
  }

  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for_each(0, length, std::forward<Lambda>(lambda));
  }

  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) noexcept
  {

    [ this, &lambda ]<std::size_t... I>(std::index_sequence<I...>, size_type first, size_type last)
    {
      for (; first < last; ++first)
      {
        lambda(link(get_ref_at_idx(first)), std::get<I>(items[first >> pool_div])[first & pool_mod]...);
      }
    }
    (index_seq, first, last);
  }

  /// @copydoc for_each
  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) const noexcept
  {
    [ this, &lambda ]<std::size_t... I>(std::index_sequence<I...>, size_type first, size_type last)
    {
      for (; first < last; ++first)
      {
        lambda(link(get_ref_at_idx(first)), std::get<I>(items[first >> pool_div])[first & pool_mod]...);
      }
    }
    (index_seq, first, last);
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
    return base_type::max_size();
  }

  /// @brief Valid range that can be iterated over
  size_type range() const noexcept
  {
    return size;
  }

  /// @brief packed_table has active pool count depending upon number of elements it contains
  /// @return active pool count
  size_type active_pools() const noexcept
  {
    return length >> pool_div;
  }

  /// @brief Get item pool and number of items give the pool number
  /// @param i Must be between [0, active_pools())
  /// @return Item pool raw array and array size
  auto get_pool(size_type i) const noexcept -> std::tuple<carray_type, size_type>
  {
    return {items[i], i == (length >> pool_div) ? length & pool_mod : pool_size};
  }

  auto get_pool(size_type i) noexcept -> std::tuple<array_type, size_type>
  {
    return {items[i], i == (length >> pool_div) ? length & pool_mod : pool_size};
  }

  template <typename... Args>
  link emplace(Args&&... args) noexcept
  {
    emplace_back(std::forward<Args>(args)...);
    return do_insert(length++);
  }

  /// @brief Construct an item in a given location, assuming the location was empty
  template <typename... Args>
  void emplace_at(link point, Args&&... args) noexcept
  {
    if constexpr (detail::debug)
      assert(!contains(point));

    emplace_back(std::forward<Args>(args)...);
    do_insert(point.value(), length++);
  }

  /// @brief Construct an item in a given location, assuming the location was empty
  void replace(link point, value_type&& args) noexcept
  {
    if constexpr (detail::debug)
      assert(contains(point));

    at(point) = std::move(args);
  }

  template <typename... Args>
  /// @brief Construct an item in a given location, assuming the location was empty
  void replace(link point, Args&&... args) noexcept
  {
    if constexpr (detail::debug)
      assert(contains(point));

    at(point) = std::tie(args...);
  }

  /// @brief Erase a single element.
  void erase(link l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    erase_at(l.value());
  }

  /// @brief Drop unused pages
  void shrink_to_fit() noexcept
  {
    auto block = (length + pool_size - 1) >> pool_div;
    [ this, block ]<std::size_t... I>(std::index_sequence<I...>)
    {
      for (auto i = block, end = static_cast<size_type>(items.size()); i < end; ++i)
      {
        (acl::deallocate_count(static_cast<allocator&>(*this), std::get<I>(items[i]), pool_size), ...);
      }
    }
    (index_seq);

    items.resize(block);
    items.shrink_to_fit();
    base_type::shrink_to_fit(length);
  }

  /// @brief Set size to 0, memory is not released, objects are destroyed
  void clear() noexcept
  {
    unpack(
      [this]<typename I>(I)
      {
        using type = std::tuple_element_t<I::value, value_type>;
        if constexpr (!std::is_trivially_destructible_v<type>)
        {
          for (size_type first = 0; first != length; ++first)
          {
            auto& v = std::get<I::value>(items[first >> pool_div])[first & pool_mod];
            std::destroy_at(std::addressof(v));
          }
        }
      },
      index_seq);

    length           = 0;
    first_free_index = 0;
    base_type::clear();
  }

  ref_type at(link l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);

    return [ this, i = base_type::link_at(base_type::to_index(l.value())) ]<std::size_t... I>(std::index_sequence<I...>)
    {
      return std::tie(std::get<I>(items[i >> pool_div])[i & pool_mod]...);
    }
    (index_seq);
  }

  cref_type at(link l) const noexcept
  {
    if constexpr (detail::debug)
      validate(l);

    return [ this, i = base_type::link_at(base_type::to_index(l.value())) ]<std::size_t... I>(std::index_sequence<I...>)
    {
      return std::tie(std::get<I>(items[i >> pool_div])[i & pool_mod]...);
    }
    (index_seq);
  }

  template <std::size_t I>
  auto& get(link l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);

    auto i = base_type::link_at(base_type::to_index(l.value()));
    return std::get<I>(items[i >> pool_div])[i & pool_mod];
  }

  template <std::size_t I>
  auto const& get(link l) const noexcept
  {
    if constexpr (detail::debug)
      validate(l);

    auto i = base_type::link_at(base_type::to_index(l.value()));
    return std::get<I>(items[i >> pool_div])[i & pool_mod];
  }

  ref_type operator[](link l) noexcept
  {
    return at(l);
  }

  cref_type operator[](link l) const noexcept
  {
    return at(l);
  }

  bool contains(link l) const noexcept
  {
    auto idx = base_type::to_index(l.value());
    return base_type::contains(idx) && detail::is_valid(base_type::link_at(idx));
  }

  bool empty() const noexcept
  {
    return length == 0;
  }

private:
  inline void ensure(auto block)
  {
    if (block >= items.size())
    {
      items.push_back({});
      unpack(
        [this]<typename I>(I)
        {
          std::get<I::value>(items.back()) =
            acl::allocate_count<std::tuple_element_t<I::value, value_type>>(*this, pool_size);
        },
        index_seq);
    }
  }

  template <typename... Args>
  inline void emplace_back(Args&&... args) noexcept requires(sizeof...(Args) == tuple_size)
  {
    auto block = length >> pool_div;
    auto index = length & pool_mod;

    ensure(block);
    auto t = std::tie(args...);
    unpack(
      [this, &t]<typename I>(I)
      {
        std::construct_at(std::get<I::value>(items[length >> pool_div]) + (length & pool_mod), std::get<I::value>(t));
      },
      index_seq);
  }

  inline void emplace_back() noexcept
  {
    auto block = length >> pool_div;
    auto index = length & pool_mod;

    ensure(block);
    unpack(
      [this]<typename I>(I)
      {
        std::construct_at(std::get<I::value>(items[length >> pool_div]) + (length & pool_mod));
      },
      index_seq);
  }

  inline void move_item(size_type src, size_type dst) noexcept
  {
    unpack(
      [this, src, dst]<typename I>(I)
      {
        auto& item = std::get<I::value>(items[dst >> pool_div])[dst & pool_mod];
        auto& last = std::get<I::value>(items[src >> pool_div])[src & pool_mod];
        item       = std::move(last);
        if (!std::is_trivially_destructible_v<value_type>)
          std::destroy_at(&last);
      },
      index_seq);
  }

  inline void erase_at(size_type l) noexcept
  {
    length--;

    auto  lnk     = base_type::to_index(l);
    auto& item_id = base_type::link_at(lnk);

    auto last_slot = pop_ref_at_idx(length, item_id);

    move_item(length, item_id);

    base_type::link_at(base_type::to_index(last_slot)) = item_id;
    item_id                                            = first_free_index;
    first_free_index                                   = detail::revise_invalidate(l);
  }

  inline link do_insert(size_type loc) noexcept
  {
    size_type lnk;
    if (first_free_index == link::null_v)
    {
      lnk = base_type::to_link(base_type::push(loc));
    }
    else
    {
      lnk              = detail::validate(first_free_index);
      auto  index      = base_type::to_index(lnk);
      auto& id         = base_type::link_at(index);
      first_free_index = id;
      id               = loc;
    }

    set_ref_at_idx(loc, lnk);

    return link(lnk);
  }

  inline void do_insert(size_type lnk, size_type loc) noexcept
  {
    auto idx = base_type::to_index(lnk);
    if (base_type::contains(idx))
    {
      break_free_chain(idx);
      base_type::link_at(idx) = loc;
    }
    else
    {
      make_free_chain(base_type::insert(idx, loc), idx);
    }

    set_ref_at_idx(loc, lnk);
  }

  inline void break_free_chain(size_type idx) noexcept
  {
    auto it = detail::validate(first_free_index);
    if (base_type::to_index(it) == idx)
    {
      first_free_index = link::null_v;
      return;
    }

    while (it != link::null_v)
    {
      auto next = base_type::link_at(base_type::to_index(it));
      if (base_type::to_index(next) == idx)
        break;
      it = next;
    }
    base_type::link_at(base_type::to_index(it)) = base_type::link_at(idx);
  }

  inline void make_free_chain(size_type first, size_type last) noexcept
  {
    for (; first < last; ++first)
      add_free_slot(first);
  }

  inline void add_free_slot(size_type slot) noexcept
  {
    base_type::link_at(slot) = first_free_index;
    first_free_index         = detail::revise_invalidate(slot);
  }

  inline void validate(link l) const noexcept
  {
    auto lnk  = base_type::to_index(l.value());
    auto idx  = base_type::link_at(lnk);
    auto self = get_ref_at_idx(idx);
    assert(self == l.value());
  }

  inline auto get_ref_at_idx(size_type idx) const noexcept
  {
    return base_type::get_ref(idx);
  }

  inline auto pop_ref_at_idx(size_type src, size_type dst) noexcept
  {
    return base_type::pop_ref(src, dst);
  }

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept
  {
    return base_type::set_ref(idx, lnk);
  }

  template <typename L, std::size_t... I>
  static void unpack(L&& l, std::index_sequence<I...>)
  {
    (l(constant<I>{}), ...);
  }

  podvector<size_type*>            refs;
  podvector<array_type, allocator> items;
  size_type                        length           = 0;
  size_type                        first_free_index = link::null_v;
};

} // namespace acl