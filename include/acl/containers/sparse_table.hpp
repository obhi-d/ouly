#pragma once

#include <acl/containers/detail/indirection.hpp>
#include <acl/containers/podvector.hpp>
#include <acl/utility/type_traits.hpp>
#include <acl/utility/utils.hpp>
#include <memory>

namespace acl
{
/**
 * @brief Represents a sparse table of elements. Free slots are reused.
 * @tparam Ty Vector type
 * @tparam allocator_type Underlying allocator_type
 * @tparam Config At minimum the config must define:
 *          - pool_size Power of 2, count of elements in a single chunk/page/pool
 *          - [optional] using self_index = typename acl::cfg::self_index<&type::offset> for self pointer
 *          - [optional] self_index_pool_size Pool size for self indices if offset is missing
 */
template <typename Ty, typename Config = acl::default_config<Ty>>
class sparse_table : public acl::detail::custom_allocator_t<Config>
{

public:
  using config          = Config;
  using value_type      = Ty;
  using reference       = Ty&;
  using const_reference = Ty const&;
  using pointer         = Ty*;
  using const_pointer   = Ty const*;
  using size_type       = acl::detail::choose_size_t<uint32_t, Config>;
  using link            = size_type;
  using allocator_type  = acl::detail::custom_allocator_t<Config>;

  static_assert(sizeof(Ty) >= sizeof(size_type), "Type must big enough to hold a link");

private:
  static constexpr auto      pool_mul       = acl::detail::log2(acl::detail::pool_size_v<Config>);
  static constexpr auto      pool_size      = static_cast<size_type>(1) << pool_mul;
  static constexpr auto      pool_mod       = pool_size - 1;
  static constexpr bool      has_self_index = acl::detail::HasSelfIndexValue<Config>;
  static constexpr size_type null_v         = 0;
  using this_type                           = sparse_table<Ty, Config>;
  using storage                             = value_type;

  struct default_index_pool_size
  {
    static constexpr uint32_t self_index_pool_size_v = 128;
  };

  struct self_index_traits_base
  {
    using size_type                              = uint32_t;
    static constexpr uint32_t pool_size_v        = std::conditional_t<acl::detail::HasSelfIndexPoolSize<Config>, Config,
                                                                      default_index_pool_size>::self_index_pool_size_v;
    static constexpr bool     use_sparse_index_v = true;
    static constexpr uint32_t null_v             = 0;
    static constexpr bool     zero_out_memory_v  = true;
  };

  template <typename TrTy>
  struct self_index_traits : self_index_traits_base
  {};

  template <acl::detail::HasSelfIndexValue TrTy>
  struct self_index_traits<TrTy> : self_index_traits_base
  {
    using self_index = typename TrTy::self_index;
  };

  using self_index = acl::detail::self_index_type<self_index_traits<Config>>;

public:
  sparse_table() noexcept = default;
  sparse_table(allocator_type&& alloc) noexcept : allocator_type(std::move<allocator_type>(alloc)) {}
  sparse_table(allocator_type const& alloc) noexcept : allocator_type(alloc) {}
  sparse_table(sparse_table&& other) noexcept
  {
    *this = std::move(other);
  }
  sparse_table(sparse_table const& other) noexcept
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  ~sparse_table()
  {
    clear();
    shrink_to_fit();
  }

  auto operator=(sparse_table&& other) noexcept -> sparse_table&
  {
    clear();
    shrink_to_fit();

    (allocator_type&)* this = std::move((allocator_type&)other);
    items_                  = std::move(other.items_);
    self_                   = std::move(other.self_);
    length_                 = other.length_;
    extents_                = other.extents_;
    free_slot_              = other.free_slot_;
    other.length_           = 0;
    other.extents_          = 1;
    other.free_slot_        = null_v;
    return *this;
  }

  auto operator=(sparse_table const& other) noexcept
   -> sparse_table& requires(std::is_copy_constructible_v<value_type>) {
     clear();
     shrink_to_fit();

     static_cast<allocator_type&>(*this) = static_cast<allocator_type const&>(other);
     items_.resize(other.items_.size());
     for (auto& data : items_)
     {
       data = acl::allocate<storage>(*this, sizeof(storage) * pool_size);
     }

     for (size_type first = 1; first != other.extents_; ++first)
     {
       // NOLINTNEXTLINE
       auto const& src = reinterpret_cast<value_type const&>(other.item_at_idx(first));
       // NOLINTNEXTLINE
       auto& dst = reinterpret_cast<value_type&>(item_at_idx(first));

       auto ref = other.get_ref_at_idx(first);
       if (is_valid_ref(ref))
       {
         std::construct_at(&dst, src);
       }
       if constexpr (has_self_index)
       {
         set_ref_at_idx(first, ref);
       }
     }
     self_      = other.self_;
     extents_   = other.extents_;
     length_    = other.length_;
     free_slot_ = other.free_slot_;
     return *this;
   }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    internal_for_each<Lambda, value_type>(std::forward<Lambda>(lambda));
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type const& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    // NOLINTNEXTLINE
    const_cast<this_type*>(this)->internal_for_each<Lambda, value_type const>(std::forward<Lambda>(lambda));
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
    internal_for_each<Lambda, value_type>(first, last, std::forward<Lambda>(lambda));
  }

