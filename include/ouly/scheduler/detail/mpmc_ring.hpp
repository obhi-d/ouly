
#pragma once

#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/utility/utils.hpp"
#include <array>
#include <memory>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly::detail
{
// ---------------------------------------------------------------------
// Lock-free multiple-producer / multiple-consumer ring buffer.
// Capacity must be a power-of-two. Algorithm based on Dmitry Vyukov's
// bounded MPMC queue (public domain).
// ---------------------------------------------------------------------
/*  Multi-producer/multi-consumer bounded queue.
 *  http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 *
 *  Copyright (c) 2010-2011, Dmitry Vyukov. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 *     2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY DMITRY VYUKOV "AS IS" AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 *  EVENT SHALL DMITRY VYUKOV OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  The views and conclusions contained in the software and documentation are
 *  those of the authors and should not be interpreted as representing official
 *  policies, either expressed or implied, of Dmitry Vyukov.
 */
template <typename T, size_t Capacity>
class mpmc_ring
{
  static_assert(std::is_trivially_destructible_v<T>, "T must be trivially destructible for lock-free ring");

  static constexpr size_t capacity_pow2 = ouly::detail::next_pow2(Capacity);
  static constexpr size_t mask          = capacity_pow2 - 1;

public:
  mpmc_ring() noexcept
  {
    for (std::size_t i = 0; i < capacity_pow2; ++i)
    {
      buffer_[i].sequence_.store(i, std::memory_order_relaxed);
    }
  }

  ~mpmc_ring() noexcept = default;

  mpmc_ring(mpmc_ring const&)                    = delete;
  auto operator=(mpmc_ring const&) -> mpmc_ring& = delete;
  mpmc_ring(mpmc_ring&&)                         = delete;
  auto operator=(mpmc_ring&&) -> mpmc_ring&      = delete;

  auto push(T&& value) noexcept -> bool
  {
    node_t*     node_ptr = nullptr;
    std::size_t pos      = head_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask];
      std::size_t seq  = node_ptr->sequence_.load(std::memory_order_acquire);
      intptr_t    diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
      if (diff == 0)
      {
        if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        {
          break;
        }
      }
      else if (diff < 0)
      {
        return false; // full
      }
      else
      {
        pos = head_.load(std::memory_order_relaxed);
      }
    }

    node_ptr->storage_ = std::move(value);
    node_ptr->sequence_.store(pos + 1, std::memory_order_release);
    return true;
  }

  template <class... Args>
  auto emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> bool
  {
    node_t*     node_ptr = nullptr;
    std::size_t pos      = head_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask];
      std::size_t seq  = node_ptr->sequence_.load(std::memory_order_acquire);
      intptr_t    diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
      if (diff == 0)
      {
        if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        {
          break;
        }
      }
      else if (diff < 0)
      {
        return false; // full
      }
      else
      {
        pos = head_.load(std::memory_order_relaxed);
      }
    }

    node_ptr->storage_ = T(std::forward<Args>(args)...);
    node_ptr->sequence_.store(pos + 1, std::memory_order_release);
    return true;
  }

  auto pop(T& out) noexcept -> bool
  {
    node_t*     node_ptr = {};
    std::size_t pos      = tail_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask];
      std::size_t seq  = node_ptr->sequence_.load(std::memory_order_acquire);
      intptr_t    diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
      if (diff == 0)
      {
        if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        {
          break;
        }
      }
      else if (diff < 0)
      {
        return false; // empty
      }
      else
      {
        pos = tail_.load(std::memory_order_relaxed);
      }
    }

    // NOLINTNEXTLINE
    out = std::move(*std::launder(reinterpret_cast<T*>(&node_ptr->storage_)));
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
      // NOLINTNEXTLINE
      std::destroy_at(std::launder(reinterpret_cast<T*>(&node_ptr->storage_)));
    }

    node_ptr->sequence_.store(pos + capacity_pow2, std::memory_order_release);
    return true;
  }

  void clear() noexcept
  {
    for (std::size_t i = 0; i < capacity_pow2; ++i)
    {
      buffer_[i].sequence_.store(i, std::memory_order_relaxed);
    }
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
  }

  [[nodiscard]] auto size() const noexcept -> std::size_t
  {
    return head_.load(std::memory_order_relaxed) - tail_.load(std::memory_order_relaxed);
  }

private:
  struct node_t
  {
    alignas(cache_line_size) std::atomic<std::size_t> sequence_{0};
    T storage_{}; // Use placement new to construct T in this storage
  };

  using node_list_t = std::array<node_t, capacity_pow2>;

  alignas(cache_line_size) std::atomic<std::size_t> head_{0};
  alignas(cache_line_size) std::atomic<std::size_t> tail_{0};
  alignas(cache_line_size) node_list_t buffer_;
};

} // namespace ouly::detail

#ifdef _MSC_VER
#pragma warning(pop)
#endif
