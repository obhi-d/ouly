// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/allocators/detail/custom_allocator.hpp"
#include "ouly/utility/type_traits.hpp"
#include "ouly/utility/utils.hpp"
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

namespace ouly
{
/**
 * @brief Represents a sparse vector with only pages/chunks/pools allocated for non-empty indexes
 * @tparam Ty Vector type
 * @tparam allocator_type Underlying allocator_type
 * @tparam Config Defined config for this type:
 *                  @note [option] pool_size Power of 2, count of elements in a single chunk/page/pool
 *                  @note [option] null_v : constexpr/static type Ty object that indicates a null value for the object
 *                  @note [option] is_null(Ty t) : method that returns true to identify null object
 *                  @note          null_construct(Ty& t) : construct a slot as null value, for non-trivial types,
 *                  constructor must be
 *                  @note          null_reset(Ty& t) : reset a slot as null value, you can choose to call destructor
 *                  here for
 *                                 non-trivial types, but it expects an assignment to a value representing null value.
 *                  @note [option] disable_pool_tracking : true to indicate if individaul pool should not be tracked,
 *                  is false by default
 */
template <typename Ty, typename Config = ouly::default_config<Ty>>
class sparse_vector : public ouly::detail::custom_allocator_t<Config>
{
public:
  using value_type      = Ty;
  using reference       = Ty&;
  using const_reference = Ty const&;
  using pointer         = Ty*;
  using const_pointer   = Ty const*;
  using size_type       = ouly::detail::choose_size_t<uint32_t, Config>;
  using allocator_type  = ouly::detail::custom_allocator_t<Config>;
  using config          = Config;

  static constexpr bool is_sparse_vector = true;

private:
  using this_type = sparse_vector<value_type, Config>;
  using base_type = allocator_type;
  using storage   = value_type; // std::conditional_t<std::is_fundamental_v<value_type>, value_type,
                                // ouly::detail::aligned_storage < sizeof(value_type), alignof(value_type)>> ;

  static constexpr bool has_null_method    = ouly::detail::HasNullMethod<config, value_type>;
  static constexpr bool has_null_value     = ouly::detail::HasNullValue<config, value_type>;
  static constexpr bool has_null_construct = ouly::detail::HasNullConstruct<config, value_type>;
  static constexpr bool has_zero_memory    = ouly::detail::HasZeroMemoryAttrib<config>;
  static constexpr bool has_no_fill        = ouly::detail::HasNoFillAttrib<config>;
  static constexpr bool has_pod            = ouly::detail::HasTrivialAttrib<config>;
  static constexpr bool has_pool_tracking  = !ouly::detail::HasDisablePoolTrackingAttrib<config>;
  static constexpr bool has_trivial_copy   = std::is_trivially_copyable_v<Ty> || has_pod;
  static constexpr bool has_self_index     = ouly::detail::HasSelfIndexValue<Config>;

  static constexpr auto pool_mul   = ouly::detail::log2(ouly::detail::pool_size_v<Config>);
  static constexpr auto pool_size  = static_cast<size_type>(1) << pool_mul;
  static constexpr auto pool_bytes = pool_size * sizeof(value_type);
  // We allocate only the pool bytes; occupancy is tracked separately in pool_storage
  static constexpr auto allocate_bytes = pool_bytes;
  static constexpr auto pool_mod       = pool_size - 1;

  using allocator_tag             = typename allocator_type::tag;
  using allocator_is_always_equal = typename ouly::allocator_traits<allocator_tag>::is_always_equal;
  using check_type                = std::conditional_t<has_pool_tracking, std::true_type, std::false_type>;

  struct pool_storage
  {
    storage* data_      = nullptr;
    size_t   occupancy_ = 0; // valid only when has_pool_tracking
  };

  constexpr static auto cast(storage* src) -> value_type* requires(std::is_same_v<storage, value_type>) { return src; }

  constexpr static auto cast(storage* src) -> value_type* requires(!std::is_same_v<storage, value_type>) {
    // NOLINTNEXTLINE
    return reinterpret_cast<value_type*>(src);
  }

  constexpr static auto cast(storage const* src)
   -> value_type const* requires(std::is_same_v<storage, value_type>) { return src; }

  constexpr static auto cast(storage const* src) -> value_type const* requires(!std::is_same_v<storage, value_type>) {
    // NOLINTNEXTLINE
    return reinterpret_cast<value_type*>(src);
  }

