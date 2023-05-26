#pragma once

#include "podvector.hpp"
#include <acl/detail/utils.hpp>
#include <acl/utility/link.hpp>
#include <acl/utility/type_traits.hpp>
#include <memory>

namespace acl
{
/// @brief Represents a sparse vector with only pages/chunks/pools allocated for non-empty indexes
/// @tparam Ty Vector type
/// @tparam allocator_type Underlying allocator_type
/// @tparam Options Defined options for this type:
///                  @note [option] pool_size Power of 2, count of elements in a single chunk/page/pool
///                  @note [option] null_v : constexpr/static type Ty object that indicates a null value for the object
///                  @note [option] is_null(Ty t) : method that returns true to identify null object
///                  @note          null_construct(Ty& t) : construct a slot as null value, for non-trivial types,
///                  constructor must be
///                  @note          null_reset(Ty& t) : reset a slot as null value, you can choose to call destructor
///                  here for
///                                 non-trivial types, but it expects an assignment to a value representing null value.
///                  @note [option] disable_pool_tracking : true to indicate if individaul pool should not be tracked,
///                  is false by default
template <typename Ty, typename Options = acl::default_options<Ty>>
class sparse_vector : public detail::custom_allocator_t<Options>
{
public:
  using value_type      = Ty;
  using reference       = Ty&;
  using const_reference = Ty const&;
  using pointer         = Ty*;
  using const_pointer   = Ty const*;
  using size_type       = detail::choose_size_t<uint32_t, Options>;
  using allocator_type  = detail::custom_allocator_t<Options>;
  using options         = Options;

private:
  using this_type = sparse_vector<value_type, Options>;
  using base_type = allocator_type;
  using storage   = value_type; // std::conditional_t<std::is_fundamental_v<value_type>, value_type,
                                // detail::aligned_storage < sizeof(value_type), alignof(value_type)>> ;

  static constexpr bool has_null_method    = detail::HasNullMethod<options, value_type>;
  static constexpr bool has_null_value     = detail::HasNullValue<options, value_type>;
  static constexpr bool has_null_construct = detail::HasNullConstruct<options, value_type>;
  static constexpr bool has_zero_memory    = detail::HasZeroMemoryAttrib<options>;
  static constexpr bool has_no_fill        = detail::HasNoFillAttrib<options>;
  static constexpr bool has_pod            = detail::HasTrivialAttrib<options>;
  static constexpr bool has_pool_tracking  = !detail::HasDisablePoolTrackingAttrib<options>;
  static constexpr bool has_trivial_copy   = std::is_trivially_copyable_v<Ty> || has_pod;
  static constexpr bool has_backref        = detail::HasBackrefValue<Options>;

  static constexpr auto pool_div       = detail::log2(detail::pool_size_v<Options>);
  static constexpr auto pool_size      = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_bytes     = pool_size * sizeof(value_type);
  static constexpr auto allocate_bytes = has_pool_tracking ? pool_bytes + sizeof(uint32_t) : pool_bytes;
  static constexpr auto pool_mod       = pool_size - 1;

  using allocator_tag             = typename allocator_type::tag;
  using allocator_is_always_equal = typename acl::allocator_traits<allocator_tag>::is_always_equal;
  using check_type                = std::conditional_t<has_pool_tracking, std::true_type, std::false_type>;

  inline constexpr static value_type* cast(storage* src)
  requires(std::is_same_v<storage, value_type>)
  {
    return src;
  }

  inline constexpr static value_type* cast(storage* src)
  requires(!std::is_same_v<storage, value_type>)
  {
    return reinterpret_cast<value_type*>(src);
  }

  inline constexpr static value_type const* cast(storage const* src)
  requires(std::is_same_v<storage, value_type>)
  {
    return src;
  }

  inline constexpr static value_type const* cast(storage const* src)
  requires(!std::is_same_v<storage, value_type>)
  {
    return reinterpret_cast<value_type*>(src);
  }

  inline constexpr static value_type& cast(storage& src)
  requires(std::is_same_v<storage, value_type>)
  {
    return src;
  }

  inline constexpr static value_type& cast(storage& src)
  requires(!std::is_same_v<storage, value_type>)
  {
    return reinterpret_cast<value_type&>(src);
  }

  inline constexpr static value_type const& cast(storage const& src)
  requires(std::is_same_v<storage, value_type>)
  {
    return src;
  }

