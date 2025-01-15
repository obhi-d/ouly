/*
 * podvector.hpp
 *
 *  Created on: 11/2/2018, 10:48:02 AM
 *      Author: obhi
 */

#pragma once
#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <acl/allocators/detail/custom_allocator.hpp>
#include <acl/utility/type_traits.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
// refer to:
// https://en.cppreference.com/w/cpp/header/vector
namespace acl
{
template <typename Ty, typename Config = acl::default_config<Ty>>
class podvector : public acl::detail::custom_allocator_t<Config>
{
public:
  using value_type                = Ty;
  using allocator_type            = acl::detail::custom_allocator_t<Config>;
  using size_type                 = acl::detail::choose_size_t<uint32_t, Config>;
  using difference_type           = size_type;
  using reference                 = value_type&;
  using const_reference           = const value_type&;
  using pointer                   = Ty*;
  using const_pointer             = Ty const*;
  using iterator                  = Ty*;
  using const_iterator            = Ty const*;
  using reverse_iterator          = std::reverse_iterator<iterator>;
  using const_reverse_iterator    = std::reverse_iterator<const_iterator>;
  using allocator                 = allocator_type;
  using allocator_tag             = typename allocator_type::tag;
  using allocator_is_always_equal = typename acl::allocator_traits<allocator_tag>::is_always_equal;
  using propagate_allocator_on_move =
   typename acl::allocator_traits<allocator_tag>::propagate_on_container_move_assignment;
  using propagate_allocator_on_copy =
   typename acl::allocator_traits<allocator_tag>::propagate_on_container_copy_assignment;
  using propagate_allocator_on_swap = typename acl::allocator_traits<allocator_tag>::propagate_on_container_swap;

  explicit podvector(const allocator_type& alloc = allocator_type()) noexcept
      : allocator_type(alloc), data_(nullptr), size_(0), capacity_(0) {};

  explicit podvector(size_type n) noexcept : data_(allocate(n)), size_(n), capacity_(n) {}

  podvector(size_type n, const Ty& value, const allocator_type& alloc = allocator_type())
      : allocator_type(alloc), data_(allocate(n)), size_(n), capacity_(n)
  {
    std::uninitialized_fill_n(data_, n, value);
  }

  template <class InputIterator>
  podvector(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type()) noexcept
      : allocator_type(alloc)
  {
    construct_from_range(first, last, std::is_integral<InputIterator>());
  }

  podvector(const podvector& x) noexcept

      : data_(allocate(x.capacity_)), size_(x.size_), capacity_(x.capacity_)
  {
    copy(std::begin(x), std::end(x), data_);
  }
  podvector(podvector&& x) noexcept : size_(x.size_), capacity_(x.capacity_), data_(x.data_)
  {
    std::memset(&x, 0, sizeof(x));
  };
  podvector(const podvector& x, const allocator_type& alloc) noexcept
      : allocator_type(alloc), data_(allocate(x.capacity_)), size_(x.size_), capacity_(x.capacity_)
  {
    copy(std::begin(x), std::end(x), data_);
  }
  podvector(podvector&& x, const allocator_type& alloc) noexcept
      : allocator_type(alloc), data_(std::move(x.data_)), size_(x.size_), capacity_(x.capacity_)
  {
    std::memset(&x, 0, sizeof(x));
  };
  podvector(std::initializer_list<Ty> x, const allocator_type& alloc = allocator_type()) noexcept
      : allocator_type(alloc), data_(allocate(static_cast<size_type>(x.size()))),
        size_(static_cast<size_type>(x.size())), capacity_(size_)
  {

    copy(std::begin(x), std::end(x), data_);
  }

