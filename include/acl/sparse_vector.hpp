#pragma once

#include "allocator.hpp"
#include "default_allocator.hpp"
#include "detail/utils.hpp"
#include "link.hpp"
#include "podvector.hpp"
#include "type_traits.hpp"
#include <memory>

namespace acl
{
/// @brief Represents a sparse vector with only pages/chunks/pools allocated for non-empty indexes
/// @tparam Ty Vector type
/// @tparam Allocator Underlying allocator
/// @tparam Traits At minimum the traits must define:
///          - pool_size Power of 2, count of elements in a single chunk/page/pool
/// [either] - null_v : constexpr/static type Ty object that indicates a null value for the object
/// [or]     - is_null(Ty t) : method that returns true to identify null object
///          - null_construct(Ty& t) : construct a slot as null value, for non-trivial types, constructor must be called
///          by this function
///          - null_reset(Ty& t) : reset a slot as null value, you can choose to call destructor here for non-trivial
///          types, but it expects an assignment
///                                to a value representing null value.
template <typename Ty, typename Allocator = default_allocator<>, typename Traits = acl::traits<Ty>>
class sparse_vector : public Allocator
{
public:
  using value_type     = Ty;
  using size_type      = detail::choose_size_t<uint32_t, Traits>;
  using allocator_type = Allocator;

private:
  static constexpr auto pool_div    = detail::log2(Traits::pool_size);
  static constexpr auto pool_size   = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_mod    = pool_size - 1;
  static constexpr bool has_backref = detail::has_backref_v<Traits>;
  using this_type                   = sparse_vector<value_type, Allocator, Traits>;
  using base_type                   = Allocator;
  using storage                     = detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
  using allocator                   = Allocator;
  using traits                      = Traits;

  static constexpr bool has_null_method    = detail::has_null_method<traits, value_type>;
  static constexpr bool has_null_value     = detail::has_null_value<traits, value_type>;
  static constexpr bool has_null_construct = detail::has_null_construct<traits, value_type>;
  static constexpr bool has_zero_memory    = detail::has_zero_memory_attrib<traits>;
  static constexpr bool has_no_fill        = detail::has_no_fill_attrib<traits>;
  static constexpr bool has_pod            = detail::has_has_pod_attrib<traits>;

  inline static bool is_null(value_type const& other) noexcept
    requires(has_null_method)
  {
    return traits::is_null(other);
  }

  inline static bool is_null(value_type const& other) noexcept
    requires(has_null_value && !has_null_method)
  {
    return other == traits::null_v;
  }

  inline static constexpr bool is_null(value_type const& other) noexcept
    requires(!has_null_value && !has_null_method)
  {
    return false;
  }

public:
  inline sparse_vector() noexcept {}
  inline sparse_vector(Allocator&& alloc) noexcept : base_type(std::move<Allocator>(alloc)) {}
  inline sparse_vector(Allocator const& alloc) noexcept : base_type(alloc) {}
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
        items_[i] = acl::allocate<storage>(*this, sizeof(storage) * pool_size + sizeof(size_type), alignof(Ty));

