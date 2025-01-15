#pragma once
#include <acl/allocators/allocator.hpp>
#include <acl/allocators/default_allocator.hpp>
#include <acl/allocators/detail/custom_allocator.hpp>
#include <acl/utility/type_traits.hpp>
#include <acl/utility/utils.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>

namespace acl
{

template <typename Tuple, typename Config = acl::default_config<Tuple>>
class soavector : public acl::detail::custom_allocator_t<Config>
{

public:
  using allocator_type = acl::detail::custom_allocator_t<Config>;
  using tuple_type     = Tuple;
  using array_type     = acl::detail::tuple_of_ptrs<Tuple>;
  using this_type      = soavector<Tuple, Config>;

  using size_type                 = acl::detail::choose_size_t<uint32_t, Config>;
  using difference_type           = std::make_signed_t<size_type>;
  using allocator_tag             = typename allocator_type::tag;
  using allocator_is_always_equal = typename acl::allocator_traits<allocator_tag>::is_always_equal;
  using propagate_allocator_on_move =
   typename acl::allocator_traits<allocator_tag>::propagate_on_container_move_assignment;
  using propagate_allocator_on_copy =
   typename acl::allocator_traits<allocator_tag>::propagate_on_container_copy_assignment;
  using propagate_allocator_on_swap = typename acl::allocator_traits<allocator_tag>::propagate_on_container_swap;
  // using visualizer                  = acl::detail::tuple_array_visualizer<this_type, Tuple>;

  template <std::size_t I>
  using ivalue_type = std::tuple_element_t<I, tuple_type>;
  template <std::size_t I>
  using ireference = ivalue_type<I>&;
  template <std::size_t I>
  using iconst_reference = const ivalue_type<I>&;
  template <std::size_t I>
  using ipointer = ivalue_type<I>*;
  template <std::size_t I>
  using iconst_pointer = ivalue_type<I> const*;
  template <std::size_t I>
  using iiterator = ivalue_type<I>*;
  template <std::size_t I>
  using iconst_iterator = ivalue_type<I> const*;
  template <std::size_t I>
  using ireverse_iterator = std::reverse_iterator<iiterator<I>>;
  template <std::size_t I>
  using iconst_reverse_iterator = std::reverse_iterator<iconst_iterator<I>>;

  static auto constexpr index_seq = std::make_index_sequence<std::tuple_size_v<Tuple>>();

  template <bool const IsConst>
  class base_iterator
  {
  public:
    using value_type      = Tuple;
    using difference_type = std::make_signed_t<size_type>;
    using pointer =
     std::conditional_t<IsConst, acl::detail::tuple_of_cptrs<value_type>, acl::detail::tuple_of_ptrs<value_type>>;
    using reference =
     std::conditional_t<IsConst, acl::detail::tuple_of_crefs<value_type>, acl::detail::tuple_of_refs<value_type>>;
    using const_reference = acl::detail::tuple_of_crefs<value_type>;

    base_iterator(base_iterator const&) noexcept = default;
    base_iterator(base_iterator&&) noexcept      = default;
    base_iterator() noexcept                     = default;
    base_iterator(pointer init) noexcept : pointers_(init) {}
    ~base_iterator() noexcept = default;

    auto operator=(base_iterator const&) noexcept -> base_iterator& = default;
    auto operator=(base_iterator&&) noexcept -> base_iterator&      = default;
    auto operator<=>(base_iterator const&) const noexcept           = default;

    auto operator*() const noexcept -> reference
    {
      return get(index_seq);
    }

    auto operator->() const noexcept -> pointer
    {
      return pointers_;
    }

    auto operator++() noexcept -> auto&
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

    auto operator--() noexcept -> auto&
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

    auto operator+(difference_type n) const noexcept -> base_iterator
    {
      return add(index_seq, n);
    }

    auto operator-(difference_type n) const noexcept -> base_iterator
    {
      return add(index_seq, -n);
    }