  ~podvector() noexcept
  {
    deallocate();
  }
  auto operator=(const podvector& x) noexcept -> podvector&
  {
    assign_copy(x, propagate_allocator_on_copy());
    return *this;
  }
  auto operator=(podvector&& x) noexcept -> podvector&
  {
    assign_move(std::move(x), propagate_allocator_on_move());
    return *this;
  }
  auto operator=(std::initializer_list<Ty> x) noexcept -> podvector&
  {
    if (capacity_ < x.size())
    {
      capacity_ = static_cast<size_type>(x.size());
      deallocate();
      data_ = allocate(static_cast<size_type>(x.size()));
    }
    size_ = static_cast<size_type>(x.size());
    copy(std::begin(x), std::end(x), data_);
    return *this;
  }
  template <class InputIterator>
  void assign(InputIterator first, InputIterator last) noexcept
  {
    auto s = static_cast<size_type>(std::distance(first, last));
    if (capacity_ < s)
    {
      capacity_ = s;
      deallocate();
      data_ = allocate(s);
    }
    size_ = s;
    copy(first, last, data_);
  }
  void assign(size_type n, const Ty& value) noexcept
  {
    if (capacity_ < n)
    {
      capacity_ = n;
      deallocate();
      data_ = allocate(n);
    }
    size_ = n;
    std::uninitialized_fill_n(data_, n, value);
  }
  void assign(std::initializer_list<Ty> x) noexcept
  {
    if (capacity_ < x.size())
    {
      deallocate();
      capacity_ = static_cast<size_type>(x.size());
      data_     = allocate(capacity_);
    }
    size_ = static_cast<size_type>(x.size());
    copy(std::begin(x), std::end(x), data_);
  }
  auto get_allocator() const noexcept -> allocator_type
  {
    return *this;
  }

  // iterators:
  auto begin() noexcept -> iterator
  {
    return data_;
  }
  auto begin() const noexcept -> const_iterator
  {
    return data_;
  }
  auto end() noexcept -> iterator
  {
    return data_ + size_;
  }
  auto end() const noexcept -> const_iterator
  {
    return data_ + size_;
  }

  auto rbegin() noexcept -> reverse_iterator
  {
    return reverse_iterator(end());
  }
  auto rbegin() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(end());
  }
  auto rend() noexcept -> reverse_iterator
  {
    return reverse_iterator(begin());
  }
  auto rend() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(begin());
  }

  auto cbegin() const noexcept -> const_iterator
  {
    return begin();
  }
  auto cend() const noexcept -> const_iterator
  {
    return end();
  }
  auto crbegin() const noexcept -> const_reverse_iterator
  {
    return rbegin();
  }
  auto crend() const noexcept -> const_reverse_iterator
  {
    return rend();
  }

  // capacity:
  auto size() const noexcept -> size_type
  {
    return size_;
  }
  auto max_size() const noexcept -> size_type
  {
    return allocator_type::max_size();
  }
  void resize(size_type sz) noexcept
  {
    reserve(sz);
    size_ = sz;
  }
  void resize(size_type sz, const Ty& c) noexcept
  {
    reserve(sz);
    if (sz > size_)
    {
      std::uninitialized_fill_n(data_ + size_, sz - size_, c);
    }
    size_ = sz;
  }
  auto capacity() const noexcept -> size_type
  {
    return capacity_;
  }
  [[nodiscard]] auto empty() const noexcept -> bool
  {
    return size_ == 0;
  }
  void reserve(size_type n) noexcept
  {
    if (capacity_ < n)
    {
      unchecked_reserve(capacity_ + (capacity_ >> 1) + n);
    }
  }
  void shrink_to_fit() noexcept
  {
    if (capacity_ != size_)
    {
      unchecked_reserve(size_);
    }
  }

  // element access:
  auto operator[](size_type n) noexcept -> reference
  {
    assert(n < size_);
    return data_[n];
  }
  auto operator[](size_type n) const noexcept -> const_reference
  {
    assert(n < size_);
    return data_[n];
  }
  auto at(size_type n) -> reference
  {
    assert(n < size_);
    return data_[n];
  }
  auto at(size_type n) const noexcept -> const_reference
  {
    assert(n < size_);
    return data_[n];
  }
  auto front() -> reference
  {
    assert(0 < size_);
    return data_[0];
  }
  auto front() const noexcept -> const_reference
  {
    assert(0 < size_);
    return data_[0];
  }
  auto back() -> reference
  {
    assert(0 < size_);
    return data_[size_ - 1];
  }
  auto back() const noexcept -> const_reference
  {
    assert(0 < size_);
    return data_[size_ - 1];
  }

  // data access
  auto data() noexcept -> Ty*
  {
    return data_;
  }
  auto data() const noexcept -> const Ty*
  {
    return data_;
  }

  // modifiers:
  template <class... Args>
  auto emplace_back(Args&&... args) noexcept -> Ty&
  {
    if (capacity_ < size_ + 1)
    {
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
    }
    return (data_[size_++] = Ty(std::forward<Args>(args)...));
  }

  void push_back(const Ty& x) noexcept
  {
    if (capacity_ < size_ + 1)
    {
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
    }
    data_[size_++] = x;
  }
  void push_back(Ty&& x) noexcept
  {
    if (capacity_ < size_ + 1)
    {
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
    }
    data_[size_++] = std::move(x);
  }
  void pop_back() noexcept
  {
    assert(size_);
    size_--;
  }

  template <class... Args>
  auto emplace(const_iterator position, Args&&... args) noexcept -> iterator
  {
    size_type p = insert_hole(position);
    data_[p]    = Ty(std::forward<Args>(args)...);
    return data_ + p;
  }
  auto insert(const_iterator position, const Ty& x) noexcept -> iterator
  {
    size_type p = insert_hole(position);
    data_[p]    = x;
    return data_ + p;
  }
  auto insert(const_iterator position, Ty&& x) noexcept -> iterator
  {
    size_type p = insert_hole(position);
    data_[p]    = std::move(x);
    return data_ + p;
  }
  auto insert(const_iterator position, size_type n, const Ty& x) noexcept -> iterator
  {
    return insert_range(position, n, x, std::true_type());
  }
  template <class InputIterator>
  auto insert(const_iterator position, InputIterator first, InputIterator last) noexcept -> iterator
  {
    return insert_range(position, first, last, std::is_integral<InputIterator>());
  }

  auto insert(const_iterator position, std::initializer_list<Ty> x) noexcept -> iterator
  {
    size_type p = insert_hole(position, static_cast<size_type>(x.size()));
    copy(std::begin(x), std::end(x), data_ + p);
    return data_ + p;
  }

  auto erase(const_iterator position) noexcept -> iterator
  {
    assert(position < end());
    // NOLINTNEXTLINE
    std::memmove(const_cast<iterator>(position), position + 1,
                 static_cast<std::size_t>((data_ + size_) - (position + 1)) * sizeof(Ty));
    size_--;
    // NOLINTNEXTLINE
    return const_cast<iterator>(position);
  }
  auto erase(const_iterator first, const_iterator last) noexcept -> iterator
  {
    assert(last < end());
    auto n = static_cast<std::uint32_t>(std::distance(first, last));
    // NOLINTNEXTLINE
    std::memmove(const_cast<iterator>(first), last, static_cast<std::size_t>((data_ + size_) - (last)) * sizeof(Ty));
    size_ -= n;
    // NOLINTNEXTLINE
    return const_cast<iterator>(first);
  }
  void swap(podvector& x) noexcept
  {
    swap(x, propagate_allocator_on_swap());
  }
  void clear() noexcept
  {
    size_ = 0;
  }

