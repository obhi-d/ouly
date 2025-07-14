// SPDX-License-Identifier: MIT
/*
 * small_vector.hpp
 *
 *  Created on: 11/2/2018, 10:48:02 AM
 *      Author: obhi
 */

#pragma once
#include "ouly/allocators/allocator.hpp"
#include "ouly/allocators/default_allocator.hpp"
#include "ouly/allocators/detail/custom_allocator.hpp"
#include "ouly/utility/type_traits.hpp"
#include "ouly/utility/utils.hpp"
#include <array>
#include "ouly/utility/user_config.hpp"
#include <cstdint>
#include <cstring>
#include <memory>
namespace ouly
{
/**
 * @brief A vector implementation that can store a small number of elements inline
 *
 * small_vector is a container that encapsulates dynamic size arrays with the ability to store
 * a small number of elements within the object itself, avoiding heap allocation for small arrays.
 * It combines the benefits of std::vector with small buffer optimization.
 *
 * @tparam Ty The type of elements
 * @tparam N The number of elements that can be stored inline before requiring heap allocation
 * @tparam Config Configuration config for the vector (allocator, memory traits, etc.)
 *
 * Features:
 * - Inline storage for small arrays (up to N elements)
 * - Automatic transition to heap storage when size exceeds inline capacity
 * - STL-compatible container interface
 * - Support for custom allocators
 * - Configurable memory traits (zero memory, no fill, POD optimizations)
 * - Move semantics support
 *
 * Performance characteristics:
 * - O(1) access to elements
 * - O(1) addition/removal at the end
 * - O(n) insertion/removal in the middle
 * - No heap allocation for small arrays ( N elements)
 *
 * Memory guarantees:
 * - Elements are stored contiguously
 * - No heap allocation until more than N elements are stored
 * - Maintains proper alignment for the stored type
 *
 * Example usage:
 * @code
 * small_vector<int, 16> vec;  // Can store up to 16 ints without heap allocation
 * vec.push_back(42);          // Stored inline
 * @endcode
 *
 * @note The actual inline capacity might be larger than N to accommodate the heap storage data
 *       when N is very small
 */
template <typename Ty, std::size_t N = 0, typename Config = ouly::default_config<Ty>>
class small_vector : public ouly::detail::custom_allocator_t<Config>
{
public:
  using value_type                = Ty;
  using allocator_type            = ouly::detail::custom_allocator_t<Config>;
  using size_type                 = ouly::detail::choose_size_t<uint32_t, Config>;
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
  using allocator_is_always_equal = typename ouly::allocator_traits<allocator_tag>::is_always_equal;
  using propagate_allocator_on_move =
   typename ouly::allocator_traits<allocator_tag>::propagate_on_container_move_assignment;
  using propagate_allocator_on_copy =
   typename ouly::allocator_traits<allocator_tag>::propagate_on_container_copy_assignment;
  using propagate_allocator_on_swap = typename ouly::allocator_traits<allocator_tag>::propagate_on_container_swap;

private:
  using config                                          = Config;
  static constexpr bool has_zero_memory                 = ouly::detail::HasZeroMemoryAttrib<config>;
  static constexpr bool has_no_fill                     = ouly::detail::HasNoFillAttrib<config>;
  static constexpr bool has_pod                         = ouly::detail::HasTrivialAttrib<config>;
  static constexpr bool has_trivial_dtor                = has_pod || std::is_trivially_destructible_v<Ty>;
  static constexpr bool has_trivially_destroyed_on_move = ouly::detail::HasTriviallyDestroyedOnMoveAttrib<config>;

  using storage = ouly::detail::aligned_storage<sizeof(value_type), alignof(value_type)>;
  struct heap_storage
  {
    storage*  pdata_    = nullptr;
    size_type capacity_ = 0;
  };

  static constexpr size_type inline_capacity = N * sizeof(Ty) > sizeof(heap_storage)
                                                ? N
                                                : (sizeof(heap_storage) + sizeof(Ty) - 1) / sizeof(Ty);

