
#pragma once

#pragma once
#include "allocator.hpp"
#include "detail/utils.hpp"
#include "type_traits.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>

namespace acl
{
template <typename Allocator /*= default_allocator<>*/, typename... Args>
class soavector : public Allocator
{

public:
  using tuple_type = std::tuple<Args...>;
  using array_type = detail::tuple_of_ptrs<Args...>;

  template <std::size_t i>
  using value_type      = std::tuple_element_t<i, std::tuple<Args...>>;
  using allocator_type  = Allocator;
  using size_type       = std::uint32_t;
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
  using allocator                   = Allocator;
  using allocator_tag               = typename Allocator::tag;
  using allocator_is_always_equal   = typename acl::traits<allocator_tag>::is_always_equal;
  using propagate_allocator_on_move = typename acl::traits<allocator_tag>::propagate_on_container_move_assignment;
  using propagate_allocator_on_copy = typename acl::traits<allocator_tag>::propagate_on_container_copy_assignment;
  using propagate_allocator_on_swap = typename acl::traits<allocator_tag>::propagate_on_container_swap;

  static auto constexpr index_seq = std::make_index_sequence<sizeof...(Args)>();

public:
  explicit soavector(Allocator const& alloc = Allocator()) noexcept
      : Allocator(alloc), data_(nullptr), size_(0), capacity_(0){};

  explicit soavector(size_type n) noexcept : data_(allocate(n)), size_(n), capacity_(n) {}

  soavector(size_type n, tuple_type const& value, Allocator const& alloc = Allocator()) noexcept
      : Allocator(alloc), data_(allocate(n)), size_(n), capacity_(n)
  {
    uninitialized_fill(0, n, value, index_seq);
  }

  template <class InputIterator>
  soavector(InputIterator first, InputIterator last, Allocator const& alloc = Allocator()) noexcept : Allocator(alloc)
  {
    construct_from_range(first, last);
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

    construct_tuple_at(size_++, index_seq, x);
  }

  void pop_back() noexcept
  {
    assert(size_);
    destroy_at(--size_, index_seq);
  }

  void emplace(size_type position, Args&&... args) noexcept
  {
    size_type p = insert_hole(position);
    construct_at(p, index_seq, std::forward<Args>(args)...);
  }

  void insert(size_type position, tuple_type const& x)
  {
    size_type p = insert_hole(position);
    construct_tuple_at(p, index_seq, x);
  }

  void insert(size_type position, tuple_type&& x)
  {
    size_type p = insert_hole(position);
    construct_tuple_at(p, index_seq, std::move(x));
  }

  void insert(size_type position, size_type n, tuple_type const& x)
  {
    insert_range(position, n, x);
  }

  template <typename InputIterator>
  void insert(size_type position, InputIterator first, InputIterator last)
  {
    insert_range(position, first, last);
  }

  void insert(size_type position, std::initializer_list<tuple_type> x)
  {
    size_type p = insert_hole(position, static_cast<size_type>(x.size()));
    construct_range(std::begin(x), std::end(x), data_, p, index_seq);
  }

  template <typename Ty>
  void erase_at(Ty* data, size_type first, size_type last) noexcept
  {
    assert(last < end());
    if constexpr (std::is_trivially_copyable_v<Ty>)
      std::memmove(data + first, data + last, (size_ - last) * sizeof(Ty));
    else
    {
      while (last < size_)
        data[first++] = std::move(data[last++]);

      if constexpr (!std::is_trivially_destructible_v<Ty>)
        for (; first < size_; ++first)
          std::destroy_at(&data[first++]);
    }
  }

  template <std::size_t... I>
  void erase_at(size_type position, std::index_sequence<I...>) noexcept
  {
    (erase_at(std::get<I>(data_), position, position + 1), ...);
  }

  template <std::size_t... I>
  void erase_at(size_type first, size_type last, std::index_sequence<I...>) noexcept
  {
    (erase_at(std::get<I>(data_), first, last), ...);
  }

