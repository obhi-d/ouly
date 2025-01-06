
#pragma once

#include <acl/utils/config.hpp>
#include <cassert>
#include <compare>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace acl
{

struct slist_hook
{
  void* pnext_ = nullptr;
};

struct list_hook
{
  void* pnext_ = nullptr;
  void* pprev_ = nullptr;
};

namespace detail
{

template <auto M>
struct intrusive_list_type_traits;

template <typename Ty, typename H, H Ty::* M>
struct intrusive_list_type_traits<M>
{
  using value_type               = Ty;
  using hook_type                = H;
  static constexpr bool is_dlist = std::is_same_v<list_hook, H>;

  static auto hook(Ty& t) noexcept -> H&
  {
    return t.*M;
  }

  static auto hook(Ty const& t) noexcept -> H const&
  {
    return t.*M;
  }

  static auto next(Ty const& t) noexcept -> Ty*
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<Ty*>(hook(t).pnext_);
  }

  static void next(Ty& t, Ty* next) noexcept
  {
    hook(t).pnext_ = next;
  }

  static auto prev(Ty const& t) noexcept -> Ty* requires(is_dlist) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<Ty*>(hook(t).pprev_);
  }

  static void prev(Ty& t, Ty* next) noexcept
    requires(is_dlist)
  {
    hook(t).pprev_ = next;
  }
};

template <typename SizeType, bool CacheSize = false>
struct size_counter
{
  void added() noexcept {}
  void added(SizeType /*unused*/) noexcept {}
  void erased() noexcept {}
  template <typename L>
  auto count(L const& list) const noexcept -> SizeType
  {
    SizeType nb = 0;
    for (auto i = std::begin(list), e = std::end(list); i != e; ++i, ++nb)
    {
      ;
    }
    return nb;
  }
  void clear() noexcept {}
};

template <typename SizeType>
struct size_counter<SizeType, true>
{
  void added() noexcept
  {
    count_++;
  }

  void erased() noexcept
  {
    count_--;
  }

  void added(SizeType a) noexcept
  {
    count_ += a;
  }

  template <typename L>
  auto count(L const& /*unused*/) const noexcept -> SizeType
  {
    return count_;
  }

  void clear() noexcept
  {
    count_ = 0;
  }

  SizeType count_ = 0;
};

template <typename Ty, auto M, typename B, bool CacheTail>
struct list_data : B
{
  Ty* head_ = nullptr;
};

template <typename Ty, auto M, typename B>
struct list_data<Ty, M, B, true> : B
{
  Ty* head_ = nullptr;
  Ty* tail_ = nullptr;
};

} // namespace detail

/**
 * @brief A templated intrusive linked list container.
 *
 * An intrusive list where nodes contain their own linking pointers, allowing for
 * efficient insertion and removal without additional memory allocation. Supports both
 * singly-linked and doubly-linked list variants.
 *
 * Features:
 * - Optional tail caching for O(1) back operations
 * - Optional size caching
 * - Bidirectional iteration when using doubly-linked nodes with tail caching
 * - Move semantics (no copy operations)
 *
 * @tparam M Member pointer type that points to the hook in the node
 * @tparam SizeType Integer type used for size tracking
 * @tparam CacheSize Whether to cache and maintain list size
 * @tparam CacheTail Whether to cache the tail node pointer
 *
 * Key operations:
 * - push_front() - O(1) insertion at front
 * - push_back() - O(1) insertion at back (requires CacheTail)
 * - pop_front() - O(1) removal from front
 * - pop_back() - O(1) removal from back (requires doubly-linked and CacheTail)
 * - insert_after() - O(1) insertion after node
 * - erase_after() - O(1) removal after node
 * - insert() - O(1) insertion before node (requires doubly-linked)
 * - erase() - O(1) removal of node (requires doubly-linked)
 *
 * @note This is an intrusive container - nodes must contain appropriate hook members.
 * @note No memory management is performed by the container.
 */
