// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/allocators/allocator.hpp"
#include "ouly/allocators/default_allocator.hpp"
#include "ouly/allocators/detail/custom_allocator.hpp"
#include "ouly/reflection/detail/base_concepts.hpp"
#include "ouly/reflection/detail/field_helpers.hpp"
#include "ouly/utility/type_traits.hpp"
#include "ouly/utility/utils.hpp"
#include "ouly/utility/user_config.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>

namespace ouly
{

template <ouly::detail::Aggregate Agg, typename Config = ouly::default_config<Agg>>
class soavector : public ouly::detail::custom_allocator_t<Config>
{

public:
  using value_type = Agg;

  using allocator_type = ouly::detail::custom_allocator_t<Config>;
  using tuple_type     = ouly::detail::field_types<Agg>;
  using array_type     = ouly::detail::field_ptr_types<Agg>;
  using carray_type    = ouly::detail::field_cptr_types<Agg>;
  using this_type      = soavector<Agg, Config>;

  static constexpr uint32_t field_count = std::tuple_size_v<tuple_type>;

  using size_type                 = ouly::detail::choose_size_t<uint32_t, Config>;
  using difference_type           = std::make_signed_t<size_type>;
  using allocator_tag             = typename allocator_type::tag;
  using allocator_is_always_equal = typename ouly::allocator_traits<allocator_tag>::is_always_equal;
  using propagate_allocator_on_move =
   typename ouly::allocator_traits<allocator_tag>::propagate_on_container_move_assignment;
  using propagate_allocator_on_copy =
   typename ouly::allocator_traits<allocator_tag>::propagate_on_container_copy_assignment;
  using propagate_allocator_on_swap = typename ouly::allocator_traits<allocator_tag>::propagate_on_container_swap;
  // using visualizer                  = ouly::detail::tuple_array_visualizer<this_type, Agg>;

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

  static auto constexpr index_seq = std::make_index_sequence<field_count>();

  template <bool const IsConst>
  class value_wrapper
  {
  public:
    using pointer =
     std::conditional_t<IsConst, ouly::detail::tuple_of_cptrs<tuple_type>, ouly::detail::tuple_of_ptrs<tuple_type>>;
    using const_reference = ouly::detail::tuple_of_crefs<tuple_type>;

    value_wrapper(auto... args) : pointer_{args...} {}
    value_wrapper(pointer pointer_) : pointer_(pointer_) {}

    auto operator=(value_type const& value) -> value_wrapper&
    {
      [&]<std::size_t... I>(std::index_sequence<I...>)
      {
        ((std::get<I>(pointer_) = get_ref<I>(value)), ...);
      }(std::make_index_sequence<field_count>());
      return *this;
    }

    operator value_type() const
    {
      return std::apply(
       [&](auto... arg)
       {
         return value_type{*arg...};
       },
       pointer_);
    }

    auto get() const -> value_type
    {
      return static_cast<value_type>(*this);
    }

  private:
    pointer pointer_ = {};
  };

  template <bool const IsConst>
  class base_iterator
  {
  public:
    using difference_type = std::make_signed_t<size_type>;
    using pointer =
     std::conditional_t<IsConst, ouly::detail::tuple_of_cptrs<tuple_type>, ouly::detail::tuple_of_ptrs<tuple_type>>;
    using reference =
     std::conditional_t<IsConst, ouly::detail::tuple_of_crefs<tuple_type>, ouly::detail::tuple_of_refs<tuple_type>>;
    using const_reference = ouly::detail::tuple_of_crefs<tuple_type>;

    base_iterator(base_iterator const&)     = default;
    base_iterator(base_iterator&&) noexcept = default;
    base_iterator()                         = default;
    base_iterator(pointer init) : pointers_(init) {}
    ~base_iterator() noexcept = default;

    auto operator=(base_iterator const&) -> base_iterator&     = default;
    auto operator=(base_iterator&&) noexcept -> base_iterator& = default;
    auto operator<=>(base_iterator const&) const               = default;

    auto operator*() const -> reference
    {
      return get(index_seq);
    }

    auto operator->() const -> pointer
    {
      return pointers_;
    }

    auto operator++() -> auto&
    {
      advance(index_seq);
      return *this;
    }

    auto operator++(int)
    {
      auto v = *this;
      advance(index_seq);
      return v;
    }

    auto operator--() -> auto&
    {
      retreat(index_seq);
      return *this;
    }

