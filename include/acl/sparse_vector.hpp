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
class sparse_vector : public Allocator
{
public:
  using value_type     = Ty;
  using size_type      = detail::choose_size_t<Traits, Allocator>;
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

public:
  inline sparse_vector() noexcept {}
  inline sparse_vector(Allocator&& alloc) noexcept : base_type(std::move<Allocator>(alloc)) {}
  inline sparse_vector(Allocator const& alloc) noexcept : base_type(alloc) {}
  inline sparse_vector(sparse_vector&& other) noexcept
  {
    *this = std::move(other);
  }
  inline sparse_vector(sparse_vector const& other) noexcept requires(std::is_copy_constructible_v<value_type>)
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
    items              = std::move(other.items);
    length             = other.length;
    other.length       = 0;
    return *this;
  }

  sparse_vector& operator=(sparse_vector const& other) noexcept requires(std::is_copy_constructible_v<value_type>)
  {
    clear();
    items.resize(other.items.size());
    for (size_type i = 0; i < items.size(); ++i)
    {
      auto const src_storage = other.items[i];
      if (src_storage)
      {
        items[i] = acl::allocate<storage>(*this, sizeof(storage) * pool_size + sizeof(size_type));

        if (src_storage)
        {
          if constexpr (std::is_trivially_copyable_v<Ty> || traits::assume_pod_v)
          {
            std::memcpy(items[i], src_storage, sizeof(storage) * pool_size + sizeof(size_type));
          }
          else
          {
            pool_occupation(i) = other.pool_occupation(i);
            for (size_type e = 0; e < pool_size; ++e)
            {
              auto const& src = reinterpret_cast<value_type const&>(src_storage[e]);
              auto&       dst = reinterpret_cast<value_type&>(items[i][e]);

              if (src != traits::null_v)
                std::construct_at(&dst, src);
            }
          }
        }
      }
      else
        items[i] = nullptr;
    }

    static_cast<base_type&>(*this) = static_cast<base_type const&>(other);
    length                         = other.length;
    return *this;
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) noexcept
  {
    for_each(items, std::forward<Lambda>(lambda), std::true_type());
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda) const noexcept
  {
    for_each(items, std::forward<Lambda>(lambda), std::true_type());
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, nocheck) noexcept
  {
    for_each(items, std::forward<Lambda>(lambda), nocheck{});
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept A link and value_type const& parameter
  template <typename Lambda>
  void for_each(Lambda&& lambda, nocheck) const noexcept
  {
    for_each(items, std::forward<Lambda>(lambda), nocheck{});
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

  /// @brief packed_table has active pool count depending upon number of elements it contains
  /// @return active pool count
  size_type max_pools() const noexcept
  {
    return static_cast<size_type>(items.size());
  }

  /// @brief Get item pool and number of items give the pool number
  /// @param i Must be between [0, active_pools())
  /// @return Item pool raw array and array size
  auto get_pool(size_type i) const noexcept -> std::tuple<value_type const*, size_type>
  {
    return reinterpret_cast<value_type const*>(items[i]);
  }

  auto get_pool(size_type i) noexcept -> std::tuple<value_type*, size_type>
  {
    return reinterpret_cast<value_type*>(items[i]);
  }

  auto& back()
  {
    return at(length - 1);
  }

  auto& front()
  {
    return at(0);
  }
    
  auto const& back() const 
  {
    return at(length - 1);
  }

  auto const& front() const 
  {
    return at(0);
  }

  template <typename... Args>
  auto& emplace_back(Args&&... args) noexcept
  {
    return emplace_at(length, std::forward<Args>(args)...);
  }
  /// @brief Emplace back an element. Order is not guranteed.
  /// @tparam ...Args Constructor args for value_type
  /// @return Returns link to the element pushed. link can be used to destroy entry.
  template <typename... Args>
  auto& emplace_at(size_type idx, Args&&... args) noexcept
  {
    auto block = idx >> pool_div;
    auto index = idx & pool_mod;

    if (block >= items.size())
    {
      items.resize(block + 1, nullptr);
    }
    if (!items[block])
    {
      items[block] = acl::allocate<storage>(*this, sizeof(storage) * pool_size + sizeof(size_type));
      if (traits::assume_pod_v ||
          (std::is_trivially_copyable_v<value_type> && std::is_trivially_constructible_v<value_type>))
        std::fill(reinterpret_cast<value_type*>(items[block]), reinterpret_cast<value_type*>(items[block] + pool_size),
                  traits::null_v);
      else
      {
        std::for_each(reinterpret_cast<value_type*>(items[block]),
                      reinterpret_cast<value_type*>(items[block] + pool_size),
                      [](value_type& dst)
                      {
                        std::construct_at(std::addressof(dst), traits::null_v);
                      });
      }
    }

    auto& dst = *reinterpret_cast<value_type*>(items[block] + index);
    dst       = value_type(std::forward<Args>(args)...);
    pool_occupation(block)++;
    length++;
    return dst;
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

  /// @brief Drop unused pages
  void shrink_to_fit() noexcept
  {
    items.shrink_to_fit();
  }

  /// @brief Set size to 0, memory is not released, objects are destroyed
  void clear() noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<value_type> && !traits::assume_pod_v)
    {
      for_each(
        [](auto, auto& v)
        {
          std::destroy_at(std::addressof(v));
        });
    }
    for (size_type block = 0; block < items.size(); ++block)
    {
      if (items[block])
        acl::deallocate(static_cast<Allocator&>(*this), items[block],
                        sizeof(size_type) * pool_size + sizeof(size_type));
    }
    items.clear();
    length = 0;
  }

  value_type& at(size_type l) noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l, items);
  }

  value_type const& at(size_type l) const noexcept
  {
    if constexpr (detail::debug)
      validate(l);
    return item_at(l, items);
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
    return block < items.size() && items[block] &&
           reinterpret_cast<value_type&>(items[block][idx & pool_mod]) != traits::null_v;
  }

  bool empty() const noexcept
  {
    return length == 0;
  }