template <auto M, bool CacheSize = true, bool CacheTail = true, typename SizeType = uint32_t>
class intrusive_list
{
  using traits         = detail::intrusive_list_type_traits<M>;
  using node_type      = typename traits::value_type;
  using hook_type      = typename traits::hook_type;
  using size_type      = SizeType;
  using list_data_type = detail::list_data<node_type, M, detail::size_counter<SizeType, CacheSize>, CacheTail>;

  static_assert(std::is_same_v<slist_hook, hook_type> || std::is_same_v<list_hook, hook_type>,
                "Pointer to member must be a list hook type.");

  static constexpr bool is_dlist = std::is_same_v<list_hook, hook_type>;

  template <typename V>
  class iterator_type
  {
  public:
    constexpr iterator_type() noexcept = default;
    constexpr iterator_type(V* item) noexcept : item_(item) {}

    auto operator->() const noexcept
    {
      assert(item_);
      return item_;
    }

    auto operator*() const noexcept -> auto&
    {
      assert(item_);
      return *item_;
    }

    auto operator++() noexcept -> auto&
    {
      assert(item_);
      item_ = traits::next(*item_);
      return *this;
    }

    auto operator++(int) noexcept
    {
      assert(item_);
      auto old = *this;
      item_    = traits::next(*item_);
      return old;
    }

    auto operator<=>(iterator_type const& other) const noexcept = default;

  private:
    V* item_ = nullptr;
  };

  template <typename V>
  class reverse_iterator_type
  {
  public:
    constexpr reverse_iterator_type() noexcept = default;
    constexpr reverse_iterator_type(V* item) noexcept : item_(item) {}

    auto operator->() const noexcept
    {
      assert(item_);
      return item_;
    }

    auto operator*() const noexcept -> auto&
    {
      assert(item_);
      return *item_;
    }

    auto operator++() noexcept -> auto& requires(is_dlist) {
      assert(item_);
      item_ = traits::prev(*item_);
      return *this;
    }

    auto operator++(int) noexcept
      requires(is_dlist)
    {
      assert(item_);
      auto old = *this;
      item_    = traits::prev(*item_);
      return old;
    }

    auto operator<=>(reverse_iterator_type const& other) const noexcept = default;

  private:
    V* item_ = nullptr;
  };

  static constexpr bool bidir = is_dlist && CacheTail;

public:
  using value_type               = node_type;
  using iterator                 = iterator_type<value_type>;
  using const_iterator           = iterator_type<value_type const>;
  using reverse_iterator         = std::conditional_t<bidir, reverse_iterator_type<value_type>, void>;
  using const_reverse_iterator   = std::conditional_t<bidir, reverse_iterator_type<value_type const>, void>;
  static constexpr bool has_tail = CacheTail;

  intrusive_list() noexcept = default;
  intrusive_list(value_type& from, size_type count) noexcept
    requires(!CacheTail)
  {
    data_.head_ = &from;
    if (CacheSize)
    {
      data_.added(count);
    }
  }
  intrusive_list(value_type& from, value_type& to, size_type count) noexcept
    requires(CacheTail)
  {
    data_.head_ = &from;
    data_.tail_ = &to;
    if (CacheSize)
    {
      data_.added(count);
    }
  }
  intrusive_list(intrusive_list const&) = delete;
  intrusive_list(intrusive_list&& other) noexcept
  {
    *this = std::move(other);
  }

  auto operator=(intrusive_list const&) -> intrusive_list& = delete;
  auto operator=(intrusive_list&& other) noexcept -> intrusive_list&
  {
    data_.head_ = other.data_.head_;
    if constexpr (CacheTail)
    {
      data_.tail_ = other.data_.tail_;
    }
    if constexpr (CacheSize)
    {
      data_.added(other.size());
    }
    other.clear();
    return *this;
  }