  constexpr static auto cast(storage& src) -> value_type& requires(std::is_same_v<storage, value_type>) { return src; }

  constexpr static auto cast(storage& src) -> value_type& requires(!std::is_same_v<storage, value_type>) {
    // NOLINTNEXTLINE
    return reinterpret_cast<value_type&>(src);
  }

  constexpr static auto cast(storage const& src) -> value_type const&
    requires(std::is_same_v<storage, value_type>)
  {
    // NOLINTNEXTLINE
    return src;
  }

  constexpr static auto cast(storage const& src) -> value_type const&
    requires(!std::is_same_v<storage, value_type>)
  {
    // NOLINTNEXTLINE
    return reinterpret_cast<value_type&>(src);
  }

  static auto is_null(value_type const& other) noexcept -> bool
    requires(has_null_method)
  {
    return config::is_null(other);
  }

  static auto is_null(value_type const& other) noexcept -> bool
    requires(has_null_value && !has_null_method)
  {
    return other == config::null_v;
  }

  static constexpr auto is_null(value_type const& /*other*/) noexcept -> bool
    requires(!has_null_value && !has_null_method)
  {
    return false;
  }

public:
  sparse_vector() noexcept = default;
  sparse_vector(allocator_type&& alloc) noexcept : base_type(std::move<allocator_type>(alloc)) {}
  sparse_vector(allocator_type const& alloc) noexcept : base_type(alloc) {}
  sparse_vector(sparse_vector&& other) noexcept
  {
    *this = std::move(other);
  }
  sparse_vector(sparse_vector const& other) noexcept
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = other;
  }
  ~sparse_vector()
  {
    clear();
    shrink_to_fit();
  }

  // ------------------- Iterators -------------------
  // Lightweight forward iterator over present elements.
  // Skips null entries when pool tracking/null semantics are enabled.
  template <bool IsConst>
  class iterator_base
  {
  public:
    using parent_type       = std::conditional_t<IsConst, sparse_vector const, sparse_vector>;
    using value_ref         = std::conditional_t<IsConst, const_reference, reference>;
    using value_ptr         = std::conditional_t<IsConst, const_pointer, pointer>;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using value_type        = Ty; // standard associated type

    iterator_base() noexcept = default;
    iterator_base(parent_type* p, size_type idx) noexcept : p_(p), idx_(idx) {}

    auto operator*() const noexcept -> auto&
    {
      return (*p_).at(idx_);
    }
    auto operator->() const noexcept -> auto*
    {
      return std::addressof((*p_).at(idx_));
    }

    auto operator++() noexcept -> iterator_base&
    {
      ++idx_;
      return *this;
    }
    auto operator++(int) noexcept -> iterator_base
    {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    friend auto operator==(iterator_base const& a, iterator_base const& b) noexcept -> bool
    {
      return a.idx_ == b.idx_ && a.p_ == b.p_;
    }
    friend auto operator!=(iterator_base const& a, iterator_base const& b) noexcept -> bool
    {
      return !(a == b);
    }

  private:
    parent_type* p_   = nullptr;
    size_type    idx_ = 0;
  };

  using iterator       = iterator_base<false>;
  using const_iterator = iterator_base<true>;

  auto begin() noexcept -> iterator
  {
    return iterator(this, 0);
  }
  auto end() noexcept -> iterator
  {
    return iterator(this, length_);
  }
  auto begin() const noexcept -> const_iterator
  {
    return const_iterator(this, 0);
  }
  auto end() const noexcept -> const_iterator
  {
    return const_iterator(this, length_);
  }
  auto cbegin() const noexcept -> const_iterator
  {
    return const_iterator(this, 0);
  }
  auto cend() const noexcept -> const_iterator
  {
    return const_iterator(this, length_);
  }

  struct index_t
  {
    size_type block_;
    size_type item_;
  };

  auto operator=(sparse_vector&& other) noexcept -> sparse_vector&
  {
    if (this == &other)
    {
      return *this;
    }
    clear();
    static_cast<base_type&>(*this) = std::move(static_cast<base_type&>(other));
    items_                         = std::move(other.items_);
    length_                        = other.length_;
    other.length_                  = 0;
    return *this;
  }