    auto operator+=(difference_type n) const noexcept -> base_iterator&
    {
      *this = base_iterator<IsConst>(add(index_seq, n));
      return *this;
    }

    auto operator-=(difference_type n) const noexcept -> base_iterator&
    {
      *this = base_iterator<IsConst>(add(index_seq, -n));
      return *this;
    }

    friend auto operator-(base_iterator const& first, base_iterator const& second) noexcept -> difference_type
    {
      return static_cast<difference_type>(std::distance(std::get<0>(second.pointers_), std::get<0>(first.pointers_)));
    }

  private:
    template <std::size_t... I>
    auto get(std::index_sequence<I...> /*unused*/) const noexcept -> reference
    {
      return reference(*std::get<I>(pointers_)...);
    }

    template <std::size_t... I>
    auto add(std::index_sequence<I...> /*unused*/, difference_type n) const noexcept
    {
      return pointer((std::get<I>(pointers_) + n)...);
    }

    template <std::size_t... I>
    void advance(std::index_sequence<I...> /*unused*/) noexcept
    {
      (std::get<I>(pointers_)++, ...);
    }

    template <std::size_t... I>
    void retreat(std::index_sequence<I...> /*unused*/) noexcept
    {
      (std::get<I>(pointers_)--, ...);
    }

    template <std::size_t... I>
    auto next(std::index_sequence<I...> /*unused*/) const noexcept -> reference
    {
      return reference((*std::get<I>(pointers_) + 1)...);
    }

    template <std::size_t... I>
    auto prev(std::index_sequence<I...> /*unused*/) const noexcept -> reference
    {
      return reference((*std::get<I>(pointers_) - 1)...);
    }

    pointer pointers_ = {};
  };

  using iterator               = base_iterator<false>;
  using const_iterator         = base_iterator<true>;
  using value_type             = Tuple;
  using reference              = acl::detail::tuple_of_refs<value_type>;
  using const_reference        = acl::detail::tuple_of_crefs<value_type>;
  using pointer                = acl::detail::tuple_of_ptrs<value_type>;
  using const_pointer          = acl::detail::tuple_of_cptrs<value_type>;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  explicit soavector(allocator_type const& alloc = allocator_type()) noexcept
      : allocator_type(alloc), size_(0), capacity_(0) {};

  explicit soavector(size_type n) noexcept : data_(allocate(n)), size_(n), capacity_(n) {}

  soavector(size_type n, tuple_type const& value, allocator_type const& alloc = allocator_type()) noexcept
      : allocator_type(alloc), data_(allocate(n)), size_(n), capacity_(n)
  {
    uninitialized_fill(0, n, value, index_seq);
  }

  template <class InputIterator>
  soavector(InputIterator first, InputIterator last, allocator_type const& alloc = allocator_type()) noexcept
      : allocator_type(alloc)
  {
    construct_from_range(first, last);
  }

  soavector(soavector&& x, const allocator_type& alloc) noexcept
      : allocator_type(alloc), data_(std::move(x.data_)), size_(x.size_), capacity_(x.capacity_)
  {
    std::memset(&x, 0, sizeof(x));
  };

  soavector(soavector const& x, allocator_type const& alloc) noexcept
      : allocator_type(alloc), data_(allocate(x.capacity_)), size_(x.size_), capacity_(x.capacity_)
  {
    construct_list(x, index_seq);
  }

  soavector(soavector&& x) noexcept : soavector(std::move(x), (allocator_type const&)x) {};

  soavector(soavector const& x) noexcept : soavector(x, (allocator_type const&)x) {}

  soavector(std::initializer_list<tuple_type> x, const allocator_type& alloc = allocator_type()) noexcept
      : soavector(std::begin(x), std::end(x), alloc)
  {}

  ~soavector() noexcept
  {
    // acl::detail::do_not_optimize((visualizer*)this);
    destroy_and_deallocate();
  }