private:
  auto insert_hole(const_iterator it, size_type n = 1) noexcept -> size_type
  {
    auto      p   = static_cast<size_type>(std::distance<const Ty*>(data_, it));
    size_type nsz = size_ + n;
    if (capacity_ < nsz)
    {
      unchecked_reserve(size_ + std::max(size_ >> 1, n), p, n);
    }
    else
    {
      // NOLINTNEXTLINE
      auto src = const_cast<iterator>(it);
      std::memmove(src + n, src, (size_ - p) * sizeof(Ty));
    }
    size_ = nsz;
    return p;
  }
  auto assign_copy(const podvector& x, std::false_type /*unused*/) noexcept -> podvector&
  {
    if (capacity_ < x.size_)
    {
      deallocate();
      data_     = allocate(x.size_);
      capacity_ = x.size_;
    }
    size_ = x.size_;
    std::memcpy(data_, x.data_, x.size_ * sizeof(Ty));
    return *this;
  }

  auto assign_copy(const podvector& x, std::true_type /*unused*/) noexcept -> podvector&
  {
    if (allocator_is_always_equal::value ||
        static_cast<const allocator_type&>(x) == static_cast<const allocator_type&>(*this))
    {
      assign(x, std::false_type());
    }
    else
    {
      deallocate();
      allocator_type::operator=(static_cast<const allocator_type&>(x));
      data_     = allocate(x.size_);
      capacity_ = x.size_;
      size_     = x.size_;
      std::memcpy(data_, x.data_, x.size_ * sizeof(Ty));
    }
    return *this;
  }

  auto assign_move(podvector&& x, std::false_type /*unused*/) noexcept -> podvector&
  {
    if (allocator_is_always_equal::value ||
        static_cast<const allocator_type&>(x) == static_cast<const allocator_type&>(*this))
    {
      deallocate();
      data_       = std::move(x.data_);
      size_       = x.size_;
      capacity_   = x.capacity_;
      x.data_     = nullptr;
      x.capacity_ = x.size_ = 0;
    }
    else
    {
      assign_copy(x, std::false_type());
    }
    return *this;
  }

  auto assign_move(podvector&& x, std::true_type /*unused*/) noexcept -> podvector&
  {
    deallocate();
    allocator_type::operator=(std::move(static_cast<allocator_type&>(x)));
    data_       = x.data_;
    size_       = x.size_;
    capacity_   = x.capacity_;
    x.data_     = nullptr;
    x.capacity_ = x.size_ = 0;
    return *this;
  }

  template <typename InputIt>
  void copy(InputIt first, InputIt last, pointer dest) noexcept
  {
    if constexpr (std::is_same_v<pointer, InputIt>)
    {
      std::memcpy(dest, first, static_cast<std::size_t>(std::distance(first, last)) * sizeof(Ty));
    }
    else
    {
      std::copy(first, last, dest);
    }
  }

  auto allocate(size_type n) noexcept -> pointer
  {
    return acl::allocate<Ty>(static_cast<allocator_type&>(*this),
                             static_cast<typename allocator::size_type>(n * sizeof(Ty)));
  }

  void deallocate() noexcept
  {
    acl::deallocate(static_cast<allocator_type&>(*this), data_, capacity_ * sizeof(Ty));
  }

  void unchecked_reserve(size_type n) noexcept
  {
    pointer d = allocate(n);
    if (data_)
    {
      std::memcpy(d, data_, size_ * sizeof(Ty));
      deallocate();
    }
    data_     = d;
    capacity_ = n;
  }

  void unchecked_reserve(size_type n, size_type at, size_type holes) noexcept
  {
    pointer d = allocate(n);
    if (data_)
    {
      std::memcpy(d, data_, at * sizeof(Ty));
      std::memcpy(d + at + holes, data_ + at, (size_ - at) * sizeof(Ty));
      deallocate();
    }
    data_     = d;
    capacity_ = n;
  }

  void swap(podvector& x, std::false_type /*unused*/) noexcept
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
  }

  void swap(podvector& x, std::true_type /*unused*/) noexcept
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
    std::swap<allocator_type>(this, x);
  }

  friend void swap(podvector& lhs, podvector& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  friend auto operator==(const podvector& x, const podvector& y) noexcept -> bool
  {
    return x.size_ == y.size_ && std::memcmp(x.data_, y.data_, x.size_) == 0;
  }

  friend auto operator<(const podvector& x, const podvector& y) noexcept -> bool
  {
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }

  friend auto operator!=(const podvector& x, const podvector& y) noexcept -> bool
  {
    return !(x == y);
  }

  friend auto operator>(const podvector& x, const podvector& y) noexcept -> bool
  {
    return std::lexicographical_compare(y.begin(), y.end(), x.begin(), x.end());
  }

  friend auto operator>=(const podvector& x, const podvector& y) noexcept -> bool
  {
    return !std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }

  friend auto operator<=(const podvector& x, const podvector& y) noexcept -> bool
  {
    return !std::lexicographical_compare(y.begin(), y.end(), x.begin(), x.end());
  }

  template <class InputIterator>
  auto insert_range(const_iterator position, InputIterator first, InputIterator last,
                    std::false_type /*unused*/) noexcept -> iterator
  {
    size_type p = insert_hole(position, static_cast<size_type>(std::distance(first, last)));
    copy(first, last, data_ + p);
    return data_ + p;
  }

  auto insert_range(const_iterator position, size_type n, const Ty& x, std::true_type /*unused*/) noexcept -> iterator
  {
    size_type p = insert_hole(position, n);
    std::uninitialized_fill_n(data_ + p, n, x);
    return data_ + p;
  }

  template <class InputIterator>
  void construct_from_range(InputIterator first, InputIterator last, std::false_type /*unused*/) noexcept
  {
    size_     = static_cast<size_type>(std::distance(first, last));
    data_     = allocate(size_);
    capacity_ = size_;
    copy(first, last, data_);
  }

  void construct_from_range(size_type n, const Ty& value, std::true_type /*unused*/) noexcept
  {
    size_     = n;
    data_     = allocate(size_);
    capacity_ = size_;
    std::uninitialized_fill_n(data_, n, value);
  }

  pointer   data_     = nullptr;
  size_type size_     = 0;
  size_type capacity_ = 0;
};
} // namespace acl
