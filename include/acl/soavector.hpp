#pragma once
#include "allocator.hpp"
#include "default_allocator.hpp"
#include "detail/utils.hpp"
#include "type_traits.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>

namespace acl
{

template <typename Tuple, typename Allocator = default_allocator<>, typename SizeType = typename Allocator::size_type>
class soavector : public Allocator
{

public:
  using tuple_type = Tuple;
  using array_type = detail::tuple_of_ptrs<Tuple>;
  using this_type  = soavector<Tuple, Allocator>;

  using allocator_type            = Allocator;
  using size_type                 = SizeType;
  using difference_type           = std::make_signed_t<size_type>;
  using allocator                 = Allocator;
  using allocator_tag             = typename Allocator::tag;
  using allocator_is_always_equal = typename acl::allocator_traits<allocator_tag>::is_always_equal;
  using propagate_allocator_on_move =
    typename acl::allocator_traits<allocator_tag>::propagate_on_container_move_assignment;
  using propagate_allocator_on_copy =
    typename acl::allocator_traits<allocator_tag>::propagate_on_container_copy_assignment;
  using propagate_allocator_on_swap = typename acl::allocator_traits<allocator_tag>::propagate_on_container_swap;
  // using visualizer                  = detail::tuple_array_visualizer<this_type, Tuple>;

  template <std::size_t i>
  using ivalue_type = std::tuple_element_t<i, tuple_type>;
  template <std::size_t i>
  using ireference = ivalue_type<i>&;
  template <std::size_t i>
  using iconst_reference = const ivalue_type<i>&;
  template <std::size_t i>
  using ipointer = ivalue_type<i>*;
  template <std::size_t i>
  using iconst_pointer = ivalue_type<i> const*;
  template <std::size_t i>
  using iiterator = ivalue_type<i>*;
  template <std::size_t i>
  using iconst_iterator = ivalue_type<i> const*;
  template <std::size_t i>
  using ireverse_iterator = std::reverse_iterator<iiterator<i>>;
  template <std::size_t i>
  using iconst_reverse_iterator = std::reverse_iterator<iconst_iterator<i>>;

  static auto constexpr index_seq = std::make_index_sequence<std::tuple_size_v<Tuple>>();

  template <bool const is_const>
  class base_iterator
  {
  public:
    using value_type      = Tuple;
    using difference_type = std::make_signed_t<size_type>;
    using pointer = std::conditional_t<is_const, detail::tuple_of_cptrs<value_type>, detail::tuple_of_ptrs<value_type>>;
    using reference =
      std::conditional_t<is_const, detail::tuple_of_crefs<value_type>, detail::tuple_of_refs<value_type>>;
    using const_reference = detail::tuple_of_crefs<value_type>;

    base_iterator(base_iterator const&) noexcept = default;
    base_iterator(base_iterator&&) noexcept      = default;
    base_iterator() noexcept                     = default;
    base_iterator(pointer init) noexcept : pointers(init) {}

    base_iterator& operator=(base_iterator const&) noexcept         = default;
    base_iterator& operator=(base_iterator&&) noexcept              = default;
    auto           operator<=>(base_iterator const&) const noexcept = default;

    reference operator*() const noexcept
    {
      return get(index_seq);
    }

    pointer operator->() const noexcept
    {
      return pointers;
    }

    auto& operator++() noexcept
    {
      advance(index_seq);
      return *this;
    }

    auto operator++(int) noexcept
    {
      auto v = *this;
      advance(index_seq);
      return v;
    }

    auto& operator--() noexcept
    {
      retreat(index_seq);
      return *this;
    }

    auto operator--(int) noexcept
    {
      auto v = *this;
      retreat(index_seq);
      return v;
    }

    base_iterator operator+(difference_type n) const noexcept
    {
      return add(index_seq, n);
    }

    base_iterator operator-(difference_type n) const noexcept
    {
      return add(index_seq, -n);
    }

    base_iterator& operator+=(difference_type n) const noexcept
    {
      *this = base_iterator<is_const>(add(index_seq, n));
      return *this;
    }

    base_iterator& operator-=(difference_type n) const noexcept
    {
      *this = base_iterator<is_const>(add(index_seq, -n));
      return *this;
    }

    friend difference_type operator-(base_iterator const& first, base_iterator const& second) noexcept
    {
      return static_cast<difference_type>(std::distance(std::get<0>(second.pointers), std::get<0>(first.pointers)));
    }

  private:
    template <std::size_t... I>
    reference get(std::index_sequence<I...>) const noexcept
    {
      return reference(*std::get<I>(pointers)...);
    }

    template <std::size_t... I>
    auto add(std::index_sequence<I...>, difference_type n) const noexcept
    {
      return pointer((std::get<I>(pointers) + n)...);
    }

    template <std::size_t... I>
    void advance(std::index_sequence<I...>) noexcept
    {
      (std::get<I>(pointers)++, ...);
    }

