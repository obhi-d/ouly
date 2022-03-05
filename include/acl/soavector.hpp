
#pragma once

#pragma once
#include "detail/utils.hpp"
#include "type_traits.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>

namespace acl
{
template <typename Allocator /*= std::allocator<void*>*/, typename... Args>
class soavector : public Allocator
{

public:
  using tuple_type = std::tuple<Args...>;
  using array_type = detail::tuple_of_ptrs<Args...>;

  template <std::size_t i>
  using value_type      = std::tuple_element_t<i, std::tuple<Args...>>;
  using allocator_type  = Allocator;
  using size_type       = uint32;
  using difference_type = size_type;
  template <std::size_t i>
  using reference = value_type<i>&;
  template <std::size_t i>
  using const_reference = const value_type<i>&;
  template <std::size_t i>
  using pointer = value_type<i>*;
  template <std::size_t i>
  using const_pointer = value_type<i> const*;
  template <std::size_t i>
  using iterator = value_type<i>*;
  template <std::size_t i>
  using const_iterator = value_type<i> const*;
  template <std::size_t i>
  using reverse_iterator = std::reverse_iterator<iterator<i>>;
  template <std::size_t i>
  using const_reverse_iterator      = std::reverse_iterator<const_iterator<i>>;
  using allocator_is_always_equal   = typename std::allocator_traits<allocator_type>::is_always_equal;
  using propagate_allocator_on_move = typename std::allocator_traits<Allocator>::propagate_on_container_move_assignment;
  using propagate_allocator_on_copy = typename std::allocator_traits<Allocator>::propagate_on_container_copy_assignment;
  using propagate_allocator_on_swap = typename std::allocator_traits<Allocator>::propagate_on_container_swap;
  using allocator_traits            = std::allocator_traits<Allocator>;

  static auto constexpr index_seq = std::make_index_sequence<sizeof... Args>{};

private:
  template <std::size_t... I>
  void uninitialized_fill(size_type start, size_type count, tuple_type const& t, std::index_sequence<I...>) noexcept
  {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    (std::uninitialized_fill_n(std::get<I>(data_) + start, count, std::get<I>(t))...);
  }

  template <std::size_t... I>
  void construct_at(size_type i, std::index_sequence<I...>, Args&&... args) noexcept
  {
    (std::construct_at(std::get<I>(data_) + i, std::forward<Args>(args))...);
  }

  template <std::size_t... I>
  void construct_at_tuple(size_type i, std::index_sequence<I...>, tuple_type const& arg) noexcept
  {
    (std::construct_at(std::get<I>(data_) + i, std::get<I>(arg))...);
  }

  template <typename T>
  void construct_list(T* data, size_type sz, T const* src) noexcept
  {
    if constexpr (std::is_trivially_copyable_v<T>)
      std::memcpy(data, src, sz * sizeof(T));
    else
    {
      for (size_type i = 0; i < sz; ++i)
        std::construct_at(data + i, *(src + i));
    }
  }

  template <typename T>
  void construct_single(T& data, T const& src) noexcept
  {
    if constexpr (std::is_trivially_copyable_v<T>)
      std::memcpy(&data, &src, sizeof(T));
    else
    {
      std::construct_at(&data, src);
    }
  }

  template <std::size_t... I>
  void construct_list(soavector const& x, std::index_sequence<I...>) noexcept
  {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here

    (construct_list(std::get<I>(data_), size_, std::get<I>(x.data_))...);
  }

  template <typename InputIterator, std::size_t... I>
  static void copy_construct(InputIterator start, InputIterator end, array_type& store,
                             std::index_sequence<I...>) noexcept
  {
    while (start != end)
    {
      (std::construct_at(std::get<I>(store) + i, std::get<I>(*(start)))...);
      start++;
    }
  }