private:
  inline size_type& pool_occupation(size_type p) noexcept
  {
    return *reinterpret_cast<size_type*>(reinterpret_cast<std::uint8_t*>(items[p]) + sizeof(storage) * pool_size);
  }
  inline size_type pool_occupation(size_type p) const noexcept
  {
    return *reinterpret_cast<size_type const*>(reinterpret_cast<std::uint8_t const*>(items[p]) +
                                               sizeof(storage) * pool_size);
  }
  inline void validate(size_type idx) const noexcept
  {
    assert(contains(idx));
  }

  template <typename Store>
  static inline auto& item_at(size_type idx, Store& store_items) noexcept
  {
    auto block = (idx >> pool_div);
    return reinterpret_cast<std::conditional_t<std::is_const_v<Store>, value_type const&, value_type&>>(
      store_items[block][idx & pool_mod]);
  }

  inline void erase_at(size_type idx) noexcept
  {
    length--;
    auto block                                                  = (idx >> pool_div);
    reinterpret_cast<value_type&>(items[block][idx & pool_mod]) = traits::null_v;
    if (!--pool_occupation(block))
    {
      delete_block(block);
    }
  }

  inline void delete_block(size_type block)
  {
    if constexpr (!std::is_trivially_destructible_v<value_type> && !traits::assume_pod_v)
    {
      for (size_type i = 0; i < pool_size; ++i)
        std::destroy_at(std::addressof(reinterpret_cast<value_type&>(items[block][i])));
    }

    acl::deallocate(static_cast<Allocator&>(*this), items[block], sizeof(size_type) * pool_size + sizeof(size_type));
    items[block] = nullptr;
  }

  /// @brief Lambda called for each element
  /// @tparam Lambda Lambda should accept value_type& parameter
  template <typename Lambda, typename Store, typename Check>
  static void for_each(Store& items, Lambda&& lambda, Check) noexcept
  {
    using Type = std::conditional_t<std::is_const_v<Store>, value_type const&, value_type&>;
    for (size_type block = 0; block < items.size(); ++block)
    {
      auto store = items[block];
      if (store)
      {
        for (size_type e = 0; e < pool_size; ++e)
        {
          if constexpr (Check::value)
          {
            if (reinterpret_cast<Type>(store[e]) == traits::null_v)
              continue;
          }
          lambda((block << pool_size) | e, reinterpret_cast<Type>(store[e]));
        }
      }
    }
  }

  podvector<storage*, allocator> items;
  size_type                      length = 0;
};

} // namespace acl