  // NOLINTNEXTLINE
  auto operator=(sparse_vector const& other) noexcept
   -> sparse_vector& requires(std::is_copy_constructible_v<value_type>) {
     if (this != &other)
     {
       clear();
       items_.resize(other.items_.size());
       for (size_type i = 0; i < items_.size(); ++i)
       {
         auto const src_pool = other.items_[i].data_;
         if (src_pool)
         {
           items_[i].data_ = ouly::allocate<storage>(*this, allocate_bytes, alignarg<Ty>);

           if constexpr (std::is_trivially_copyable_v<Ty> || has_pod)
           {
             std::memcpy(items_[i].data_, src_pool, allocate_bytes);
           }
           else
           {
             if constexpr (has_pool_tracking)
             {
               pool_occupation(i) = other.pool_occupation(i);
             }
             for (size_type e = 0; e < pool_size; ++e)
             {
               auto const& src = cast(src_pool[e]);
               auto&       dst = cast(items_[i].data_[e]);

               if (!is_null(src))
               {
                 std::construct_at(&dst, src);
               }
             }
           }
         }
         else
         {
           items_[i].data_      = nullptr;
           items_[i].occupancy_ = 0;
         }
       }

       static_cast<base_type&>(*this) = static_cast<base_type const&>(other);
       length_                        = other.length_;
     }
     return *this;
   }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for_each(items_, 0, length_, std::forward<Lambda>(lambda), check_type{});
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type const& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for_each(items_, 0, length_, std::forward<Lambda>(lambda), check_type{});
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda, size_type start, size_type end) noexcept
  {
    for_each(items_, start, end, std::forward<Lambda>(lambda), check_type{});
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type const& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda, size_type start, size_type end) const noexcept
  {
    for_each(items_, start, end, std::forward<Lambda>(lambda), check_type{});
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda, nocheck /*unused*/) noexcept
  {
    for_each(items_, 0, length_, std::forward<Lambda>(lambda), nocheck{});
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type const& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda, nocheck /*unused*/) const noexcept
  {
    for_each(items_, 0, length_, std::forward<Lambda>(lambda), nocheck{});
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda, size_type start, size_type end, nocheck /*unused*/) noexcept
  {
    for_each(items_, start, end, std::forward<Lambda>(lambda), nocheck());
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept A link and value_type const& parameter
   */
  template <typename Lambda>
  void for_each(Lambda&& lambda, size_type start, size_type end, nocheck /*unused*/) const noexcept
  {
    for_each(items_, start, end, std::forward<Lambda>(lambda), nocheck());
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
   * @brief packed_table has active pool count depending upon number of elements it contains
   * @return active pool count
   */
  auto max_pools() const noexcept -> size_type
  {
    return static_cast<size_type>(items_.size());
  }

  /**
   * @brief Get item pool and number of items_ give the pool number
   * @param i Must be between [0, active_pools())
   * @return Item pool raw array and array size
   */
  auto get_pool(size_type i) const noexcept -> std::tuple<value_type const*, size_type>
  {
    return {cast(items_[i].data_), pool_size};
  }

  auto get_pool(size_type i) noexcept -> std::tuple<value_type*, size_type>
  {
    return {cast(items_[i].data_), pool_size};
  }

  auto back() -> auto&
  {
    return at(length_ - 1);
  }

  auto front() -> auto&
  {
    return at(0);
  }

  auto back() const -> auto const&
  {
    return at(length_ - 1);
  }

  auto front() const -> auto const&
  {
    return at(0);
  }

  void push_back(value_type const& v) noexcept
  {
    emplace_at_idx(length_++, v);
  }

  template <typename... Args>
  auto emplace_back(Args&&... args) noexcept -> auto&
  {
    // length_ is increased by emplace_at
    return emplace_at_idx(length_++, std::forward<Args>(args)...);
  }
  /**
   * @brief Emplace back an element. Order is not guranteed.
   * @tparam ...Args Constructor args for value_type
   * @return Returns link to the element pushed. link can be used to destroy entry.
   */
  template <typename... Args>
  auto emplace_at(size_type idx, Args&&... args) noexcept -> auto&
  {
    auto& dst = emplace_at_idx(idx, std::forward<Args>(args)...);
    length_   = std::max(idx, length_) + 1;
    return dst;
  }

  /**
   * @brief Emplace back an element. Order is not guranteed.
   * @tparam ...Args Constructor args for value_type
   * @return Returns link to the element pushed. link can be used to destroy entry.
   */
  template <typename... Args>
  auto ensure(size_type idx) noexcept -> auto&
  {
    auto block = idx >> pool_mul;
    auto index = idx & pool_mod;

    ensure_block(block);

    return *cast(items_[block] + index);
  }

  /**
   * @brief Merge another sparse_vector to this one
   */
  //  @note The merge will alter the order of elements
  //  Assumes vector is shrinked.
  void unordered_merge(sparse_vector&& other) noexcept
    requires(allocator_is_always_equal::value && !has_pool_tracking)
  {
    if (other.items_.empty())
    {
      return;
    }
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
      auto back              = items_.back().data_;
      auto length_to_move_in = std::min(pool_size - other_back_length, back_length);
      items_.pop_back();
      items_.insert(items_.end(), other.items_.begin(), other.items_.end());

      if constexpr (has_trivial_copy)
      {
        std::memcpy(items_.back().data_ + other_back_length, back, length_to_move_in * sizeof(storage));
      }
      else
      {
        auto dest = cast(items_.back().data_ + other_back_length);
        auto src  = cast(back);
        std::move(src, src + length_to_move_in, dest);
      }
      auto length_to_shift = back_length - length_to_move_in;
      if (length_to_shift)
      {
        items_.push_back(pool_storage{back, 0});
        if constexpr (has_trivial_copy)
        {
          std::memmove(back, back + length_to_move_in, length_to_shift * sizeof(storage));
        }
        else
        {
          auto dest = cast(back);
          auto src  = cast(back + length_to_move_in);
          std::move(src, src + length_to_shift, dest);
        }
      }
      else
      {
        delete_block(back);
      }
    }
    else
    {
      auto back = items_.back().data_;
      items_.pop_back();
      items_.insert(items_.end(), other.items_.begin(), other.items_.end());
      items_.push_back(pool_storage{back, 0});
    }

    length_ += other.length_;
    other.items_.clear();
    other.length_ = 0;
    // memcpy the rest
  }

  /**
   * @brief Merge multiple sparse_vectors into a single one, thereby destroying them
   * @tparam SparseVectorIt
   * @param first
   * @param end
   * @return
   */
  template <typename SparseVectorIt>
  void unordered_merge(SparseVectorIt first, SparseVectorIt end) noexcept
  {
    // merge multiple sparse vectors into a single
    size_type sz = 0;
    for (auto it = first; it != end; ++it)
    {
      sz += static_cast<size_type>(it->items_.size());
    }

    items_.reserve(sz);

    for (; first != end; ++first)
    {
      unordered_merge(std::move(*first));
    }
  }

  /**
   * @brief Construct an item in a given location, assuming the location was empty
   */
  void replace(size_type point, value_type&& args) noexcept
  {
    at(point) = std::move(args);
  }

  /**
   * @brief Erase a single element.
   */
  void erase(size_type l) noexcept
  {
    if constexpr (ouly::debug)
    {
      validate(l);
    }
    erase_at(l);
  }

  void pop_back()
  {
    OULY_ASSERT(length_ > 0);
    if constexpr (ouly::debug)
    {
      validate(length_ - 1);
    }
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

  void fill(value_type const& v) noexcept
  {
    for (auto const& pool : items_)
    {
      if (pool.data_)
      {
        std::fill(cast(pool.data_), cast(pool.data_ + pool_size), v);
      }
    }
  }

  void shrink(size_type idx) noexcept
  {
    OULY_ASSERT(length_ > idx);
    if constexpr (!std::is_trivially_destructible_v<value_type> && !has_pod)
    {
      for (size_type i = idx; i < length_; ++i)
      {
        std::destroy_at(std::addressof(item_at(i)));
      }
    }
    length_ = idx;
  }

  void grow(size_type idx) noexcept
  {
    OULY_ASSERT(length_ < idx);
    auto block = idx >> pool_mul;
    // auto index = idx & pool_mod;

    ensure_block(block);

    if constexpr (!has_zero_memory && !has_no_fill &&
                  !((has_pod ||
                     (std::is_trivially_copyable_v<value_type> && std::is_trivially_constructible_v<value_type>))))
    {
      for (size_type i = length_; i < idx; ++i)
      {
        std::construct_at(std::addressof(item_at(i)));
      }
    }

    length_ = idx;
  }

  /**
   * @brief Drop unused pages
   */
  void shrink_to_fit() noexcept
  {
    auto from = (length_ + pool_mod) >> pool_mul;
    for (size_type block = from; block < items_.size(); ++block)
    {
      if (items_[block].data_)
      {
        ouly::deallocate(static_cast<allocator_type&>(*this), items_[block].data_, allocate_bytes, alignarg<Ty>);
        items_[block].data_      = nullptr;
        items_[block].occupancy_ = 0;
      }
    }
    items_.resize(from);
    items_.shrink_to_fit();
  }

  /**
   * @brief Set size to 0, memory is not released, objects are destroyed
   */
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

  void swap(sparse_vector& other) noexcept
  {
    using std::swap;
    swap(items_, other.items_);
    swap(length_, other.length_);
  }

  friend void swap(sparse_vector& lhs, sparse_vector& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  auto at(size_type l) noexcept -> value_type&
  {
    OULY_ASSERT(l < length_);
    return item_at(l);
  }

  auto at(size_type l) const noexcept -> value_type const&
  {
    OULY_ASSERT(l < length_);
    return item_at(l);
  }

  auto operator[](size_type l) noexcept -> value_type&
  {
    return at(l);
  }

  auto operator[](size_type l) const noexcept -> value_type const&
  {
    return at(l);
  }

  auto contains(size_type idx) const noexcept -> bool
  {
    if (idx >= length_)
    {
      return false;
    }

    auto block = (idx >> pool_mul);
    return block < items_.size() && items_[block].data_ && !is_null(cast(items_[block].data_[idx & pool_mod]));
  }

  auto get_if(size_type idx) const noexcept -> Ty const*
  {
    auto block = (idx >> pool_mul);
    return block < items_.size() && items_[block].data_ ? &cast(items_[block].data_[idx & pool_mod]) : nullptr;
  }

  auto get_if(size_type idx) noexcept -> Ty*
  {
    auto block = (idx >> pool_mul);
    return block < items_.size() && items_[block].data_ ? &cast(items_[block].data_[idx & pool_mod]) : nullptr;
  }

  auto get_or(size_type idx, Ty&& other) const noexcept -> Ty const& = delete;
  auto get_or(size_type idx, Ty const& other) const noexcept -> Ty const&
  {
    auto block = (idx >> pool_mul);
    return block < items_.size() && items_[block].data_ ? cast(items_[block].data_[idx & pool_mod]) : other;
  }

  auto get_or(size_type idx, Ty& other) noexcept -> Ty&
  {
    auto block = (idx >> pool_mul);
    return block < items_.size() && items_[block].data_ ? cast(items_[block].data_[idx & pool_mod]) : other;
  }

  auto get_value(size_type idx) const noexcept -> Ty
    requires(has_null_value)
  {
    auto block = (idx >> pool_mul);
    return block < items_.size() && items_[block].data_ ? cast(items_[block].data_[idx & pool_mod]) : config::null_v;
  }

  auto get_unsafe(size_type idx) const noexcept -> Ty&
  {
    auto block = (idx >> pool_mul);
    return cast(items_[block].data_[idx & pool_mod]);
  }

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return length_ == 0;
  }

  static constexpr auto index(size_type i) -> index_t
  {
    return index_t{i >> pool_mul, i & pool_mod};
  }

  template <typename VTy>
  class data_view
  {
  public:
    data_view() noexcept = default;
    data_view(pool_storage const* items, size_type block_count) noexcept : items_(items), block_count_(block_count) {}

    auto contains(index_t i) const noexcept -> bool
    {
      return i.block_ < block_count_ && items_[i.block_].data_;
    }

    auto operator[](index_t i) const noexcept -> VTy&
    {
      OULY_ASSERT(contains(i));
      if constexpr (std::is_const_v<VTy>)
      {
        return cast(static_cast<storage const*>(items_[i.block_].data_)[i.item_]);
      }
      else
      {
        return cast(items_[i.block_].data_[i.item_]);
      }
    }

    auto operator()(uint32_t i, VTy& default_value) const noexcept -> VTy&
    {
      auto block = (i >> pool_mul);
      if (block < block_count_)
      {
        if (items_[block].data_)
        {
          if constexpr (std::is_const_v<VTy>)
          {
            return cast(static_cast<storage const*>(items_[block].data_)[i & pool_mod]);
          }
          else
          {
            return cast(items_[block].data_[i & pool_mod]);
          }
        }
        return default_value;
      }
      return default_value;
    }

    auto operator()(uint32_t i, std::reference_wrapper<value_type const> default_value) const noexcept
     -> value_type const&
      requires(!std::is_const_v<VTy>)
    {
      auto block = (i >> pool_mul);
      if (block < block_count_)
      {
        if (items_[block].data_)
        {
          return cast(static_cast<storage const*>(items_[block].data_)[i & pool_mod]);
        }
        return default_value;
      }
      return default_value.get();
    }

    auto operator[](uint32_t i) const noexcept -> VTy&
    {
      auto block = (i >> pool_mul);
      OULY_ASSERT(block < block_count_);
      if constexpr (std::is_const_v<VTy>)
      {
        return cast(static_cast<storage const*>(items_[block].data_)[i & pool_mod]);
      }
      else
      {
        return cast(items_[block].data_[i & pool_mod]);
      }
    }

  private:
    pool_storage const* items_;
    size_type           block_count_ = 0;
  };

  using readonly_view  = data_view<value_type const>;
  using readwrite_view = data_view<value_type>;

  auto view() const noexcept
  {
    return readonly_view(items_.data(), static_cast<size_type>(items_.size()));
  }

  auto view() noexcept
  {
    return readwrite_view(items_.data(), static_cast<size_type>(items_.size()));
  }

private:
  // NOLINTNEXTLINE
  void ensure_block(size_type block)
  {
    if (block >= items_.size())
    {
      items_.resize(block + 1);
    }

    if (!items_[block].data_)
    {
      if constexpr (has_zero_memory)
      {
        items_[block].data_ = ouly::zallocate<storage>(*this, allocate_bytes, alignarg<Ty>);
      }
      else
      {
        items_[block].data_ = ouly::allocate<storage>(*this, allocate_bytes, alignarg<Ty>);
        if constexpr (!has_no_fill)
        {
          if constexpr (has_pod ||
                        (std::is_trivially_copyable_v<value_type> && std::is_trivially_constructible_v<value_type>))
          {
            if constexpr (has_null_value)
            {
              std::fill(cast(items_[block].data_), cast(items_[block].data_ + pool_size), config::null_v);
            }
            else if constexpr (has_null_construct)
            {
              std::for_each(cast(items_[block].data_), cast(items_[block].data_ + pool_size), config::null_construct);
            }
            else
            {
              std::fill(cast(items_[block].data_), cast(items_[block].data_ + pool_size), value_type());
            }
          }
          else
          {
            std::for_each(cast(items_[block].data_), cast(items_[block].data_ + pool_size),
                          [](value_type& dst)
                          {
                            if constexpr (has_null_value)
                            {
                              std::construct_at(std::addressof(dst), config::null_v);
                            }
                            else if constexpr (has_null_construct)
                            {
                              config::null_construct(dst);
                            }
                            else
                            {
                              std::construct_at(std::addressof(dst));
                            }
                          });
          }
        }
      }
      if constexpr (has_pool_tracking)
      {
        items_[block].occupancy_ = 0;
      }
    }
  }

  auto pool_occupation(size_type p) noexcept -> size_t& requires(has_pool_tracking) { return items_[p].occupancy_; }

  auto pool_occupation(size_type p) const noexcept -> size_t
    requires(has_pool_tracking)
  {
    return items_[p].occupancy_;
  }

  void validate([[maybe_unused]] size_type idx) const noexcept
  {
    OULY_ASSERT(contains(idx));
  }

  auto item_at(size_type idx) noexcept -> auto&
  {
    auto block = (idx >> pool_mul);
    ensure_block(block);
    return cast(items_[block].data_[idx & pool_mod]);
  }

  auto item_at(size_type idx) const noexcept -> auto&
  {
    auto block = (idx >> pool_mul);
    return cast(items_[block].data_[idx & pool_mod]);
  }

  void erase_at(size_type idx) noexcept
  {
    auto block = (idx >> pool_mul);

    if constexpr (has_null_value)
    {
      cast(items_[block].data_[idx & pool_mod]) = config::null_v;
    }
    else if constexpr (has_null_construct)
    {
      config::null_reset(cast(items_[block].data_[idx & pool_mod]));
    }
    else
    {
      cast(items_[block].data_[idx & pool_mod]) = value_type();
    }
    if constexpr (has_pool_tracking)
    {
      if (!--pool_occupation(block))
      {
        delete_block(block);
      }
    }
  }

  void delete_block(storage* block)
  {
    if constexpr (!std::is_trivially_destructible_v<value_type> && !has_pod)
    {
      for (size_type i = 0; i < pool_size; ++i)
      {
        std::destroy_at(std::addressof(cast(block[i])));
      }
    }

    ouly::deallocate(static_cast<allocator_type&>(*this), block, allocate_bytes, alignarg<Ty>);
  }

  void delete_block(size_type block)
  {
    auto& store = items_[block];
    delete_block(store.data_);
    store.data_      = nullptr;
    store.occupancy_ = 0;
  }

  /**
   * @brief Lambda called for each element
   * @tparam Lambda Lambda should accept value_type& parameter
   */
  template <typename Lambda, typename StoreContainer, typename Check>
  static void for_each(StoreContainer& items_, size_type start, size_type end, Lambda&& lambda,
                       Check /*unused*/) noexcept
  {
    if (start == end)
    {
      return;
    }
    auto bstart     = start >> pool_mul;
    auto bend       = end >> pool_mul;
    auto item_start = start & pool_mod;
    OULY_ASSERT(bstart <= bend);
    constexpr auto arity = function_traits<std::remove_reference_t<Lambda>>::arity;
    for (size_type block = bstart; block != bend; ++block)
    {
      storage* store = items_[block].data_;
      if constexpr (arity == 2)
      {
        for_each_value(store, block, item_start, pool_size, std::forward<Lambda>(lambda), Check());
      }
      else
      {
        for_each_value(store, item_start, pool_size, std::forward<Lambda>(lambda), Check());
      }
      item_start = 0;
    }
    // Final block
    if (end & pool_mod)
    {
      if constexpr (arity == 2)
      {
        for_each_value(items_[bend].data_, bend, item_start, end & pool_mod, std::forward<Lambda>(lambda), Check());
      }
      else
      {
        for_each_value(items_[bend].data_, item_start, end & pool_mod, std::forward<Lambda>(lambda), Check());
      }
    }
  }

  template <typename Lambda, typename Check>
  static void for_each_value(storage* store, size_type start, size_type end, Lambda lambda, Check /*unused*/) noexcept
  {
    if (store)
    {
      for (size_type e = start; e < end; ++e)
      {
        if constexpr (Check::value)
        {
          if (is_null(cast(store[e])))
          {
            continue;
          }
        }
        lambda(cast(store[e]));
      }
    }
  }

  template <typename Lambda, typename Check>
  static void for_each_value(storage* store, size_type block, size_type start, size_type end, Lambda lambda,
                             Check /*unused*/) noexcept
  {
    if (store)
    {
      for (size_type e = start; e < end; ++e)
      {
        if constexpr (Check::value)
        {
          if (is_null(cast(store[e])))
          {
            continue;
          }
        }
        lambda((block << pool_mul) | e, cast(store[e]));
      }
    }
  }

  template <typename... Args>
  auto emplace_at_idx(size_type idx, Args&&... args) noexcept -> auto&
  {
    auto block = idx >> pool_mul;
    auto index = idx & pool_mod;

    ensure_block(block);

    value_type& dst = *cast((items_[block].data_ + index));
    dst             = value_type(std::forward<Args>(args)...);
    if constexpr (has_pool_tracking)
    {
      pool_occupation(block)++;
    }
    return dst;
  }

  std::vector<pool_storage> items_;
  size_type                 length_ = 0;
};

namespace detail
{

template <typename T>
concept HasCustomVector = requires {
  typename T::custom_vector_t;
  typename T::custom_vector_t::value_type;
  typename T::custom_vector_t::reference;
  typename T::custom_vector_t::const_reference;
  typename T::custom_vector_t::pointer;
  typename T::custom_vector_t::const_pointer;
};

template <typename Config, typename V>
struct custom_vector_type
{
  using type = std::conditional_t<HasUseSparseAttrib<Config>, sparse_vector<V, Config>,
                                  vector<V, ouly::detail::custom_allocator_t<Config>>>;
};

template <HasCustomVector Config>
struct custom_vector_type<Config, typename Config::custom_vector_t::value_type>
{
  using type = Config::custom_vector_t;
};

} // namespace detail

} // namespace ouly