    auto operator--(int)
    {
      auto v = *this;
      retreat(index_seq);
      return v;
    }

    auto operator+(difference_type n) const -> base_iterator
    {
      return add(index_seq, n);
    }

    auto operator-(difference_type n) const -> base_iterator
    {
      return add(index_seq, -n);
    }

    auto operator+=(difference_type n) const -> base_iterator&
    {
      *this = base_iterator<IsConst>(add(index_seq, n));
      return *this;
    }

    auto operator-=(difference_type n) const -> base_iterator&
    {
      *this = base_iterator<IsConst>(add(index_seq, -n));
      return *this;
    }

    template <std::size_t I>
    auto get() const -> auto&
    {
      return *std::get<I>(pointers_);
    }

    friend auto operator-(base_iterator const& first, base_iterator const& second) -> difference_type
    {
      return static_cast<difference_type>(std::distance(std::get<0>(second.pointers_), std::get<0>(first.pointers_)));
    }

  private:
    template <std::size_t... I>
    auto get(std::index_sequence<I...> /*unused*/) const -> reference
    {
      return reference(*std::get<I>(pointers_)...);
    }

    template <std::size_t... I>
    auto add(std::index_sequence<I...> /*unused*/, difference_type n) const
    {
      return pointer((std::get<I>(pointers_) + n)...);
    }

    template <std::size_t... I>
    void advance(std::index_sequence<I...> /*unused*/)
    {
      (std::get<I>(pointers_)++, ...);
    }

    template <std::size_t... I>
    void retreat(std::index_sequence<I...> /*unused*/)
    {
      (std::get<I>(pointers_)--, ...);
    }

    template <std::size_t... I>
    auto next(std::index_sequence<I...> /*unused*/) const -> reference
    {
      return reference((*std::get<I>(pointers_) + 1)...);
    }

    template <std::size_t... I>
    auto prev(std::index_sequence<I...> /*unused*/) const -> reference
    {
      return reference((*std::get<I>(pointers_) - 1)...);
    }

    pointer pointers_ = {};
  };

  using iterator               = base_iterator<false>;
  using const_iterator         = base_iterator<true>;
  using reference              = value_wrapper<false>;
  using const_reference        = value_wrapper<true>;
  using pointer                = ouly::detail::tuple_of_ptrs<tuple_type>;
  using const_pointer          = ouly::detail::tuple_of_cptrs<tuple_type>;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  explicit soavector(allocator_type const& alloc = allocator_type()) : allocator_type(alloc), size_(0), capacity_(0) {};

  explicit soavector(size_type n) : data_(allocate(n)), size_(n), capacity_(n)
  {
    uninitialized_fill(0, n, value_type{}, index_seq);
  }

  soavector(size_type n, value_type const& value, allocator_type const& alloc = allocator_type())
      : allocator_type(alloc), data_(allocate(n)), size_(n), capacity_(n)
  {
    uninitialized_fill(0, n, value, index_seq);
  }

  template <class InputIterator>
  soavector(InputIterator first, InputIterator last, allocator_type const& alloc = allocator_type())
      : allocator_type(alloc)
  {
    construct_from_range(first, last);
  }

  soavector(soavector&& x, const allocator_type& alloc)
      : allocator_type(alloc), data_(std::move(x.data_)), size_(x.size_), capacity_(x.capacity_)
  {
    x.data_     = {};
    x.size_     = 0;
    x.capacity_ = 0;
  };

  soavector(soavector const& x, allocator_type const& alloc)
      : allocator_type(alloc), data_(allocate(x.capacity_)), size_(x.size_), capacity_(x.capacity_)
  {
    construct_list(x, index_seq);
  }

  soavector(soavector&& x) noexcept : soavector(std::move(x), (allocator_type const&)x) {};

  soavector(soavector const& x) : soavector(x, (allocator_type const&)x) {}

  soavector(std::initializer_list<value_type> x, const allocator_type& alloc = allocator_type())
      : soavector(std::begin(x), std::end(x), alloc)
  {}

  ~soavector() noexcept
  {
    // ouly::detail::do_not_optimize((visualizer*)this);
    destroy_and_deallocate();
  }

  auto operator=(const soavector& x) -> soavector&
  {
    if (this != &x)
    {
      assign_copy(x, propagate_allocator_on_copy());
    }
    return *this;
  }

