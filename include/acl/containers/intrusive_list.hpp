
#pragma once

#include <acl/detail/config.hpp>
#include <cassert>
#include <compare>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace acl
{

struct slist_hook
{
  void* next = nullptr;
};

struct list_hook
{
  void* next = nullptr;
  void* prev = nullptr;
};

namespace detail
{

template <auto M>
struct intrusive_list_type_traits;

template <typename Ty, typename H, H Ty::*M>
struct intrusive_list_type_traits<M>
{
  using value_type               = Ty;
  using hook_type                = H;
  static constexpr bool is_dlist = std::is_same_v<list_hook, H>;

  inline static H& hook(Ty& t) noexcept
  {
    return t.*M;
  }

  inline static H const& hook(Ty const& t) noexcept
  {
    return t.*M;
  }

  inline static Ty* next(Ty const& t) noexcept
  {
    return reinterpret_cast<Ty*>(hook(t).next);
  }

  inline static void next(Ty& t, Ty* next) noexcept
  {
    hook(t).next = next;
  }

  inline static Ty* prev(Ty const& t) noexcept
  requires(is_dlist)
  {
    return reinterpret_cast<Ty*>(hook(t).prev);
  }

  inline static void prev(Ty& t, Ty* next) noexcept
  requires(is_dlist)
  {
    hook(t).prev = next;
  }
};

template <typename SizeType, bool CacheSize = false>
struct size_counter
{
  inline void added() noexcept {}
  inline void added(SizeType) noexcept {}
  inline void erased() noexcept {}
  template <typename L>
  inline SizeType count(L const& list) const noexcept
  {
    SizeType nb = 0;
    for (auto i = std::begin(list), e = std::end(list); i != e; ++i, ++nb)
      ;
    return nb;
  }
  inline void clear() noexcept {}
};

template <typename SizeType>
struct size_counter<SizeType, true>
{
  inline void added() noexcept
  {
    count_++;
  }

  inline void erased() noexcept
  {
    count_--;
  }

  inline void added(SizeType a) noexcept
  {
    count_ += a;
  }

  template <typename L>
  inline SizeType count(L const&) const noexcept
  {
    return count_;
  }

  inline void clear() noexcept
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
    inline constexpr iterator_type(V* item) noexcept : item_(item) {}

    inline auto operator->() const noexcept
    {
      ACL_ASSERT(item_);
      return item_;
    }

    inline auto& operator*() const noexcept
    {
      ACL_ASSERT(item_);
      return *item_;
    }

    auto& operator++() noexcept
    {
      ACL_ASSERT(item_);
      item_ = traits::next(*item_);
      return *this;
    }

    auto operator++(int) noexcept
    {
      ACL_ASSERT(item_);
      auto old = *this;
      item_    = traits::next(*item_);
      return old;
    }

    inline auto operator<=>(iterator_type const& other) const noexcept = default;

  private:
    V* item_ = nullptr;
  };

  template <typename V>
  class reverse_iterator_type
  {
  public:
    constexpr reverse_iterator_type() noexcept = default;
    inline constexpr reverse_iterator_type(V* item) noexcept : item_(item) {}

    inline auto operator->() const noexcept
    {
      ACL_ASSERT(item_);
      return item_;
    }

    inline auto& operator*() const noexcept
    {
      ACL_ASSERT(item_);
      return *item_;
    }

    auto& operator++() noexcept
    requires(is_dlist)
    {
      ACL_ASSERT(item_);
      item_ = traits::prev(*item_);
      return *this;
    }

    auto operator++(int) noexcept
    requires(is_dlist)
    {
      ACL_ASSERT(item_);
      auto old = *this;
      item_    = traits::prev(*item_);
      return old;
    }

    inline auto operator<=>(reverse_iterator_type const& other) const noexcept = default;

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
      data_.added(count);
  }
  intrusive_list(value_type& from, value_type& to, size_type count) noexcept
  requires(CacheTail)
  {
    data_.head_ = &from;
    data_.tail_ = &to;
    if (CacheSize)
      data_.added(count);
  }
  intrusive_list(intrusive_list const&) = delete;
  intrusive_list(intrusive_list&& other) noexcept
  {
    *this = std::move(other);
  }

  intrusive_list& operator=(intrusive_list const&) = delete;
  intrusive_list& operator=(intrusive_list&& other) noexcept
  {
    data_.head_ = other.data_.head_;
    if constexpr (CacheTail)
      data_.tail_ = other.data_.tail_;
    if constexpr (CacheSize)
      data_.added(other.size());
    other.clear();
    return *this;
  }

  inline bool empty() const noexcept
  {
    return data_.head_ == nullptr;
  }

  inline size_type size() const noexcept
  {
    return data_.count(*this);
  }

  inline iterator begin() noexcept
  {
    return iterator(data_.head_);
  }

  inline constexpr iterator end() noexcept
  {
    return iterator();
  }

  inline const_iterator begin() const noexcept
  {
    return const_iterator(data_.head_);
  }

  inline constexpr const_iterator end() const noexcept
  {
    return const_iterator();
  }

  inline reverse_iterator rbegin() noexcept
  requires(bidir)
  {
    return reverse_iterator(data_.tail_);
  }

  inline constexpr reverse_iterator rend() noexcept
  requires(bidir)
  {
    return reverse_iterator();
  }

  inline const_reverse_iterator rbegin() const noexcept
  requires(bidir)
  {
    return const_reverse_iterator(data_.tail_);
  }

  inline constexpr const_reverse_iterator rend() const noexcept
  requires(bidir)
  {
    return const_reverse_iterator();
  }

  inline const_iterator cbegin() const noexcept
  {
    return begin();
  }

  inline constexpr iterator cend() const noexcept
  {
    return iterator();
  }

  inline value_type& front() noexcept
  {
    return *data_.head_;
  }

  inline value_type& back() noexcept
  requires(CacheTail)
  {
    return *data_.tail_;
  }

  inline value_type const& front() const noexcept
  {
    return *data_.head_;
  }

  inline value_type const& back() const noexcept
  requires(CacheTail)
  {
    return *data_.tail_;
  }

  inline void push_back(value_type& obj) noexcept
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

  inline void push_front(value_type& obj) noexcept
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
        data_.tail_ = &obj;
    }
    data_.added();
  }

  inline void append_front(intrusive_list&& other) noexcept
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

  inline void append_back(intrusive_list&& other) noexcept
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

  inline void clear() noexcept
  {
    data_.head_ = nullptr;
    if constexpr (CacheTail)
      data_.tail_ = nullptr;
    data_.clear();
  }

  inline void erase_after(iterator it) noexcept
  {
    auto& l = *it;
    erase_after(l);
  }

  inline void erase_after(value_type& l) noexcept
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
          data_.tail_ = &l;
      }
      else if constexpr (is_dlist)
      {
        if (next_next)
          traits::prev(*next_next, &l);
      }

      traits::next(*next, nullptr);
      if constexpr (is_dlist)
        traits::prev(*next, nullptr);
      data_.erased();
    }
  }

  inline void insert_after(iterator it, value_type& obj) noexcept
  {
    insert_after(*it, obj);
  }

  inline void insert_after(value_type& l, value_type& obj) noexcept
  {
    auto next = traits::next(l);
    traits::next(l, &obj);
    if (!next)
    {
      if constexpr (CacheTail)
        data_.tail_ = &obj;
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

  inline void append_after(iterator it, intrusive_list&& other) noexcept
  {
    append_after(*it, std::move(other));
  }

  inline void append_after(value_type& l, intrusive_list&& other) noexcept
  {
    auto next = traits::next(l);
    traits::next(l, &other.front());
    if (!next)
    {
      if constexpr (CacheTail)
        data_.tail_ = &other.back();
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
      data_.added(other.size());
    other.clear();
  }

  inline void insert(iterator it, value_type& obj) noexcept
  requires(is_dlist)
  {
    insert(*it, obj);
  }

  inline void insert(value_type& l, value_type& obj) noexcept
  requires(is_dlist)
  {
    if (&l == data_.head_)
      push_front(obj);
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

  inline void append(iterator it, intrusive_list&& other) noexcept
  requires(is_dlist && CacheTail)

  {
    append(*it, std::move(other));
  }

  inline void append(value_type& l, intrusive_list&& other) noexcept
  requires(is_dlist && CacheTail)
  {
    if (&l == data_.head_)
      append_front(std::move(other));
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
        data_.added(other.size());
      other.clear();
    }
  }

  inline void erase(iterator it) noexcept
  requires(is_dlist)
  {
    erase(*it);
  }

  inline void erase(value_type& l) noexcept
  requires(is_dlist)
  {
    auto prev = traits::prev(l);
    auto next = traits::next(l);
    traits::next(l, nullptr);
    traits::prev(l, nullptr);
    if (prev)
      traits::next(*prev, next);
    else
      data_.head_ = next;
    if (next)
      traits::prev(*next, prev);
    else
    {
      if constexpr (CacheTail)
        data_.tail_ = prev;
    }
    data_.erased();
  }

  inline void pop_back() noexcept
  requires(CacheTail && is_dlist)
  {
    auto& l    = *data_.tail_;
    auto  prev = traits::prev(l);
    traits::prev(l, nullptr);
    if (prev)
      traits::next(*prev, nullptr);
    else
      data_.head_ = nullptr;
    data_.tail_ = prev;
    data_.erased();
  }

  inline void pop_front() noexcept
  {
    auto& l    = *data_.head_;
    auto  next = traits::next(l);
    traits::next(l, nullptr);
    if (next)
    {
      if constexpr (is_dlist)
        traits::prev(*next, nullptr);
    }
    else
    {
      if constexpr (CacheTail)
        data_.tail_ = nullptr;
    }
    data_.head_ = next;
    data_.erased();
  }

private:
  list_data_type data_;
};

} // namespace acl
