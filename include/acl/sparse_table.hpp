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
class sparse_table : public Allocator
{
  static_assert(sizeof(Ty) >= sizeof(typename Traits::size_type), "Type must big enough to hold a link");

public:
  using value_type     = Ty;
  using size_type      = detail::choose_size_t<uint32_t, Traits>;
  using link           = acl::link<value_type, size_type>;
  using allocator_type = Allocator;

private:
  static constexpr auto pool_div    = detail::log2(Traits::pool_size);
  static constexpr auto pool_size   = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_mod    = pool_size - 1;
  static constexpr bool has_backref = detail::HasBackrefValue<Traits>;
  using this_type                   = sparse_table<Ty, Allocator, Traits>;
  using storage                     = detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
  using allocator                   = Allocator;

  struct default_index_pool_size
  {
    static constexpr uint32_t self_index_pool_size = 128;
  };

  struct self_index_traits_base
  {
    using size_type = uint32_t;
    static constexpr uint32_t pool_size =
      std::conditional_t<detail::HasSelfIndexPoolSize<Traits>, Traits, default_index_pool_size>::self_index_pool_size;
    static constexpr bool     use_sparse_index = true;
    static constexpr uint32_t null_v           = 0;
    static constexpr bool     zero_memory      = true;
  };

  template <typename TrTy>
  struct self_index_traits : self_index_traits_base
  {};

  template <detail::HasBackrefValue TrTy>
  struct self_index_traits<TrTy> : self_index_traits_base
  {
    using offset = typename TrTy::offset;
  };

  using self_index = detail::backref_type<Allocator, self_index_traits<Traits>>;

public:
  inline sparse_table() noexcept {}
  inline sparse_table(Allocator&& alloc) noexcept : Allocator(std::move<Allocator>(alloc)) {}
  inline sparse_table(Allocator const& alloc) noexcept : Allocator(alloc) {}
  inline sparse_table(sparse_table&& other) noexcept
  {
    *this = std::move(other);
  }
  inline sparse_table(sparse_table const& other) noexcept
  requires(std::is_copy_constructible_v<value_type>)
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

    (Allocator&)* this = std::move((Allocator&)other);
    items_             = std::move(other.items_);
    self_              = std::move(other.self_);
    length_            = other.length_;
    extend_            = other.extend_;
    free_slot_         = other.free_slot_;
    other.length_      = 0;
    other.extend_      = 1;
    other.free_slot_   = link::null_v;
    return *this;
  }

  sparse_table& operator=(sparse_table const& other) noexcept
  requires(std::is_copy_constructible_v<value_type>)
  {
    clear();
    shrink_to_fit();

    static_cast<Allocator&>(*this) = static_cast<Allocator const&>(other);
    items_.resize(other.items_.size());
    for (auto& data : items_)
      data = acl::allocate<storage>(*this, sizeof(storage) * pool_size);

    for (size_type first = 1; first != other.extend_; ++first)
    {
      auto const& src = reinterpret_cast<value_type const&>(other.item_at_idx(first));
      auto&       dst = reinterpret_cast<value_type&>(item_at_idx(first));

      auto ref = other.get_ref_at_idx(first);
      if (is_valid_ref(ref))
        std::construct_at(&dst, src);
      if constexpr (has_backref)
        set_ref_at_idx(first, ref);
    }
    self_      = other.self_;
    extend_    = other.extend_;
    length_    = other.length_;
    free_slot_ = other.free_slot_;
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
    return length_;
  }

  /// @brief Returns capacity of packed array
  size_type capacity() const noexcept
  {
    return static_cast<size_type>(items_.size()) * pool_size;
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
    return extend_;
  }

  /// @brief packed_table has active pool count depending upon number of elements it contains
  /// @return active pool count
  size_type active_pools() const noexcept
  {
    return extend_ >> pool_div;
  }

  /// @brief Get item pool and number of items_ give the pool number
  /// @param i Must be between [0, active_pools())
  /// @return Item pool raw array and array size
  auto get_pool(size_type i) const noexcept -> std::tuple<value_type const*, size_type>
  {
    return {reinterpret_cast<value_type const*>(items_[i]),
            items_[i] == items_.back() ? extend_ & pool_mod : pool_size};
  }

  auto get_pool(size_type i) noexcept -> std::tuple<value_type*, size_type>
  {
    return {reinterpret_cast<value_type*>(items_[i]), items_[i] == items_.back() ? extend_ & pool_mod : pool_size};
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

    std::construct_at(reinterpret_cast<value_type*>(items_[block] + index), std::forward<Args>(args)...);
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
  void erase(value_type const& obj) noexcept
  requires(has_backref)
  {
    erase_at(self_.get(obj));
  }

  /// @brief Drop unused pages
  void shrink_to_fit() noexcept
  {
    auto block = (extend_ + pool_size - 1) >> pool_div;
    for (auto i = block, end = static_cast<size_type>(items_.size()); i < end; ++i)
      acl::deallocate(*this, items_[i], sizeof(storage) * pool_size);
    items_.resize(block);
    items_.shrink_to_fit();
    self_.shrink_to_fit();
  }

  /// @brief Set size to 0, memory is not released, objects are destroyed
  void clear() noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<value_type>)
      for_each(
        [](Ty& v)
        {
          std::destroy_at(std::addressof(v));
        });
    extend_    = 1;
    length_    = 0;
    free_slot_ = link::null_v;
    self_.clear();
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
    return idx < extend_ && is_valid_ref(get_ref_at_idx(idx));
  }

  bool empty() const noexcept
  {
    return length_ == 0;
  }