  auto operator=(const soavector& x) noexcept -> soavector&
  {
    assign_copy(x, propagate_allocator_on_copy(), index_seq);
    return *this;
  }

  auto operator=(soavector&& x) noexcept -> soavector&
  {
    assign_move(std::move(x), propagate_allocator_on_move(), index_seq);
    return *this;
  }

  auto operator=(std::initializer_list<tuple_type> x) noexcept -> soavector&
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
    auto s = static_cast<size_type>(std::distance(first, last));
    if (capacity_ < s)
    {
      capacity_ = s;
      deallocate();
      data_ = allocate(s);
    }
    size_ = s;
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
    size_ = n;
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

  auto get_allocator() const noexcept -> allocator_type
  {
    return *this;
  }

  // iterators:
  template <std::size_t I>
  auto ibegin() noexcept -> iiterator<I>
  {
    return std::get<I>(data_);
  }

  template <std::size_t I>
  auto ibegin() const noexcept -> iconst_iterator<I>
  {
    return std::get<I>(data_);
  }

  template <std::size_t I>
  auto iend() noexcept -> iiterator<I>
  {
    return std::get<I>(data_) + size_;
  }

  template <std::size_t I>
  auto iend() const noexcept -> iconst_iterator<I>
  {
    return std::get<I>(data_) + size_;
  }

  template <std::size_t I>
  auto irbegin() noexcept -> ireverse_iterator<I>
  {
    return ireverse_iterator<I>(iend<I>());
  }

  template <std::size_t I>
  auto irbegin() const noexcept -> iconst_reverse_iterator<I>
  {
    return iconst_reverse_iterator<I>(iend<I>());
  }

  template <std::size_t I>
  auto irend() noexcept -> ireverse_iterator<I>
  {
    return ireverse_iterator<I>(ibegin<I>());
  }

  template <std::size_t I>
  auto irend() const noexcept -> iconst_reverse_iterator<I>
  {
    return iconst_reverse_iterator<I>(ibegin<I>());
  }

  template <std::size_t I>
  auto icbegin() const noexcept -> iconst_iterator<I>
  {
    return ibegin<I>();
  }

  template <std::size_t I>
  auto icend() const noexcept -> iconst_iterator<I>
  {
    return iend<I>();
  }

  template <std::size_t I>
  auto icrbegin() const noexcept -> iconst_reverse_iterator<I>
  {
    return irbegin<I>();
  }

  template <std::size_t I>
  auto icrend() const noexcept -> iconst_reverse_iterator<I>
  {
    return irend<I>();
  }

  // iterators:
  auto begin() noexcept -> iterator
  {
    return iterator(data_);
  }

  auto begin() const noexcept -> const_iterator
  {
    return const_iterator(data_);
  }

  auto end() noexcept -> iterator
  {
    return begin() + static_cast<difference_type>(size_);
  }