  auto operator=(soavector&& x) noexcept -> soavector&
  {
    if (this == &x)
    {
      return *this;
    }
    assign_move(x, propagate_allocator_on_move());
    return *this;
  }

  auto operator=(std::initializer_list<value_type> x) -> soavector&
  {
    assign(x.begin(), x.end());
    return *this;
  }

  template <class InputIterator>
  void assign(InputIterator first, InputIterator last)
  {
    auto n = distance(first, last);

    if (capacity_ < n)
    {
      auto new_data = allocate(n);
      uninitialized_copy_range(first, last, new_data, 0, index_seq);
      destroy_all(0, size_, index_seq);
      deallocate();
      capacity_ = n;
      data_     = new_data;
    }
    else
    {
      copy_range(first, first + size_, data_, 0, index_seq);
      if (n < size_)
      {
        destroy_all(n, size_ - n, index_seq);
      }
      else
      {
        uninitialized_fill_n(data_, size_, n - size_, value_type{}, index_seq);
      }
    }
    size_ = n;
  }

  void assign(size_type n, const value_type& value)
  {
    if (capacity_ < n)
    {
      auto new_data = allocate(n);
      uninitialized_fill_n(new_data, 0, n, value, index_seq);
      destroy_all(0, size_, index_seq);
      deallocate();
      capacity_ = n;
      data_     = new_data;
    }
    else
    {
      fill_n(data_, 0, size_, value, index_seq);
      if (n < size_)
      {
        destroy_all(n, size_ - n, index_seq);
      }
      else
      {
        uninitialized_fill_n(data_, size_, n - size_, value, index_seq);
      }
    }
    size_ = n;
  }

  void assign(std::initializer_list<value_type> x)
  {
    assign(std::begin(x), std::end(x));
  }

  auto get_allocator() const -> allocator_type
  {
    return *this;
  }

  // iterators:
  template <std::size_t I>
  auto ibegin() -> iiterator<I>
  {
    return std::get<I>(data_);
  }

  template <std::size_t I>
  auto ibegin() const -> iconst_iterator<I>
  {
    return std::get<I>(data_);
  }

  template <std::size_t I>
  auto iend() -> iiterator<I>
  {
    return std::get<I>(data_) + size_;
  }

  template <std::size_t I>
  auto iend() const -> iconst_iterator<I>
  {
    return std::get<I>(data_) + size_;
  }

  template <std::size_t I>
  auto irbegin() -> ireverse_iterator<I>
  {
    return ireverse_iterator<I>(iend<I>());
  }

  template <std::size_t I>
  auto irbegin() const -> iconst_reverse_iterator<I>
  {
    return iconst_reverse_iterator<I>(iend<I>());
  }

  template <std::size_t I>
  auto irend() -> ireverse_iterator<I>
  {
    return ireverse_iterator<I>(ibegin<I>());
  }

  template <std::size_t I>
  auto irend() const -> iconst_reverse_iterator<I>
  {
    return iconst_reverse_iterator<I>(ibegin<I>());
  }

  template <std::size_t I>
  auto icbegin() const -> iconst_iterator<I>
  {
    return ibegin<I>();
  }

  template <std::size_t I>
  auto icend() const -> iconst_iterator<I>
  {
    return iend<I>();
  }

  template <std::size_t I>
  auto icrbegin() const -> iconst_reverse_iterator<I>
  {
    return irbegin<I>();
  }

  template <std::size_t I>
  auto icrend() const -> iconst_reverse_iterator<I>
  {
    return irend<I>();
  }

  // iterators:
  auto begin() -> iterator
  {
    return iterator(data_);
  }

  auto begin() const -> const_iterator
  {
    return const_iterator(data_);
  }

  auto end() -> iterator
  {
    return begin() + static_cast<difference_type>(size_);
  }

  auto end() const -> const_iterator
  {
    return begin() + static_cast<difference_type>(size_);
  }

  auto rbegin() -> reverse_iterator
  {
    return reverse_iterator(end());
  }

  auto rbegin() const -> const_reverse_iterator
  {
    return const_reverse_iterator(end());
  }

  auto rend() -> reverse_iterator
  {
    return reverse_iterator(begin());
  }

  auto rend() const -> const_reverse_iterator
  {
    return const_reverse_iterator(begin());
  }

  auto cbegin() const -> const_iterator
  {
    return begin();
  }

  auto cend() const -> const_iterator
  {
    return end();
  }

  auto crbegin() const -> const_reverse_iterator
  {
    return rbegin();
  }