  /**
   * @copydoc for_each
   */
  template <typename Lambda>
  void for_each(size_type first, size_type last, Lambda&& lambda) const noexcept
  {
    // NOLINTNEXTLINE
    const_cast<this_type*>(this)->internal_for_each<Lambda, value_type const>(first, last,
                                                                              std::forward<Lambda>(lambda));
  }

  /**
   * @brief Returns size of packed array
   */
  auto size() const noexcept -> size_type
  {
    return length_;
  }

  /**
   * @brief Returns capacity of packed array
   */
  auto capacity() const noexcept -> size_type
  {
    return static_cast<size_type>(items_.size()) * pool_size;
  }

  /**
   * @brief Returns the maximum entry slot currently already reserved for the table.
   * @remarks This value is more than item capacity, and is the current max link value.
   */
  auto max_size() const noexcept -> size_type
  {
    return capacity();
  }

  /**
   * @brief Valid range that can be iterated over
   */
  auto range() const noexcept -> size_type
  {
    return extents_;
  }

  /**
   * @brief packed_table has active pool count depending upon number of elements it contains
   * @return active pool count
   */
  auto active_pools() const noexcept -> size_type
  {
    return extents_ >> pool_mul;
  }

  /**
   * @brief Get item pool and number of items_ give the pool number
   * @param i Must be between [0, active_pools())
   * @return Item pool raw array and array size
   */
  auto get_pool(size_type i) const noexcept -> std::tuple<value_type const*, size_type>
  {
    // NOLINTNEXTLINE
    return {reinterpret_cast<value_type const*>(items_[i]),
            items_[i] == items_.back() ? extents_ & pool_mod : pool_size};
  }

  auto get_pool(size_type i) noexcept -> std::tuple<value_type*, size_type>
  {
    // NOLINTNEXTLINE
    return {reinterpret_cast<value_type*>(items_[i]), items_[i] == items_.back() ? extents_ & pool_mod : pool_size};
  }

  /**
   * @brief Emplace back an element. Order is not guranteed.
   * @tparam ...Args Constructor args for value_type
   * @return Returns link to the element pushed. link can be used to destroy entry.
   */
  template <typename... Args>
  auto emplace(Args&&... args) noexcept -> link
  {
    auto lnk = ensure_slot();
    auto idx = acl::detail::index_val(lnk);

    auto block = idx >> pool_mul;
    auto index = idx & pool_mod;

    // NOLINTNEXTLINE
    std::construct_at(reinterpret_cast<value_type*>(items_[block] + index), std::forward<Args>(args)...);
    set_ref_at_idx(idx, lnk);

    return link(lnk);
  }

  /**
   * @brief Construct an item in a given location, assuming the location was empty
   */
  void replace(link point, value_type&& args) noexcept
  {
    if constexpr (acl::debug)
    {
      assert(contains(point));
    }

    at(point) = std::move(args);
  }

  /**
   * @brief Erase a single element.
   */
  void erase(link l) noexcept
  {
    if constexpr (acl::debug)
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
    erase_at(self_.get(obj));
  }

  /**
   * @brief Drop unused pages
   */
  void shrink_to_fit() noexcept
  {
    auto block = (extents_ + pool_size - 1) >> pool_mul;
    for (auto i = block, end = static_cast<size_type>(items_.size()); i < end; ++i)
    {
      acl::deallocate(*this, items_[i], sizeof(storage) * pool_size);
    }
    items_.resize(block);
    items_.shrink_to_fit();
    self_.shrink_to_fit();
  }

