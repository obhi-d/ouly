/*
 * small_vector.hpp
 *
 *  Created on: 11/2/2018, 10:48:02 AM
 *      Author: obhi
 */

#pragma once
#include "allocator.hpp"
#include "default_allocator.hpp"
#include "detail/utils.hpp"
#include "type_traits.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
// refer to:
// https://en.cppreference.com/w/cpp/header/vector
namespace acl
{
template <typename Ty, size_t N = 0, typename Allocator = default_allocator<>, typename Traits = acl::traits<Ty>>
class small_vector : public Allocator
{
public:
  using value_type                = Ty;
  using allocator_type            = Allocator;
  using size_type                 = detail::choose_size_t<uint32_t, Traits>;
  using difference_type           = size_type;
  using reference                 = value_type&;
  using const_reference           = const value_type&;
  using pointer                   = Ty*;
  using const_pointer             = Ty const*;
  using iterator                  = Ty*;
  using const_iterator            = Ty const*;
  using reverse_iterator          = std::reverse_iterator<iterator>;
  using const_reverse_iterator    = std::reverse_iterator<const_iterator>;
  using allocator                 = Allocator;
  using allocator_tag             = typename Allocator::tag;
  using allocator_is_always_equal = typename acl::allocator_traits<allocator_tag>::is_always_equal;
  using propagate_allocator_on_move =
    typename acl::allocator_traits<allocator_tag>::propagate_on_container_move_assignment;
  using propagate_allocator_on_copy =
    typename acl::allocator_traits<allocator_tag>::propagate_on_container_copy_assignment;
  using propagate_allocator_on_swap = typename acl::allocator_traits<allocator_tag>::propagate_on_container_swap;

private:
  using traits                                          = Traits;
  static constexpr bool has_null_method                 = detail::has_null_method<traits, value_type>;
  static constexpr bool has_null_value                  = detail::has_null_value<traits, value_type>;
  static constexpr bool has_null_construct              = detail::has_null_construct<traits, value_type>;
  static constexpr bool has_zero_memory                 = detail::has_zero_memory_attrib<traits>;
  static constexpr bool has_no_fill                     = detail::has_no_fill_attrib<traits>;
  static constexpr bool has_pod                         = detail::has_has_pod_attrib<traits>;
  static constexpr bool has_trivial_dtor                = has_pod || std::is_trivially_destructible_v<traits>;
  static constexpr bool has_trivially_destroyed_on_move = detail::has_trivially_destroyed_on_move_attrib<traits>;

  using storage = detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
  struct heap_storage
  {
    storage*  pdata_    = nullptr;
    size_type capacity_ = 0;
  };

  static constexpr size_type inline_capacity = N * sizeof(Ty) > sizeof(heap_storage)
                                                 ? N
                                                 : (sizeof(heap_storage) + sizeof(Ty) - 1) / sizeof(Ty);

  using inline_storage                       = std::array<storage, inline_capacity>;
  union data_store
  {
    inline_storage ldata_ = {};
    heap_storage   hdata_;

    inline data_store() noexcept : ldata_() {}
  };

  struct tail_type : std::true_type
  {
    size_type tail;
  };

public:
  explicit small_vector(const Allocator& alloc = Allocator()) noexcept : Allocator(alloc){};

  explicit small_vector(size_type n) noexcept
  {
    resize(n);
  }

  small_vector(size_type n, const Ty& value, const Allocator& alloc = Allocator()) : Allocator(alloc)
  {
    if (n > inline_capacity)
      unchecked_reserve_in_heap(n);
    resize(n, value);
  }

  template <class InputIterator>
  small_vector(InputIterator first, InputIterator last, const Allocator& alloc = Allocator()) noexcept
      : Allocator(alloc)
  {
    construct_from_range(first, last, std::is_integral<InputIterator>());
  }

  small_vector(const small_vector& x) noexcept : Allocator((Allocator const&)x)
  {
    if (x.size() > inline_capacity)
      unchecked_reserve_in_heap(x.size());
    size_ = x.size();
    copy_construct(std::begin(x), std::end(x), get_data());
  }

  small_vector(small_vector&& x) noexcept
  {
    *this = std::move(x);
  }

  small_vector(const small_vector& x, const Allocator& alloc) noexcept : Allocator(alloc)
  {
    if (x.size() > inline_capacity)
      unchecked_reserve_in_heap(x.size());
    size_ = x.size();
    copy_construct(std::begin(x), std::end(x), get_data());
  }