  auto crend() const -> const_reverse_iterator
  {
    return rend();
  }

  // capacity:
  auto size() const -> size_type
  {
    return size_;
  }

  auto max_size() const -> size_type
  {
    return std::numeric_limits<size_type>::max();
  }

  void resize(size_type sz)
  {
    resize(sz, value_type());
  }

  void resize(size_type sz, value_type const& c)
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

  auto capacity() const -> size_type
  {
    return capacity_;
  }

  [[nodiscard]] auto empty() const -> bool
  {
    return size_ == 0;
  }

  void reserve(size_type n)
  {
    if (capacity_ < n)
    {
      unchecked_reserve(n);
    }
  }

  void shrink_to_fit()
  {
    if (capacity_ != size_)
    {
      unchecked_reserve(size_);
    }
  }

  auto operator[](size_type n) -> reference
  {
    return get<reference>(index_seq, n);
  }

  auto operator[](size_type n) const -> const_reference
  {
    return get<const_reference>(index_seq, n);
  }

  auto at(size_type n) -> reference
  {
    return get<reference>(index_seq, n);
  }

  auto at(size_type n) const -> const_reference
  {
    return get<const_reference>(index_seq, n);
  }

  auto front() -> reference
  {
    OULY_ASSERT(!empty());
    return get<reference>(index_seq, 0);
  }

  auto front() const -> const_reference
  {
    OULY_ASSERT(!empty());
    return get<const_reference>(index_seq, 0);
  }

  auto back() -> reference
  {
    OULY_ASSERT(!empty());
    return get<reference>(index_seq, size_ - 1);
  }

  auto back() const -> const_reference
  {
    OULY_ASSERT(!empty());
    return get<const_reference>(index_seq, size_ - 1);
  }

  template <std::size_t I>
  auto at(size_type n) -> ireference<I>
  {
    OULY_ASSERT(n < size_);
    return std::get<I>(data_)[n];
  }

  template <std::size_t I>
  auto at(size_type n) const -> iconst_reference<I>
  {
    OULY_ASSERT(n < size_);
    return std::get<I>(data_)[n];
  }

  template <std::size_t I>
  auto front() -> ireference<I>
  {
    OULY_ASSERT(0 < size_);
    return std::get<I>(data_)[0];
  }

  template <std::size_t I>
  auto front() const -> iconst_reference<I>
  {
    OULY_ASSERT(0 < size_);
    return std::get<I>(data_)[0];
  }

  template <std::size_t I>
  auto back() -> ireference<I>
  {
    OULY_ASSERT(0 < size_);
    return std::get<I>(data_)[size_ - 1];
  }

  template <std::size_t I>
  auto back() const -> iconst_reference<I>
  {
    OULY_ASSERT(0 < size_);
    return std::get<I>(data_)[size_ - 1];
  }

  // data access
  template <std::size_t I>
  auto data() -> auto*
  {
    return std::get<I>(data_);
  }

  template <std::size_t I>
  auto data() const -> const auto*
  {
    return std::get<I>(data_);
  }

  // modifiers:
  template <typename... Args>
  void emplace_back(Args&&... args)
  {
    if (capacity_ < size_ + 1)
    {
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
    }

    construct_at(size_++, index_seq, std::forward<Args>(args)...);
  }

  void push_back(const value_type& x)
  {
    if (capacity_ < size_ + 1)
    {
      unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
    }

    construct_tuple_at(size_++, index_seq, x);
  }

  void pop_back()
  {
    OULY_ASSERT(size_);
    destroy_at(--size_, index_seq);
  }

  template <typename... Args>
  void emplace(size_type position, Args&&... args)
  {
    size_type p = insert_hole(position);
    construct_at(p, index_seq, std::forward<Args>(args)...);
  }

  void insert(size_type position, value_type const& x)
  {
    size_type p = insert_hole(position);
    construct_tuple_at(p, index_seq, x);
  }

  void insert(size_type position, value_type&& x)
  {
    size_type p = insert_hole(position);
    construct_tuple_at(p, index_seq, std::move(x));
  }

  void insert(size_type position, size_type n, value_type const& x)
  {
    insert_range(position, n, x);
  }

  template <typename InputIterator>
  void insert(size_type position, InputIterator first, InputIterator last)
  {
    insert_range(position, first, last);
  }

