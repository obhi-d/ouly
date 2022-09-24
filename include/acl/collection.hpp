
#pragma once

#include "allocator.hpp"
#include "default_allocator.hpp"
#include "detail/config.hpp"
#include "detail/utils.hpp"
#include "link.hpp"
#include "podvector.hpp"
#include "type_traits.hpp"
#include <memory>

namespace acl
{

/// @brief A collection of links, but not stored in vectors, instead managed by bitmap
/// @tparam Ty
/// @tparam Allocator any allocator type under acl
template <typename Cont, typename Allocator = default_allocator<>,
          typename Traits = acl::traits<typename Cont::value_type>>
class collection : public Allocator
{
public:
  using value_type     = typename Cont::value_type;
  using size_type      = detail::choose_size_t<Traits, Cont, Allocator>;
  using link           = acl::link<value_type, size_type>;
  using allocator_type = Allocator;

private:
  static constexpr auto pool_div  = detail::log2(Traits::pool_size);
  static constexpr auto pool_size = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_mod  = pool_size - 1;
  using this_type                 = collection<Cont, Allocator>;
  using base_type                 = Allocator;
  using storage                   = std::uint8_t;
  using allocator                 = Allocator;

public:
  inline collection() noexcept {}
  inline collection(Allocator&& alloc) noexcept : base_type(std::move<Allocator>(alloc)) {}
  inline collection(Allocator const& alloc) noexcept : base_type(alloc) {}
  inline collection(collection&& other) noexcept = default;
  inline collection(collection const& other) noexcept
  {
    *this = other;
  }

  inline ~collection() noexcept
  {
    clear();
    shrink_to_fit();
  }

  collection& operator=(collection&& other) noexcept = default;

  collection& operator=(collection const& other) noexcept
  {
    constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
    constexpr auto haz_page_size = sizeof(storage) * pool_size;

    items.resize(other.items.size());
    if constexpr (detail::debug)
    {
      for (std::size_t i = 0, end = items.size() / 2; i < end; ++i)
      {
        items[i * 2 + 0] = acl::allocate<storage>(*this, bit_page_size);
        items[i * 2 + 1] = acl::allocate<storage>(*this, haz_page_size);
        std::memcpy(items[i * 2 + 0], other.items[i * 2 + 0], bit_page_size);
        std::memcpy(items[i * 2 + 1], other.items[i * 2 + 1], haz_page_size);
      }
    }
    else
    {
      for (std::size_t i = 0, end = items.size(); i < end; ++i)
      {
        items[i] = acl::allocate<storage>(*this, bit_page_size);
        std::memcpy(items[i], other.items[i], bit_page_size);
      }
    }

    length = other.length;
    return *this;
  }

  template <typename Lambda>
  void for_each(Cont& cont, Lambda&& lambda) noexcept
  {
    for_each_l<Cont, Lambda, value_type>(cont, 0, range(), std::forward<Lambda>(lambda));
  }
  template <typename Lambda>
  void for_each(Cont const& cont, Lambda&& lambda) const noexcept
  {
    const_cast<this_type*>(this)->for_each_l<Cont const, Lambda, value_type const>(cont, 0, range(),
                                                                                   std::forward<Lambda>(lambda));
  }

  template <typename Lambda>
  void for_each(Cont& cont, size_type first, size_type last, Lambda&& lambda) noexcept
  {
    for_each_l<Cont, Lambda, value_type>(cont, first, last, std::forward<Lambda>(lambda));
  }

  template <typename Lambda>
  void for_each(Cont const& cont, size_type first, size_type last, Lambda&& lambda) const noexcept
  {
    const_cast<this_type*>(this)->for_each_l<Cont const, Lambda, value_type const>(cont, first, last,
                                                                                   std::forward<Lambda>(lambda));
  }

  inline void emplace(link l) noexcept
  {
    auto idx = detail::index_val(l.value());
    max_lnk  = std::max(idx, max_lnk);
    set_bit(idx);
    if constexpr (detail::debug)
      set_hazard(idx, detail::hazard_val(l.value()));
    length++;
  }

  inline void remove(link l) noexcept
  {
    auto idx = detail::index_val(l.value());
    if constexpr (detail::debug)
      validate_hazard(idx, detail::hazard_val(l.value()));
    unset_bit(idx);
    length--;
  }