  small_vector(small_vector&& x, const Allocator& alloc) noexcept : Allocator(alloc)
  {
    *this = std::move(x);
  };
  small_vector(std::initializer_list<Ty> x, const Allocator& alloc = Allocator()) noexcept : Allocator(alloc)
  {
    if (x.size() > inline_capacity)
      unchecked_reserve_in_heap(static_cast<size_type>(x.size()));
    size_ = static_cast<size_type>(x.size());
    copy_construct(std::begin(x), std::end(x), get_data());
  }

  ~small_vector() noexcept
  {
    clear();
  }

  small_vector& operator=(const small_vector& x) noexcept
  {
    return assign_copy(x, propagate_allocator_on_copy());
  }

  small_vector& operator=(small_vector&& x) noexcept
  {
    return assign_move(std::move(x), propagate_allocator_on_move());
  }

  small_vector& operator=(std::initializer_list<Ty> x) noexcept
  {
    clear();
    resize_fill(x.size(), std::false_type());
    size_ = x.size();
    copy_construct(std::begin(x), std::end(x), get_data());
    return *this;
  }

  template <class InputIterator>
  void assign(InputIterator first, InputIterator last) noexcept
  {
    size_type xsize = static_cast<size_type>(std::distance(first, last));
    clear();
    resize(xsize);
    copy_construct(first, last, get_data());
  }

  void assign(size_type n, const Ty& value) noexcept
  {
    clear();
    resize(n, value);
    std::uninitialized_fill_n(data(), n, value);
  }

  void assign(std::initializer_list<Ty> x) noexcept
  {
    clear();
    resize(static_cast<size_type>(x.size()));
    copy_construct(std::begin(x), std::end(x), get_data());
  }

  allocator_type get_allocator() const noexcept
  {
    return *this;
  }

  // iterators:
  iterator begin() noexcept
  {
    return get_data();
  }

  const_iterator begin() const noexcept
  {
    return get_data();
  }

  iterator end() noexcept
  {
    return get_data() + size();
  }

  const_iterator end() const noexcept
  {
    return get_data() + size();
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
    return Allocator::max_size();
  }

  void resize(size_type sz) noexcept
  {
    resize_fill(sz, std::false_type{});
  }

  void resize(size_type sz, const Ty& c) noexcept
  {
    resize_fill(sz, c);
  }

  size_type capacity() const noexcept
  {
    return is_inlined() ? inline_capacity : data_store_.hdata_.capacity_;
  }

  bool empty() const noexcept
  {
    return size() == 0;
  }

  /// @brief Use resize to really reserve space
  void reserve(size_type n) noexcept
  {
    if (is_inlined())
      return;
    if (capacity() < n)
    {
      unchecked_reserve_in_heap(n);
    }
  }

  void shrink_to_fit() noexcept
  {
    if (is_inlined())
      return;

    auto size_val = size();
    if (capacity() != size_val)
    {
      unchecked_reserve_in_heap(size_val);
    }
  }

  // element access:
  reference operator[](size_type n) noexcept
  {
    assert(n < size());
    return get_data()[n];
  }

  const_reference operator[](size_type n) const noexcept
  {
    assert(n < size());
    return get_data()[n];
  }

  reference at(size_type n)
  {
    assert(n < size());
    return get_data()[n];
  }

  const_reference at(size_type n) const noexcept
  {
    assert(n < size());
    return get_data()[n];
  }

  reference front()
  {
    return at(0);
  }

  const_reference front() const noexcept
  {
    return at(0);
  }

  reference back()
  {
    return at(size() - 1);
  }

  const_reference back() const noexcept
  {
    return at(size() - 1);
  }

  // data access
  Ty* data() noexcept
  {
    return get_data();
  }

  const Ty* data() const noexcept
  {
    return get_data();
  }

  // modifiers:
  template <class... Args>
  void emplace_back(Args&&... args) noexcept
  {
    auto sz = size();
    if (capacity() <= sz)
      unchecked_reserve_in_heap(sz + std::max<size_type>(sz >> 1, 1));

    std::construct_at(get_data() + sz, std::forward<Args>(args)...);
    size_ = sz + 1;
  }

  void push_back(const Ty& x) noexcept
  {
    emplace_back(x);
  }

  void push_back(Ty&& x) noexcept
  {
    emplace_back(std::move(x));
  }

  void pop_back() noexcept
  {
    auto sz = size();
    assert(sz);

    auto last = size_--;
    if (size_ <= inline_capacity && last > inline_capacity)
    {
      transfer_to_ib(size_, tail_type{.tail=last});
    }
    else
    {
      if constexpr (!has_trivial_dtor)
        std::destroy_at(get_data() + size_);
    }
  }