    template <std::size_t... I>
    void retreat(std::index_sequence<I...>) noexcept
    {
      (std::get<I>(pointers)--, ...);
    }

    template <std::size_t... I>
    reference next(std::index_sequence<I...>) const noexcept
    {
      return reference((*std::get<I>(pointers) + 1)...);
    }

    template <std::size_t... I>
    reference prev(std::index_sequence<I...>) const noexcept
    {
      return reference((*std::get<I>(pointers) - 1)...);
    }

    pointer pointers = {};
  };

  using iterator               = base_iterator<false>;
  using const_iterator         = base_iterator<true>;
  using value_type             = Tuple;
  using reference              = detail::tuple_of_refs<value_type>;
  using const_reference        = detail::tuple_of_crefs<value_type>;
  using pointer                = detail::tuple_of_ptrs<value_type>;
  using const_pointer          = detail::tuple_of_cptrs<value_type>;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  explicit soavector(Allocator const& alloc = Allocator()) noexcept
      : Allocator(alloc), data_{}, size_(0), capacity_(0){};

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
    // detail::do_not_optimize((visualizer*)this);
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
    destroy_all(0, size_, index_seq);
    if (capacity_ < x.size())
    {
      capacity_ = static_cast<size_type>(x.size());
      deallocate();
      data_ = allocate(static_cast<size_type>(x.size()));
    }
    size_ = static_cast<size_type>(x.size());
    construct_range(std::begin(x), std::end(x), data_, 0, index_seq);
    return *this;
  }

  template <class InputIterator>
  void assign(InputIterator first, InputIterator last) noexcept
  {
    destroy_all(0, size_, index_seq);
    size_type s = static_cast<size_type>(std::distance(first, last));
    if (capacity_ < s)
    {
      capacity_ = static_cast<size_type>(s);
      deallocate();
      data_ = allocate(s);
    }
    size_ = static_cast<size_type>(s);
    construct_range(first, last, data_, 0, index_seq);
  }

  void assign(size_type n, const tuple_type& value) noexcept
  {
    destroy_all(0, size_, index_seq);
    if (capacity_ < n)
    {
      capacity_ = n;
      deallocate();
      data_ = allocate(n);
    }

    uninitialized_fill(0, n, value, index_seq);
    size_ = static_cast<size_type>(n);
  }

  void assign(std::initializer_list<tuple_type> x) noexcept
  {
    destroy_all(0, size_, index_seq);
    if (capacity_ < x.size())
    {
      capacity_ = static_cast<size_type>(x.size());
      deallocate();
      data_ = allocate(x.size());
    }
    size_ = static_cast<size_type>(x.size());
    construct_range(std::begin(x), std::end(x), data_, 0, index_seq);
  }

  allocator_type get_allocator() const noexcept
  {
    return *this;
  }

  // iterators:
  template <std::size_t i>
  iiterator<i> ibegin() noexcept
  {
    return std::get<i>(data_);
  }

  template <std::size_t i>
  iconst_iterator<i> ibegin() const noexcept
  {
    return std::get<i>(data_);
  }

  template <std::size_t i>
  iiterator<i> iend() noexcept
  {
    return std::get<i>(data_) + size_;
  }

  template <std::size_t i>
  iconst_iterator<i> iend() const noexcept
  {
    return std::get<i>(data_) + size_;
  }

  template <std::size_t i>
  ireverse_iterator<i> irbegin() noexcept
  {
    return ireverse_iterator<i>(iend<i>());
  }

  template <std::size_t i>
  iconst_reverse_iterator<i> irbegin() const noexcept
  {
    return iconst_reverse_iterator<i>(iend<i>());
  }

  template <std::size_t i>
  ireverse_iterator<i> irend() noexcept
  {
    return ireverse_iterator<i>(ibegin<i>());
  }

  template <std::size_t i>
  iconst_reverse_iterator<i> irend() const noexcept
  {
    return iconst_reverse_iterator<i>(ibegin<i>());
  }

  template <std::size_t i>
  iconst_iterator<i> icbegin() const noexcept
  {
    return ibegin<i>();
  }

  template <std::size_t i>
  iconst_iterator<i> icend() const noexcept
  {
    return iend<i>();
  }

  template <std::size_t i>
  iconst_reverse_iterator<i> icrbegin() const noexcept
  {
    return irbegin<i>();
  }

  template <std::size_t i>
  iconst_reverse_iterator<i> icrend() const noexcept
  {
    return irend<i>();
  }

  // iterators:
  iterator begin() noexcept
  {
    return iterator(data_);
  }

  const_iterator begin() const noexcept
  {
    return const_iterator(data_);
  }

  iterator end() noexcept
  {
    return begin() + static_cast<difference_type>(size_);
  }

  const_iterator end() const noexcept
  {
    return begin() + static_cast<difference_type>(size_);
  }

