
#pragma once
#include "link_registry.hpp"
#include "sparse_vector.hpp"
#include <memory>

namespace acl
{
template <typename Ty>
struct default_link_container_traits
{
  using size_type                      = uint32_t;
  static constexpr bool     use_sparse = true;
  static constexpr uint32_t pool_size  = 1024;
};

/// @brief Container for link_registry items. Item type must be standard layout, pod like objects.
/// @tparam Ty
/// @tparam Traits
template <typename Ty, typename Traits>
class basic_link_container
{
public:
  using allocator_type = detail::custom_allocator_t<Traits>;
  using traits         = Traits;

private:
  using storage_type = acl::detail::aligned_storage<sizeof(Ty), alignof(Ty)>;
  using vector_type  = std::conditional_t<detail::HasUseSparseAttrib<Traits>, sparse_vector<storage_type, Traits>,
                                         vector<storage_type, allocator_type>>;
  using size_type    = detail::choose_size_t<uint32_t, Traits>;

public:
  using registry = basic_link_registry<Ty, size_type>;
  using link     = typename registry::link;

  vector_type& data()
  {
    return items_;
  }

  vector_type const& data() const
  {
    return items_;
  }

  template <typename ITy>
  requires std::same_as<ITy, Ty> || std::same_as<ITy, void> void sync(basic_link_registry<ITy, size_type> const& imax)
  {
    items_.resize(imax.max_size());
#ifdef ACL_DEBUG
    revisions_.resize(imax.max_size(), 0);
#endif
  }

  void resize(size_type imax)
  {
    items_.resize(imax);
#ifdef ACL_DEBUG
    revisions_.resize(imax, 0);
#endif
  }

  template <typename ITy, typename... Args>
  requires std::same_as<ITy, Ty> || std::same_as<ITy, void> auto& emplace(acl::link<ITy, size_type> l, Args&&... args)
  {
    Ty* obj = (Ty*)&items_[l.as_index()];
    std::construct_at<Ty>(obj, std::forward<Args>(args)...);
    return *obj;
  }

  template <typename ITy>
  requires std::same_as<ITy, Ty> || std::same_as<ITy, void> void erase(acl::link<ITy, size_type> l)
  {
    if constexpr (!std::is_trivially_destructible_v<Ty>)
      std::destroy_at((Ty*)&items_[l.as_index()]);
#ifdef ACL_DEBUG
    revisions_[l.as_index()]++;
#endif
  }

  template <typename ITy>
  requires std::same_as<ITy, Ty> || std::same_as<ITy, void> Ty& at(acl::link<ITy, size_type> l)
  {
#ifdef ACL_DEBUG
    ACL_ASSERT(revisions_[l.as_index()] == l.revision());
#endif
    return *(Ty*)&items_[l.as_index()];
  }

  template <typename ITy>
  requires std::same_as<ITy, Ty> || std::same_as<ITy, void> Ty const& at(acl::link<ITy, size_type> l) const
  {
#ifdef ACL_DEBUG
    ACL_ASSERT(revisions_[l.as_index()] == l.revision());
#endif
    return *(Ty const*)&items_[l.as_index()];
  }

private:
  vector_type items_;
#ifdef ACL_DEBUG
  vector<uint8_t> revisions_;
#endif
};

template <typename Ty, typename Traits = default_link_container_traits<Ty>>
using link_container = basic_link_container<Ty, Traits>;

} // namespace acl