  template <class... Args>
  iterator emplace(const_iterator position, Args&&... args) noexcept
  {
    size_type p   = insert_hole(position);
    auto      ptr = get_data() + p;
    std::construct_at(ptr, std::forward<Args>(args)...);
    return ptr;
  }

  iterator insert(const_iterator position, const Ty& x) noexcept
  {
    return emplace(position, x);
  }

  iterator insert(const_iterator position, Ty&& x) noexcept
  {
    return emplace(position, x);
  }

  iterator insert(const_iterator position, size_type n, const Ty& x) noexcept
  {
    return insert_range(position, n, x, std::true_type());
  }

  template <class InputIterator>
  iterator insert(const_iterator position, InputIterator first, InputIterator last) noexcept
  {
    return insert_range(position, first, last, std::is_integral<InputIterator>());
  }

  iterator insert(const_iterator position, std::initializer_list<Ty> x) noexcept
  {
    size_type p   = insert_hole(position, static_cast<size_type>(x.size()));
    auto      ptr = get_data() + p;
    copy_construct(std::begin(x), std::end(x), ptr);
    return ptr;
  }

  iterator erase(const_iterator position) noexcept
  {
    assert(position < end());
    auto last = size_--;
    if constexpr (std::is_trivially_copyable_v<Ty> || has_pod)
    {
      auto data = get_data();
      std::memmove(const_cast<iterator>(position), position + 1,
                   static_cast<std::size_t>((data + size_) - (position)) * sizeof(Ty));
      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
        std::destroy_at(data + size_);
    }
    else
    {
      auto it  = const_cast<iterator>(position);
      auto end = get_data() + size_;
      for (; it != end; ++it)
        *it = std::move(*(it + 1));
      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
        std::destroy_at(end);
    }
    if (size_ <= inline_capacity && last > inline_capacity)
    {
      // already destroyed end objects
      std::uint32_t p = static_cast<std::uint32_t>(std::distance((Ty const*)data_store_.hdata_.pdata_, position));
      transfer_to_ib(size_);
      return const_cast<iterator>((Ty*)data_store_.ldata_.data() + p);
    }
    else
      return const_cast<iterator>(position);
  }

  iterator erase(const_iterator first, const_iterator last) noexcept
  {
    assert(last < end());
    std::uint32_t n = static_cast<std::uint32_t>(std::distance(first, last));
    if constexpr (std::is_trivially_copyable_v<Ty> || has_pod)
    {
      auto data = get_data();
      std::memmove(const_cast<iterator>(first), last, static_cast<std::size_t>((data + size_) - (last)) * sizeof(Ty));
    }
    else
    {
      auto dst = const_cast<iterator>(first);
      auto src = const_cast<iterator>(last);
      auto end = get_data() + size_;
      for (; src != end; ++src)
        *dst = std::move(*(src + 1));
    }
    auto data = get_data();
    auto old_size = size_;
    size_ -= n;
    if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
      std::destroy_n(data + size_, n);

    if (size_ <= inline_capacity && old_size > inline_capacity)
    {
      // already destroyed end objects
      std::uint32_t p = static_cast<std::uint32_t>(std::distance((Ty const*)data_store_.hdata_.pdata_, first));
      transfer_to_ib(size_);
      return const_cast<iterator>((Ty*)data_store_.ldata_.data() + p);
    }
    else
    {
      return const_cast<iterator>(first);
    }
  }

  void swap(small_vector& x) noexcept
  {
    swap(x, propagate_allocator_on_swap());
  }

  void clear() noexcept
  {
    clear_data();
  }

  static constexpr inline size_type get_inlined_capacity()
  {
    return inline_capacity;
  }


private:
  inline void resize_no_fill(size_type sz) noexcept
  {
    if (sz <= inline_capacity)
    {
      if (size_ > inline_capacity)
        transfer_to_ib(sz, tail_type{.tail = size_});
    }
    else
    {
      if (capacity() < sz)
      {
        unchecked_reserve_in_heap(sz);
      }
    }
  }

  template <typename tail = std::false_type>
  void resize_fill(size_type sz, tail const& t) noexcept
  {
    resize_no_fill(sz);
    if (sz > size_)
    {
      if constexpr (!std::is_trivially_constructible_v<Ty> && std::is_same_v<tail, Ty>)
        std::uninitialized_fill_n(data() + size_, sz - size_, t);
    }
    else
    {
      if constexpr (!has_trivial_dtor)
      {
        std::destroy_n(data(), size_ - sz);
      }
    }
    size_ = sz;
  }

  inline Ty* get_data() const
  {
    return is_inlined() ? (Ty*)data_store_.ldata_.data() : (Ty*)data_store_.hdata_.pdata_;
  }

