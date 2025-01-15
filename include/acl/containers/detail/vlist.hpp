#pragma once
#include <acl/utility/common.hpp>
#include <functional>

namespace acl::detail
{

struct list_node
{
  std::uint32_t next_ = 0;
  std::uint32_t prev_ = 0;
};

template <typename Accessor>
class vlist
{
public:
  using container = typename Accessor::container;

  std::uint32_t first_ = 0;
  std::uint32_t last_  = 0;

  template <typename ContainerTy>
  struct iterator_t
  {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = typename Accessor::value_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type*;
    using reference         = value_type&;

    iterator_t(const iterator_t& i_other) : owner_(i_other.owner_), index_(i_other.index_) {}
    iterator_t(iterator_t&& i_other) noexcept : owner_(i_other.owner_), index_(i_other.index_)
    {
      i_other.index_ = 0;
    }

    explicit iterator_t(ContainerTy& i_owner) : owner_(i_owner) {}
    iterator_t(ContainerTy& i_owner, std::uint32_t start) : owner_(i_owner), index_(start) {}
    ~iterator_t() noexcept = default;

    auto operator=(iterator_t&& i_other) noexcept -> iterator_t&
    {
      index_         = i_other.index_;
      i_other.index_ = 0;
      return *this;
    }

    auto operator=(const iterator_t& i_other) -> iterator_t&
    {
      index_ = i_other.index_;
      return *this;
    }

    auto operator==(const iterator_t& i_other) const -> bool
    {
      return static_cast<int>(index_ == i_other.index_) != 0;
    }

    auto operator!=(const iterator_t& i_other) const -> bool
    {
      return static_cast<int>(index_ != i_other.index_) != 0;
    }

    auto operator++() -> iterator_t&
    {
      index_ = Accessor::node(owner_.get(), index_).next_;
      return *this;
    }

    auto operator++(int) -> iterator_t
    {
      iterator_t ret(*this);
      index_ = Accessor::node(owner_.get(), index_).next_;
      return ret;
    }

    auto operator--() -> iterator_t&
    {
      index_ = Accessor::node(owner_.get(), index_).prev_;
      return *this;
    }

    auto operator--(int) -> iterator_t
    {
      iterator_t ret(*this);
      index_ = Accessor::node(owner_, index_).prev_;
      return ret;
    }

    auto operator*() const
    {
      return Accessor::get(owner_.get(), index_);
    }

    auto operator->() const -> const value_type*
    {
      return &Accessor::get(owner_.get(), index_);
    }

    auto operator*() -> value_type& requires(!std::is_const_v<ContainerTy>) { return Accessor::get(owner_, index_); }

    auto operator->() -> value_type* requires(!std::is_const_v<ContainerTy>) { return &Accessor::get(owner_, index_); }

    [[nodiscard]] auto prev() const -> std::uint32_t
    {
      // assert(index < owner.size());
      return Accessor::node(owner_.get(), index_).prev_;
    }

    [[nodiscard]] auto next() const -> std::uint32_t
    {
      // assert(index < owner.size());
      return Accessor::node(owner_.get(), index_).next_;
    }

    [[nodiscard]] auto value() const -> std::uint32_t
    {
      return index_;
    }

    explicit operator bool() const
    {
      return index_ != 0;
    }

    std::reference_wrapper<ContainerTy> owner_;
    std::uint32_t                       index_ = 0;
  };

  using iterator       = iterator_t<container>;
  using const_iterator = iterator_t<container const>;

  [[nodiscard]] auto begin() const -> std::uint32_t
  {
    return first_;
  }

  [[nodiscard]] constexpr auto end() const -> std::uint32_t
  {
    return 0;
  }

  [[nodiscard]] auto begin(container const& cont) const -> const_iterator
  {
    return const_iterator(cont, first_);
  }

  [[nodiscard]] auto end(container const& cont) const -> const_iterator
  {
    return const_iterator(cont);
  }

  auto begin(container& cont) -> iterator
  {
    return iterator(cont, first_);
  }

  auto end(container& cont) -> iterator
  {
    return iterator(cont);
  }

