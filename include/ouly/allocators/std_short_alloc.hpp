// SPDX-License-Identifier: MIT
#pragma once
// The MIT License (MIT)
//
// Copyright (c) 2015 Howard Hinnant
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ouly/utility/user_config.hpp"
#include <cstddef>

namespace ouly
{
template <std::size_t N, std::size_t Alignment = alignof(std::max_align_t)>
class arena
{
  alignas(Alignment) char buf_[N]{};
  char* ptr_ = nullptr;

public:
  arena() noexcept : ptr_(buf_) {}
  arena(arena&&)                         = delete;
  auto operator=(arena&&) -> arena&      = delete;
  arena(const arena&)                    = delete;
  auto operator=(const arena&) -> arena& = delete;
  ~arena() noexcept                      = default;

  template <std::size_t ReqAlign>
  [[nodiscard]] auto allocate(std::size_t n) -> void*;
  void               deallocate(void* p, std::size_t n) noexcept;

  static constexpr auto size() noexcept -> std::size_t
  {
    return N;
  }
  [[nodiscard]] auto used() const noexcept -> std::size_t
  {
    return static_cast<std::size_t>(ptr_ - buf_);
  }
  void reset() noexcept
  {
    ptr_ = buf_;
  }

private:
  static auto align_up(std::size_t n) noexcept -> std::size_t
  {
    return (n + (Alignment - 1)) & ~(Alignment - 1);
  }

  auto pointer_in_buffer(const char* p) noexcept -> bool
  {
    return buf_ <= p && p <= buf_ + N;
  }
};

template <std::size_t N, std::size_t Alignment>
template <std::size_t ReqAlign>
[[nodiscard]] auto arena<N, Alignment>::allocate(std::size_t n) -> void*
{
  static_assert(ReqAlign <= Alignment, "alignment is too small for this arena");
  OULY_ASSERT(pointer_in_buffer(ptr_) && "std_short_alloc has outlived arena");
  auto const aligned_n = align_up(n);
  if (static_cast<decltype(aligned_n)>(buf_ + N - ptr_) >= aligned_n)
  {
    char* r = ptr_;
    ptr_ += aligned_n;
    return r;
  }

  static_assert(Alignment <= alignof(std::max_align_t), "you've chosen an "
                                                        "alignment that is larger than alignof(std::max_align_t), and "
                                                        "cannot be guaranteed by normal operator new");
  return static_cast<char*>(::operator new(n));
}

template <std::size_t N, std::size_t Alignment>
void arena<N, Alignment>::deallocate(void* ptr, std::size_t n) noexcept
{
  auto* p = static_cast<char*>(ptr);
  OULY_ASSERT(pointer_in_buffer(ptr_) && "std_short_alloc has outlived arena");
  if (pointer_in_buffer(p))
  {
    n = align_up(n);
    if (p + n == ptr_)
    {
      ptr_ = p;
    }
  }
  else
  {
    ::operator delete(p);
  }
}

template <class T, std::size_t N, std::size_t Align = alignof(std::max_align_t)>
class std_short_alloc
{
public:
  using value_type                = T;
  static auto constexpr alignment = Align;
  static auto constexpr size      = N;
  using arena_type                = arena<size, alignment>;

private:
  arena_type* arena_;

public:
  std_short_alloc(std_short_alloc&&)                         = delete;
  auto operator=(std_short_alloc&&) -> std_short_alloc&      = delete;
  std_short_alloc(const std_short_alloc&)                    = default;
  auto operator=(const std_short_alloc&) -> std_short_alloc& = delete;
  ~std_short_alloc() noexcept                                = default;

  std_short_alloc(arena_type& a) noexcept : arena_(&a)
  {
    static_assert(size % alignment == 0, "size N needs to be a multiple of alignment Align");
  }
  template <class U>
  std_short_alloc(const std_short_alloc<U, N, alignment>& a) noexcept : arena_(a.arena_)
  {}

  template <class Up>
  struct rebind
  {
    using other = std_short_alloc<Up, N, alignment>;
  };

  [[nodiscard]] auto allocate(std::size_t n) -> T*
  {
    return static_cast<T*>(arena_->template allocate<alignof(T)>(n * sizeof(T)));
  }
  void deallocate(T* p, std::size_t n) noexcept
  {
    arena_->deallocate(p, n * sizeof(T));
  }

  template <class T1, std::size_t N1, std::size_t A1, class U, std::size_t M, std::size_t A2>
  friend auto operator==(const std_short_alloc<T1, N1, A1>& x, const std_short_alloc<U, M, A2>& y) noexcept -> bool;

  template <class U, std::size_t M, std::size_t A>
  friend class std_short_alloc;
};

template <class T, std::size_t N, std::size_t A1, class U, std::size_t M, std::size_t A2>
inline auto operator==(const std_short_alloc<T, N, A1>& x, const std_short_alloc<U, M, A2>& y) noexcept -> bool
{
  return N == M && A1 == A2 && &x.arena_ == &y.arena_;
}

template <class T, std::size_t N, std::size_t A1, class U, std::size_t M, std::size_t A2>
inline auto operator!=(const std_short_alloc<T, N, A1>& x, const std_short_alloc<U, M, A2>& y) noexcept -> bool
{
  return !(x == y);
}
} // namespace ouly
