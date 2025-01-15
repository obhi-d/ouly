#pragma once

#include <acl/utility/common.hpp>

namespace acl::detail
{
//
template <uint32_t Tombstone>
struct tree_node
{
  std::uint32_t parent_ = Tombstone;
  std::uint32_t left_   = Tombstone;
  std::uint32_t right_  = Tombstone;
};

/**
 * @remarks
 * Accessor adapter
 *
 * struct accessor_adapter
 * {
 *   using value_type = int;
 *   struct node_type
 *   {
 *     value_type value;
 *     tree_node  node;
 *   };
 *
 *   using container = acl::vector<value_type>;
 *
 *   node_type const& node(container const&, std::uint32_t);
 *   node_type&       node(container&, std::uint32_t);
 *   tree_node const& links(node_type const&);
 *   tree_node&       links(node_type&);
 *   auto             value(node_type const&);
 *   bool             is_set(node_type const&);
 *   void             set_flag(node_type&);
 *   void             set_flag(node_type&, bool);
 *   bool             unset_flag(node_type&);
 * };
 *
 **/

template <typename Accessor, uint32_t Tombstone = 0>
class rbtree
{

private:
  using value_type = typename Accessor::value_type;
  using node_type  = typename Accessor::node_type;
  using container  = typename Accessor::container;
  using tree_node  = acl::detail::tree_node<Tombstone>;

  static auto is_set(node_type const& node) -> bool
  {
    return Accessor::is_set(node);
  }

  static void set_parent(container& cont, std::uint32_t node, std::uint32_t parent)
  {
    tree_node& lnk_ref = Accessor::links(Accessor::node(cont, node));
    lnk_ref.parent     = parent;
  }

  static void unset_flag(node_type& node_ref)
  {
    Accessor::unset_flag(node_ref);
  }

  static void set_flag(node_type& node_ref)
  {
    Accessor::set_flag(node_ref);
  }

  template <typename Container>
  struct tnode_it
  {
    void set()
      requires(!std::is_const_v<Container>)
    {
      Accessor::set_flag(*ref_);
    }

    void set(bool b)
      requires(!std::is_const_v<Container>)
    {
      Accessor::set_flag(*ref_, b);
    }

    void unset()
      requires(!std::is_const_v<Container>)
    {
      Accessor::unset_flag(*ref_);
    }

    [[nodiscard]] auto is_set() const -> bool
    {
      return Accessor::is_set(*ref_);
    }

    void set_parent(std::uint32_t par)
      requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref_).parent_ = par;
    }

