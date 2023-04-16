#pragma once

#include "common.hpp"

namespace acl::detail
{
//
template <uint32_t Tombstone>
struct tree_node
{
  std::uint32_t parent = Tombstone;
  std::uint32_t left   = Tombstone;
  std::uint32_t right  = Tombstone;
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

  inline static bool is_set(node_type const& node)
  {
    return Accessor::is_set(node);
  }

  inline static void set_parent(container& cont, std::uint32_t node, std::uint32_t parent)
  {
    tree_node& lnk_ref = Accessor::links(Accessor::node(cont, node));
    lnk_ref.parent     = parent;
  }

  inline static void unset_flag(node_type& node_ref)
  {
    Accessor::unset_flag(node_ref);
  }

  inline static void set_flag(node_type& node_ref)
  {
    Accessor::set_flag(node_ref);
  }

  template <typename Container>
  struct tnode_it
  {
    void set() requires(!std::is_const_v<Container>)
    {
      Accessor::set_flag(*ref);
    }

    void set(bool b) requires(!std::is_const_v<Container>)
    {
      Accessor::set_flag(*ref, b);
    }

    void unset() requires(!std::is_const_v<Container>)
    {
      Accessor::unset_flag(*ref);
    }

    bool is_set() const
    {
      return Accessor::is_set(*ref);
    }

    void set_parent(std::uint32_t par) requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref).parent = par;
    }

    void set_left(std::uint32_t left) requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref).left = left;
    }

    void set_right(std::uint32_t right) requires(!std::is_const_v<Container>)
    {
      Accessor::links(*ref).right = right;
    }

    tnode_it parent(Container& cont) const
    {
      return tnode_it(cont, Accessor::links(*ref).parent);
    }

    tnode_it left(Container& cont) const
    {
      return tnode_it(cont, Accessor::links(*ref).left);
    }

    tnode_it right(Container& cont) const
    {
      return tnode_it(cont, Accessor::links(*ref).right);
    }

    std::uint32_t parent() const
    {
      return Accessor::links(*ref).parent;
    }

    std::uint32_t left() const
    {
      return Accessor::links(*ref).left;
    }

    std::uint32_t right() const
    {
      return Accessor::links(*ref).right;
    }

    std::uint32_t index() const
    {
      return node;
    }

    value_type const& value() const
    {
      return Accessor::value(*ref);
    }

    using node_t = std::conditional_t<std::is_const_v<Container>, node_type const, node_type>;

    tnode_it(Container& cont, std::uint32_t inode) : node(inode), ref(&Accessor::node(cont, inode)) {}
    tnode_it(std::uint32_t inode, node_t* iref) : node(inode), ref(iref) {}
    tnode_it()                           = delete;
    tnode_it(tnode_it const&)            = default;
    tnode_it(tnode_it&&)                 = default;
    tnode_it& operator=(tnode_it const&) = default;
    tnode_it& operator=(tnode_it&&)      = default;

    bool operator==(tnode_it const& iother) const
    {
      return node == iother.index();
    }

    bool operator==(std::uint32_t iother) const
    {
      return node == iother;
    }
    bool operator!=(tnode_it const& iother) const
    {
      return node != iother.index();
    }
    bool operator!=(std::uint32_t iother) const
    {
      return node != iother;
    }

    inline explicit operator bool() const
    {
      return node != Tombstone;
    }

    std::uint32_t node = Tombstone;
    node_t*       ref  = nullptr;
  };

  using node_it  = tnode_it<container>;
  using cnode_it = tnode_it<container const>;

  inline cnode_it minimum(container const& cont, cnode_it u) const
  {
    while (u.left() != Tombstone)
      u = u.left(cont);
    return u;
  }

  inline cnode_it maximum(container const& cont, cnode_it u) const
  {
    while (u.right() != Tombstone)
      u = u.right(cont);
    return u;
  }

  inline node_it minimum(container& cont, node_it u) const
  {
    while (u.left() != Tombstone)
      u = u.left(cont);
    return u;
  }

  inline node_it maximum(container& cont, node_it u) const
  {
    while (u.right() != Tombstone)
      u = u.right(cont);
    return u;
  }

  inline void left_rotate(container& cont, node_it x)
  {
    node_it y = x.right(cont);
    x.set_right(y.left());
    if (y.left() != Tombstone)
      y.left(cont).set_parent(x.index());
    y.set_parent(x.parent());
    if (x.parent() == Tombstone)
      root = y.index();
    else if (x.index() == x.parent(cont).left())
      x.parent(cont).set_left(y.index());
    else
      x.parent(cont).set_right(y.index());
    y.set_left(x.index());
    x.set_parent(y.index());
  }

  inline void right_rotate(container& cont, node_it x)
  {
    node_it y = x.left(cont);
    x.set_left(y.right());
    if (y.right() != Tombstone)
      y.right(cont).set_parent(x.index());
    y.set_parent(x.parent());
    if (x.parent() == Tombstone)
      root = y.index();
    else if (x.index() == x.parent(cont).right())
      x.parent(cont).set_right(y.index());
    else
      x.parent(cont).set_left(y.index());
    y.set_right(x.index());
    x.set_parent(y.index());
  }

  inline void transplant(container& cont, node_it u, node_it v)
  {
    auto parent = u.parent(cont);
    if (!parent)
      root = v.index();
    else if (parent.left() == u.index())
      parent.set_left(v.index());
    else
      parent.set_right(v.index());
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

    unset_flag(Accessor::node(cont, root));
  }

  void erase_fix(container& cont, node_it x)
  {
    node_it w(cont, Tombstone);
    while (x.index() != root && !x.is_set())
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
          x = node_it(cont, root);
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
          x = node_it(cont, root);
        }
      }
    }
    x.unset();
  }

