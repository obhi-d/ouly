
#pragma once

#include <acl/allocators/default_allocator.hpp>
#include <acl/utils/rlink.hpp>
#include <algorithm>

namespace acl
{

/**
 * @brief This class stores a list of reusable links, and allows for vector allocating objects on a seperate container
 * based on the links. This class also stores link revisions that are updated when links are deleted, and allows for
 * verification if links are still alive.
 * @tparam Ty
 * @tparam SizeType
 */
template <typename Ty = void, typename SizeType = uint32_t, SizeType RevisionBytes = 1>
class basic_rlink_registry
{
public:
  static_assert(RevisionBytes < sizeof(SizeType), "Revision bits must be less than the SizeType");

  using size_type = SizeType;

  using revision_type =
    std::conditional_t<RevisionBytes == 4, uint32_t, std::conditional_t<RevisionBytes == 2, uint16_t, uint8_t>>;

  using link = acl::rlink<Ty, size_type, sizeof(revision_type) * 8>;

  inline link emplace()
  {
    if (free_.empty())
    {
      auto offset = max_size_++;
      if (max_size_ > revisions_.size())
        revisions_.resize(max_size_, 0);
      return link(offset);
    }

    auto ret = free_.back();
    free_.pop_back();
    sorted = false;
    return link(ret, revisions_[ret]);
  }

  inline void erase(link l)
  {
    l = l.revise();
    revisions_[l.as_index()]++;
    free_.emplace_back(l.as_index());
    sorted = false;
  }

  inline bool is_valid(link l) const noexcept
  {
    return l.revision() == revisions_[l.as_index()];
  }

  inline uint8_t get_revision(link l) const noexcept
  {
    return revisions_[l.as_index()];
  }

  inline uint8_t get_revision(size_type l) const noexcept
  {
    return revisions_[l];
  }

  template <typename Lambda>
  inline void for_each_index(Lambda&& l)
  {
    if (!sorted)
    {
      sort_free();
    }
    for_each_(std::forward<Lambda>(l), free_, max_size_);
  }

  template <typename Lambda>
  inline void for_each_index(Lambda&& l) const
  {
    if (sorted)
      for_each_(std::forward<Lambda>(l), free_, max_size_);
    else
    {
      auto copy = free_;
      std::ranges::sort(copy);
      for_each_(std::forward<Lambda>(l), copy, max_size_);
    }
  }

  inline uint32_t max_size() const
  {
    return max_size_;
  }

  void sort_free()
  {
    std::ranges::sort(free_);
    sorted = true;
  }

private:
  template <typename Lambda>
  static inline void for_each_(Lambda&& l, auto const& copy, uint32_t max_size)
  {
    for (uint32_t i = 1, fi = 0; i < max_size; ++i)
    {
      if (fi < copy.size() && copy[fi] == i)
        fi++;
      else
        l(i);
    }
  }

  vector<size_type>     free_;
  vector<revision_type> revisions_;
  uint32_t              max_size_ = 1;
  bool                  sorted    = false;
};

template <typename Ty = void>
using rlink_registry = basic_rlink_registry<Ty>;

} // namespace acl