    void set_left(std::uint32_t left)
      requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref_).left_ = left;
    }

    void set_right(std::uint32_t right)
      requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref_).right_ = right;
    }

    auto parent(Container& cont) const -> tnode_it
    {
      return tnode_it(cont, Accessor::links(*ref_).parent_);
    }

    auto left(Container& cont) const -> tnode_it
    {
      return tnode_it(cont, Accessor::links(*ref_).left_);
    }

    auto right(Container& cont) const -> tnode_it
    {
      return tnode_it(cont, Accessor::links(*ref_).right_);
    }

    [[nodiscard]] auto parent() const -> std::uint32_t
    {
      return Accessor::links(*ref_).parent_;
    }

    [[nodiscard]] auto left() const -> std::uint32_t
    {
      return Accessor::links(*ref_).left_;
    }

    [[nodiscard]] auto right() const -> std::uint32_t
    {
      return Accessor::links(*ref_).right_;
    }

    [[nodiscard]] auto index() const -> std::uint32_t
    {
      return node_;
    }

    auto value() const -> value_type const&
    {
      return Accessor::value(*ref_);
    }

    using node_t = std::conditional_t<std::is_const_v<Container>, node_type const, node_type>;

    tnode_it(Container& cont, std::uint32_t inode) : node_(inode), ref_(&Accessor::node(cont, inode)) {}
    tnode_it(std::uint32_t inode, node_t* iref) : node_(inode), ref_(iref) {}
    tnode_it()                                       = delete;
    tnode_it(tnode_it const&)                        = default;
    tnode_it(tnode_it&&) noexcept                    = default;
    auto operator=(tnode_it const&) -> tnode_it&     = default;
    auto operator=(tnode_it&&) noexcept -> tnode_it& = default;
    ~tnode_it() noexcept                             = default;

    auto operator==(tnode_it const& iother) const -> bool
    {
      return node_ == iother.index();
    }

    auto operator==(std::uint32_t iother) const -> bool
    {
      return node_ == iother;
    }
    auto operator!=(tnode_it const& iother) const -> bool
    {
      return node_ != iother.index();
    }
    auto operator!=(std::uint32_t iother) const -> bool
    {
      return node_ != iother;
    }

    explicit operator bool() const
    {
      return node_ != Tombstone;
    }

    std::uint32_t node_ = Tombstone;
    node_t*       ref_  = nullptr;
  };

  using node_it  = tnode_it<container>;
  using cnode_it = tnode_it<container const>;

  auto minimum(container const& cont, cnode_it u) const -> cnode_it
  {
    while (u.left() != Tombstone)
    {
      u = u.left(cont);
    }
    return u;
  }

  auto maximum(container const& cont, cnode_it u) const -> cnode_it
  {
    while (u.right() != Tombstone)
    {
      u = u.right(cont);
    }
    return u;
  }

  auto minimum(container& cont, node_it u) const -> node_it
  {
    while (u.left() != Tombstone)
    {
      u = u.left(cont);
    }
    return u;
  }

  auto maximum(container& cont, node_it u) const -> node_it
  {
    while (u.right() != Tombstone)
    {
      u = u.right(cont);
    }
    return u;
  }

  void left_rotate(container& cont, node_it x)
  {
    node_it y = x.right(cont);
    x.set_right(y.left());
    if (y.left() != Tombstone)
    {
      y.left(cont).set_parent(x.index());
    }
    y.set_parent(x.parent());
    if (x.parent() == Tombstone)
    {
      root_ = y.index();
    }
    else if (x.index() == x.parent(cont).left())
    {
      x.parent(cont).set_left(y.index());
    }
    else
    {
      x.parent(cont).set_right(y.index());
    }
    y.set_left(x.index());
    x.set_parent(y.index());
  }

  void right_rotate(container& cont, node_it x)
  {
    node_it y = x.left(cont);
    x.set_left(y.right());
    if (y.right() != Tombstone)
    {
      y.right(cont).set_parent(x.index());
    }
    y.set_parent(x.parent());
    if (x.parent() == Tombstone)
    {
      root_ = y.index();
    }
    else if (x.index() == x.parent(cont).right())
    {
      x.parent(cont).set_right(y.index());
    }
    else
    {
      x.parent(cont).set_left(y.index());
    }
    y.set_right(x.index());
    x.set_parent(y.index());
  }

  void transplant(container& cont, node_it u, node_it v)
  {
    auto parent = u.parent(cont);
    if (!parent)
    {
      root_ = v.index();
    }
    else if (parent.left() == u.index())
    {
      parent.set_left(v.index());
    }
    else
    {
      parent.set_right(v.index());
    }
    v.set_parent(parent.index());
  }

  void insert_fixup(container& cont, node_it z)
  {
    for (node_it z_p = z.parent(cont); z_p.is_set(); z_p = z.parent(cont))
    {
      node_it z_p_p = z_p.parent(cont);
      if (z_p == z_p_p.left())
      {
        node_it y = z_p_p.right(cont);
        if (y.is_set())
        {
          z_p.unset();
          y.unset();
          z_p_p.set();
          z = z_p_p;
        }
        else
        {
          if (z == z_p.right())
          {
            z = z_p;
            left_rotate(cont, z);
            z_p   = z.parent(cont);
            z_p_p = z_p.parent(cont);
          }
          z_p.unset();
          z_p_p.set();
          right_rotate(cont, z_p_p);
        }
      }
      else
      {
        node_it y = z_p_p.left(cont);
        if (y.is_set())
        {
          z_p.unset();
          y.unset();
          z_p_p.set();
          z = z_p_p;
        }
        else
        {
          if (z == z_p.left())
          {
            z = z_p;
            right_rotate(cont, z);
            z_p   = z.parent(cont);
            z_p_p = z_p.parent(cont);
          }
          z_p.unset();
          z_p_p.set();
          left_rotate(cont, z_p_p);
        }
      }
    }

    unset_flag(Accessor::node(cont, root_));
  }

  // NOLINTNEXTLINE
  void erase_fix(container& cont, node_it x)
  {
    node_it w(cont, Tombstone);
    while (x.index() != root_ && !x.is_set())
    {
      auto x_p = x.parent(cont);
      if (x.index() == x_p.left())
      {
        w = x_p.right(cont);
        if (w.is_set())
        {
          w.unset();
          x_p.set();
          left_rotate(cont, x_p);
          w = x.parent(cont).right(cont);
        }

        if (!w.left(cont).is_set() && !w.right(cont).is_set())
        {
          w.set();
          x = x.parent(cont);
        }
        else
        {
          if (!w.right(cont).is_set())
          {
            w.left(cont).unset();
            w.set();
            right_rotate(cont, w);
            w = x.parent(cont).right(cont);
          }

          x_p = x.parent(cont);
          w.set(x_p.is_set());
          x_p.unset();
          w.right(cont).unset();
          left_rotate(cont, x_p);
          x = node_it(cont, root_);
        }
      }
      else
      {
        w = x_p.left(cont);
        if (w.is_set())
        {
          w.unset();
          x_p.set();
          right_rotate(cont, x_p);
          w = x.parent(cont).left(cont);
        }

        if (!w.right(cont).is_set() && !w.left(cont).is_set())
        {
          w.set();
          x = x.parent(cont);
        }
        else
        {
          if (!w.left(cont).is_set())
          {
            w.right(cont).unset();
            w.set();
            left_rotate(cont, w);
            w = x.parent(cont).left(cont);
          }

          x_p = x.parent(cont);
          w.set(x_p.is_set());
          x_p.unset();
          w.left(cont).unset();
          right_rotate(cont, x_p);
          x = node_it(cont, root_);
        }
      }
    }
    x.unset();
  }