  using inline_storage = std::array<storage, inline_capacity>;
  union data_store
  {
    inline_storage ldata_ = {};
    heap_storage   hdata_;

    data_store() : ldata_() {}
  };

  struct tail_type : std::true_type
  {
    size_type tail_;
  };

public:
  constexpr small_vector() = default;
  constexpr explicit small_vector(const allocator_type& alloc) : allocator_type(alloc) {}

  constexpr explicit small_vector(size_type n)
  {
    resize(n);
  }

  constexpr small_vector(size_type n, const Ty& value, const allocator_type& alloc = allocator_type())
      : allocator_type(alloc)
  {
    resize(n, value);
  }

  template <class InputIterator>
  constexpr small_vector(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type())
      : allocator_type(alloc)
  {
    construct_from_range(first, last, std::is_integral<InputIterator>());
  }

  constexpr small_vector(const small_vector& other) : allocator_type(static_cast<allocator_type const&>(other))
  {
    if (other.size() > inline_capacity)
    {
      unchecked_reserve_in_heap(other.size());
    }
    size_ = other.size();
    copy_construct(std::begin(other), std::end(other), get_data());
  }

  constexpr small_vector(small_vector&& other) noexcept
      : allocator_type(std::move(static_cast<allocator_type&&>(other)))
  {
    *this = std::move(other);
  }

  constexpr small_vector(const small_vector& other, const allocator_type& alloc) : allocator_type(alloc)
  {
    if (other.size() > inline_capacity)
    {
      unchecked_reserve_in_heap(other.size());
    }
    size_ = other.size();
    copy_construct(std::begin(other), std::end(other), get_data());
  }

  constexpr small_vector(small_vector&& other, const allocator_type& alloc) : allocator_type(alloc)
  {
    *this = std::move(other);
  };
  constexpr small_vector(std::initializer_list<Ty> other, const allocator_type& alloc = allocator_type())
      : allocator_type(alloc)
  {
    if (other.size() > inline_capacity)
    {
      unchecked_reserve_in_heap(static_cast<size_type>(other.size()));
    }
    size_ = static_cast<size_type>(other.size());
    copy_construct(std::begin(other), std::end(other), get_data());
  }

  constexpr ~small_vector() noexcept
  {
    clear();
  }

  constexpr auto operator=(const small_vector& other) -> small_vector&
  {
    if (this == &other)
    {
      return *this;
    }
    assign_copy(other, propagate_allocator_on_copy());
    return *this;
  }

  constexpr auto operator=(small_vector&& other) noexcept -> small_vector&
  {
    if (this == &other)
    {
      return *this;
    }
    assign_move(other, propagate_allocator_on_move());
    return *this;
  }

  constexpr auto operator=(std::initializer_list<Ty> other) -> small_vector&
  {
    clear();
    resize_fill(other.size(), std::false_type());
    size_ = other.size();
    copy_construct(std::begin(other), std::end(other), get_data());
    return *this;
  }

  template <class InputIterator>
  constexpr void assign(InputIterator first, InputIterator last)
  {
    auto n = static_cast<size_type>(std::distance(first, last));
    if (n > capacity())
    {
      // Reserve space in heap to do the copy first, then clear the old data and assign the iterator range
      // then free
      auto [ldata, copy] = heap_allocate(n);
      // initialize
      std::uninitialized_copy(first, last, copy.pdata_->template as<Ty>());

      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
      {
        std::destroy_n(ldata, size_);
      }

      if (!is_inlined())
      {
        ouly::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                         alignarg<storage>);
      }

      data_store_.hdata_ = copy;
      size_              = n;
    }
    else if (n > size_)
    {
      // Copy until size_, then construct the rest
      auto ptr = get_data();
      std::copy(first, first + size_, ptr);
      std::uninitialized_copy(first + size_, last, ptr + size_);
      size_ = n;
    }
    else
    {
      auto ptr = get_data();
      std::copy(first, last, ptr);
      resize(n);
    }
    shrink_to_fit();
  }