  auto end() const noexcept -> const_iterator
  {
    return begin() + static_cast<difference_type>(size_);
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
    {
      destroy_all(sz, size_ - sz, index_seq);
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

  auto operator[](size_type n) noexcept -> reference
  {
    return get<reference>(index_seq, n);
  }

  auto operator[](size_type n) const noexcept -> const_reference
  {
    return get<const_reference>(index_seq, n);
  }

  auto at(size_type n) noexcept -> reference
  {
    return get<reference>(index_seq, n);
  }

  auto at(size_type n) const noexcept -> const_reference
  {
    return get<const_reference>(index_seq, n);
  }

  auto front() noexcept -> reference
  {
    assert(!empty());
    return get<reference>(index_seq, 0);
  }

  auto front() const noexcept -> const_reference
  {
    assert(!empty());
    return get<const_reference>(index_seq, 0);
  }

  auto back() noexcept -> reference
  {
    assert(!empty());
    return get<reference>(index_seq, size_ - 1);
  }

  auto back() const noexcept -> const_reference
  {
    assert(!empty());
    return get<const_reference>(index_seq, size_ - 1);
  }

  template <std::size_t I>
  auto at(size_type n) noexcept -> ireference<I>
  {
    assert(n < size_);
    return std::get<I>(data_)[n];
  }

  template <std::size_t I>
  auto at(size_type n) const noexcept -> iconst_reference<I>
  {
    assert(n < size_);
    return std::get<I>(data_)[n];
  }

  template <std::size_t I>
  auto front() noexcept -> ireference<I>
  {
    assert(0 < size_);
    return std::get<I>(data_)[0];
  }

  template <std::size_t I>
  auto front() const noexcept -> iconst_reference<I>
  {
    assert(0 < size_);
    return std::get<I>(data_)[0];
  }

  template <std::size_t I>
  auto back() noexcept -> ireference<I>
  {
    assert(0 < size_);
    return std::get<I>(data_)[size_ - 1];
  }

  template <std::size_t I>
  auto back() const noexcept -> iconst_reference<I>
  {
    assert(0 < size_);
    return std::get<I>(data_)[size_ - 1];
  }

  // data access
  template <std::size_t I>
  auto data() noexcept -> auto*
  {
    return std::get<I>(data_);
  }

  template <std::size_t I>
  auto data() const noexcept -> const auto*
  {
    return std::get<I>(data_);
  }

  // modifiers:
  template <typename... Args>
  void emplace_back(Args&&... args) noexcept
  {
    if (capacity_ < size_ + 1)
    {
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
    }

    construct_at(size_++, index_seq, std::forward<Args>(args)...);
  }

  void push_back(const tuple_type& x) noexcept
  {
    if (capacity_ < size_ + 1)
    {
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
    }

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
    {
      std::memmove(data + first, data + last, (size_ - last) * sizeof(Ty));
    }
    else
    {
      while (last < size_)
      {
        data[first++] = std::move(data[last++]);
      }

      if constexpr (!std::is_trivially_destructible_v<Ty>)
      {
        for (; first < size_; ++first)
        {
          std::destroy_at(&data[first++]);
        }
      }
    }
  }

  template <std::size_t... I>
  void erase_at(size_type position, std::index_sequence<I...> /*unused*/) noexcept
  {
    (erase_at(std::get<I>(data_), position, position + 1), ...);
  }

  template <std::size_t... I>
  void erase_at(size_type first, size_type last, std::index_sequence<I...> /*unused*/) noexcept
  {
    (erase_at(std::get<I>(data_), first, last), ...);
  }

  auto erase(size_type position) noexcept -> size_type
  {
    assert(position < size());
    erase_at(position, index_seq);
    size_--;
    return position;
  }

  auto erase(size_type first, size_type last) noexcept -> size_type
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
  void uninitialized_fill(size_type start, size_type count, tuple_type const& t,
                          std::index_sequence<I...> /*unused*/) noexcept
  {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    (std::uninitialized_fill_n(std::get<I>(data_) + start, count, std::get<I>(t)), ...);
  }

  template <typename It, typename Arg>
  void copy_fill_n(It it, It end_it, size_type count, Arg&& arg) noexcept
  {
    for (; it != end_it; ++it)
    {
      *it = std::forward<Arg>(arg);
    }
  }

  template <std::size_t... I>
  void copy_fill(size_type start, size_type count, tuple_type const& t, std::index_sequence<I...> /*unused*/) noexcept
  {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    (copy_fill_n(std::get<I>(data_) + start, count, std::get<I>(t)), ...);
  }

  template <std::size_t... I, typename... Args>
  void construct_at(size_type i, std::index_sequence<I...> /*unused*/, Args&&... args) noexcept
  {
    if constexpr (sizeof...(Args) != 0)
    {
      (std::construct_at(std::get<I>(data_) + i, std::forward<Args>(args)), ...);
    }
    else
    {
      (std::construct_at(std::get<I>(data_) + i), ...);
    }
  }

  template <std::size_t... I>
  void construct_tuple_at(size_type i, std::index_sequence<I...> /*unused*/, tuple_type const& arg) noexcept
  {
    (std::construct_at(std::get<I>(data_) + i, std::get<I>(arg)), ...);
  }

  template <std::size_t... I>
  void construct_tuple_at(size_type i, std::index_sequence<I...> /*unused*/, tuple_type&& arg) noexcept
  {
    (std::construct_at(std::get<I>(data_) + i, std::move(std::get<I>(arg))), ...);
  }

  template <typename T, typename Arg>
  void move_at(T* data, Arg&& arg) noexcept
  {
    *data = std::forward<Arg>(arg);
  }

  template <std::size_t... I, typename... Args>
  void move_at(size_type i, std::index_sequence<I...> /*unused*/, Args&&... args) noexcept
  {
    (move_at(std::get<I>(data_) + i, std::forward<Args>(args)), ...);
  }

  template <std::size_t... I>
  void move_tuple_at(size_type i, std::index_sequence<I...> /*unused*/, tuple_type const& arg) noexcept
  {
    (move_at(std::get<I>(data_) + i, std::get<I>(arg)), ...);
  }

  template <typename Ty>
  void move_construct(Ty* to, Ty* from, size_type n) noexcept
  {
    for (size_type i = 0; i < n; ++i)
    {
      std::construct_at(to + i, std::move(from[i]));
    }
  }

  template <std::size_t... I>
  void move_construct(array_type& dst, size_type dst_offset, array_type& src, size_type src_offset, size_type n,
                      std::index_sequence<I...> /*unused*/) noexcept
  {
    (move_construct(std::get<I>(dst) + dst_offset, std::get<I>(src) + src_offset, n), ...);
  }

  template <typename Ty>
  void copy_assign(Ty* to, Ty const* from, size_type n) noexcept
  {
    if constexpr (std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(to, from, n * sizeof(Ty));
    }
    else
    {
      std::copy(from, from + n, to);
    }
  }

  template <std::size_t... I>
  void copy(array_type& dst, array_type const& src, size_type n, std::index_sequence<I...> /*unused*/) noexcept
  {
    (copy_assign(std::get<I>(dst), std::get<I>(src), n), ...);
  }

  template <typename T>
  void construct_list(T* data, size_type sz, T const* src) noexcept
  {
    if constexpr (std::is_trivially_copyable_v<T>)
    {
      std::memcpy(data, src, sz * sizeof(T));
    }
    else
    {
      for (size_type i = 0; i < sz; ++i)
      {
        std::construct_at(data + i, *(src + i));
      }
    }
  }

  template <typename T>
  void construct_single(T& data, T const& src) noexcept
  {
    if constexpr (std::is_trivially_copyable_v<T>)
    {
      std::memcpy(&data, &src, sizeof(T));
    }
    else
    {
      std::construct_at(&data, src);
    }
  }

  template <std::size_t... I>
  void construct_list(soavector const& x, std::index_sequence<I...> /*unused*/) noexcept
  {
    (construct_list(std::get<I>(data_), size_, std::get<I>(x.data_)), ...);
  }

  template <typename InputIterator, std::size_t... I>
  static void construct_range(InputIterator start, InputIterator end, array_type& store, size_type offset,
                              std::index_sequence<I...> /*unused*/) noexcept
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
    {
      std::for_each(data, data + size,
                    [](T& d)
                    {
                      std::destroy_at(&d);
                    });
    }
  }

  template <typename T>
  static void destroy_single(T* data) noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
      std::destroy_at(data);
    }
  }

