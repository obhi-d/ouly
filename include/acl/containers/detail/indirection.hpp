#pragma once
#include <acl/allocators/allocator.hpp>
#include <acl/containers/podvector.hpp>
#include <acl/containers/sparse_vector.hpp>
#include <acl/utility/type_traits.hpp>
#include <acl/utility/utils.hpp>
#include <tuple>

namespace acl::detail
{
//============================================================
template <typename Traits>
class vector_indirection
{
protected:
  using size_type = acl::detail::choose_size_t<uint32_t, Traits>;

public:
  auto get(size_type i) noexcept -> size_type&
  {
    return links_[i];
  }

  auto get(size_type i) const noexcept -> size_type
  {
    return links_[i];
  }

  auto get_if(size_type i) const noexcept -> size_type
  {
    return i < links_.size() ? links_[i] : Traits::null_v;
  }

  auto size() const -> size_type
  {
    return static_cast<size_type>(links_.size());
  }

  void push_back(size_type s) noexcept
  {
    links_.push_back(s);
  }

  void pop_back() noexcept
  {
    links_.pop_back();
  }

  auto best_erase(size_type s)
  {
    auto& v = links_[s];
    auto  r = links_.back();
    v       = r;
    links_.pop_back();
    return r;
  }

  auto ensure_at(size_type i) noexcept -> size_type&
  {
    if (i >= links_.size())
    {
      links_.resize(i + 1, Traits::null_v);
    }
    return links_[i];
  }

  void clear()
  {
    links_.clear();
  }

  void shrink_to_fit()
  {
    links_.shrink_to_fit();
  }

  auto contains(size_type i) const -> bool
  {
    return (i < links_.size() && links_[i] != Traits::null_v);
  }

  auto contains_valid(size_type i) const -> bool
  {
    return i < links_.size() && links_[i] != Traits::null_v && acl::detail::is_valid(links_[i]);
  }

private:
  vector<size_type, custom_allocator_t<Traits>> links_;
};

//============================================================
template <typename Traits>
class sparse_indirection
{
protected:
  using size_type = acl::detail::choose_size_t<uint32_t, Traits>;

  struct default_index_pool_size
  {
    static constexpr uint32_t index_pool_size = 4096;
  };

  struct index_traits
  {
    using size_type = uint32_t;
    static constexpr uint32_t pool_size_v =
     std::conditional_t<acl::detail::HasIndexPoolSize<Traits>, Traits, default_index_pool_size>::index_pool_size;
    static constexpr uint32_t null_v            = Traits::null_v;
    static constexpr bool     no_fill_v         = Traits::null_v == 0;
    static constexpr bool     zero_out_memory_v = Traits::null_v == 0;
  };

public:
  auto get(size_type i) noexcept -> size_type&
  {
    return links_[i];
  }

  auto get(size_type i) const noexcept -> size_type
  {
    return links_[i];
  }

  auto size() const -> size_type
  {
    return static_cast<size_type>(links_.size());
  }

  void push_back(size_type i) noexcept
  {
    links_.emplace_back(i);
  }

  void pop_back() noexcept
  {
    links_.pop_back();
  }

  auto ensure_at(size_type i) noexcept -> size_type&
  {
    if (i >= links_.size())
    {
      links_.grow(i + 1);
    }
    return links_[i];
  }

  auto best_erase(size_type s) -> size_type
  {
    auto& v = links_[s];
    auto  r = links_.back();
    v       = r;
    links_.pop_back();
    return r;
  }

  auto contains(size_type i) const -> bool
  {
    return links_.contains(i);
  }

  auto get_if(size_type i) const noexcept -> size_type
  {
    return i < links_.size() ? links_[i] : Traits::null_v;
  }

  auto contains_valid(size_type i) const -> bool
  {
    if (i < links_.size())
    {
      auto v = links_[i];
      return v != Traits::null_v && acl::detail::is_valid(v);
    }
    return false;
  }

  void clear()
  {
    links_.clear();
  }

  void shrink_to_fit()
  {
    links_.shrink_to_fit();
  }

private:
  sparse_vector<size_type, index_traits> links_;
};

//============================================================
template <typename Traits>
class back_indirection
{
protected:
  using size_type  = acl::detail::choose_size_t<uint32_t, Traits>;
  using self_index = typename Traits::self_index;

public:
  template <typename T>
  auto get(T& i) noexcept -> size_type&
  {
    return self_index::get(i);
  }

  template <typename T>
  auto get(T& i) const noexcept -> size_type
  {
    return self_index::get(i);
  }

  template <typename T>
  auto ensure_at(T& i) noexcept -> size_type&
  {
    return self_index::get(i);
  }

  template <typename T>
  constexpr auto contains(T& /*unused*/) const noexcept -> bool
  {
    return true;
  }

  void clear() noexcept {}

  void shrink_to_fit() noexcept {}
};

template <typename Traits>
using indirection_type = std::conditional_t<HasUseSparseIndexAttrib<Traits>, acl::detail::sparse_indirection<Traits>,
                                            acl::detail::vector_indirection<Traits>>;

template <typename Traits>
using self_index_type =
 std::conditional_t<HasSelfIndexValue<Traits>, acl::detail::back_indirection<Traits>, indirection_type<Traits>>;

} // namespace acl::detail