  size_type erase(size_type position) noexcept
  {
    assert(position < size());
    erase_at(position, index_seq);
    size_--;
    return position;
  }

  size_type erase(size_type first, size_type last) noexcept
  {
    assert(last < size());
    erase_at(first, last, index_seq);
    size_ -= (last - first);
    return const_cast<iterator>(first);
  }

  void swap(soavector& x) noexcept
  {
    swap(x, propagate_allocator_on_swap());
  }

  void clear() noexcept
  {
    size_ = 0;
  }

private:
  template <std::size_t... I>
  void uninitialized_fill(size_type start, size_type count, tuple_type const& t, std::index_sequence<I...>) noexcept
  {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    (std::uninitialized_fill_n(std::get<I>(data_) + start, count, std::get<I>(t)), ...);
  }

  template <std::size_t... I>
  void construct_at(size_type i, std::index_sequence<I...>, Args&&... args) noexcept
  {
    (std::construct_at(std::get<I>(data_) + i, std::forward<Args>(args)), ...);
  }

  template <std::size_t... I>
  void construct_tuple_at(size_type i, std::index_sequence<I...>, tuple_type const& arg) noexcept
  {
    (std::construct_at(std::get<I>(data_) + i, std::get<I>(arg)), ...);
  }

  template <std::size_t... I>
  void construct_tuple_at(size_type i, std::index_sequence<I...>, tuple_type&& arg) noexcept
  {
    (std::construct_at(std::get<I>(data_) + i, std::move(std::get<I>(arg))), ...);
  }

  template <typename T, typename Arg>
  void move_at(T* data, Arg&& arg) noexcept
  {
    *data = std::forward<Arg>(arg);
  }

  template <std::size_t... I>
  void move_at(size_type i, std::index_sequence<I...>, Args&&... args) noexcept
  {
    (move_at(std::get<I>(data_) + i, std::forward<Args>(args)), ...);
  }

  template <std::size_t... I>
  void move_tuple_at(size_type i, std::index_sequence<I...>, tuple_type const& arg) noexcept
  {
    (move_at(std::get<I>(data_) + i, std::get<I>(arg)), ...);
  }

  template <typename Ty>
  void move_assign(Ty* to, Ty* from, size_type n) noexcept
  {
    for (size_type i = 0; i < n; ++i)
      to[i] = std::move(from[i]);
  }

  template <std::size_t... I>
  void move(array_type& dst, size_type dst_offset, array_type&& src, size_type src_offset, size_type n,
            std::index_sequence<I...>) noexcept
  {
    (move_assign(std::get<I>(dst) + dst_offset, std::get<I>(src) + src_offset, n), ...);
  }

  template <typename Ty>
  void copy_assign(Ty* to, Ty const* from, size_type n) noexcept
  {
    if constexpr (std::is_trivially_copyable_v<Ty>)
      std::memcpy(to, from, n * sizeof(Ty));
    else
      std::copy(from, from + n, to);
  }

  template <std::size_t... I>
  void copy(array_type& dst, array_type const& src, size_type n, std::index_sequence<I...>) noexcept
  {
    (copy_assign(std::get<I>(dst), std::get<I>(src), n), ...);
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
    (construct_list(std::get<I>(data_), size_, std::get<I>(x.data_)), ...);
  }