public:
  std::uint32_t get_root() const
  {
    return root;
  }

  value_type minimum(container const& cont) const
  {
    return root == Tombstone ? value_type() : minimum(cont, cnode_it(cont, root)).value();
  }

  value_type maximum(container const& cont) const
  {
    return root == Tombstone ? value_type() : maximum(cont, cnode_it(cont, root)).value();
  }

  std::uint32_t find(container const& cont, std::uint32_t iroot, value_type ivalue) const
  {
    std::uint32_t node = iroot;
    while (node != Tombstone)
    {
      auto const& node_ref = Accessor::node(cont, node);
      if (Accessor::value(node_ref) == ivalue)
        break;
      else if (Accessor::value(node_ref) <= ivalue)
        node = Accessor::links(node_ref).right;
      else
        node = Accessor::links(node_ref).left;
    }
    return node;
  }

  std::uint32_t next_less(container& cont, std::uint32_t node)
  {
    if (node != Tombstone)
      return Accessor::links(Accessor::node(cont, node)).left;
    return Tombstone;
  }

  std::uint32_t next_more(container& cont, std::uint32_t node)
  {
    if (node != Tombstone)
      return Accessor::links(Accessor::node(cont, node)).right;
    return Tombstone;
  }

  std::uint32_t lower_bound(container& cont, value_type ivalue) const
  {
    return lower_bound(cont, root, ivalue);
  }

  std::uint32_t lower_bound(container& cont, std::uint32_t iroot, value_type ivalue) const
  {
    std::uint32_t node = iroot;
    std::uint32_t lb   = iroot;
    while (node != Tombstone)
    {
      auto const& node_ref = Accessor::node(cont, node);
      if (Accessor::value(node_ref) >= ivalue)
      {
        lb   = node;
        node = Accessor::links(node_ref).left;
      }
      else
        node = Accessor::links(node_ref).right;
    }
    return lb;
  }

  std::uint32_t find(container& cont, value_type ivalue) const
  {
    return find(cont, root, ivalue);
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
        x = x.left(cont);
      else
        x = x.right(cont);
      ACL_PREFETCH_ONETIME(x.ref);
    }
    z.set_parent(y.index());
    if (!y)
    {
      root = z.index();
      return;
    }
    else if (z.value() < y.value())
      y.set_left(z.index());
    else
      y.set_right(z.index());
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

      if (ACL_UNLIKELY(x.index() == Tombstone))
      {
        x = node_it(cont, root);
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
        x = x.left(cont);
      else
        x = x.right(cont);
      ACL_PREFETCH_ONETIME(x.ref);
    }
    z.set_parent(y.index());
    if (!y)
    {
      root = z.index();
      return;
    }
    else if (z.value() < y.value())
      y.set_left(z.index());
    else
      y.set_right(z.index());
    z.set();
    insert_fixup(cont, z);
#ifdef ACL_VALIDITY_CHECKS
    validate_integrity(cont);
#endif
  }

  inline void insert(container& cont, std::uint32_t iz)
  {
    insert_after(cont, root, iz);
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
        x.set_parent(y.index());
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
      erase_fix(cont, x);
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
      in_order_traversal(blocks, Accessor::links(n).left, std::forward<L>(visitor));
      visitor(Accessor::node(blocks, node));
      in_order_traversal(blocks, Accessor::links(n).right, std::forward<L>(visitor));
    }
  }
  template <typename L>
  void in_order_traversal(container& blocks, L&& visitor)
  {
    in_order_traversal(blocks, root, std::forward<L>(visitor));
  }

  template <typename L>
  void in_order_traversal(container& blocks, std::uint32_t node, L&& visitor)
  {
    if (node != Tombstone)
    {
      auto& n = Accessor::node(blocks, node);
      in_order_traversal(blocks, Accessor::links(n).left, std::forward<L>(visitor));
      visitor(Accessor::node(blocks, node));
      in_order_traversal(blocks, Accessor::links(n).right, std::forward<L>(visitor));
    }
  }

  template <typename L>
  void in_order_traversal(container const& blocks, L&& visitor) const
  {
    in_order_traversal(blocks, root, std::forward<L>(visitor));
  }

  std::uint32_t node_count(container const& blocks) const
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
    if (root == Tombstone)
      return;
    value_type last = minimum(blocks);
    in_order_traversal(blocks,
                       [&last](node_type const& n)
                       {
                         assert(last <= Accessor::value(n));
                         last = Accessor::value(n);
                       });

    validate_parents(blocks, Tombstone, root);
  }

  void validate_parents(container const& blocks, std::uint32_t parent, std::uint32_t node) const
  {
    if (node == Tombstone)
      return;
    auto& n = Accessor::node(blocks, node);
    assert(Accessor::links(n).parent == parent);
    validate_parents(blocks, node, Accessor::links(n).left);
    validate_parents(blocks, node, Accessor::links(n).right);
  }

private:
  std::uint32_t root = Tombstone;
};

} // namespace acl::detail