  void insert(size_type position, std::initializer_list<value_type> x)
  {
    size_type p = insert_hole(position, static_cast<size_type>(x.size()));
    uninitialized_copy_range(std::begin(x), std::end(x), data_, p, index_seq);
  }

  template <typename Ty>
  void erase_at(Ty* data, size_type first, size_type last)
  {
    OULY_ASSERT(last < size());
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
  void erase_at(size_type position, std::index_sequence<I...> /*unused*/)
  {
    (erase_at(std::get<I>(data_), position, position + 1), ...);
  }

  template <std::size_t... I>
  void erase_at(size_type first, size_type last, std::index_sequence<I...> /*unused*/)
  {
    (erase_at(std::get<I>(data_), first, last), ...);
  }

  auto erase(size_type position) -> size_type
  {
    OULY_ASSERT(position < size());
    erase_at(position, index_seq);
    size_--;
    return position;
  }

  auto erase(size_type first, size_type last) -> size_type
  {
    OULY_ASSERT(last < size());
    erase_at(first, last, index_seq);
    size_ -= (last - first);
    return first;
  }

  void swap(soavector& x) noexcept
  {
    swap(x, propagate_allocator_on_swap());
  }

  void clear()
  {
    size_ = 0;
  }

private:
  template <typename T>
  static void copy(T* to, T const& from)
  {
    if constexpr (std::is_trivially_copyable_v<T>)
    {
      std::memcpy(to, &from, sizeof(T));
    }
    else
    {
      *to = from;
    }
  }

  template <std::size_t I, typename T>
  static auto get_ref(T&& tup) -> auto&
  {
    if constexpr (requires { ouly::detail::get_field_ref<I>(std::forward<T &&>(tup)); })
    {
      return ouly::detail::get_field_ref<I>(std::forward<T>(tup));
    }
    else
    {
      return std::get<I>(std::forward<T>(tup));
    }
  }

  template <std::size_t I, typename T>
  static auto get_from_iter(T& tup) -> auto&
  {
    if constexpr (requires { tup.template get<I>(); })
    {
      return tup.template get<I>();
    }
    else if constexpr (requires { ouly::detail::get_field_ref<I>(*tup); })
    {
      return ouly::detail::get_field_ref<I>(*tup);
    }
    else
    {
      return std::get<I>(tup);
    }
  }

  template <typename InputIterator, std::size_t... I>
  static void copy_range(InputIterator start, InputIterator end, array_type& store, size_type offset,
                         std::index_sequence<I...> /*unused*/)
  {
    while (start != end)
    {
      (copy(std::get<I>(store) + offset, get_from_iter<I>(start)), ...);

      start++;
      offset++;
    }
  }

  template <typename InputIterator, std::size_t... I>
  static void uninitialized_copy_range(InputIterator start, InputIterator end, array_type& store, size_type offset,
                                       std::index_sequence<I...> /*unused*/)
  {
    while (start != end)
    {
      (std::construct_at(std::get<I>(store) + offset, get_from_iter<I>(start)), ...);

      start++;
      offset++;
    }
  }

  template <std::size_t... I>
  static void fill_n(array_type& store, size_type start, size_type count, value_type const& t,
                     std::index_sequence<I...> /*unused*/)
  {
    (std::fill_n(std::get<I>(store) + start, count, get_ref<I>(t)), ...);
  }

  template <std::size_t... I>
  static void uninitialized_fill_n(array_type& store, size_type start, size_type count, value_type const& t,
                                   std::index_sequence<I...> /*unused*/)
  {
    (std::uninitialized_fill_n(std::get<I>(store) + start, count, get_ref<I>(t)), ...);
  }

  template <std::size_t... I>
  void uninitialized_fill(size_type start, size_type count, value_type const& t, std::index_sequence<I...> /*unused*/)
  {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    (std::uninitialized_fill_n(std::get<I>(data_) + start, count, get_ref<I>(t)), ...);
  }

  template <typename It, typename Arg>
  static void copy_fill_n(It it, size_type count, Arg&& arg)
  {
    It end_it = it + count;
    for (; it != end_it; ++it)
    {
      *it = std::forward<Arg>(arg);
    }
  }

  template <std::size_t... I>
  void copy_fill(size_type start, size_type count, value_type const& t, std::index_sequence<I...> /*unused*/)
  {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    (copy_fill_n(std::get<I>(data_) + start, count, get_ref<I>(t)), ...);
  }

  template <std::size_t... I, typename... Args>
  void construct_at(size_type i, std::index_sequence<I...> /*unused*/, Args&&... args)
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
  void construct_tuple_at(size_type i, std::index_sequence<I...> /*unused*/, value_type const& arg)
  {
    (std::construct_at(std::get<I>(data_) + i, get_ref<I>(arg)), ...);
  }

  template <std::size_t... I>
  void construct_tuple_at(size_type i, std::index_sequence<I...> /*unused*/, value_type&& arg)
  {
    (std::construct_at(std::get<I>(data_) + i, std::move(get_ref<I>(arg))), ...);
  }

  template <typename T, typename Arg>
  void move_at(T* data, Arg&& arg)
  {
    *data = std::forward<Arg>(arg);
  }

  template <std::size_t... I, typename... Args>
  void move_at(size_type i, std::index_sequence<I...> /*unused*/, Args&&... args)
  {
    (move_at(std::get<I>(data_) + i, std::forward<Args>(args)), ...);
  }

  template <std::size_t... I>
  void move_tuple_at(size_type i, std::index_sequence<I...> /*unused*/, value_type const& arg)
  {
    (move_at(std::get<I>(data_) + i, get_ref<I>(arg)), ...);
  }

  template <typename Ty>
  void move_construct(Ty* to, Ty* from, size_type n)
  {
    for (size_type i = 0; i < n; ++i)
    {
      std::construct_at(to + i, std::move(from[i]));
    }
  }

  template <std::size_t... I>
  void move_construct(array_type& dst, size_type dst_offset, array_type& src, size_type src_offset, size_type n,
                      std::index_sequence<I...> /*unused*/)
  {
    (move_construct(std::get<I>(dst) + dst_offset, std::get<I>(src) + src_offset, n), ...);
  }

  template <typename Ty>
  void copy_assign(Ty* to, Ty const* from, size_type n)
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
  void copy(array_type& dst, array_type const& src, size_type n, std::index_sequence<I...> /*unused*/)
  {
    (copy_assign(std::get<I>(dst), std::get<I>(src), n), ...);
  }

  template <typename T>
  void construct_list(T* data, size_type sz, T const* src)
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
  void construct_single(T& data, T const& src)
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
  void construct_list(soavector const& x, std::index_sequence<I...> /*unused*/)
  {
    (construct_list(std::get<I>(data_), size_, std::get<I>(x.data_)), ...);
  }

  template <typename T>
  static void destroy_seq(T* data, size_type size)
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
  static void destroy_single(T* data)
  {
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
      std::destroy_at(data);
    }
  }