  [[nodiscard]] auto front() const -> std::uint32_t
  {
    return first_;
  }

  [[nodiscard]] auto back() const -> std::uint32_t
  {
    return last_;
  }

  [[nodiscard]] auto next(container const& cont, std::uint32_t node) const -> std::uint32_t
  {
    return Accessor::node(cont, node).next_;
  }

  void push_back(container& cont, std::uint32_t node)
  {
    if (last_ != 0)
    {
      Accessor::node(cont, last_).next_ = node;
    }
    if (first_ == 0)
    {
      first_ = node;
    }
    Accessor::node(cont, node).prev_ = last_;
    last_                            = node;
  }

  void insert_after(container& cont, std::uint32_t loc, std::uint32_t node)
  {
    assert(loc != 0);
    auto& l_node = Accessor::node(cont, node);
    auto& l_loc  = Accessor::node(cont, loc);

    if (l_loc.next_ != 0)
    {
      Accessor::node(cont, l_loc.next_).prev_ = node;
      l_node.next_                            = l_loc.next_;
    }
    else
    {
      last_ = node;
      assert(l_node.next_ == 0);
    }
    l_node.prev_ = loc;
    l_loc.next_  = node;
  }

  void insert(container& cont, std::uint32_t loc, std::uint32_t node)
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

      if (l_loc.prev_ != 0)
      {
        Accessor::node(cont, l_loc.prev_).next_ = node;
        l_node.prev_                            = l_loc.prev_;
      }
      else
      {
        first_ = node;
        assert(l_node.prev_ == 0);
      }

      l_loc.prev_  = node;
      l_node.next_ = loc;
    }
  }

  // retunrs the next
  auto unlink(container& cont, std::uint32_t node) -> std::uint32_t
  {
    auto&         l_node = Accessor::node(cont, node);
    std::uint32_t next   = l_node.next_;

    if (l_node.prev_ != 0)
    {
      Accessor::node(cont, l_node.prev_).next_ = l_node.next_;
    }
    else
    {
      first_ = l_node.next_;
    }

    if (next != 0)
    {
      Accessor::node(cont, next).prev_ = l_node.prev_;
    }
    else
    {
      last_ = l_node.prev_;
    }

    l_node.prev_ = 0;
    l_node.next_ = 0;
    return next;
  }

  // special method to unlink two consequetive nodes
  // assumes current node's next is valid
  auto unlink2(container& cont, std::uint32_t node) -> std::uint32_t
  {
    auto&         l_node = Accessor::node(cont, node);
    auto&         l_next = Accessor::node(cont, l_node.next_);
    std::uint32_t next   = l_next.next_;

    if (l_node.prev_ != 0)
    {
      Accessor::node(cont, l_node.prev_).next_ = l_next.next_;
    }
    else
    {
      first_ = l_next.next_;
    }

    if (l_next.next_ != 0)
    {
      Accessor::node(cont, l_next.next_).prev_ = l_node.prev_;
    }
    else
    {
      last_ = l_node.prev_;
    }

    l_next.next_ = 0;
    l_next.prev_ = 0;
    l_node.prev_ = 0;
    l_node.next_ = 0;
    return next;
  }

  auto erase(iterator node) -> iterator
  {
    auto r = unlink(node.owner_, node.index_);
    Accessor::erase(node.owner_, node.index_);
    return iterator(node.owner_, r);
  }

  auto erase(container& cont, std::uint32_t node) -> std::uint32_t
  {
    auto r = unlink(cont, node);
    Accessor::erase(cont, node);
    return r;
  }

  auto erase2(container& cont, std::uint32_t node) -> std::uint32_t
  {
    auto next = Accessor::node(cont, node).next_;
    auto r    = unlink2(cont, node);
    Accessor::erase(cont, node);
    Accessor::erase(cont, next);
    return r;
  }

  void clear(container& cont)
  {
    std::uint32_t node = first_;
    while (node != 0)
    {
      auto l_next = Accessor::node(cont, node).next_;
      Accessor::erase(cont, node);
      node = l_next;
    }
    first_ = last_ = 0;
  }
};

} // namespace acl::detail