  reverse_iterator rbegin() noexcept
  {
    return reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const noexcept
  {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() noexcept
  {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rend() const noexcept
  {
    return const_reverse_iterator(begin());
  }

  const_iterator cbegin() const noexcept
  {
    return begin();
  }

  const_iterator cend() const noexcept
  {
    return end();
  }

  const_reverse_iterator crbegin() const noexcept
  {
    return rbegin();
  }

  const_reverse_iterator crend() const noexcept
  {
    return rend();
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
      uninitialized_fill(size_, sz - size_, c, index_seq);
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

  inline reference operator[](size_type n) noexcept
  {
    return get<reference>(index_seq);
  }

  inline const_reference operator[](size_type n) const noexcept
  {
    return get<const_reference>(index_seq);
  }

  template <std::size_t i>
  ireference<i> at(size_type n) noexcept
  {
    assert(n < size_);
    return std::get<i>(data_)[n];
  }

  template <std::size_t i>
  iconst_reference<i> at(size_type n) const noexcept
  {
    assert(n < size_);
    return std::get<i>(data_)[n];
  }

  template <std::size_t i>
  ireference<i> front() noexcept
  {
    assert(0 < size_);
    return std::get<i>(data_)[0];
  }

  template <std::size_t i>
  iconst_reference<i> front() const noexcept
  {
    assert(0 < size_);
    return std::get<i>(data_)[0];
  }

  template <std::size_t i>
  ireference<i> back() noexcept
  {
    assert(0 < size_);
    return std::get<i>(data_)[size_ - 1];
  }

  template <std::size_t i>
  iconst_reference<i> back() const noexcept
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
  template <typename... Args>
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

  template <typename... Args>
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
    assert(last < size());
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
    return first;
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

  template <typename It, typename Arg>
  void copy_fill_n(It it, It end_it, size_type count, Arg&& arg) noexcept
  {
    for (; it != end_it; ++it)
      *it = std::forward<Arg>(arg);
  }

  template <std::size_t... I>
  void copy_fill(size_type start, size_type count, tuple_type const& t, std::index_sequence<I...>) noexcept
  {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    (copy_fill_n(std::get<I>(data_) + start, count, std::get<I>(t)), ...);
  }

  template <std::size_t... I, typename... Args>
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

  template <std::size_t... I, typename... Args>
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
  void move_construct(Ty* to, Ty* from, size_type n) noexcept
  {
    for (size_type i = 0; i < n; ++i)
      std::construct_at(to + i, std::move(from[i]));
  }

  template <std::size_t... I>
  void move_construct(array_type& dst, size_type dst_offset, array_type& src, size_type src_offset, size_type n,
                      std::index_sequence<I...>) noexcept
  {
    (move_construct(std::get<I>(dst) + dst_offset, std::get<I>(src) + src_offset, n), ...);
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
      offset++;
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
      move_construct(dst, src, n);
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
      data_ = allocate(x.size_);
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
    data_       = x.data_;
    size_       = x.size_;
    capacity_   = x.capacity_;
    x.data_     = nullptr;
    x.capacity_ = x.size_ = 0;
    return *this;
  }

  template <typename Ty>
  void allocate(Ty*& out_ref, size_type n) noexcept
  {
    out_ref = acl::allocate<Ty>(static_cast<Allocator&>(*this), n * sizeof(Ty));
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
    acl::deallocate(static_cast<Allocator&>(*this), out_ref, n * sizeof(Ty));
  }

  template <std::size_t... I>
  inline void deallocate(size_type n, std::index_sequence<I...>) noexcept
  {
    (deallocate(std::get<I>(data_), n), ...);
  }

  inline void deallocate() noexcept
  {
    deallocate(capacity_, index_seq);
  }

  inline void unchecked_reserve(size_type n) noexcept
  {
    auto d = allocate(n);
    if (size_)
    {
      move_construct(d, 0, data_, 0, size_, index_seq);
      destroy_and_deallocate();
    }
    data_     = d;
    capacity_ = n;
  }

  inline void unchecked_reserve(size_type n, size_type at, size_type holes) noexcept
  {
    auto d = allocate(n);
    if (size_)
    {
      move_construct(d, 0, data_, 0, at, index_seq);
      move_construct(d, at + holes, data_, at, (size_ - at), index_seq);
      destroy_and_deallocate();
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
    return x.size_ == y.size_ && equals(x.data_, y.data_, x.size_, index_seq);
  }

  friend bool operator<(soavector const& x, soavector const& y) noexcept
  {
    auto n = std::min(x.size(), y.size());
    return (less(x, y, n, index_seq) | (x.size < y.size));
  }

  friend bool operator<=(soavector const& x, soavector const& y) noexcept
  {
    auto n = std::min(x.size(), y.size());
    return (lesseq(x, y, n, index_seq) | (x.size <= y.size));
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
    construct_range(first, last, data_, 0, index_seq);
  }

  template <typename reftype, std::size_t... I>
  reftype get(std::index_sequence<I...>) const noexcept
  {
    return reftype(*std::get<I>(data_)...);
  }

  array_type data_{};
  size_type  size_     = 0;
  size_type  capacity_ = 0;
};

} // namespace acl