  inline constexpr static value_type const& cast(storage const& src)
  requires(!std::is_same_v<storage, value_type>)
  {
    return reinterpret_cast<value_type&>(src);
  }

  inline static bool is_null(value_type const& other) noexcept
  requires(has_null_method)
  {
    return options::is_null(other);
  }

  inline static bool is_null(value_type const& other) noexcept
  requires(has_null_value && !has_null_method)
  {
    return other == options::null_v;
  }

  inline static constexpr bool is_null(value_type const& other) noexcept
  requires(!has_null_value && !has_null_method)
  {
    return false;
  }

public:
  inline sparse_vector() noexcept {}
  inline sparse_vector(allocator_type&& alloc) noexcept : base_type(std::move<allocator_type>(alloc)) {}
  inline sparse_vector(allocator_type const& alloc) noexcept : base_type(alloc) {}
  inline sparse_vector(sparse_vector&& other) noexcept
  {
    *this = std::move(other);
  }
  inline sparse_vector(sparse_vector const& other) noexcept
  requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  inline ~sparse_vector()
  {
    clear();
  }

  sparse_vector& operator=(sparse_vector&& other) noexcept
  {
    clear();
    (base_type&)* this = std::move((base_type&)other);
    items_             = std::move(other.items_);
    length_            = other.length_;
    other.length_      = 0;
    return *this;
  }