        if (src_storage)
        {
          if constexpr (std::is_trivially_copyable_v<Ty> || has_pod)
          {
            std::memcpy(items_[i], src_storage, sizeof(storage) * pool_size + sizeof(size_type));
          }
          else
          {
            pool_occupation(i) = other.pool_occupation(i);
            for (size_type e = 0; e < pool_size; ++e)
            {
              auto const& src = reinterpret_cast<value_type const&>(src_storage[e]);
              auto&       dst = reinterpret_cast<value_type&>(items_[i][e]);

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
    for_each(items_, std::forward<Lambda>(lambda), std::true_type());
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for_each(items_, std::forward<Lambda>(lambda), std::true_type());
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, nocheck) noexcept
  {
    for_each(items_, std::forward<Lambda>(lambda), nocheck{});
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, nocheck) const noexcept
  {
    for_each(items_, std::forward<Lambda>(lambda), nocheck{});
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
    return reinterpret_cast<value_type const*>(items_[i]);
  }

  auto get_pool(size_type i) noexcept -> std::tuple<value_type*, size_type>
  {
    return reinterpret_cast<value_type*>(items_[i]);
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

    return *reinterpret_cast<value_type*>(items_[block] + index);
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
    assert(length_ > 0);
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
    assert(length_ > idx);
    if constexpr (!std::is_trivially_destructible_v<value_type> && !has_pod)
    {
      for (size_type i = idx; i < length_; ++i)
        std::destroy_at(std::addressof(item_at(i)));
    }
    length_ = idx;
  }

  void grow(size_type idx) noexcept
  {
    assert(length_ < idx);
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
    for (size_type block = 0; block < items_.size(); ++block)
    {
      if (items_[block])
        acl::deallocate(static_cast<Allocator&>(*this), items_[block],
                        sizeof(size_type) * pool_size + sizeof(size_type), alignof(Ty));
    }
    items_.clear();
    length_ = 0;
  }

  value_type& at(size_type l) noexcept
  {
    assert(l < length_);
    return item_at(l);
  }

  value_type const& at(size_type l) const noexcept
  {
    assert(l < length_);
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
    return block < items_.size() && items_[block] &&
           !is_null(reinterpret_cast<value_type const&>(items_[block][idx & pool_mod]));
  }

  Ty get_value(size_type idx) const noexcept
    requires(has_null_value)
  {
    auto block = (idx >> pool_div);
    return block < items_.size() && items_[block] ? reinterpret_cast<value_type const&>(items_[block][idx & pool_mod])
                                                  : traits::null_v;
  }

  Ty& get_unsafe(size_type idx) const noexcept
  {
    return reinterpret_cast<value_type const&>(items_[(idx >> pool_div)][idx & pool_mod]);
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
        items_[block] = acl::zallocate<storage>(*this, sizeof(storage) * pool_size + sizeof(size_type), alignof(Ty));
      else
      {
        items_[block] = acl::allocate<storage>(*this, sizeof(storage) * pool_size + sizeof(size_type), alignof(Ty));
        if constexpr (!has_no_fill)
        {
          if constexpr (has_pod ||
                        (std::is_trivially_copyable_v<value_type> && std::is_trivially_constructible_v<value_type>))
          {
            if constexpr (has_null_value)
              std::fill(reinterpret_cast<value_type*>(items_[block]),
                        reinterpret_cast<value_type*>(items_[block] + pool_size), traits::null_v);
            else if constexpr (has_null_construct)
            {
              std::for_each(reinterpret_cast<value_type*>(items_[block]),
                            reinterpret_cast<value_type*>(items_[block] + pool_size), traits::null_construct);
            }
            else
            {
              std::fill(reinterpret_cast<value_type*>(items_[block]),
                        reinterpret_cast<value_type*>(items_[block] + pool_size), value_type());
            }
          }
          else
          {
            std::for_each(reinterpret_cast<value_type*>(items_[block]),
                          reinterpret_cast<value_type*>(items_[block] + pool_size),
                          [](value_type& dst)
                          {
                            if constexpr (has_null_value)
                              std::construct_at(std::addressof(dst), traits::null_v);
                            else if constexpr (has_null_construct)
                              traits::null_construct(dst);
                            else
                              std::construct_at(std::addressof(dst));
                          });
          }
        }
      }
    }
  }

  inline size_type& pool_occupation(size_type p) noexcept
  {
    return *reinterpret_cast<size_type*>(reinterpret_cast<std::uint8_t*>(items_[p]) + sizeof(storage) * pool_size);
  }

  inline size_type pool_occupation(size_type p) const noexcept
  {
    return *reinterpret_cast<size_type const*>(reinterpret_cast<std::uint8_t const*>(items_[p]) +
                                               sizeof(storage) * pool_size);
  }

  inline void validate(size_type idx) const noexcept
  {
    assert(contains(idx));
  }

  inline auto& item_at(size_type idx) noexcept
  {
    auto block = (idx >> pool_div);
    ensure_block(block);
    return reinterpret_cast<value_type&>(items_[block][idx & pool_mod]);
  }

  inline auto& item_at(size_type idx) const noexcept
  {
    auto block = (idx >> pool_div);
    return reinterpret_cast<value_type const&>(items_[block][idx & pool_mod]);
  }

  inline void erase_at(size_type idx) noexcept
  {
    auto block = (idx >> pool_div);

    if constexpr (has_null_value)
      reinterpret_cast<value_type&>(items_[block][idx & pool_mod]) = traits::null_v;
    else if constexpr (has_null_construct)
      traits::null_reset(reinterpret_cast<value_type&>(items_[block][idx & pool_mod]));
    else
      reinterpret_cast<value_type&>(items_[block][idx & pool_mod]) = value_type();
    if (!--pool_occupation(block))
    {
      delete_block(block);
    }
  }

  inline void delete_block(size_type block)
  {
    if constexpr (!std::is_trivially_destructible_v<value_type> && !has_pod)
    {
      for (size_type i = 0; i < pool_size; ++i)
        std::destroy_at(std::addressof(reinterpret_cast<value_type&>(items_[block][i])));
    }

    acl::deallocate(static_cast<Allocator&>(*this), items_[block], sizeof(size_type) * pool_size + sizeof(size_type),
                    alignof(Ty));
    items_[block] = nullptr;
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept value_type& parameter
  template <typename Lambda, typename Store, typename Check>
  inline static void for_each(Store& items_, Lambda&& lambda, Check) noexcept
  {
    using Type = std::conditional_t<std::is_const_v<Store>, value_type const&, value_type&>;
    for (size_type block = 0; block < items_.size(); ++block)
    {
      constexpr auto arity = detail::function_traits<Lambda>::arity;

      auto store = items_[block];
      if (store)
      {
        for (size_type e = 0; e < pool_size; ++e)
        {
          if constexpr (Check::value)
          {
            if (is_null(reinterpret_cast<Type>(store[e])))
              continue;
          }
          if constexpr (arity == 2)
            lambda((block << pool_div) | e, reinterpret_cast<Type>(store[e]));
          else
            lambda(reinterpret_cast<Type>(store[e]));
        }
      }
    }
  }

  template <typename... Args>
  auto& emplace_at_idx(size_type idx, Args&&... args) noexcept
  {
    auto block = idx >> pool_div;
    auto index = idx & pool_mod;

    ensure_block(block);

    auto& dst = *reinterpret_cast<value_type*>(items_[block] + index);
    dst       = value_type(std::forward<Args>(args)...);
    pool_occupation(block)++;
    return dst;
  }

  podvector<storage*, allocator> items_;
  size_type                      length_ = 0;
};

} // namespace acl