  constexpr void assign(size_type n, const Ty& value)
  {
    if (n > capacity())
    {
      // Reserve space in heap to do the copy first, then clear the old data and assign the iterator range
      // then free
      auto [ldata, copy] = heap_allocate(n);
      // initialize
      std::uninitialized_fill_n(copy.pdata_->template as<Ty>(), n, value);

      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
      {
        std::destroy_n(ldata, size_);
      }

      if (!is_inlined())
      {
        ouly::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                         alignarg<storage>);
      }

      data_store_.hdata_ = copy;
      size_              = n;
    }
    else if (n > size_)
    {
      // Copy until size_, then construct the rest
      auto ptr = get_data();
      std::fill_n(ptr, size_, value);
      std::uninitialized_fill_n(ptr + size_, n - size_, value);
      size_ = n;
    }
    else
    {
      auto ptr = get_data();
      std::fill_n(ptr, n, value);
      resize(n);
    }
    shrink_to_fit();
  }

  constexpr void assign(std::initializer_list<Ty> other)
  {
    assign(other.begin(), other.end());
  }

  template <typename R>
  constexpr void assign_range(R&& rg)
  {
    assign(std::begin(std::forward<R>(rg)), std::end(std::forward<R>(rg)));
  }

  [[nodiscard]] constexpr auto get_allocator() const -> allocator_type
  {
    return *this;
  }

  // iterators:
  constexpr auto begin() -> iterator
  {
    return get_data();
  }

  [[nodiscard]] constexpr auto begin() const -> const_iterator
  {
    return get_data();
  }

  constexpr auto end() -> iterator
  {
    return get_data() + size();
  }

  [[nodiscard]] constexpr auto end() const -> const_iterator
  {
    return get_data() + size();
  }

  constexpr auto rbegin() -> reverse_iterator
  {
    return reverse_iterator(end());
  }

  [[nodiscard]] constexpr auto rbegin() const -> const_reverse_iterator
  {
    return const_reverse_iterator(end());
  }

  constexpr auto rend() -> reverse_iterator
  {
    return reverse_iterator(begin());
  }

  [[nodiscard]] constexpr auto rend() const -> const_reverse_iterator
  {
    return const_reverse_iterator(begin());
  }

  [[nodiscard]] constexpr auto cbegin() const -> const_iterator
  {
    return begin();
  }

  [[nodiscard]] constexpr auto cend() const -> const_iterator
  {
    return end();
  }

  [[nodiscard]] constexpr auto crbegin() const -> const_reverse_iterator
  {
    return rbegin();
  }

  [[nodiscard]] constexpr auto crend() const -> const_reverse_iterator
  {
    return rend();
  }

  // capacity:
  [[nodiscard]] constexpr auto size() const -> size_type
  {
    return size_;
  }

  [[nodiscard]] constexpr auto max_size() const -> size_type
  {
    return allocator_type::max_size();
  }

  constexpr void resize(size_type size)
  {
    resize_fill(size, value_type());
  }

  constexpr void resize(size_type size, const Ty& value)
  {
    resize_fill(size, value);
  }

  [[nodiscard]] constexpr auto capacity() const -> size_type
  {
    return is_inlined() ? inline_capacity : data_store_.hdata_.capacity_;
  }

  [[nodiscard]] constexpr auto empty() const -> bool
  {
    return size() == 0;
  }

  /**
   * @brief Use resize to really reserve space
   */
  constexpr void reserve(size_type n)
  {
    if (is_inlined())
    {
      return;
    }
    if (capacity() < n)
    {
      unchecked_reserve_in_heap(n);
    }
  }

  constexpr void shrink_to_fit()
  {
    if (is_inlined())
    {
      return;
    }

    auto size_val = size();
    if (capacity() != size_val)
    {
      unchecked_reserve_in_heap(size_val);
    }
  }

  // element access:
  constexpr auto operator[](size_type n) -> reference
  {
    OULY_ASSERT(n < size());
    return get_data()[n];
  }

  constexpr auto operator[](size_type n) const -> const_reference
  {
    OULY_ASSERT(n < size());
    return get_data()[n];
  }

  constexpr auto at(size_type n) -> reference
  {
    OULY_ASSERT(n < size());
    return get_data()[n];
  }

  [[nodiscard]] constexpr auto at(size_type n) const -> const_reference
  {
    OULY_ASSERT(n < size());
    return get_data()[n];
  }

  constexpr auto front() -> reference
  {
    return at(0);
  }

  [[nodiscard]] constexpr auto front() const -> const_reference
  {
    return at(0);
  }

  constexpr auto back() -> reference
  {
    return at(size() - 1);
  }

  [[nodiscard]] constexpr auto back() const -> const_reference
  {
    return at(size() - 1);
  }

  // data access
  constexpr auto data() -> Ty*
  {
    return get_data();
  }

  [[nodiscard]] constexpr auto data() const -> const Ty*
  {
    return get_data();
  }

  // modifiers:
  template <class... Args>
  constexpr auto emplace_back(Args&&... args) -> Ty&
  {
    auto new_size = size_ + 1;
    if (capacity() <= size_)
    {
      auto [ldata, copy] = heap_allocate(new_size + (size_ >> 1));
      auto data          = copy.pdata_->template as<Ty>();
      auto ptr           = data + size_;

      std::construct_at(ptr, std::forward<Args>(args)...);

      if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
      {
        std::memcpy(data, ldata, size_ * sizeof(Ty));
      }
      else
      {
        std::uninitialized_move(ldata, ldata + size_, data);
      }

      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
      {
        std::destroy_n(ldata, size_);
      }

      if (!is_inlined())
      {
        ouly::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                         alignarg<storage>);
      }

      size_              = new_size;
      data_store_.hdata_ = copy;

      return *ptr;
    }

    auto ptr = get_data() + size_;
    size_    = new_size;
    std::construct_at(ptr, std::forward<Args>(args)...);
    return *ptr;
  }

  constexpr void push_back(const Ty& other)
  {
    emplace_back(other);
  }

  constexpr void push_back(Ty&& other)
  {
    emplace_back(std::move(other));
  }

  constexpr void pop_back()
  {
    OULY_ASSERT(size() > 0);

    auto last = size_--;
    if (size_ <= inline_capacity && last > inline_capacity)
    {
      transfer_to_ib(size_, tail_type{.tail_ = last});
    }
    else
    {
      if constexpr (!has_trivial_dtor)
      {
        std::destroy_at(get_data() + size_);
      }
    }
  }

  template <class... Args>
  constexpr auto emplace(const_iterator position, Args&&... args) -> iterator
  {
    size_type pos = insert_hole(position);
    auto      ptr = get_data() + pos;
    std::construct_at(ptr, std::forward<Args>(args)...);
    return ptr;
  }

  constexpr auto insert(const_iterator position, const Ty& other) -> iterator
  {
    return emplace(position, other);
  }

  constexpr auto insert(const_iterator position, Ty&& other) -> iterator
  {
    return emplace(position, std::move(other));
  }

  constexpr auto insert(const_iterator position, size_type n, const Ty& other) -> iterator
  {
    return insert_range(position, n, other, std::true_type());
  }

  template <class InputIterator>
  constexpr auto insert(const_iterator position, InputIterator first, InputIterator last) -> iterator
  {
    return insert_range(position, first, last, std::is_integral<InputIterator>());
  }

  constexpr auto insert(const_iterator position, std::initializer_list<Ty> other) -> iterator
  {
    size_type pos = insert_hole(position, static_cast<size_type>(other.size()));
    auto      ptr = get_data() + pos;
    copy_construct(std::begin(other), std::end(other), ptr);
    return ptr;
  }

  constexpr auto erase(const_iterator position) -> iterator
  {
    OULY_ASSERT(position < end());
    auto data = get_data();
    auto last = size_--;
    auto pos  = const_cast<iterator>(position); // NOLINT
    if constexpr (std::is_trivially_copyable_v<Ty> || has_pod)
    {
      std::memmove(pos, position + 1, static_cast<std::size_t>((data + size_) - (position)) * sizeof(Ty));
      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
      {
        std::destroy_at(data + size_);
      }
    }
    else
    {
      auto pos_it = pos;
      auto end    = data + size_;
      for (; pos_it != end; ++pos_it)
      {
        *pos_it = std::move(*(pos_it + 1));
      }
      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
      {
        std::destroy_at(end);
      }
    }
    if (size_ <= inline_capacity && last > inline_capacity)
    {
      // already destroyed end objects
      auto dist = static_cast<size_t>(std::distance(data_store_.hdata_.pdata_->template as<Ty const>(), position));
      transfer_to_ib(size_);
      return const_cast<iterator>(data_store_.ldata_[dist].template as<Ty>());
    }
    return pos;
  }

  constexpr auto erase(const_iterator first, const_iterator last) -> iterator
  {
    OULY_ASSERT(last < end());
    auto dist      = static_cast<std::uint32_t>(std::distance(first, last));
    auto first_pos = const_cast<iterator>(first); // NOLINT
    auto last_pos  = const_cast<iterator>(last);  // NOLINT
    if constexpr (std::is_trivially_copyable_v<Ty> || has_pod)
    {
      auto data = get_data();
      std::memmove(first_pos, last, static_cast<std::size_t>((data + size_) - (last)) * sizeof(Ty));
    }
    else
    {
      auto dst = first_pos;
      auto src = last_pos;
      auto end = get_data() + size_;
      for (; src != end; ++src, ++dst)
      {
        *dst = std::move(*src);
      }
    }
    auto data     = get_data();
    auto old_size = size_;
    size_ -= dist;
    if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
    {
      std::destroy_n(data + size_, dist);
    }

    if (size_ <= inline_capacity && old_size > inline_capacity)
    {
      // already destroyed end objects
      auto p_dist = static_cast<size_t>(std::distance(data_store_.hdata_.pdata_->template as<Ty const>(), first));
      transfer_to_ib(size_);
      return data_store_.ldata_[p_dist].template as<Ty>();
    }

    return first_pos;
  }

  constexpr void swap(small_vector& other) noexcept
  {
    swap(other, propagate_allocator_on_swap());
  }

  constexpr void clear()
  {
    clear_data();
  }

  static constexpr auto get_inlined_capacity() -> size_type
  {
    return inline_capacity;
  }

  [[nodiscard]] constexpr auto is_inlined() const -> bool
  {
    return (size_ <= inline_capacity);
  }