  template <typename InputIterator, std::size_t... I>
  static void construct_range(InputIterator start, InputIterator end, array_type& store, size_type offset,
                              std::index_sequence<I...>) noexcept
  {
    while (start != end)
    {
      (std::construct_at(std::get<I>(store) + offset, std::get<I>(*(start))), ...);
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
    (destroy_seq(std::get<I>(data_) + start, count), ...);
  }

  template <std::size_t... I>
  void destroy_at(size_type at, std::index_sequence<I...>) noexcept
  {
    (destroy_single(std::get<I>(data_) + at), ...);
  }

  void destroy_and_deallocate() noexcept
  {
    destroy_all(0, size_, index_seq);
    deallocate();
  }

  template <typename Ty>
  inline void memmove(Ty* dst, Ty* src, size_type n)
  {
    if constexpr (std::is_trivially_copyable_v<Ty>)
      std::memmove(dst, src, (n * sizeof(Ty)));
    else
    {
      move_assign(dst, src, n);
    }
  }

  template <std::size_t... I>
  inline void memmove(size_type to, size_type from, size_type n, std::index_sequence<I...>)
  {
    (memmove(std::get<I>(data_) + to, std::get<I>(data_) + from, n), ...);
  }

  inline size_type insert_hole(size_type p, size_type n = 1) noexcept
  {
    size_type nsz = size_ + n;
    if (capacity_ < nsz)
    {
      unchecked_reserve(size_ + std::max(size_ >> 1, n), p, n);
    }
    else
    {
      memmove(p + n, p, n, index_seq);
      destroy_all(p, n, index_seq);
    }
    size_ = nsz;
    return p;
  }

  inline soavector& assign_copy(soavector const& x, std::false_type) noexcept
  {
    if (capacity_ < x.size_)
    {
      destroy_and_deallocate();
      data_ = allocate(x.size_);
      size_ = capacity_ = x.size_;
      construct_list(x, index_seq);
    }
    else
    {
      resize(x.size_);
      copy(data_, x.data_, x.size_, index_seq);
    }
    return *this;
  }

  inline soavector& assign_copy(soavector const& x, std::true_type) noexcept
  {
    if (allocator_is_always_equal::value || static_cast<const Allocator&>(x) == static_cast<const Allocator&>(*this))
      assign(x, std::false_type());
    else
    {
      destroy_and_deallocate();
      Allocator::operator=(static_cast<const Allocator&>(x));
      data_              = allocate(x.size_);
      size_ = capacity_ = x.size_;
      construct_list(x, index_seq);
    }
    return *this;
  }

  inline soavector& assign_move(soavector&& x, std::false_type) noexcept
  {
    if (allocator_is_always_equal::value || static_cast<const Allocator&>(x) == static_cast<const Allocator&>(*this))
    {
      destroy_and_deallocate();
      data_       = x.data_;
      size_       = x.size_;
      capacity_   = x.capacity_;
      x.data_     = {};
      x.capacity_ = x.size_ = 0;
    }
    else
      assign_copy(x, std::false_type());
    return *this;
  }

  inline soavector& assign_move(soavector&& x, std::true_type) noexcept
  {
    destroy_and_deallocate();
    Allocator::operator=(std::move(static_cast<Allocator&>(x)));
    data_              = x.data_;
    size_              = x.size_;
    capacity_          = x.capacity_;
    x.data_            = nullptr;
    x.capacity_ = x.size_ = 0;
    return *this;
  }

  template <typename Ty>
  void allocate(Ty*& out_ref, size_type n) noexcept
  {
    out_ref = acl::allocate(*this, n * sizeof(Ty));
  }

  template <std::size_t... I>
  inline array_type allocate(size_type n, std::index_sequence<I...>) noexcept
  {
    array_type r;
    (allocate(std::get<I>(r), n), ...);
    return r;
  }

  inline array_type allocate(size_type n) noexcept
  {
    return allocate(n, index_seq);
  }

  template <typename Ty>
  void deallocate(Ty* out_ref, size_type n) noexcept
  {
    acl::deallocate(*this, out_ref, n * sizeof(Ty));
  }

  template <std::size_t... I>
  inline void deallocate(size_type n, std::index_sequence<I...>) noexcept
  {
    (deallocate(std::get<I>(data_), n), ...);
  }

  inline void deallocate() noexcept
  {
    deallocate(data_, capacity_, index_seq);
  }

  inline void unchecked_reserve(size_type n) noexcept
  {
    auto d = allocate(n);
    if (data_)
    {
      move(d, 0, data_, 0, size_);
      destroy_and_deallocate();
    }
    data_     = d;
    capacity_ = n;
  }

  inline void unchecked_reserve(size_type n, size_type at, size_type holes) noexcept
  {
    auto d = allocate(n);
    if (data_)
    {
      move(d, 0, data_, 0, at);
      move(d, at + holes, data_, at, (size_ - at));
      deallocate();
    }
    data_     = d;
    capacity_ = n;
  }

  void swap(soavector& x, std::false_type) noexcept
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
  }