  template <std::size_t... I>
  void destroy_all(size_type start, size_type count, std::index_sequence<I...> /*unused*/) noexcept
  {
    (destroy_seq(std::get<I>(data_) + start, count), ...);
  }

  template <std::size_t... I>
  void destroy_at(size_type at, std::index_sequence<I...> /*unused*/) noexcept
  {
    (destroy_single(std::get<I>(data_) + at), ...);
  }

  void destroy_and_deallocate() noexcept
  {
    destroy_all(0, size_, index_seq);
    deallocate();
  }

  template <typename Ty>
  void memmove(Ty* dst, Ty* src, size_type n)
  {
    if constexpr (std::is_trivially_copyable_v<Ty>)
    {
      std::memmove(dst, src, (n * sizeof(Ty)));
    }
    else
    {
      move_construct(dst, src, n);
    }
  }

  template <std::size_t... I>
  void memmove(size_type to, size_type from, size_type n, std::index_sequence<I...> /*unused*/)
  {
    (memmove(std::get<I>(data_) + to, std::get<I>(data_) + from, n), ...);
  }

  auto insert_hole(size_type p, size_type n = 1) noexcept -> size_type
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

  auto assign_copy(soavector const& x, std::false_type /*unused*/) noexcept -> soavector&
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

  auto assign_copy(soavector const& x, std::true_type /*unused*/) noexcept -> soavector&
  {
    if (allocator_is_always_equal::value ||
        static_cast<const allocator_type&>(x) == static_cast<const allocator_type&>(*this))
    {
      assign(x, std::false_type());
    }
    else
    {
      destroy_and_deallocate();
      allocator_type::operator=(static_cast<const allocator_type&>(x));
      data_ = allocate(x.size_);
      size_ = capacity_ = x.size_;
      construct_list(x, index_seq);
    }
    return *this;
  }