public:
  [[nodiscard]] auto get_root() const -> std::uint32_t
  {
    return root_;
  }

  auto minimum(container const& cont) const -> value_type
  {
    return root_ == Tombstone ? value_type() : minimum(cont, cnode_it(cont, root_)).value();
  }

  auto maximum(container const& cont) const -> value_type
  {
    return root_ == Tombstone ? value_type() : maximum(cont, cnode_it(cont, root_)).value();
  }

  auto find(container const& cont, std::uint32_t iroot, value_type ivalue) const -> std::uint32_t
  {
    std::uint32_t node = iroot;
    while (node != Tombstone)
    {
      auto const& node_ref = Accessor::node(cont, node);
      if (Accessor::value(node_ref) == ivalue)
      {
        break;
      }
      if (Accessor::value(node_ref) <= ivalue)
      {
        node = Accessor::links(node_ref).right_;
      }
      else
      {
        node = Accessor::links(node_ref).left_;
      }
    }
    return node;
  }

  auto next_less(container& cont, std::uint32_t node) -> std::uint32_t
  {
    if (node != Tombstone)
    {
      return Accessor::links(Accessor::node(cont, node)).left_;
    }
    return Tombstone;
  }

  auto next_more(container& cont, std::uint32_t node) -> std::uint32_t
  {
    if (node != Tombstone)
    {
      return Accessor::links(Accessor::node(cont, node)).right_;
    }
    return Tombstone;
  }

  auto lower_bound(container& cont, value_type ivalue) const -> std::uint32_t
  {
    return lower_bound(cont, root_, ivalue);
  }

  auto lower_bound(container& cont, std::uint32_t iroot, value_type ivalue) const -> std::uint32_t
  {
    std::uint32_t node = iroot;
    std::uint32_t lb   = iroot;
    while (node != Tombstone)
    {
      auto const& node_ref = Accessor::node(cont, node);
      if (Accessor::value(node_ref) >= ivalue)
      {
        lb   = node;
        node = Accessor::links(node_ref).left_;
      }
      else
      {
        node = Accessor::links(node_ref).right_;
      }
    }
    return lb;
  }

  auto find(container& cont, value_type ivalue) const -> std::uint32_t
  {
    return find(cont, root_, ivalue);
  }

  void insert_after(container& cont, std::uint32_t n, std::uint32_t iz)
  {
    node_it y(cont, Tombstone);
    node_it x(cont, n);
    node_it z(cont, iz);
    while (x)
    {
      y = x;
      if (z.value() < x.value())
      {
        x = x.left(cont);
      }
      else
      {
        x = x.right(cont);
      }
    }
    z.set_parent(y.index());
    if (!y)
    {
      root_ = z.index();
      return;
    }
    if (z.value() < y.value())
    {
      y.set_left(z.index());
    }
    else
    {
      y.set_right(z.index());
    }
    z.set();
    insert_fixup(cont, z);
#ifdef ACL_VALIDITY_CHECKS
    validate_integrity(cont);
#endif
  }

  void insert_hint(container& cont, std::uint32_t ih, std::uint32_t iz)
  {
    node_it y(cont);
    node_it z(cont, iz);
    node_it x(cont, ih);

    bool left_seen  = z.value() < x.value();
    bool right_seen = !left_seen;

    while (!(left_seen && right_seen))
    {
      auto prev = x;
      x         = x.parent(cont);

      if ((x.index() == Tombstone)) [[unlikely]]
      {
        x = node_it(cont, root_);
        break;
      }

      const bool ascended_left  = (x.left() == prev.index());
      const bool should_go_left = z.value() < x.value();
      left_seen |= ascended_left;
      right_seen |= !ascended_left;

      if (ascended_left && !should_go_left)
      {
        // goes right below cur
        right_seen = true;
        left_seen  = false;
      }
      else if (!ascended_left && should_go_left)
      {
        right_seen = false;
        left_seen  = true;
      }
    }

    while (x)
    {
      y = x;
      if (z.value() < x.value())
      {
        x = x.left(cont);
      }
      else
      {
        x = x.right(cont);
      }
      ACL_PREFETCH_ONETIME(x.ref);
    }
    z.set_parent(y.index());
    if (!y)
    {
      root_ = z.index();
      return;
    }
    if (z.value() < y.value())
    {
      y.set_left(z.index());
    }
    else
    {
      y.set_right(z.index());
    }
    z.set();
    insert_fixup(cont, z);
#ifdef ACL_VALIDITY_CHECKS
    validate_integrity(cont);
#endif
  }

  void insert(container& cont, std::uint32_t iz)
  {
    insert_after(cont, root_, iz);
  }

  void erase(container& cont, std::uint32_t iz)
  {
    assert(iz != Tombstone);

    node_it z(cont, iz);
    node_it y                = z;
    bool    y_original_color = y.is_set();
    node_it x(cont, Tombstone);
    if (z.left() == Tombstone)
    {
      x = z.right(cont);
      transplant(cont, z, x);
    }
    else if (z.right() == Tombstone)
    {
      x = z.left(cont);
      transplant(cont, z, x);
    }
    else
    {
      y                = minimum(cont, z.right(cont));
      y_original_color = y.is_set();
      x                = y.right(cont);
      if (y.parent() == iz)
      {
        x.set_parent(y.index());
      }
      else
      {
        transplant(cont, y, x);
        y.set_right(z.right());
        y.right(cont).set_parent(y.index());
      }

      transplant(cont, z, y);
      y.set_left(z.left());
      y.left(cont).set_parent(y.index());
      y.set(z.is_set());
    }
    if (!y_original_color)
    {
      erase_fix(cont, x);
    }
    z.unset();
    z.set_left(Tombstone);
    z.set_right(Tombstone);
    z.set_parent(Tombstone);
#ifdef ACL_VALIDITY_CHECKS
    validate_integrity(cont);
#endif
  }

  template <typename L>
  void in_order_traversal(container const& blocks, std::uint32_t node, L&& visitor) const
  {
    if (node != Tombstone)
    {
      auto& n = Accessor::node(blocks, node);
      in_order_traversal(blocks, Accessor::links(n).left_, std::forward<L>(visitor));
      visitor(Accessor::node(blocks, node));
      in_order_traversal(blocks, Accessor::links(n).right_, std::forward<L>(visitor));
    }
  }
  template <typename L>
  void in_order_traversal(container& blocks, L&& visitor)
  {
    in_order_traversal(blocks, root_, std::forward<L>(visitor));
  }

  template <typename L>
  void in_order_traversal(container& blocks, std::uint32_t node, L&& visitor)
  {
    if (node != Tombstone)
    {
      auto& n = Accessor::node(blocks, node);
      in_order_traversal(blocks, Accessor::links(n).left_, std::forward<L>(visitor));
      visitor(Accessor::node(blocks, node));
      in_order_traversal(blocks, Accessor::links(n).right_, std::forward<L>(visitor));
    }
  }

  template <typename L>
  void in_order_traversal(container const& blocks, L&& visitor) const
  {
    in_order_traversal(blocks, root_, std::forward<L>(visitor));
  }

  auto node_count(container const& blocks) const -> std::uint32_t
  {
    std::uint32_t cnt = 0;
    in_order_traversal(blocks,
                       [&cnt](node_type const&)
                       {
                         cnt++;
                       });
    return cnt;
  }

  void validate_integrity(container const& blocks) const
  {
    if (root_ == Tombstone)
    {
      return;
    }
    value_type last = minimum(blocks);
    in_order_traversal(blocks,
                       [&last](node_type const& n)
                       {
                         assert(last <= Accessor::value(n));
                         last = Accessor::value(n);
                       });

    validate_parents(blocks, Tombstone, root_);
  }

  void validate_parents(container const& blocks, std::uint32_t parent, std::uint32_t node) const
  {
    if (node == Tombstone)
    {
      return;
    }
    auto& n = Accessor::node(blocks, node);
    assert(Accessor::links(n).parent == parent);
    validate_parents(blocks, node, Accessor::links(n).left_);
    validate_parents(blocks, node, Accessor::links(n).right_);
  }

private:
  std::uint32_t root_ = Tombstone;
};

} // namespace acl::detail