  /**
   * @brief Set size to 0, memory is not released, objects are destroyed
   */
  void clear() noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<value_type>)
    {
      for_each(
       [](Ty& v)
       {
         std::destroy_at(std::addressof(v));
       });
    }
    extents_   = 1;
    length_    = 0;
    free_slot_ = null_v;
    self_.clear();
  }

  auto at(link l) noexcept -> value_type&
  {
    if constexpr (acl::debug)
    {
      validate(l);
    }
    return item_at(acl::detail::index_val(l));
  }

  auto at(link l) const noexcept -> value_type const&
  {
    // NOLINTNEXTLINE
    return const_cast<this_type*>(this)->at(l);
  }

  auto get_if(link l) const noexcept -> value_type const*
  {
    // NOLINTNEXTLINE
    return const_cast<this_type*>(this)->at(l);
  }

  auto operator[](link l) noexcept -> value_type&
  {
    return at(l);
  }

  auto operator[](link l) const noexcept -> value_type const&
  {
    return at(l);
  }

  auto get_if(link l) noexcept -> value_type*
  {
    auto idx = acl::detail::index_val(l.value());
    if (idx < extents_)
    {
      if constexpr (has_self_index)
      {
        value_type& val = item_at_idx(idx);
        if (self_.get(val) == l)
        {
          return &val;
        }
      }
      else
      {
        if (get_ref_at_idx(idx) == l.value())
        {
          return &(item_at_idx(idx));
        }
      }
    }

    return nullptr;
  }

  auto contains(link l) const noexcept -> bool
  {
    assert(is_valid_ref(l.value()));
    auto idx = acl::detail::index_val(l.value());
    return idx < extents_ && (l.value() == get_ref_at_idx(idx));
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return length_ == 0;
  }

private:
  void validate(link l) const noexcept
  {
    auto idx  = acl::detail::index_val(l);
    auto self = get_ref_at_idx(idx);
    assert(self == l);
  }

  auto get_ref_at_idx(size_type idx) const noexcept
    requires(!has_self_index)
  {
    return self_.get(idx);
  }

  auto get_ref_at_idx(size_type idx) const noexcept
    requires(has_self_index)
  {
    return self_.get(item_at_idx(idx));
  }

  auto set_ref_at_idx(size_type idx, size_type lnk) noexcept
    requires(!has_self_index)
  {
    return self_.ensure_at(idx) = lnk;
  }

  auto set_ref_at_idx(size_type idx, size_type lnk) noexcept
    requires(has_self_index)
  {
    return self_.get(item_at_idx(idx)) = lnk;
  }

  auto item_at(size_type l) noexcept -> auto&
  {
    return item_at_idx(l);
  }

  auto item_at_idx(size_type item_id) noexcept -> auto&
  {
    // NOLINTNEXTLINE
    return reinterpret_cast<value_type&>(items_[item_id >> pool_mul][item_id & pool_mod]);
  }

  auto item_at_idx(size_type item_id) const noexcept -> auto const&
  {
    // NOLINTNEXTLINE
    return reinterpret_cast<value_type const&>(items_[item_id >> pool_mul][item_id & pool_mod]);
  }

  void erase_at(size_type l) noexcept
  {
    length_--;

    auto lnk = acl::detail::index_val(l);

    // NOLINTNEXTLINE
    auto& item = reinterpret_cast<value_type&>(items_[lnk >> pool_mul][lnk & pool_mod]);

    if constexpr (!std::is_trivially_destructible_v<value_type>)
    {
      std::destroy_at(&item);
    }

    auto newlnk = acl::detail::revise_invalidate(l);

    if constexpr (has_self_index)
    {
      self_.get(item) = free_slot_;
    }
    else
    {
      set_ref_at_idx(lnk, free_slot_);
    }

    free_slot_ = newlnk;
  }

  auto ensure_slot() noexcept
  {
    length_++;
    size_type lnk;
    if (free_slot_ == null_v)
    {
      auto block = extents_ >> pool_mul;
      if (block >= items_.size())
      {
        items_.emplace_back(acl::allocate<storage>(*this, sizeof(storage) * pool_size));
      }

      lnk = extents_++;
    }
    else
    {
      lnk        = acl::detail::validate(free_slot_);
      free_slot_ = get_ref_at_idx(acl::detail::index_val(lnk));
    }
    return lnk;
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept value_type& parameter
   */
  template <typename Lambda, typename Cast>
  void internal_for_each(Lambda&& lambda) noexcept
  {
    internal_for_each<Lambda, Cast>(1, extents_, std::forward<Lambda>(lambda));
  }

  template <typename Lambda, typename Cast>
  void internal_for_each(size_type first, size_type last, Lambda& lambda) noexcept
  {
    constexpr auto arity = function_traits<Lambda>::arity;

    for (; first < last; ++first)
    {
      auto ref = get_ref_at_idx(first);
      if (is_valid_ref(ref))
      {
        if constexpr (arity == 2)
        {
          // NOLINTNEXTLINE
          lambda(link(get_ref_at_idx(first)), reinterpret_cast<Cast&>(items_[first >> pool_mul][first & pool_mod]));
        }
        else
        {
          // NOLINTNEXTLINE
          lambda(reinterpret_cast<Cast&>(items_[first >> pool_mul][first & pool_mod]));
        }
      }
    }
  }

  static auto is_valid_ref(size_type r) -> bool
  {
    return (r && acl::detail::is_valid(r));
  }

  podvector<storage*, allocator_type> items_;
  self_index                          self_;
  size_type                           length_    = 0;
  size_type                           extents_   = 1;
  size_type                           free_slot_ = null_v;
};

} // namespace acl