  template <std::size_t... I>
  void destroy_all(size_type start, size_type count, std::index_sequence<I...> /*unused*/)
  {
    (destroy_seq(std::get<I>(data_) + start, count), ...);
  }

  template <std::size_t... I>
  void destroy_at(size_type at, std::index_sequence<I...> /*unused*/)
  {
    (destroy_single(std::get<I>(data_) + at), ...);
  }

  void destroy_and_deallocate()
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

  auto insert_hole(size_type p, size_type n = 1) -> size_type
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

  auto assign_copy(soavector const& x, std::false_type /*unused*/) -> soavector&
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

  auto assign_copy(soavector const& x, std::true_type /*unused*/) -> soavector&
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

  auto assign_move(soavector& x, std::false_type /*unused*/) -> soavector&
  {
    if constexpr (allocator_is_always_equal::value ||
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

  auto assign_move(soavector& x, std::true_type /*unused*/) -> soavector&
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
  void allocate(Ty*& out_ref, size_type n)
  {
    out_ref = ouly::allocate<Ty>(static_cast<allocator_type&>(*this), n * sizeof(Ty));
  }

  template <std::size_t... I>
  auto allocate(size_type n, std::index_sequence<I...> /*unused*/) -> array_type
  {
    array_type r;
    (allocate(std::get<I>(r), n), ...);
    return r;
  }

  auto allocate(size_type n) -> array_type
  {
    return allocate(n, index_seq);
  }

  template <typename Ty>
  void deallocate(Ty* out_ref, size_type n)
  {
    ouly::deallocate(static_cast<allocator_type&>(*this), out_ref, n * sizeof(Ty));
  }

  template <std::size_t... I>
  void deallocate(size_type n, std::index_sequence<I...> /*unused*/)
  {
    (deallocate(std::get<I>(data_), n), ...);
  }

  void deallocate()
  {
    deallocate(capacity_, index_seq);
  }

  void unchecked_reserve(size_type n)
  {
    auto d = allocate(n);
    if (size_)
    {
      move_construct(d, 0, data_, 0, size_, index_seq);
    }

    destroy_and_deallocate();
    data_     = d;
    capacity_ = n;
  }

  void unchecked_reserve(size_type n, size_type at, size_type holes)
  {
    auto d = allocate(n);
    if (size_)
    {
      move_construct(d, 0, data_, 0, at, index_seq);
      move_construct(d, at + holes, data_, at, (size_ - at), index_seq);
    }
    destroy_and_deallocate();
    data_     = d;
    capacity_ = n;
  }

  void swap(soavector& x, std::false_type /*unused*/)
  {
    std::swap(capacity_, x.capacity_);
    std::swap(size_, x.size_);
    std::swap(data_, x.data_);
  }

  void swap(soavector& x, std::true_type /*unused*/)
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
  static auto equals(Ty const* first, Ty const* second, size_type n) -> bool
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
                     std::index_sequence<I...> /*unused*/) -> bool
  {
    return (equals(std::get<I>(first), std::get<I>(second), n) && ...);
  }

  /**
   * Less
   */
  template <typename Ty>
  static auto less(Ty const* first, Ty const* second, size_type n) -> bool
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
  static auto less(array_type const& first, array_type const& second, size_type n, std::index_sequence<I...> /*unused*/)
   -> bool
  {
    return (less(std::get<I>(first), std::get<I>(second), n) && ...);
  }

  template <typename Ty>
  static auto lesseq(Ty const* first, Ty const* second, size_type n) -> bool
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
                     std::index_sequence<I...> /*unused*/) -> bool
  {
    return (lesseq(std::get<I>(first), std::get<I>(second), n) && ...);
  }
  /**
   *
   */

