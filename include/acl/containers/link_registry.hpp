#pragma once

#include <acl/allocators/default_allocator.hpp>
#include <acl/utils/link.hpp>
#include <algorithm>

namespace acl
{ // namespace detail

/**
 * @brief This class stores a list of reusable links, and allows for vector allocating objects
 *        on a seperate container based on the links.
 * @tparam Ty
 * @tparam SizeType
 */
template <typename Ty = void, typename SizeType = uint32_t>
class basic_link_registry
{
public:
  using size_type = SizeType;
  using link      = acl::link<Ty, size_type>;

  inline link emplace()
  {
    if (free_.empty())
      return link(max_size_++);

    auto ret = free_.back();
    free_.pop_back();
    return ret.revise();
  }

  inline void erase(link l)
  {
    free_.emplace_back(l);
  }

  template <typename Lambda>
  inline void for_each(Lambda&& l)
  {
    std::ranges::sort(free_);
    for (uint32_t i = 0, fi = 0; i < max_size_; ++i)
    {
      if (fi < free_.size() && free_[fi].as_index() == i)
        fi++;
      else
        l(i);
    }
  }

  template <typename Lambda>
  inline void for_each(Lambda&& l) const
  {
    auto copy = free_;
    std::ranges::sort(copy);
    for (uint32_t i = 1, fi = 0; i < max_size_; ++i)
    {
      if (fi < copy.size() && copy[fi].as_index() == i)
        fi++;
      else
        l(i);
    }
  }

  inline uint32_t max_size() const
  {
    return max_size_;
  }

private:
  vector<link> free_;
  uint32_t     max_size_ = 1;
};

template <typename Ty = void>
using link_registry = basic_link_registry<Ty>;

/**
 * @brief This class stores a list of reusable links, and allows for vector allocating objects on a seperate container
 * based on the links. This class also stores link revisions that are updated when links are deleted, and allows for
 * verification if links are still alive.
 * @tparam Ty
 * @tparam SizeType
 */
template <typename Ty = void, typename SizeType = uint32_t>
class basic_rlink_registry
{
public:
  using size_type = SizeType;
  using link      = acl::rlink<Ty, size_type, 8>;

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
    return ret;
  }

  inline void erase(link l)
  {
    l = l.revise();
    revisions_[l.as_index()]++;
    free_.emplace_back(l);
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
    for_each_(std::forward<Lambda>(l), free_, max_size_);
  }

  template <typename Lambda>
  inline void for_each_index(Lambda&& l) const
  {
    auto copy = free_;
    for_each_(std::forward<Lambda>(l), copy, max_size_);
  }

  inline uint32_t max_size() const
  {
    return max_size_;
  }

private:
  template <typename Lambda>
  static inline void for_each_(Lambda&& l, vector<link>& copy, uint32_t max_size) 
  {
    std::ranges::sort(copy);
    for (uint32_t i = 1, fi = 0; i < max_size; ++i)
    {
      if (fi < copy.size() && copy[fi].as_index() == i)
        fi++;
      else
        l(i);
    }
  }

  vector<link>    free_;
  vector<uint8_t> revisions_;
  uint32_t        max_size_ = 1;
};

template <typename Ty = void>
using rlink_registry = basic_rlink_registry<Ty>;

} // namespace acl