  sparse_vector& operator=(sparse_vector const& other) noexcept
  requires(std::is_copy_constructible_v<value_type>)
  {
    clear();
    items_.resize(other.items_.size());
    for (size_type i = 0; i < items_.size(); ++i)
    {
      auto const src_storage = other.items_[i];
      if (src_storage)
      {
        items_[i] = acl::allocate<storage>(*this, allocate_bytes, alignarg<Ty>);

        if (src_storage)
        {
          if constexpr (std::is_trivially_copyable_v<Ty> || has_pod)
          {
            std::memcpy(items_[i], src_storage, allocate_bytes);
          }
          else
          {
            if constexpr (has_pool_tracking)
              pool_occupation(i) = other.pool_occupation(i);
            for (size_type e = 0; e < pool_size; ++e)
            {
              auto const& src = cast(src_storage[e]);
              auto&       dst = cast(items_[i][e]);

              if (!is_null(src))
                std::construct_at(&dst, src);
            }
          }
        }
      }
      else
        items_[i] = nullptr;
    }

    static_cast<base_type&>(*this) = static_cast<base_type const&>(other);
    length_                        = other.length_;
    return *this;
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for_each(items_, 0, length_, std::forward<Lambda>(lambda), check_type{});
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for_each(items_, 0, length_, std::forward<Lambda>(lambda), check_type{});
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, size_type start, size_type end) noexcept
  {
    for_each(items_, start, end, std::forward<Lambda>(lambda), check_type{});
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, size_type start, size_type end) const noexcept
  {
    for_each(items_, start, end, std::forward<Lambda>(lambda), check_type{});
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, nocheck) noexcept
  {
    for_each(items_, 0, length_, std::forward<Lambda>(lambda), nocheck{});
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, nocheck) const noexcept
  {
    for_each(items_, 0, length_, std::forward<Lambda>(lambda), nocheck{});
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, size_type start, size_type end, nocheck) noexcept
  {
    for_each(items_, start, end, std::forward<Lambda>(lambda), nocheck());
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, size_type start, size_type end, nocheck) const noexcept
  {
    for_each(items_, start, end, std::forward<Lambda>(lambda), nocheck());
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

  /// @brief packed_table has active pool count depending upon number of elements it contains
  /// @return active pool count
  size_type max_pools() const noexcept
  {
    return static_cast<size_type>(items_.size());
  }

  /// @brief Get item pool and number of items_ give the pool number
  /// @param i Must be between [0, active_pools())
  /// @return Item pool raw array and array size
  auto get_pool(size_type i) const noexcept -> std::tuple<value_type const*, size_type>
  {
    return cast(items_[i]);
  }

  auto get_pool(size_type i) noexcept -> std::tuple<value_type*, size_type>
  {
    return cast(items_[i]);
  }

  auto& back()
  {
    return at(length_ - 1);
  }

  auto& front()
  {
    return at(0);
  }

  auto const& back() const
  {
    return at(length_ - 1);
  }

  auto const& front() const
  {
    return at(0);
  }

  inline void push_back(value_type const& v) noexcept
  {
    emplace_at_idx(length_++, v);
  }

  template <typename... Args>
  auto& emplace_back(Args&&... args) noexcept
  {
    // length_ is increased by emplace_at
    return emplace_at_idx(length_++, std::forward<Args>(args)...);
  }
  /// @brief Emplace back an element. Order is not guranteed.
  /// @tparam ...Args Constructor args for value_type
  /// @return Returns link to the element pushed. link can be used to destroy entry.
  template <typename... Args>
  auto& emplace_at(size_type idx, Args&&... args) noexcept
  {
    auto& dst = emplace_at_idx(idx, std::forward<Args>(args)...);
    length_   = std::max(idx, length_) + 1;
    return dst;
  }

  /// @brief Emplace back an element. Order is not guranteed.
  /// @tparam ...Args Constructor args for value_type
  /// @return Returns link to the element pushed. link can be used to destroy entry.
  template <typename... Args>
  auto& ensure(size_type idx) noexcept
  {
    auto block = idx >> pool_div;
    auto index = idx & pool_mod;

    ensure_block(block);

    return *cast(items_[block] + index);
  }

  /// @brief Merge another sparse_vector to this one
  //  @note The merge will alter the order of elements
  //  Assumes vector is shrinked.
  void unordered_merge(sparse_vector&& other) noexcept
  requires(allocator_is_always_equal::value && !has_pool_tracking)
  {
    if (other.items_.empty())
      return;
    if (items_.empty())
    {
      *this = std::move(other);
      return;
    }
    // merge another vector onto this one
    // allocators must be equal

    items_.reserve(items_.size() + other.items_.size());
    auto other_back_length = other.length_ & pool_mod;
    auto back_length       = length_ & pool_mod;
    if (!back_length)
    {
      items_.insert(items_.end(), other.items_.begin(), other.items_.end());
    }
    else if (other_back_length)
    {
      auto back              = items_.back();
      auto length_to_move_in = std::min(pool_size - other_back_length, back_length);
      items_.pop_back();
      items_.insert(items_.end(), other.items_.begin(), other.items_.end());

      if constexpr (has_trivial_copy)
        std::memcpy(items_.back() + other_back_length, back, length_to_move_in * sizeof(storage));
      else
      {
        auto dest = cast(items_.back() + other_back_length);
        auto src  = cast(back);
        std::move(src, src + length_to_move_in, dest);
      }
      auto length_to_shift = back_length - length_to_move_in;
      if (length_to_shift)
      {
        items_.push_back(back);
        if constexpr (has_trivial_copy)
          std::memmove(back, back + length_to_move_in, length_to_shift * sizeof(storage));
        else
        {
          auto dest = cast(back);
          auto src  = cast(back + length_to_move_in);
          std::move(src, src + length_to_shift, dest);
        }
      }
    }
    else
    {
      auto back = items_.back();
      items_.pop_back();
      items_.insert(items_.end(), other.items_.begin(), other.items_.end());
      items_.push_back(back);
    }

    length_ += other.length_;
    other.items_.clear();
    other.length_ = 0;
    // memcpy the rest
  }

  /// @brief Merge multiple sparse_vectors into a single one, thereby destroying them
  /// @tparam SparseVectorIt
  /// @param first
  /// @param end
  /// @return
  template <typename SparseVectorIt>
  inline void unordered_merge(SparseVectorIt first, SparseVectorIt end) noexcept
  {
    // merge multiple sparse vectors into a single
    size_type sz = 0;
    for (auto it = first; it != end; ++it)
      sz += it->items_.size();

    items_.reserve(sz);

    for (; first != end; ++first)
      unordered_merge(std::move(*first));
  }

  /// @brief Construct an item in a given location, assuming the location was empty
  void replace(size_type point, value_type&& args) noexcept
  {
    at(point) = std::move(args);
  }

  /// @brief Erase a single element.
  void erase(size_type l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    erase_at(l);
  }

  void pop_back()
  {
    ACL_ASSERT(length_ > 0);
    if constexpr (detail::debug)
      validate(length_ - 1);
    erase_at(--length_);
  }

  void resize(size_type idx) noexcept
  {
    if (length_ > idx)
    {
      shrink(idx);
    }
    else if (length_ < idx)
    {
      grow(idx);
    }
  }

  void shrink(size_type idx) noexcept
  {
    ACL_ASSERT(length_ > idx);
    if constexpr (!std::is_trivially_destructible_v<value_type> && !has_pod)
    {
      for (size_type i = idx; i < length_; ++i)
        std::destroy_at(std::addressof(item_at(i)));
    }
    length_ = idx;
  }

  void grow(size_type idx) noexcept
  {
    ACL_ASSERT(length_ < idx);
    auto block = idx >> pool_div;
    auto index = idx & pool_mod;

    ensure_block(block);

    if constexpr (!has_zero_memory && !has_no_fill)
    {
      for (size_type i = length_; i < idx; ++i)
      {
        std::construct_at(std::addressof(item_at(i)));
      }
    }

    length_ = idx;
  }

  /// @brief Drop unused pages
  void shrink_to_fit() noexcept
  {
    auto from = (length_ + pool_mod) >> pool_div;
    for (size_type block = from; block < items_.size(); ++block)
    {
      if (items_[block])
        acl::deallocate(static_cast<allocator_type&>(*this), items_[block], allocate_bytes, alignarg<Ty>);
    }
    items_.resize(from);
    items_.shrink_to_fit();
  }

  /// @brief Set size to 0, memory is not released, objects are destroyed
  void clear() noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<value_type> && !has_pod)
    {
      for_each(
        [](Ty& v)
        {
          std::destroy_at(std::addressof(v));
        });
    }
    length_ = 0;
  }