  auto assign_move(soavector& x, std::false_type /*unused*/) noexcept -> soavector&
  {
    if (allocator_is_always_equal::value ||
        static_cast<const allocator_type&>(x) == static_cast<const allocator_type&>(*this))
    {
      destroy_and_deallocate();
      data_       = x.data_;
      size_       = x.size_;
      capacity_   = x.capacity_;
      x.data_     = {};
      x.capacity_ = x.size_ = 0;
    }
    else
    {
      assign_copy(x, std::false_type());
    }
    return *this;
  }

  auto assign_move(soavector& x, std::true_type /*unused*/) noexcept -> soavector&
  {
    destroy_and_deallocate();
    allocator_type::operator=(std::move(static_cast<allocator_type&>(x)));
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
    out_ref = acl::allocate<Ty>(static_cast<allocator_type&>(*this), n * sizeof(Ty));
  }

  template <std::size_t... I>
  auto allocate(size_type n, std::index_sequence<I...> /*unused*/) noexcept -> array_type
  {
    array_type r;
    (allocate(std::get<I>(r), n), ...);
    return r;
  }

  auto allocate(size_type n) noexcept -> array_type
  {
    return allocate(n, index_seq);
  }

  template <typename Ty>
  void deallocate(Ty* out_ref, size_type n) noexcept
  {
    acl::deallocate(static_cast<allocator_type&>(*this), out_ref, n * sizeof(Ty));
  }

  template <std::size_t... I>
  void deallocate(size_type n, std::index_sequence<I...> /*unused*/) noexcept
  {
    (deallocate(std::get<I>(data_), n), ...);
  }

  void deallocate() noexcept
  {
    deallocate(capacity_, index_seq);
  }

  void unchecked_reserve(size_type n) noexcept
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

  void unchecked_reserve(size_type n, size_type at, size_type holes) noexcept
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