  ~intrusive_list() noexcept = default;

  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return data_.head_ == nullptr;
  }

  auto size() const noexcept -> size_type
  {
    return data_.count(*this);
  }

  auto begin() noexcept -> iterator
  {
    return iterator(data_.head_);
  }

  constexpr auto end() noexcept -> iterator
  {
    return iterator();
  }

  auto begin() const noexcept -> const_iterator
  {
    return const_iterator(data_.head_);
  }

  constexpr auto end() const noexcept -> const_iterator
  {
    return const_iterator();
  }

  auto rbegin() noexcept -> reverse_iterator
    requires(bidir)
  {
    return reverse_iterator(data_.tail_);
  }

  constexpr auto rend() noexcept -> reverse_iterator
    requires(bidir)
  {
    return reverse_iterator();
  }

  auto rbegin() const noexcept -> const_reverse_iterator
    requires(bidir)
  {
    return const_reverse_iterator(data_.tail_);
  }

  constexpr auto rend() const noexcept -> const_reverse_iterator
    requires(bidir)
  {
    return const_reverse_iterator();
  }

  auto cbegin() const noexcept -> const_iterator
  {
    return begin();
  }

  constexpr auto cend() const noexcept -> iterator
  {
    return iterator();
  }

  auto front() noexcept -> value_type&
  {
    return *data_.head_;
  }

  auto back() noexcept -> value_type& requires(CacheTail) { return *data_.tail_; }

  auto front() const noexcept -> value_type const&
  {
    return *data_.head_;
  }

  auto back() const noexcept -> value_type const&
    requires(CacheTail)
  {
    return *data_.tail_;
  }

  void push_back(value_type& obj) noexcept
    requires(CacheTail)
  {
    if (data_.tail_)
    {
      traits::next(*data_.tail_, &obj);
      if constexpr (is_dlist)
      {
        traits::prev(obj, data_.tail_);
      }
      data_.tail_ = &obj;
    }
    else
    {
      data_.head_ = data_.tail_ = &obj;
    }
    data_.added();
  }

  void push_front(value_type& obj) noexcept
  {
    if (data_.head_)
    {
      traits::next(obj, data_.head_);
      if constexpr (is_dlist)
      {
        traits::prev(*data_.head_, &obj);
      }
      data_.head_ = &obj;
    }
    else
    {
      data_.head_ = &obj;
      if constexpr (CacheTail)
      {
        data_.tail_ = &obj;
      }
    }
    data_.added();
  }
  // NOLINTNEXTLINE
  void append_front(intrusive_list&& other) noexcept
    requires(is_dlist && CacheTail)
  {
    if (data_.head_)
    {
      traits::next(other.back(), data_.head_);
      traits::prev(*data_.head_, &other.back());
      data_.head_ = &other.front();
    }
    else
    {
      data_.head_ = other.data_.head_;
      data_.tail_ = other.data_.tail_;
    }
    data_.added(other.size());
    other.clear();
  }
  // NOLINTNEXTLINE
  void append_back(intrusive_list&& other) noexcept
    requires(is_dlist && CacheTail)
  {
    if (data_.tail_)
    {
      traits::next(*data_.tail_, &other.front());
      traits::prev(other.front(), data_.tail_);
      data_.tail_ = &other.back();
    }
    else
    {
      data_.head_ = other.data_.head_;
      data_.tail_ = other.data_.tail_;
    }
    data_.added(other.size());
    other.clear();
  }

  void clear() noexcept
  {
    data_.head_ = nullptr;
    if constexpr (CacheTail)
    {
      data_.tail_ = nullptr;
    }
    data_.clear();
  }

  void erase_after(iterator it) noexcept
  {
    auto& l = *it;
    erase_after(l);
  }

  void erase_after(value_type& l) noexcept
  {
    auto next = traits::next(l);
    if (next)
    {
      auto next_next = traits::next(*next);
      traits::next(l, next_next);
      if constexpr (CacheTail)
      {
        if (next_next)
        {
          if constexpr (is_dlist)
          {
            traits::prev(*next_next, &l);
          }
        }
        else
        {
          data_.tail_ = &l;
        }
      }
      else if constexpr (is_dlist)
      {
        if (next_next)
        {
          traits::prev(*next_next, &l);
        }
      }

      traits::next(*next, nullptr);
      if constexpr (is_dlist)
      {
        traits::prev(*next, nullptr);
      }
      data_.erased();
    }
  }

  void insert_after(iterator it, value_type& obj) noexcept
  {
    insert_after(*it, obj);
  }

  void insert_after(value_type& l, value_type& obj) noexcept
  {
    auto next = traits::next(l);
    traits::next(l, &obj);
    if (!next)
    {
      if constexpr (CacheTail)
      {
        data_.tail_ = &obj;
      }
    }
    else
    {
      traits::next(obj, next);
      if constexpr (is_dlist)
      {
        traits::prev(*next, &obj);
      }
    }
    data_.added();
  }

  void append_after(iterator it, intrusive_list&& other) noexcept
  {
    append_after(*it, std::move(other));
  }

  // NOLINTNEXTLINE
  void append_after(value_type& l, intrusive_list&& other) noexcept
  {
    auto next = traits::next(l);
    traits::next(l, &other.front());
    if (!next)
    {
      if constexpr (CacheTail)
      {
        data_.tail_ = &other.back();
      }
    }
    else
    {
      traits::next(other.back(), next);
      if constexpr (is_dlist)
      {
        traits::prev(*next, &other.back());
      }
    }
    if constexpr (CacheSize)
    {
      data_.added(other.size());
    }
    other.clear();
  }

  void insert(iterator it, value_type& obj) noexcept
    requires(is_dlist)
  {
    insert(*it, obj);
  }

  void insert(value_type& l, value_type& obj) noexcept
    requires(is_dlist)
  {
    if (&l == data_.head_)
    {
      push_front(obj);
    }
    else
    {
      auto prev = traits::prev(l);
      traits::prev(obj, prev);
      traits::next(*prev, &obj);
      traits::prev(l, &obj);
      traits::next(obj, &l);
      data_.added();
    }
  }

  void append(iterator it, intrusive_list&& other) noexcept
    requires(is_dlist && CacheTail)

  {
    append(*it, std::move(other));
  }

  void append(value_type& l, intrusive_list&& other) noexcept
    requires(is_dlist && CacheTail)
  {
    if (&l == data_.head_)
    {
      append_front(std::move(other));
    }
    else
    {
      auto  prev = traits::prev(l);
      auto& f    = other.front();
      traits::prev(f, prev);
      traits::next(*prev, &f);
      auto& b = other.back();
      traits::prev(l, &b);
      traits::next(b, &l);
      if constexpr (CacheSize)
      {
        data_.added(other.size());
      }
      other.clear();
    }
  }

  void erase(iterator it) noexcept
    requires(is_dlist)
  {
    erase(*it);
  }

  void erase(value_type& l) noexcept
    requires(is_dlist)
  {
    auto prev = traits::prev(l);
    auto next = traits::next(l);
    traits::next(l, nullptr);
    traits::prev(l, nullptr);
    if (prev)
    {
      traits::next(*prev, next);
    }
    else
    {
      data_.head_ = next;
    }
    if (next)
    {
      traits::prev(*next, prev);
    }
    else
    {
      if constexpr (CacheTail)
      {
        data_.tail_ = prev;
      }
    }
    data_.erased();
  }

  void pop_back() noexcept
    requires(CacheTail && is_dlist)
  {
    auto& l    = *data_.tail_;
    auto  prev = traits::prev(l);
    traits::prev(l, nullptr);
    if (prev)
    {
      traits::next(*prev, nullptr);
    }
    else
    {
      data_.head_ = nullptr;
    }
    data_.tail_ = prev;
    data_.erased();
  }

  void pop_front() noexcept
  {
    auto& l    = *data_.head_;
    auto  next = traits::next(l);
    traits::next(l, nullptr);
    if (next)
    {
      if constexpr (is_dlist)
      {
        traits::prev(*next, nullptr);
      }
    }
    else
    {
      if constexpr (CacheTail)
      {
        data_.tail_ = nullptr;
      }
    }
    data_.head_ = next;
    data_.erased();
  }

private:
  list_data_type data_;
};

} // namespace acl