  template <typename T>
  static void destroy_seq(T* data, size_type size) noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<T>)
      std::for_each(data, data + size,
                    [](T& d)
                    {
                      std::destroy_at(&d);
                    });
  }

  template <typename T>
  static void destroy_single(T* data) noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<T>)
      std::destroy_at(data);
  }

  template <std::size_t... I>
  void destroy_all(size_type start, size_type count, std::index_sequence<I...>) noexcept
  {
    (destroy_seq(std::get<I>(data_) + start, count)...);
  }

  template <std::size_t... I>
  void destroy_at(size_type at, std::index_sequence<I...>) noexcept
  {
    (destroy_single(std::get<I>(data_) + at)...);
  }

  void destroy_and_deallocate() noexcept
  {
    destroy_all(0, size_, index_seq);
    deallocate();
  }

public:
  explicit soavector(Allocator const& alloc = Allocator()) noexcept
      : Allocator(alloc), data_(nullptr), size_(0), capacity_(0){};

  explicit soavector(size_type n) : data_(allocate(n)), size_(n), capacity_(n) noexcept {}

  soavector(size_type n, tuple_type const& value, Allocator const& alloc = Allocator()) noexcept
      : Allocator(alloc), data_(allocate(n)), size_(n), capacity_(n)
  {
    uninitialized_fill(0, n, value, index_seq);
  }

  template <class InputIterator>
  soavector(InputIterator first, InputIterator last, Allocator const& alloc = Allocator()) noexcept : Allocator(alloc)
  {
    construct_from_range(first, last, std::is_integral<InputIterator>(), index_seq);
  }

  soavector(soavector&& x, const Allocator& alloc) noexcept
      : Allocator(alloc), data_(x.data_), size_(x.size_), capacity_(x.capacity_)
  {
    std::memset(&x, 0, sizeof(x));
  };

  soavector(soavector const& x, Allocator const& alloc) noexcept
      : Allocator(alloc), data_(allocate(x.capacity_)), size_(x.size_), capacity_(x.capacity_)
  {
    construct_list(x, index_seq);
  }

  soavector(soavector&& x) noexcept : soavector(std::move(x), (Allocator const&)x){};

  soavector(soavector const& x) noexcept : soavector(x, (Allocator const&)x) {}

  soavector(std::initializer_list<tuple_type> x, const Allocator& alloc = Allocator()) noexcept
      : soavector(std::begin(x), std::end(x), alloc)
  {}

  ~soavector() noexcept
  {
    destroy_and_deallocate();
  }

  soavector& operator=(const soavector& x) noexcept
  {
    return assign_copy(x, propagate_allocator_on_copy(), index_seq);
  }

  soavector& operator=(soavector&& x) noexcept
  {
    return assign_move(std::move(x), propagate_allocator_on_move(), index_seq);
  }

  soavector& operator=(std::initializer_list<tuple_type> x) noexcept
  {
    if (capacity_ < x.size())
    {
      capacity_ = static_cast<size_type>(x.size());
      destroy_and_deallocate();
      data_ = allocate(static_cast<size_type>(x.size()));
    }
    size_ = static_cast<size_type>(x.size());
    copy_construct(std::begin(x), std::end(x), data_, index_seq);
    return *this;
  }

  template <class InputIterator>
  void assign(InputIterator first, InputIterator last) noexcept
  {
    size_type s = static_cast<size_type>(std::distance(first, last));
    if (capacity_ < s)
    {
      capacity_ = static_cast<size_type>(s);
      destroy_and_deallocate();
      data_ = allocate(s);
    }
    size_ = static_cast<size_type>(s);
    copy_construct(first, last, data_, index_seq);
  }

  void assign(size_type n, const tuple_type& value) noexcept
  {
    if (capacity_ < n)
    {
      capacity_ = n;
      destroy_and_deallocate();
      data_ = allocate(n);
    }
    size_ = static_cast<size_type>(n);
    uninitialized_fill(0, n, value, index_seq);
  }

  void assign(std::initializer_list<tuple_type> x) noexcept
  {
    if (capacity_ < x.size())
    {
      capacity_ = static_cast<size_type>(x.size());
      destroy_and_deallocate();
      data_ = allocate(x.size());
    }
    size_ = static_cast<size_type>(x.size());
    copy_construct(std::begin(x), std::end(x), data_, index_seq);
  }

  allocator_type get_allocator() const noexcept
  {
    return *this;
  }

  // iterators:
  template <std::size_t i>
  iterator<i> begin() noexcept
  {
    return std::get<i>(data_);
  }

  template <std::size_t i>
  const_iterator<i> begin() const noexcept
  {
    return std::get<i>(data_);
  }

  template <std::size_t i>
  iterator<i> end() noexcept
  {
    return std::get<i>(data_) + size_;
  }

  template <std::size_t i>
  const_iterator<i> end() const noexcept
  {
    return std::get<i>(data_) + size_;
  }

  template <std::size_t i>
  reverse_iterator<i> rbegin() noexcept
  {
    return reverse_iterator<i>(end<i>());
  }

  template <std::size_t i>
  const_reverse_iterator<i> rbegin() const noexcept
  {
    return const_reverse_iterator<i>(end<i>());
  }

  template <std::size_t i>
  reverse_iterator<i> rend() noexcept
  {
    return reverse_iterator<i>(begin<i>());
  }

  template <std::size_t i>
  const_reverse_iterator<i> rend() const noexcept
  {
    return const_reverse_iterator<i>(begin<i>());
  }

  template <std::size_t i>
  const_iterator<i> cbegin() noexcept
  {
    return begin<i>();
  }

  template <std::size_t i>
  const_iterator<i> cend() noexcept
  {
    return end<i>();
  }

  template <std::size_t i>
  const_reverse_iterator<i> crbegin() const noexcept
  {
    return rbegin<i>();
  }

  template <std::size_t i>
  const_reverse_iterator<i> crend() const noexcept
  {
    return rend<i>();
  }

  // capacity:
  size_type size() const noexcept
  {
    return size_;
  }

  size_type max_size() const noexcept
  {
    return std::numeric_limits<size_type>::max();
  }

  void resize(size_type sz) noexcept
  {
    resize(sz, tuple_type());
  }

  void resize(size_type sz, tuple_type const& c) noexcept
  {
    if (sz > size_)
    {
      reserve(sz);
      uninitialized_fill(size_, sz - size, c, index_seq);
    }
    else
      destroy_all(sz, size_ - sz, index_seq);
    size_ = sz;
  }

  size_type capacity() const noexcept
  {
    return capacity_;
  }

  bool empty() const noexcept
  {
    return size_ != 0;
  }

  void reserve(size_type n) noexcept
  {
    if (capacity_ < n)
    {
      unchecked_reserve(n);
    }
  }

  void shrink_to_fit() noexcept
  {
    if (capacity_ != size_)
    {
      unchecked_reserve(size_);
    }
  }

  template <std::size_t i>
  reference<i> at(size_type n) noexcept
  {
    assert(n < size_);
    return std::get<i>(data_)[n];
  }

  template <std::size_t i>
  const_reference<i> at(size_type n) const noexcept
  {
    assert(n < size_);
    return std::get<i>(data_)[n];
  }

  template <std::size_t i>
  reference<i> front() noexcept
  {
    assert(0 < size_);
    return std::get<i>(data_)[0];
  }

  template <std::size_t i>
  const_reference<i> front() const noexcept
  {
    assert(0 < size_);
    return std::get<i>(data_)[0];
  }

  template <std::size_t i>
  reference<i> back() noexcept
  {
    assert(0 < size_);
    return std::get<i>(data_)[size_ - 1];
  }

  template <std::size_t i>
  const_reference<i> back() const noexcept
  {
    assert(0 < size_);
    return std::get<i>(data_)[size_ - 1];
  }

  // data access
  template <std::size_t i>
  auto* data() noexcept
  {
    return std::get<i>(data_);
  }

  template <std::size_t i>
  const auto* data() const noexcept
  {
    return std::get<i>(data_);
  }

  // modifiers:
  void emplace_back(Args&&... args) noexcept
  {
    if (capacity_ < size_ + 1)
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));

    construct_at(size_++, index_seq, std::forward<Args>(args)...);
  }

  void push_back(const tuple_type& x) noexcept
  {
    if (capacity_ < size_ + 1)
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));

    construct_at(size_++, index_seq, x);
  }

  void pop_back() noexcept
  {
    assert(size_);
    destroy_at(--size_, index_seq);
  }

  template <std::size_t i, class... Args>
  iterator emplace(const_iterator position, Args&&... args) noexcept
  {
    size_type p = insert_hole(position);
    data_[p]    = Ty(std::forward<Args>(args)...);
    return data_ + p;
  }

  template <std::size_t i>
  iterator insert(const_iterator position, const Ty& x)
  {
    size_type p = insert_hole(position);
    data_[p]    = x;
    return data_ + p;
  }

  template <std::size_t i>
  iterator insert(const_iterator position, Ty&& x)
  {
    size_type p = insert_hole(position);
    data_[p]    = std::move(x);
    return data_ + p;
  }

  template <std::size_t i>
  iterator insert(const_iterator position, size_type n, const Ty& x)
  {
    return insert_range(position, n, x, std::true_type());
  }

  template <std::size_t i, class InputIterator>
  iterator insert(const_iterator position, InputIterator first, InputIterator last)
  {
    return insert_range(position, first, last, std::is_integral<InputIterator>());
  }

  iterator insert(const_iterator position, std::initializer_list<Ty> x)
  {
    size_type p = insert_hole(position, static_cast<size_type>(x.size()));
    copy(std::begin(x), std::end(x), data_ + p);
    return data_ + p;
  }

  iterator erase(const_iterator position)
  {
    assert(position < end());
    std::memmove(const_cast<iterator>(position), position + 1,
                 static_cast<std::size_t>((data_ + size_) - (position + 1)) * sizeof(Ty));
    size_--;
    return const_cast<iterator>(position);
  }
  iterator erase(const_iterator first, const_iterator last)
  {
    assert(last < end());
    std::uint32_t n = static_cast<std::uint32_t>(std::distance(first, last));
    std::memmove(const_cast<iterator>(first), last,
                 reinterpret_cast<std::size_t>((data_ + size_) - (last)) * sizeof(Ty));
    size_ -= n;
    return const_cast<iterator>(first);
  }
  void swap(soavector<Ty, Allocator>& x)
  {
    swap(x, propagate_allocator_on_swap());
  }
  void clear() noexcept
  {
    size_ = 0;
  }