  friend auto operator==(soavector const& x, soavector const& y) -> bool
  {
    return x.size_ == y.size_ && equals(x.data_, y.data_, x.size_, index_seq);
  }

  friend auto operator<(soavector const& x, soavector const& y) -> bool
  {
    auto n = std::min(x.size(), y.size());
    return (less(x.data_, y.data_, n, index_seq) | (x.size_ < y.size_));
  }

  friend auto operator<=(soavector const& x, soavector const& y) -> bool
  {
    auto n = std::min(x.size(), y.size());
    return (lesseq(x.data_, y.data_, n, index_seq) | (x.size_ <= y.size_));
  }

  friend auto operator!=(soavector const& x, soavector const& y) -> bool
  {
    return !(x == y);
  }

  friend auto operator>(soavector const& x, soavector const& y) -> bool
  {
    return y < x;
  }

  friend auto operator>=(soavector const& x, soavector const& y) -> bool
  {
    return y <= x;
  }

  template <class InputIterator>
  auto insert_range(size_type position, InputIterator first, InputIterator last) -> size_type
  {
    size_type p = insert_hole(position, static_cast<size_type>(std::distance(first, last)));
    uninitialized_copy_range(first, last, data_, p, index_seq);
    return p;
  }

  auto insert_range(size_type position, size_type n, value_type const& x) -> size_type
  {
    size_type p = insert_hole(position, n);
    uninitialized_fill(p, n, x, index_seq);
    return p;
  }

  template <typename InputIterator>
  void construct_from_range(InputIterator first, InputIterator last)
  {
    size_     = static_cast<size_type>(std::distance(first, last));
    data_     = allocate(size_);
    capacity_ = size_;
    uninitialized_copy_range(first, last, data_, 0, index_seq);
  }

  template <typename Reftype, std::size_t... I>
  auto get(std::index_sequence<I...> /*unused*/) const -> Reftype
  {
    return Reftype(*std::get<I>(data_)...);
  }

  template <typename Reftype, std::size_t... I>
  auto get(std::index_sequence<I...> /*unused*/, size_type i) const -> Reftype
  {
    return Reftype((std::get<I>(data_) + i)...);
  }

  template <typename InputIterator>
  static auto distance(InputIterator first, InputIterator last) -> size_type
  {
    if constexpr (requires {
                    { static_cast<size_type>(last - first) } -> std::convertible_to<size_type>;
                  })
    {
      return static_cast<size_type>(last - first);
    }
    else
    {
      return std::distance(first, last);
    }
  }

  array_type data_{};
  size_type  size_     = 0;
  size_type  capacity_ = 0;
};

} // namespace ouly