  void swap(soavector& x, std::false_type /*unused*/) noexcept
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
  }

  void swap(soavector& x, std::true_type /*unused*/) noexcept
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
    std::swap<allocator_type>(this, x);
  }

  friend void swap(soavector& lhs, soavector& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  template <typename Ty>
  static auto equals(Ty const* first, Ty const* second, size_type n) noexcept -> bool
  {
    for (size_type i = 0; i < n; ++i)
    {
      if (first[i] != second[i])
      {
        return false;
      }
    }
    return true;
  }

  template <std::size_t... I>
  static auto equals(array_type const& first, array_type const& second, size_type n,
                     std::index_sequence<I...> /*unused*/) noexcept -> bool
  {
    return (equals(std::get<I>(first), std::get<I>(second), n) && ...);
  }

  /**
   * Less
   */
  template <typename Ty>
  static auto less(Ty const* first, Ty const* second, size_type n) noexcept -> bool
  {
    for (size_type i = 0; i < n; ++i)
    {
      if (first[i] >= second[i])
      {
        return false;
      }
    }
    return true;
  }

  template <std::size_t... I>
  static auto less(array_type const& first, array_type const& second, size_type n,
                   std::index_sequence<I...> /*unused*/) noexcept -> bool
  {
    return (less(std::get<I>(first), std::get<I>(second), n) && ...);
  }

  template <typename Ty>
  static auto lesseq(Ty const* first, Ty const* second, size_type n) noexcept -> bool
  {
    for (size_type i = 0; i < n; ++i)
    {
      if (first[i] > second[i])
      {
        return false;
      }
    }
    return true;
  }

  template <std::size_t... I>
  static auto lesseq(array_type const& first, array_type const& second, size_type n,
                     std::index_sequence<I...> /*unused*/) noexcept -> bool
  {
    return (lesseq(std::get<I>(first), std::get<I>(second), n) && ...);
  }
  /**
   *
   */

  friend auto operator==(soavector const& x, soavector const& y) noexcept -> bool
  {
    return x.size_ == y.size_ && equals(x.data_, y.data_, x.size_, index_seq);
  }

  friend auto operator<(soavector const& x, soavector const& y) noexcept -> bool
  {
    auto n = std::min(x.size(), y.size());
    return (less(x, y, n, index_seq) | (x.size_ < y.size_));
  }

  friend auto operator<=(soavector const& x, soavector const& y) noexcept -> bool
  {
    auto n = std::min(x.size(), y.size());
    return (lesseq(x, y, n, index_seq) | (x.size_ <= y.size_));
  }

  friend auto operator!=(soavector const& x, soavector const& y) noexcept -> bool
  {
    return !(x == y);
  }

  friend auto operator>(soavector const& x, soavector const& y) noexcept -> bool
  {
    return y < x;
  }

  friend auto operator>=(soavector const& x, soavector const& y) noexcept -> bool
  {
    return y <= x;
  }

  template <class InputIterator>
  auto insert_range(size_type position, InputIterator first, InputIterator last) noexcept -> size_type
  {
    size_type p = insert_hole(position, static_cast<size_type>(std::distance(first, last)));
    construct_range(first, last, data_, p, index_seq);
    return p;
  }

  auto insert_range(size_type position, size_type n, tuple_type const& x) noexcept -> size_type
  {
    size_type p = insert_hole(position, n);
    uninitialized_fill(p, n, x, index_seq);
    return p;
  }

  template <typename InputIterator>
  void construct_from_range(InputIterator first, InputIterator last) noexcept
  {
    size_     = static_cast<size_type>(std::distance(first, last));
    data_     = allocate(size_);
    capacity_ = size_;
    construct_range(first, last, data_, 0, index_seq);
  }

  template <typename Reftype, std::size_t... I>
  auto get(std::index_sequence<I...> /*unused*/) const noexcept -> Reftype
  {
    return Reftype(*std::get<I>(data_)...);
  }

  template <typename Reftype, std::size_t... I>
  auto get(std::index_sequence<I...> /*unused*/, size_type i) const noexcept -> Reftype
  {
    return Reftype(*(std::get<I>(data_) + i)...);
  }

  array_type data_{};
  size_type  size_     = 0;
  size_type  capacity_ = 0;
};

} // namespace acl
