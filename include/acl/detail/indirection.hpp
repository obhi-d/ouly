#pragma once
#include "acl/allocator.hpp"
#include "acl/link.hpp"
#include "acl/podvector.hpp"
#include "acl/sparse_vector.hpp"
#include "acl/type_traits.hpp"
#include "utils.hpp"
#include <tuple>

namespace acl
{

namespace detail
{
//============================================================
template <typename Allocator, typename Traits>
class vector_indirection
{
protected:
  using size_type = detail::choose_size_t<uint32_t, Traits>;
  using allocator = Allocator;

public:
  inline size_type& get(size_type i) noexcept
  {
    return links_[i];
  }

  inline size_type get(size_type i) const noexcept
  {
    return links_[i];
  }

  inline size_type size() const
  {
    return static_cast<size_type>(links_.size());
  }

  inline void push_back(size_type s) noexcept
  {
    links_.push_back(s);
  }

  inline void pop_back() noexcept
  {
    links_.pop_back();
  }

  inline auto best_erase(size_type s)
  {
    auto& v = links_[s];
    auto  r = links_.back();
    v       = r;
    links_.pop_back();
    return r;
  }

  inline size_type& ensure_at(size_type i) noexcept
  {
    if (i >= links_.size())
      links_.resize(i + 1);
    return links_[i];
  }

  inline void clear()
  {
    links_.clear();
  }

  inline void shrink_to_fit()
  {
    links_.shrink_to_fit();
  }

  inline bool contains(size_type i) const
  {
    return (i < links_.size() && links_[i] != Traits::null_v);
  }
    
  inline bool contains_valid(size_type i) const
  {
    return i < links_.size() && detail::is_valid(links_[i]);
  }

private:
  vector<size_type, Allocator> links_;
};

//============================================================
template <typename Allocator, typename Traits>
class sparse_indirection
{
protected:
  using size_type = detail::choose_size_t<uint32_t, Traits>;
  using allocator = Allocator;

  struct default_index_pool_size
  {
    static constexpr uint32_t index_pool_size = 4096;
  };

  struct index_traits
  {
    using size_type = uint32_t;
    static constexpr uint32_t pool_size =
      std::conditional_t<detail::HasIndexPoolSize<Traits>, Traits, default_index_pool_size>::index_pool_size;
    static constexpr uint32_t null_v      = Traits::null_v;
    static constexpr bool     no_fill = true;
    static constexpr bool     zero_memory = true;
  };

public:
  inline size_type& get(size_type i) noexcept
  {
    return links_[i];
  }

  inline size_type get(size_type i) const noexcept
  {
    return links_[i];
  }
  
  inline size_type size() const
  {
    return static_cast<size_type>(links_.size());
  }

  inline void push_back(size_type i) noexcept
  {
    links_.emplace_back(i);
  }

  inline void pop_back() noexcept
  {
    links_.pop_back();
  }

  inline size_type& ensure_at(size_type i) noexcept
  {
    if (i >= links_.size())
      links_.grow(i + 1);
    return links_[i];
  }

  inline size_type best_erase(size_type s)
  {
    auto& v = links_[s];
    auto  r = links_.back();
    v       = r;
    links_.pop_back();
    return r;
  }

  inline bool contains(size_type i) const
  {
    return links_.contains(i);
  }

  inline bool contains_valid(size_type i) const
  {
    return i < links_.size() && detail::is_valid(links_[i]);
  }

  inline void clear()
  {
    links_.clear();
  }

  inline void shrink_to_fit()
  {
    links_.shrink_to_fit();
  }

private:
  sparse_vector<size_type, Allocator, index_traits> links_;
};

//============================================================
template <typename Allocator, typename Traits>
class back_indirection
{
protected:
  using size_type = detail::choose_size_t<uint32_t, Traits>;
  using allocator = Allocator;
  using backref   = typename Traits::offset;

public:
  template <typename T>
  inline size_type& get(T& i) noexcept
  {
    return backref::get(i);
  }

  template <typename T>
  inline size_type get(T& i) const noexcept
  {
    return backref::get(i);
  }

  template <typename T>
  inline size_type& ensure_at(T& i) noexcept
  {
    return backref::get(i);
  }

  template <typename T>
  constexpr inline bool contains(T&) const noexcept
  {
    return true;
  }

  inline void clear() noexcept
  {
  }

  inline void shrink_to_fit() noexcept
  {
  }
};

template <typename Allocator, typename Traits>
using indirection_type =
  std::conditional_t<HasUseSparseIndexAttrib<Traits>, detail::sparse_indirection<Allocator, Traits>,
                     detail::vector_indirection<Allocator, Traits>>;

template <typename Allocator, typename Traits>
using backref_type = std::conditional_t<HasBackrefValue<Traits>, detail::back_indirection<Allocator, Traits>,
                                        indirection_type<Allocator, Traits>>;

} // namespace detail

} // namespace acl
