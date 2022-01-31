#pragma once
#include "link.hpp"
#include "podvector.hpp"
#include "type_traits.hpp"
#include "utils.hpp"

namespace acl
{

namespace detail
{

template <typename Ty, typename Allocator, typename Traits, typename Base = Allocator>
class base_indirection : public Base
{
protected:
  using size_type        = typename Traits::size_type;
  using allocator_traits = std::allocator_traits<Allocator>;
  template <typename RebT>
  using allocator = typename allocator_traits::template rebind_alloc<RebT>;

public:
  base_indirection()                        = default;
  base_indirection(base_indirection&&)      = default;
  base_indirection(base_indirection const&) = default;
  template <typename... BaseArgs>
  base_indirection(BaseArgs&&... base) : Base(std::forward<BaseArgs>(base)...)
  {}

  base_indirection& operator=(base_indirection&&) = default;
  base_indirection& operator=(base_indirection const&) = default;

protected:
  inline size_type& link_at(size_type i) noexcept
  {
    return links[i];
  }

  inline size_type link_at(size_type i) const noexcept
  {
    return links[i];
  }

  static inline size_type to_index(auto l) noexcept
  {
    if constexpr (detail::debug)
    {
      auto id = detail::index_val(l);
      return id;
    }
    else
      return l;
  }

  static inline size_type to_link(size_type index) noexcept
  {
    return index;
  }

  inline size_type push(size_type loc) noexcept
  {
    auto idx = static_cast<size_type>(links.size());
    links.emplace_back(loc);
    return idx;
  }

  inline void shrink_to_fit(size_type) noexcept
  {
    links.shrink_to_fit();
  }

  inline void clear() noexcept
  {
    links.clear();
  }

  inline size_type max_size() const noexcept
  {
    return links.size();
  }

  podvector<size_type, allocator<size_type>> links;
};

template <typename Ty, typename Allocator, typename Traits, typename Base = Allocator>
class ref_indirection : public Base
{
protected:
  using size_type = typename Traits::size_type;

  static constexpr auto pool_div          = detail::log2(Traits::idx_pool_size);
  static constexpr auto pool_size         = static_cast<size_type>(1) << pool_div;
  static constexpr auto pool_mod          = pool_size - 1;
  static constexpr bool base_is_allocator = std::is_same_v<std::decay_t<Base>, std::decay_t<Allocator>>;
  using allocator_traits                  = std::allocator_traits<Allocator>;
  using allocator                         = typename allocator_traits::template rebind_alloc<size_type*>;

public:
  ref_indirection() noexcept                  = default;
  ref_indirection(ref_indirection&&) noexcept = default;
  ref_indirection(ref_indirection const& other) noexcept
  {
    *this = other;
  }
  template <typename... BaseArgs>
  ref_indirection(BaseArgs&&... base) : Base(std::forward<BaseArgs>(base)...)
  {}

  ref_indirection& operator=(ref_indirection&&) noexcept = default;

  ref_indirection& operator=(ref_indirection const& other) noexcept
  {
    refs.resize(other.refs.size());
    for (size_type i = 0, end = static_cast<size_type>(other.refs.size()); i != end; ++i)
    {
      refs[i] = reinterpret_cast<size_type*>(allocator_traits::allocate(*this, sizeof(size_type) * pool_size));
      std::memcpy(refs[i], other.refs[i], sizeof(size_type) * pool_size);
    }
    static_cast<Base&>(*this) = static_cast<Base const&>(other);
    return *this;
  }

protected:
  size_type get_ref(size_type link) const noexcept
  {
    return refs[link >> pool_div][link & pool_mod];
  }

  inline size_type pop_ref(size_type src, size_type dst)
  {
    return (refs[dst >> pool_div][dst & pool_mod] = refs[src >> pool_div][src & pool_mod]);
  }

  inline void set_ref(size_type loc, size_type lnk) noexcept
  {
    auto block = loc >> pool_div;
    auto index = loc & pool_mod;
    if (block >= refs.size())
      refs.resize(block + 1, nullptr);
    if (!refs[block])
      refs[block] = reinterpret_cast<size_type*>(allocator_traits::allocate(*this, sizeof(size_type) * pool_size));
    refs[block][index] = lnk;
  }

  inline void shrink_to_fit(size_type length) noexcept
  {
    auto block = (length + pool_size - 1) >> pool_div;
    for (auto i = block, end = static_cast<size_type>(refs.size()); i < end; ++i)
      allocator_traits::deallocate(*this, reinterpret_cast<allocator_traits::pointer>(refs[i]),
                                   sizeof(size_type) * pool_size);
    refs.resize(block);
    refs.shrink_to_fit();
    if constexpr (!base_is_allocator)
      Base::shrink_to_fit(length);
  }

  inline void clear() noexcept
  {
    if constexpr (!base_is_allocator)
      Base::clear();
  }

  podvector<size_type*, allocator> refs;
};

template <typename Ty, typename Allocator, typename Traits, typename Base = Allocator>
class ref_backref : public Base
{
protected:
  using size_type                         = typename Traits::size_type;
  using link                              = acl::link<Ty, size_type>;
  using backref                           = typename Traits::offset;
  static constexpr bool base_is_allocator = std::is_same_v<std::decay_t<Base>, std::decay_t<Allocator>>;

public:
  ref_backref() noexcept              = default;
  ref_backref(ref_backref&&) noexcept = default;
  ref_backref(ref_backref const& other) noexcept
  {
    *this = other;
  }
  template <typename... BaseArgs>
  ref_backref(BaseArgs&&... base) : Base(std::forward<BaseArgs>(base)...)
  {}

  ref_backref& operator=(ref_backref&&) noexcept = default;
  ref_backref& operator=(ref_backref const& other) noexcept = default;

protected:
  static size_type get_ref(Ty const& obj) noexcept
  {
    return backref::get(obj);
  }

  static inline size_type pop_ref(Ty const& src, Ty& dst)
  {
    auto l = backref::get(src);
    backref::set(dst, l);
    return l;
  }

  static inline void set_ref(Ty& dst, size_type lnk) noexcept
  {
    backref::set(dst, lnk);
  }

  inline void shrink_to_fit(size_type length) noexcept
  {
    if constexpr (!base_is_allocator)
      Base::shrink_to_fit(length);
  }

  inline void clear() noexcept
  {
    if constexpr (!base_is_allocator)
      Base::clear();
  }
};

template <typename Ty, typename Allocator, typename Traits>
using packed_table_base =
  std::conditional_t<has_backref_v<Traits>,
                     detail::ref_backref<Ty, Allocator, Traits, detail::base_indirection<Ty, Allocator, Traits>>,
                     detail::ref_indirection<Ty, Allocator, Traits, detail::base_indirection<Ty, Allocator, Traits>>>;

template <typename Ty, typename Allocator, typename Traits>
using sparse_table_base = std::conditional_t<has_backref_v<Traits>, detail::ref_backref<Ty, Allocator, Traits>,
                                             detail::ref_indirection<Ty, Allocator, Traits>>;

} // namespace detail

} // namespace acl