  bool contains(link l) const noexcept
  {
    return is_bit_set(detail::index_val(l.value()));
  }

  size_type size() const noexcept
  {
    return length;
  }

  size_type capacity() const noexcept
  {
    return static_cast<size_type>(items.size()) * pool_size;
  }

  size_type range() const noexcept
  {
    return max_lnk + 1;
  }

  void shrink_to_fit() noexcept
  {
    if (!length)
    {
      constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
      constexpr auto haz_page_size = sizeof(storage) * pool_size;

      if constexpr (detail::debug)
      {
        for (size_type i = 0, end = static_cast<size_type>(items.size()) / 2; i < end; ++i)
        {
          acl::deallocate(static_cast<Allocator&>(*this), items[i * 2 + 0], bit_page_size);
          acl::deallocate(static_cast<Allocator&>(*this), items[i * 2 + 1], haz_page_size);
        }
      }
      else
      {
        for (auto i : items)
        {
          acl::deallocate(static_cast<Allocator&>(*this), items[i], bit_page_size);
        }
      }
    }
  }

  void clear() noexcept
  {
    length  = 0;
    max_lnk = 0;
  }

private:
  inline void validate_hazard(size_type nb, std::uint8_t hz) const noexcept
  {
    auto block = hazard_page(nb >> pool_div);
    auto index = nb & pool_mod;

    assert(items[block][index] == hz);
  }

  inline size_type bit_page(size_type p) const noexcept
  {
    if constexpr (detail::debug)
      return p * 2;
    else
      return p;
  }

  inline size_type hazard_page(size_type p) const noexcept
  {
    if constexpr (detail::debug)
      return (p * 2) + 1;
    else
      return 0xffffffff;
  }

  inline bool is_bit_set(size_type nb) const noexcept
  {
    auto                   block = bit_page(nb >> pool_div);
    auto                   index = nb & pool_mod;
    constexpr std::uint8_t one   = 1;

    return (block < items.size()) && (items[block][index >> 3] & (one << static_cast<std::uint8_t>(index & 0x7)));
  }

  inline void unset_bit(size_type nb) noexcept
  {
    auto block = bit_page(nb >> pool_div);
    auto index = nb & pool_mod;

    constexpr std::uint8_t one = 1;
    items[block][index >> 3] &= ~(one << static_cast<std::uint8_t>(index & 0x7));
  }

  inline void set_bit(size_type nb) noexcept
  {
    auto block = bit_page(nb >> pool_div);
    auto index = nb & pool_mod;

    if (block >= items.size())
    {
      constexpr auto bit_page_size = sizeof(storage) * (pool_size >> 3);
      constexpr auto haz_page_size = sizeof(storage) * pool_size;

      items.emplace_back(acl::allocate<storage>(*this, bit_page_size));
      std::memset(items.back(), 0, bit_page_size);
      if constexpr (detail::debug)
      {
        items.emplace_back(acl::allocate<storage>(*this, haz_page_size));
        std::memset(items.back(), 0, haz_page_size);
      }
    }

    constexpr std::uint8_t one = 1;
    items[block][index >> 3] |= one << static_cast<std::uint8_t>(index & 0x7);
  }

  inline void set_hazard(size_type nb, std::uint8_t hz) noexcept
  {
    auto block          = hazard_page(nb >> pool_div);
    auto index          = nb & pool_mod;
    items[block][index] = hz;
  }

  inline std::uint8_t get_hazard(size_type nb) noexcept
  {
    auto block = hazard_page(nb >> pool_div);
    auto index = nb & pool_mod;
    return items[block][index];
  }

  template <typename ContT, typename Lambda, typename Cast>
  void for_each_l(ContT& cont, size_type first, size_type last, Lambda&& lambda) noexcept
  {
    for (; first != last; ++first)
    {
      if (is_bit_set(first))
      {
        link l{(detail::debug) ? detail::hazard_idx(first, get_hazard(first)) : first};
        lambda(l, reinterpret_cast<Cast&>(cont.at(l)));
      }
    }
  }

  podvector<storage*, allocator> items;
  size_type                      length  = 0;
  size_type                      max_lnk = 0;
};

} // namespace acl