  void swap(soavector& x, std::true_type) noexcept
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
    std::swap<Allocator>(this, x);
  }

  friend void swap(soavector& lhs, soavector& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  template <typename Ty>
  static bool equals(Ty const* first, Ty const* second, size_type n) noexcept
  {
    for (size_type i = 0; i < n; ++i)
    {
      if (first[i] != second[i])
        return false;
    }
    return true;
  }

  template <std::size_t... I>
  static bool equals(array_type const& first, array_type const& second, size_type n, std::index_sequence<I...>) noexcept
  {
    return (equals(std::get<I>(first), std::get<I>(second), n) && ...);
  }

  /// Less
  template <typename Ty>
  static bool less(Ty const* first, Ty const* second, size_type n) noexcept
  {
    for (size_type i = 0; i < n; ++i)
    {
      if (first[i] >= second[i])
        return false;
    }
    return true;
  }

  template <std::size_t... I>
  static bool less(array_type const& first, array_type const& second, size_type n, std::index_sequence<I...>) noexcept
  {
    return (less(std::get<I>(first), std::get<I>(second), n) && ...);
  }

  template <typename Ty>
  static bool lesseq(Ty const* first, Ty const* second, size_type n) noexcept
  {
    for (size_type i = 0; i < n; ++i)
    {
      if (first[i] > second[i])
        return false;
    }
    return true;
  }

  template <std::size_t... I>
  static bool lesseq(array_type const& first, array_type const& second, size_type n, std::index_sequence<I...>) noexcept
  {
    return (lesseq(std::get<I>(first), std::get<I>(second), n) && ...);
  }
  ///

  friend bool operator==(soavector const& x, soavector const& y) noexcept
  {
    return x.size_ == y.size_ && equals(x.data_, y.data_, x.size_) == 0;
  }

  friend bool operator<(soavector const& x, soavector const& y) noexcept
  {
    auto n = std::min(x.size(), y.size());
    return (less(x, y, n, index_seq) && x.size < y.size);
  }

  friend bool operator<=(soavector const& x, soavector const& y) noexcept
  {
    auto n = std::min(x.size(), y.size());
    return (lesseq(x, y, n, index_seq) && x.size <= y.size);
  }

  friend bool operator!=(soavector const& x, soavector const& y) noexcept
  {
    return !(x == y);
  }

  friend bool operator>(soavector const& x, soavector const& y) noexcept
  {
    return y < x;
  }

  friend bool operator>=(soavector const& x, soavector const& y) noexcept
  {
    return y <= x;
  }

  template <class InputIterator>
  inline size_type insert_range(size_type position, InputIterator first, InputIterator last) noexcept
  {
    size_type p = insert_hole(position, static_cast<size_type>(std::distance(first, last)));
    construct_range(first, last, data_, p, index_seq);
    return p;
  }

  inline size_type insert_range(size_type position, size_type n, tuple_type const& x) noexcept
  {
    size_type p = insert_hole(position, n);
    uninitialized_fill(p, n, x, index_seq);
    return p;
  }

  template <typename InputIterator>
  inline void construct_from_range(InputIterator first, InputIterator last) noexcept
  {
    size_     = static_cast<size_type>(std::distance(first, last));
    data_     = allocate(size_);
    capacity_ = size_;
    copy_construct(first, last, data_, index_seq);
  }

  array_type data_{};
  size_type  size_     = 0;
  size_type  capacity_ = 0;
};

} // namespace acl