  value_type& at(size_type l) noexcept
  {
    ACL_ASSERT(l < length_);
    return item_at(l);
  }

  value_type const& at(size_type l) const noexcept
  {
    ACL_ASSERT(l < length_);
    return item_at(l);
  }

  value_type& operator[](size_type l) noexcept
  {
    return at(l);
  }

  value_type const& operator[](size_type l) const noexcept
  {
    return at(l);
  }

  bool contains(size_type idx) const noexcept
  {
    auto block = (idx >> pool_div);
    return block < items_.size() && items_[block] && !is_null(cast(items_[block][idx & pool_mod]));
  }

  Ty get_value(size_type idx) const noexcept
  requires(has_null_value)
  {
    auto block = (idx >> pool_div);
    return block < items_.size() && items_[block] ? cast(items_[block][idx & pool_mod]) : options::null_v;
  }

  Ty& get_unsafe(size_type idx) const noexcept
  {
    return cast(items_[(idx >> pool_div)][idx & pool_mod]);
  }

  bool empty() const noexcept
  {
    return length_ == 0;
  }

private:
  inline void ensure_block(size_type block)
  {
    if (block >= items_.size())
    {
      items_.resize(block + 1, nullptr);
    }

    if (!items_[block])
    {
      if constexpr (has_zero_memory)
        items_[block] = acl::zallocate<storage>(*this, allocate_bytes, alignarg<Ty>);
      else
      {
        items_[block] = acl::allocate<storage>(*this, allocate_bytes, alignarg<Ty>);
        if constexpr (!has_no_fill)
        {
          if constexpr (has_pod ||
                        (std::is_trivially_copyable_v<value_type> && std::is_trivially_constructible_v<value_type>))
          {
            if constexpr (has_null_value)
              std::fill(cast(items_[block]), cast(items_[block] + pool_size), options::null_v);
            else if constexpr (has_null_construct)
            {
              std::for_each(cast(items_[block]), cast(items_[block] + pool_size), options::null_construct);
            }
            else
            {
              std::fill(cast(items_[block]), cast(items_[block] + pool_size), value_type());
            }
          }
          else
          {
            std::for_each(cast(items_[block]), cast(items_[block] + pool_size),
                          [](value_type& dst)
                          {
                            if constexpr (has_null_value)
                              std::construct_at(std::addressof(dst), options::null_v);
                            else if constexpr (has_null_construct)
                              options::null_construct(dst);
                            else
                              std::construct_at(std::addressof(dst));
                          });
          }
        }
      }
    }
  }