  inline bool is_inlined() const
  {
    return (size_ <= inline_capacity);
  }

  inline void unchecked_reserve_in_heap(size_type n)
  {
    heap_storage copy;
    copy.capacity_ = n;
    copy.pdata_    = acl::allocate<storage>(*this, n * sizeof(storage), alignarg<storage>);
    auto ldata     = data();
    auto d         = copy.pdata_;
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(d, ldata, size_ * sizeof(Ty));
    }
    else
    {
      std::uninitialized_move(ldata, ldata + size_, d);
      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
        std::destroy_at(ldata, size_);
    }
    if (!is_inlined())
      acl::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                      alignarg<storage>);
    data_store_.hdata_ = copy;
  }

  inline void unchecked_reserve_in_heap(size_type n, size_type at, size_type holes) noexcept
  {
    heap_storage copy;
    copy.capacity_ = n;
    copy.pdata_    = acl::allocate<storage>(*this, n * sizeof(storage), alignarg<storage>);
    auto ldata     = data();
    auto d         = (Ty*)copy.pdata_;
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(d, ldata, at * sizeof(Ty));
      std::memcpy(d + at + holes, ldata + at, (size_ - at) * sizeof(Ty));
    }
    else
    {
      std::uninitialized_move(ldata, ldata + at, d);
      std::uninitialized_move(ldata + at, ldata + size_, d + at + holes);
      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
        std::destroy_at(ldata, size_);
    }
    if (!is_inlined())
      acl::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                      alignarg<storage>);
    data_store_.hdata_ = copy;
  }

  inline void clear_data() noexcept
  {
    if (empty())
      return;
    auto d = data();
    if constexpr (!has_trivial_dtor)
      std::destroy_n(d, size_);
    if (!is_inlined())
      acl::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                      alignarg<storage>);
    size_ = 0;
  }

  template <typename tail = std::false_type>
  inline void transfer_to_ib(size_type nb, tail last_size = tail())
  {
    heap_storage copy  = data_store_.hdata_;
    auto         d     = (Ty*)copy.pdata_;
    auto         ldata = (Ty*)data_store_.ldata_.data();

    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(ldata, d, sizeof(storage) * nb);
    }
    else
    {
      std::uninitialized_move(d, d + nb, ldata);
    }

    if constexpr (!has_trivial_dtor)
    {
      if constexpr (!tail::value)
        std::destroy_n(d, size_);
      else if constexpr (!has_trivially_destroyed_on_move)
        std::destroy_n(d + nb, last_size.tail - nb);
    }
    acl::deallocate(*this, copy.pdata_, copy.capacity_ * sizeof(storage), alignarg<storage>);
  }

  inline void transfer_to_heap(size_type nb, size_type cap)
  {
    heap_storage copy;
    copy.capacity_ = cap;
    copy.pdata_    = acl::allocate<storage>(*this, cap * sizeof(storage), alignarg<storage>);
    auto d         = (Ty*)copy.pdata_;
    auto ldata     = (Ty*)data_store_.ldata_.data();

    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
      std::memcpy(d, ldata, sizeof(storage) * nb);
    else
    {
      std::uninitialized_move(ldata, ldata + nb, d);
    }
    if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
    {
      std::destroy_n(ldata, size_);
    }
    data_store_.hdata_ = copy;
  }

  inline size_type insert_hole(const_iterator it, size_type n = 1) noexcept
  {
    size_type p   = static_cast<size_type>(std::distance<const Ty*>(get_data(), it));
    size_type nsz = size_ + n;

    if (capacity() < nsz)
    {
      unchecked_reserve_in_heap(size_ + std::max(size_ >> 1, n), p, n);
    }
    else
    {
      pointer src = const_cast<iterator>(it);
      if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
        std::memmove(src + n, src, (size_ - p) * sizeof(Ty));
      else
      {
        std::move(src, src + (size_ - p), src + n);
        if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
          std::destroy_n(src, n);
      }
    }
    size_ = nsz;
    return p;
  }

  inline small_vector& assign_copy(const small_vector& x, std::false_type) noexcept
  {
    clear();
    if (capacity() < x.size())
    {
      unchecked_reserve_in_heap(x.size());
    }

    size_ = x.size_;
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
      std::memcpy(data(), x.data(), size_ * sizeof(Ty));
    else
      std::uninitialized_copy(x.begin(), x.end(), begin());
    return *this;
  }

  inline small_vector& assign_copy(const small_vector& x, std::true_type) noexcept
  {
    clear();
    Allocator::operator=(static_cast<const Allocator&>(x));
    if (capacity() < x.size())
    {
      unchecked_reserve_in_heap(x.size());
    }

    size_ = x.size_;
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
      std::memcpy(data(), x.data(), size_ * sizeof(Ty));
    else
      std::uninitialized_copy(x.begin(), x.end(), begin());

    return *this;
  }

  inline small_vector& assign_move(small_vector&& x, std::false_type) noexcept
  {
    if (allocator_is_always_equal::value || static_cast<const Allocator&>(x) == static_cast<const Allocator&>(*this))
    {
      clear();
      if (x.is_inlined())
      {
        size_ = x.size_;
        if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
          std::memcpy(data(), x.data(), x.size() * sizeof(storage));
        else
          std::uninitialized_move(x.begin(), x.end(), begin());
      }
      else
      {
        data_store_.hdata_   = x.data_store_.hdata_;
        x.data_store_.hdata_ = {};
      }
      x.size_ = 0;
    }
    else
      assign_copy(x, std::false_type());
    return *this;
  }

  inline small_vector& assign_move(small_vector&& x, std::true_type) noexcept
  {
    clear();

    Allocator::operator=(std::move(static_cast<Allocator&>(x)));
    if (x.is_inlined())
    {
      size_ = x.size_;
      if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
        std::memcpy(data(), x.data(), x.size() * sizeof(storage));
      else
        std::uninitialized_move(x.begin(), x.end(), begin());
    }
    else
    {
      data_store_.hdata_   = x.data_store_.hdata_;
      x.data_store_.hdata_ = {};
    }
    x.size_ = 0;
    return *this;
  }

  template <typename InputIt>
  inline void copy_construct(InputIt first, InputIt last, pointer dest) noexcept
  {
    if constexpr (std::is_same<pointer, InputIt>::value && (has_pod || std::is_trivially_copyable_v<Ty>))
    {
      std::memcpy(dest, first, static_cast<std::size_t>(std::distance(first, last)) * sizeof(Ty));
    }
    else
    {
      std::uninitialized_copy(first, last, dest);
    }
  }

  void swap(small_vector& x, std::false_type) noexcept
  {
    static_assert(std::is_trivially_copyable_v<Ty>);
    std::swap(x.data_store_.ldata_, data_store_.ldata_);
    std::swap(size_, x.size_);
  }

  void swap(small_vector& x, std::true_type) noexcept
  {
    static_assert(std::is_trivially_copyable_v<Ty>);
    std::swap(x.data_store_.ldata_, data_store_.ldata_);
    std::swap(size_, x.size_);
    std::swap<Allocator>(this, x);
  }

  friend void swap(small_vector& lhs, small_vector& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  friend bool operator==(const small_vector& x, const small_vector& y) noexcept
  {
    return x.size_ == y.size_ && std::equal(x.begin(), x.end(), y.begin());
  }

  friend bool operator<(const small_vector& x, const small_vector& y) noexcept
  {
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }

  friend bool operator!=(const small_vector& x, const small_vector& y) noexcept
  {
    return !(x == y);
  }

  friend bool operator>(const small_vector& x, const small_vector& y) noexcept
  {
    return std::lexicographical_compare(y.begin(), y.end(), x.begin(), x.end());
  }

  friend bool operator>=(const small_vector& x, const small_vector& y) noexcept
  {
    return !std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }

  friend bool operator<=(const small_vector& x, const small_vector& y) noexcept
  {
    return !std::lexicographical_compare(y.begin(), y.end(), x.begin(), x.end());
  }

  template <class InputIterator>
  inline iterator insert_range(const_iterator position, InputIterator first, InputIterator last,
                               std::false_type) noexcept
  {
    size_type p     = insert_hole(position, static_cast<size_type>(std::distance(first, last)));
    auto      ldata = data();
    std::uninitialized_copy(first, last, ldata + p);
    return ldata + p;
  }

  inline iterator insert_range(const_iterator position, size_type n, const Ty& x, std::true_type) noexcept
  {
    size_type p     = insert_hole(position, n);
    auto      ldata = data();
    std::uninitialized_fill_n(ldata + p, n, x);
    return ldata + p;
  }

  template <class InputIterator>
  inline void construct_from_range(InputIterator first, InputIterator last, std::false_type) noexcept
  {
    resize_fill(static_cast<size_type>(std::distance(first, last)), std::false_type{});
    std::uninitialized_copy(first, last, data());
  }

  inline void construct_from_range(size_type n, const Ty& value, std::true_type) noexcept
  {
    resize_fill(n, std::false_type{});
    std::uninitialized_fill_n(data(), n, value);
  }

  data_store data_store_;
  size_type  size_ = 0;
};
} // namespace acl
