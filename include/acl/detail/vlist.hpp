#pragma once
#include "common.hpp"

namespace acl::detail
{

struct list_node
{
  std::uint32_t next = 0;
  std::uint32_t prev = 0;
};

template <typename Accessor>
class vlist
{
public:
  using container = typename Accessor::container;

  std::uint32_t first = 0;
  std::uint32_t last  = 0;

  template <typename ContainerTy>
  struct iterator_t
  {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = typename Accessor::value_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type*;
    using reference         = value_type&;

    iterator_t(const iterator_t& i_other) : owner(i_other.owner), index(i_other.index) {}
    iterator_t(iterator_t&& i_other) noexcept : owner(i_other.owner), index(i_other.index)
    {
      i_other.index = 0;
    }

    explicit iterator_t(ContainerTy& i_owner) : owner(i_owner), index(0) {}
    iterator_t(ContainerTy& i_owner, std::uint32_t start) : owner(i_owner), index(start) {}

    inline iterator_t& operator=(iterator_t&& i_other) noexcept
    {
      index         = i_other.index;
      i_other.index = 0;
      return *this;
    }

    inline iterator_t& operator=(const iterator_t& i_other)
    {
      index = i_other.index;
      return *this;
    }

    inline bool operator==(const iterator_t& i_other) const
    {
      return (index == i_other.index) != 0;
    }

    inline bool operator!=(const iterator_t& i_other) const
    {
      return (index != i_other.index) != 0;
    }

    inline iterator_t& operator++()
    {
      index = Accessor::node(owner, index).next;
      return *this;
    }

    inline iterator_t operator++(int)
    {
      iterator_t ret(*this);
      index = Accessor::node(owner, index).next;
      return ret;
    }

    inline iterator_t& operator--()
    {
      index = Accessor::node(owner, index).prev;
      return *this;
    }

    inline iterator_t operator--(int)
    {
      iterator_t ret(*this);
      index = Accessor::node(owner, index).prev;
      return ret;
    }

    inline const value_type& operator*() const
    {
      return Accessor::get(owner, index);
    }

    inline const value_type* operator->() const
    {
      return &Accessor::get(owner, index);
    }

    inline value_type& operator*()
      requires(!std::is_const_v<ContainerTy>)
    {
      return Accessor::get(owner, index);
    }

    inline value_type* operator->()
      requires(!std::is_const_v<ContainerTy>)
    {
      return &Accessor::get(owner, index);
    }

    [[nodiscard]] inline std::uint32_t prev() const
    {
      // assert(index < owner.size());
      return Accessor::node(owner, index).prev;
    }

    [[nodiscard]] inline std::uint32_t next() const
    {
      // assert(index < owner.size());
      return Accessor::node(owner, index).next;
    }

    [[nodiscard]] inline std::uint32_t value() const
    {
      return index;
    }

    inline explicit operator bool() const
    {
      return index != 0;
    }

    ContainerTy&  owner;
    std::uint32_t index = 0;
  };

  using iterator       = iterator_t<container>;
  using const_iterator = iterator_t<container const>;

  inline std::uint32_t begin() const
  {
    return first;
  }

  inline constexpr std::uint32_t end() const
  {
    return 0;
  }

  inline const_iterator begin(container const& cont) const
  {
    return const_iterator(cont, first);
  }

  inline const_iterator end(container const& cont) const
  {
    return const_iterator(cont);
  }

  inline iterator begin(container& cont)
  {
    return iterator(cont, first);
  }

  inline iterator end(container& cont)
  {
    return iterator(cont);
  }

  inline std::uint32_t front() const
  {
    return first;
  }

  inline std::uint32_t back() const
  {
    return last;
  }

  inline std::uint32_t next(container const& cont, std::uint32_t node) const
  {
    return Accessor::node(cont, node).next;
  }

  inline void push_back(container& cont, std::uint32_t node)
  {
    if (last != 0)
      Accessor::node(cont, last).next = node;
    if (first == 0)
      first = node;
    Accessor::node(cont, node).prev = last;
    last                            = node;
  }

  inline void insert_after(container& cont, std::uint32_t loc, std::uint32_t node)
  {
    assert(loc != 0);
    auto& l_node = Accessor::node(cont, node);
    auto& l_loc  = Accessor::node(cont, loc);

    if (l_loc.next != 0)
    {
      Accessor::node(cont, l_loc.next).prev = node;
      l_node.next                           = l_loc.next;
    }
    else
    {
      last = node;
      assert(l_node.next == 0);
    }
    l_node.prev = loc;
    l_loc.next  = node;
  }

  inline void insert(container& cont, std::uint32_t loc, std::uint32_t node)
  {
    // end?
    if (loc == 0)
    {
      push_back(cont, node);
    }
    else
    {
      auto& l_node = Accessor::node(cont, node);
      auto& l_loc  = Accessor::node(cont, loc);

      if (l_loc.prev != 0)
      {
        Accessor::node(cont, l_loc.prev).next = node;
        l_node.prev                           = l_loc.prev;
      }
      else
      {
        first = node;
        assert(l_node.prev == 0);
      }

      l_loc.prev  = node;
      l_node.next = loc;
    }
  }

  // retunrs the next
  inline std::uint32_t unlink(container& cont, std::uint32_t node)
  {
    auto&         l_node = Accessor::node(cont, node);
    std::uint32_t next   = l_node.next;

    if (l_node.prev != 0)
      Accessor::node(cont, l_node.prev).next = l_node.next;
    else
      first = l_node.next;

    if (next != 0)
      Accessor::node(cont, next).prev = l_node.prev;
    else
      last = l_node.prev;

    l_node.prev = 0;
    l_node.next = 0;
    return next;
  }

  // special method to unlink two consequetive nodes
  // assumes current node's next is valid
  inline std::uint32_t unlink2(container& cont, std::uint32_t node)
  {
    auto&         l_node = Accessor::node(cont, node);
    auto&         l_next = Accessor::node(cont, l_node.next);
    std::uint32_t next   = l_next.next;

    if (l_node.prev != 0)
      Accessor::node(cont, l_node.prev).next = l_next.next;
    else
      first = l_next.next;

    if (l_next.next != 0)
      Accessor::node(cont, l_next.next).prev = l_node.prev;
    else
      last = l_node.prev;

    l_next.next = 0;
    l_next.prev = 0;
    l_node.prev = 0;
    l_node.next = 0;
    return next;
  }

  inline iterator erase(iterator node)
  {
    auto r = unlink(node.owner, node.index);
    Accessor::erase(node.owner, node.index);
    return iterator(node.owner, r);
  }

  inline std::uint32_t erase(container& cont, std::uint32_t node)
  {
    auto r = unlink(cont, node);
    Accessor::erase(cont, node);
    return r;
  }

  inline std::uint32_t erase2(container& cont, std::uint32_t node)
  {
    auto next = Accessor::node(cont, node).next;
    auto r    = unlink2(cont, node);
    Accessor::erase(cont, node);
    Accessor::erase(cont, next);
    return r;
  }

  inline void clear(container& cont)
  {
    std::uint32_t node = first;
    while (node != 0)
    {
      auto l_next = Accessor::node(cont, node).next;
      Accessor::erase(cont, node);
      node = l_next;
    }
    first = last = 0;
  }
};

} // namespace acl::detail