  inline uint32_t& pool_occupation(storage* p) noexcept
  requires(has_pool_tracking)
  {
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<std::uint8_t*>(p) + pool_bytes);
  }

  inline uint32_t pool_occupation(storage const* p) const noexcept
  requires(has_pool_tracking)
  {
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<std::uint8_t const*>(p) + pool_bytes);
  }

  inline uint32_t& pool_occupation(size_type p) noexcept
  requires(has_pool_tracking)
  {
    return pool_occupation(items_[p]);
  }

  inline uint32_t pool_occupation(size_type p) const noexcept
  requires(has_pool_tracking)
  {
    return pool_occupation(items_[p]);
  }

  inline void validate(size_type idx) const noexcept
  {
    ACL_ASSERT(contains(idx));
  }

  inline auto& item_at(size_type idx) noexcept
  {
    auto block = (idx >> pool_div);
    ensure_block(block);
    return cast(items_[block][idx & pool_mod]);
  }

  inline auto& item_at(size_type idx) const noexcept
  {
    auto block = (idx >> pool_div);
    return cast(items_[block][idx & pool_mod]);
  }

  inline void erase_at(size_type idx) noexcept
  {
    auto block = (idx >> pool_div);

    if constexpr (has_null_value)
      cast(items_[block][idx & pool_mod]) = options::null_v;
    else if constexpr (has_null_construct)
      options::null_reset(cast(items_[block][idx & pool_mod]));
    else
      cast(items_[block][idx & pool_mod]) = value_type();
    if constexpr (has_pool_tracking)
    {
      if (!--pool_occupation(block))
      {
        delete_block(block);
      }
    }
  }

  inline void delete_block(size_type block)
  {
    if constexpr (!std::is_trivially_destructible_v<value_type> && !has_pod)
    {
      for (size_type i = 0; i < pool_size; ++i)
        std::destroy_at(std::addressof(cast(items_[block][i])));
    }

    acl::deallocate(static_cast<allocator_type&>(*this), items_[block], allocate_bytes, alignarg<Ty>);
    items_[block] = nullptr;
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept value_type& parameter
  template <typename Lambda, typename Store, typename Check>
  inline static void for_each(Store& items_, size_type start, size_type end, Lambda&& lambda, Check) noexcept
  {
    if (start == end)
      return;
    auto bstart     = start >> pool_div;
    auto bend       = end >> pool_div;
    auto item_start = start & pool_mod;
    ACL_ASSERT(bstart <= bend);
    constexpr auto arity = function_traits<Lambda>::arity;
    for (size_type block = bstart; block != bend; ++block)
    {
      auto store = items_[block];
      if constexpr (arity == 2)
        for_each_value(store, block, item_start, pool_size, std::forward<Lambda>(lambda), Check());
      else
        for_each_value(store, item_start, pool_size, std::forward<Lambda>(lambda), Check());
      item_start = 0;
    }
    // Final block
    if (end & pool_mod)
    {
      if constexpr (arity == 2)
        for_each_value(items_[bend], bend, item_start, end & pool_mod, std::forward<Lambda>(lambda), Check());
      else
        for_each_value(items_[bend], item_start, end & pool_mod, std::forward<Lambda>(lambda), Check());
    }
  }

  template <typename Lambda, typename Store, typename Check>
  inline static void for_each_value(Store* store, size_type start, size_type end, Lambda&& lambda, Check) noexcept
  {
    if (store)
    {
      for (size_type e = start; e < end; ++e)
      {
        if constexpr (Check::value)
        {
          if (is_null(cast(store[e])))
            continue;
        }
        lambda(cast(store[e]));
      }
    }
  }

  template <typename Lambda, typename Store, typename Check>
  inline static void for_each_value(Store* store, size_type block, size_type start, size_type end, Lambda&& lambda,
                                    Check) noexcept
  {
    constexpr auto arity = function_traits<Lambda>::arity;
    if (store)
    {
      for (size_type e = start; e < end; ++e)
      {
        if constexpr (Check::value)
        {
          if (is_null(cast(store[e])))
            continue;
        }
        lambda((block << pool_div) | e, cast(store[e]));
      }
    }
  }

  template <typename... Args>
  auto& emplace_at_idx(size_type idx, Args&&... args) noexcept
  {
    auto block = idx >> pool_div;
    auto index = idx & pool_mod;

    ensure_block(block);

    value_type& dst = *cast((items_[block] + index));
    dst             = value_type(std::forward<Args>(args)...);
    if constexpr (has_pool_tracking)
      pool_occupation(block)++;
    return dst;
  }

  podvector<storage*, Options> items_;
  size_type                    length_ = 0;
};

} // namespace acl