private:
  inline void validate(link l) const noexcept
  {
    auto idx  = detail::index_val(l.value());
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

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept
  requires(!has_backref)
  {
    return self_.ensure_at(idx) = lnk;
  }

  inline auto set_ref_at_idx(size_type idx, size_type lnk) noexcept
  requires(has_backref)
  {
    return self_.get(item_at_idx(idx)) = lnk;
  }

  inline auto& item_at(size_type l) noexcept
  {
    return item_at_idx(l);
  }

  inline auto& item_at_idx(size_type item_id) noexcept
  {
    return reinterpret_cast<value_type&>(items_[item_id >> pool_div][item_id & pool_mod]);
  }

  inline auto const& item_at_idx(size_type item_id) const noexcept
  {
    return reinterpret_cast<value_type const&>(items_[item_id >> pool_div][item_id & pool_mod]);
  }

  inline void erase_at(size_type l) noexcept
  {
    length_--;

    auto lnk = detail::index_val(l);

    auto& item = reinterpret_cast<value_type&>(items_[lnk >> pool_div][lnk & pool_mod]);

    if constexpr (!std::is_trivially_destructible_v<value_type>)
      std::destroy_at(&item);

    auto newlnk = detail::revise_invalidate(l);

    if constexpr (has_backref)
      self_.get(item) = free_slot_;
    else
      set_ref_at_idx(lnk, free_slot_);

    free_slot_ = newlnk;
  }

  inline auto ensure_slot() noexcept
  {
    length_++;
    size_type lnk;
    if (free_slot_ == link::null_v)
    {
      auto block = extend_ >> pool_div;
      if (block >= items_.size())
        items_.emplace_back(acl::allocate<storage>(*this, sizeof(storage) * pool_size));

      lnk = extend_++;
    }
    else
    {
      lnk        = detail::validate(free_slot_);
      free_slot_ = get_ref_at_idx(detail::index_val(lnk));
    }
    return lnk;
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept value_type& parameter
  template <typename Lambda, typename Cast>
  void for_each_l(Lambda&& lambda) noexcept
  {
    for_each_l<Lambda, Cast>(1, extend_, std::forward<Lambda>(lambda));
  }

  template <typename Lambda, typename Cast>
  void for_each_l(size_type first, size_type last, Lambda&& lambda) noexcept
  {
    constexpr auto arity = function_traits<Lambda>::arity;

    for (; first < last; ++first)
    {
      auto ref = get_ref_at_idx(first);
      if (is_valid_ref(ref))
      {
        if constexpr (arity == 2)
          lambda(link(get_ref_at_idx(first)), reinterpret_cast<Cast&>(items_[first >> pool_div][first & pool_mod]));
        else
          lambda(reinterpret_cast<Cast&>(items_[first >> pool_div][first & pool_mod]));
      }
    }
  }

  static inline bool is_valid_ref(size_type r)
  {
    return (r && detail::is_valid(r));
  }

  podvector<storage*, allocator> items_;
  self_index                     self_;
  size_type                      length_    = 0;
  size_type                      extend_    = 1;
  size_type                      free_slot_ = link::null_v;
};

} // namespace acl