private:
  inline size_type insert_hole(const_iterator it, size_type n = 1)
  {
    size_type p   = static_cast<size_type>(std::distance<const Ty*>(data_, it));
    size_type nsz = size_ + n;
    if (capacity_ < nsz)
    {
      unchecked_reserve(size_ + std::max(size_ >> 1, n), p, n);
    }
    else
    {
      pointer src = const_cast<iterator>(it);
      std::memmove(src + n, src, (size_ - p) * sizeof(Ty));
    }
    size_ = nsz;
    return p;
  }
  inline soavector& assign_copy(const soavector& x, std::false_type)
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

  inline soavector& assign_copy(const soavector& x, std::true_type)
  {
    if (allocator_is_always_equal::value || static_cast<const Allocator&>(x) == static_cast<const Allocator&>(*this))
      assign(x, std::false_type());
    else
    {
      deallocate();
      Allocator::operator=(static_cast<const Allocator&>(x));
      data_              = allocate(x.size_);
      capacity_          = x.size_;
      size_              = x.size_;
      std::memcpy(data_, x.data_, x.size_ * sizeof(Ty));
    }
    return *this;
  }

  inline soavector& assign_move(soavector&& x, std::false_type)
  {
    if (allocator_is_always_equal::value || static_cast<const Allocator&>(x) == static_cast<const Allocator&>(*this))
    {
      deallocate();
      data_       = x.data_;
      size_       = x.size_;
      capacity_   = x.capacity_;
      x.data_     = nullptr;
      x.capacity_ = x.size_ = 0;
    }
    else
      assign_copy(x, std::false_type());
    return *this;
  }

  inline soavector& assign_move(soavector&& x, std::true_type)
  {
    deallocate();
    Allocator::operator=(std::move(static_cast<Allocator&>(x)));
    data_              = x.data_;
    size_              = x.size_;
    capacity_          = x.capacity_;
    x.data_            = nullptr;
    x.capacity_ = x.size_ = 0;
    return *this;
  }

  template <typename InputIt>
  inline void copy(InputIt first, InputIt last, pointer dest)
  {
    if constexpr (std::is_same<pointer, InputIt>::value)
    {
      std::memcpy(dest, first, static_cast<std::size_t>(std::distance(first, last)) * sizeof(Ty));
    }
    else
    {
      std::copy(first, last, dest);
    }
  }

  inline pointer allocate(size_type n)
  {
    return allocator_traits::allocate(*this, n);
  }

  inline void deallocate()
  {
    allocator_traits::deallocate(*this, data_, capacity_);
  }

  inline void unchecked_reserve(size_type n)
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

  inline void unchecked_reserve(size_type n, size_type at, size_type holes)
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

  void swap(soavector<Ty, Allocator>& x, std::false_type)
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
  }

  void swap(soavector<Ty, Allocator>& x, std::true_type)
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
    std::swap<Allocator>(this, x);
  }

  friend void swap(soavector<Ty, Allocator>& lhs, soavector<Ty, Allocator>& rhs)
  {
    lhs.swap(rhs);
  }

  friend bool operator==(const soavector<Ty, Allocator>& x, const soavector<Ty, Allocator>& y)
  {
    return x.size_ == y.size_ && std::memcmp(x.data_, y.data_, x.size_) == 0;
  }

  friend bool operator<(const soavector<Ty, Allocator>& x, const soavector<Ty, Allocator>& y)
  {
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }

  friend bool operator!=(const soavector<Ty, Allocator>& x, const soavector<Ty, Allocator>& y)
  {
    return !(x == y);
  }

  friend bool operator>(const soavector<Ty, Allocator>& x, const soavector<Ty, Allocator>& y)
  {
    return std::lexicographical_compare(y.begin(), y.end(), x.begin(), x.end());
  }

  friend bool operator>=(const soavector<Ty, Allocator>& x, const soavector<Ty, Allocator>& y)
  {
    return !std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }

  friend bool operator<=(const soavector<Ty, Allocator>& x, const soavector<Ty, Allocator>& y)
  {
    return !std::lexicographical_compare(y.begin(), y.end(), x.begin(), x.end());
  }

  template <class InputIterator>
  inline iterator insert_range(const_iterator position, InputIterator first, InputIterator last, std::false_type)
  {
    size_type p = insert_hole(position, static_cast<size_type>(std::distance(first, last)));
    copy(first, last, data_ + p);
    return data_ + p;
  }

  inline iterator insert_range(const_iterator position, size_type n, const Ty& x, std::true_type)
  {
    size_type p = insert_hole(position, n);
    std::uninitialized_fill_n(data_ + p, n, x);
    return data_ + p;
  }

  template <class InputIterator>
  inline void construct_from_range(InputIterator first, InputIterator last, std::false_type)
  {
    size_     = static_cast<size_type>(std::distance(first, last));
    data_     = allocate(size_);
    capacity_ = size_;
    copy(first, last, data_);
  }

  inline void construct_from_range(size_type n, const Ty& value, std::true_type)
  {
    size_     = n;
    data_     = allocate(size_);
    capacity_ = size_;
    std::uninitialized_fill_n(data_, n, value);
  }

  array_type data_{};
  size_type  size_     = 0;
  size_type  capacity_ = 0;
};

} // namespace acl