private:
  constexpr auto resize_no_fill(size_type size) -> Ty*
  {
    if (size <= inline_capacity)
    {
      if (size_ > inline_capacity)
      {
        transfer_to_ib(size, tail_type{.tail_ = size_});
      }
      return data_store_.ldata_.data()->template as<Ty>();
    }

    if (capacity() < size)
    {
      unchecked_reserve_in_heap(size);
    }
    return data_store_.hdata_.pdata_->template as<Ty>();
  }

  template <typename Tail = std::false_type>
  constexpr void resize_fill(size_type size, Tail const& tail)
  {
    auto data_ptr = resize_no_fill(size);
    if (size > size_)
    {
      if constexpr (std::is_constructible_v<Ty, Tail>)
      {
        std::uninitialized_fill_n(data_ptr + size_, size - size_, tail);
      }
    }
    size_ = size;
  }

  [[nodiscard]] constexpr auto get_data() const -> Ty const*
  {
    return is_inlined() ? data_store_.ldata_.data()->template as<Ty>() : data_store_.hdata_.pdata_->template as<Ty>();
  }

  constexpr auto get_data() -> Ty*
  {
    return is_inlined() ? data_store_.ldata_.data()->template as<Ty>() : data_store_.hdata_.pdata_->template as<Ty>();
  }

  constexpr auto heap_allocate(size_type n) -> std::pair<Ty*, heap_storage>
  {
    heap_storage copy;
    copy.capacity_ = n;
    copy.pdata_    = ouly::allocate<storage>(*this, n * sizeof(storage), alignarg<storage>);
    auto ldata     = data();

    return {ldata, copy};
  }

  constexpr void unchecked_reserve_in_heap(size_type n)
  {
    auto [ldata, copy] = heap_allocate(n);
    auto data          = copy.pdata_->template as<Ty>();
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(data, ldata, size_ * sizeof(Ty));
    }
    else
    {
      std::uninitialized_move(ldata, ldata + size_, data);
    }

    if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
    {
      std::destroy_n(ldata, size_);
    }

    if (!is_inlined())
    {
      ouly::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                       alignarg<storage>);
    }
    data_store_.hdata_ = copy;
  }

  constexpr void unchecked_reserve_in_heap(size_type n, size_type at_position, size_type holes)
  {
    heap_storage copy;
    copy.capacity_ = n;
    copy.pdata_    = ouly::allocate<storage>(*this, n * sizeof(storage), alignarg<storage>);
    auto ldata     = data();
    auto p_data    = copy.pdata_->template as<Ty>();
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(p_data, ldata, at_position * sizeof(Ty));
      std::memcpy(p_data + at_position + holes, ldata + at_position, (size_ - at_position) * sizeof(Ty));
    }
    else
    {
      std::uninitialized_move(ldata, ldata + at_position, p_data);
      std::uninitialized_move(ldata + at_position, ldata + size_, p_data + at_position + holes);
      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
      {
        std::destroy_n(ldata, size_);
      }
    }
    if (!is_inlined())
    {
      ouly::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                       alignarg<storage>);
    }
    data_store_.hdata_ = copy;
  }

  constexpr void clear_data()
  {
    if (empty())
    {
      return;
    }
    auto p_data = data();
    if constexpr (!has_trivial_dtor)
    {
      std::destroy_n(p_data, size_);
    }
    if (!is_inlined())
    {
      ouly::deallocate(*this, data_store_.hdata_.pdata_, data_store_.hdata_.capacity_ * sizeof(storage),
                       alignarg<storage>);
    }
    size_ = 0;
  }

  template <typename Tail = std::false_type>
  constexpr void transfer_to_ib(size_type item_count, Tail last_size = Tail())
  {
    heap_storage copy  = data_store_.hdata_;
    auto         data  = copy.pdata_->template as<Ty>();
    auto         ldata = data_store_.ldata_.data()->template as<Ty>();

    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(ldata, data, sizeof(storage) * item_count);
    }
    else
    {
      std::uninitialized_move(data, data + item_count, ldata);
    }

    if constexpr (!has_trivial_dtor)
    {
      if constexpr (!Tail::value)
      {
        std::destroy_n(data, size_);
      }
      else if constexpr (!has_trivially_destroyed_on_move)
      {
        std::destroy_n(data + item_count, last_size.tail_ - item_count);
      }
    }
    ouly::deallocate(*this, copy.pdata_, copy.capacity_ * sizeof(storage), alignarg<storage>);
  }

  constexpr void transfer_to_heap(size_type item_count, size_type cap)
  {
    heap_storage copy;
    copy.capacity_ = cap;
    copy.pdata_    = ouly::allocate<storage>(*this, cap * sizeof(storage), alignarg<storage>);
    auto data      = copy.pdata_->template as<Ty>();
    auto ldata     = data_store_.ldata_.data()->template as<Ty>();

    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(data, ldata, sizeof(storage) * item_count);
    }
    else
    {
      std::uninitialized_move(ldata, ldata + item_count, data);
    }
    if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
    {
      std::destroy_n(ldata, size_);
    }
    data_store_.hdata_ = copy;
  }

  constexpr void static uninitialized_move_backward(Ty* src, Ty* dst, size_type n)
  {
    for (size_type i = 0; i < n; ++i)
    {
      new (--dst) Ty(std::move(*(--src)));
    }
  }

  constexpr auto fill_hole(iterator src, size_type p_dist, size_type n)
  {
    auto ldata = get_data();
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memmove(src + n, src, (size_ - p_dist) * sizeof(Ty));
    }
    else
    {
      auto min_ctor = std::min(size_ - p_dist, n);
      auto src_it   = ldata + size_;
      auto dst_it   = src_it + n;
      for (size_type i = 0; i < min_ctor; ++i)
      {
        new (--dst_it) Ty(std::move(*(--src_it)));
      }
      for (; src_it != src;)
      {
        *(--dst_it) = std::move(*(--src_it));
      }
      OULY_ASSERT(src_it == src);

      if constexpr (!has_trivial_dtor && !has_trivially_destroyed_on_move)
      {
        OULY_ASSERT(src_it <= dst_it);
        auto next_dst_it = src_it + min_ctor;
        for (; src_it != next_dst_it; ++src_it)
        {
          std::destroy_at(src_it);
        }
      }
    }
  }

  constexpr auto insert_hole(const_iterator pos, size_type n = 1) -> size_type
  {
    auto      ldata  = get_data();
    auto      p_dist = static_cast<size_type>(std::distance<const Ty*>(ldata, pos));
    size_type nsz    = size_ + n;

    if (capacity() < nsz)
    {
      unchecked_reserve_in_heap(size_ + std::max(size_ >> 1, n), p_dist, n);
    }
    else
    {
      fill_hole(const_cast<iterator>(pos), // NOLINT
                p_dist, n);
    }
    size_ = nsz;
    return p_dist;
  }

  constexpr auto assign_copy(const small_vector& other, std::false_type /*unused*/) -> small_vector&
  {
    clear();
    if (capacity() < other.size())
    {
      unchecked_reserve_in_heap(other.size());
    }

    size_ = other.size_;
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(data(), other.data(), size_ * sizeof(Ty));
    }
    else
    {
      std::uninitialized_copy(other.begin(), other.end(), begin());
    }
    return *this;
  }

  constexpr auto assign_copy(const small_vector& other, std::true_type /*unused*/) -> small_vector&
  {
    clear();
    allocator_type::operator=(static_cast<const allocator_type&>(other));
    if (capacity() < other.size())
    {
      unchecked_reserve_in_heap(other.size());
    }

    size_ = other.size_;
    if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
    {
      std::memcpy(data(), other.data(), size_ * sizeof(Ty));
    }
    else
    {
      std::uninitialized_copy(other.begin(), other.end(), begin());
    }

    return *this;
  }

  constexpr auto assign_move(small_vector& other, std::false_type /*unused*/) -> small_vector&
  {
    if constexpr (allocator_is_always_equal::value ||
        static_cast<const allocator_type&>(other) == static_cast<const allocator_type&>(*this))
    {
      clear();
      if (other.is_inlined())
      {
        size_ = other.size_;
        if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
        {
          std::memcpy(data(), other.data(), other.size() * sizeof(storage));
        }
        else
        {
          std::uninitialized_move(other.begin(), other.end(), begin());
        }
      }
      else
      {
        size_                    = other.size_;
        data_store_.hdata_       = other.data_store_.hdata_;
        other.data_store_.hdata_ = {};
      }
      other.size_ = 0;
    }
    else
    {
      if constexpr (std::is_copy_assignable_v<value_type>)
      {
        assign_copy(other, std::false_type());
      }
      else
      {
        []<bool Flag = false>()
        {
          static_assert(Flag, "This type is not assign_moveable");
        };
      }
    }
    return *this;
  }

  constexpr auto assign_move(small_vector& other, std::true_type /*unused*/) -> small_vector&
  {
    clear();

    allocator_type::operator=(std::move(static_cast<allocator_type&>(other)));
    size_ = other.size_;
    if (other.is_inlined())
    {
      if constexpr (has_pod || std::is_trivially_copyable_v<Ty>)
      {
        std::memcpy(data(), other.data(), other.size() * sizeof(storage));
      }
      else
      {
        std::uninitialized_move(other.begin(), other.end(), begin());
      }
    }
    else
    {
      data_store_.hdata_       = other.data_store_.hdata_;
      other.data_store_.hdata_ = {};
    }
    other.size_ = 0;
    return *this;
  }

  template <typename InputIt>
  constexpr void copy_construct(InputIt first, InputIt last, pointer dest)
  {
    if constexpr (std::is_same_v<pointer, InputIt> && (has_pod || std::is_trivially_copyable_v<Ty>))
    {
      std::memcpy(dest, first, static_cast<std::size_t>(std::distance(first, last)) * sizeof(Ty));
    }
    else
    {
      std::uninitialized_copy(first, last, dest);
    }
  }

  constexpr void swap(small_vector& other, std::false_type /*unused*/) noexcept
  {
    if constexpr (std::is_trivially_copyable_v<Ty>)
    {
      std::swap(other.data_store_.ldata_, data_store_.ldata_);
      std::swap(size_, other.size_);
    }
    else
    {
      if (other.is_inlined() || is_inlined())
      {
        auto tmp = std::move(*this);
        *this    = std::move(other);
        other    = std::move(tmp);
      }
      else
      {
        std::swap(other.data_store_.hdata_.pdata_, data_store_.hdata_.pdata_);
        std::swap(other.data_store_.hdata_.capacity_, data_store_.hdata_.capacity_);
        std::swap(size_, other.size_);
      }
    }
  }

  constexpr void swap(small_vector& other, std::true_type /*unused*/) noexcept
  {
    if constexpr (std::is_trivially_copyable_v<Ty>)
    {
      std::swap(other.data_store_.ldata_, data_store_.ldata_);
      std::swap(size_, other.size_);
      std::swap<allocator_type>(this, other);
    }
    else
    {
      if (other.is_inlined() || is_inlined())
      {
        auto tmp = std::move(*this);
        *this    = std::move(other);
        other    = std::move(tmp);
      }
      else
      {
        std::swap(other.data_store_.hdata_.pdata_, data_store_.hdata_.pdata_);
        std::swap(other.data_store_.hdata_.capacity_, data_store_.hdata_.capacity_);
        std::swap(size_, other.size_);
        std::swap<allocator_type>(this, other);
      }
    }
  }

  constexpr friend void swap(small_vector& lhs, small_vector& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  constexpr friend auto operator==(const small_vector& first, const small_vector& second) -> bool
  {
    return first.size_ == second.size_ && std::equal(first.begin(), first.end(), second.begin());
  }

  constexpr friend auto operator<(const small_vector& first, const small_vector& second) -> bool
  {
    return std::lexicographical_compare(first.begin(), first.end(), second.begin(), second.end());
  }

  constexpr friend auto operator!=(const small_vector& first, const small_vector& second) -> bool
  {
    return !(first == second);
  }

  constexpr friend auto operator>(const small_vector& first, const small_vector& second) -> bool
  {
    return std::lexicographical_compare(second.begin(), second.end(), first.begin(), first.end());
  }

  constexpr friend auto operator>=(const small_vector& first, const small_vector& second) -> bool
  {
    return !std::lexicographical_compare(first.begin(), first.end(), second.begin(), second.end());
  }

  constexpr friend auto operator<=(const small_vector& first, const small_vector& second) -> bool
  {
    return !std::lexicographical_compare(second.begin(), second.end(), first.begin(), first.end());
  }

  template <class InputIterator>
  constexpr auto insert_range(const_iterator position, InputIterator first, InputIterator last,
                              std::false_type /*unused*/) -> iterator
  {
    size_type pos   = insert_hole(position, static_cast<size_type>(std::distance(first, last)));
    auto      ldata = data();
    std::uninitialized_copy(first, last, ldata + pos);
    return ldata + pos;
  }

  constexpr auto insert_range(const_iterator position, size_type n, const Ty& other, std::true_type /*unused*/)
   -> iterator
  {
    size_type pos   = insert_hole(position, n);
    auto      ldata = data();
    std::uninitialized_fill_n(ldata + pos, n, other);
    return ldata + pos;
  }

  template <class InputIterator>
  constexpr void construct_from_range(InputIterator first, InputIterator last, std::false_type /*unused*/)
  {
    resize_fill(static_cast<size_type>(std::distance(first, last)), std::false_type{});
    std::uninitialized_copy(first, last, data());
  }

  constexpr void construct_from_range(size_type n, const Ty& value, std::true_type /*unused*/)
  {
    resize_fill(n, std::false_type{});
    std::uninitialized_fill_n(data(), n, value);
  }

  data_store data_store_;
  size_type  size_ = 0;
};
} // namespace ouly
