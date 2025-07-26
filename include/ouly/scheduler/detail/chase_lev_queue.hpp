// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/utility/user_config.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly::detail
{

static constexpr size_t initial_chase_lev_capacity = 256;

/**
 * @brief Chase-Lev work-stealing deque implementation
 *
 * This is a lock-free deque that supports owner push/pop from front (LIFO)
 * and thief pop from back (FIFO). Based on the Chase-Lev algorithm.
 *
 * Owner operations (single-threaded):
 * - push_front: Add work to the front (most recent)
 * - pop_front: Remove work from the front (LIFO order)
 *
 * Thief operations (multi-threaded):
 * - pop_back: Remove work from the back (FIFO order)
 *
 * @tparam T The type of elements stored in the queue
 * @tparam InitialCapacity Initial capacity (must be power of 2)
 */

template <typename T, std::size_t block_tasks = 2>
class chase_lev_queue
{
  static_assert(block_tasks >= 1 && (block_tasks & (block_tasks - 1)) == 0);
  static_assert(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>);

  struct buffer;

public:
  chase_lev_queue(std::size_t cap, std::size_t max_threads)
      : max_threads_{max_threads}, epoch_{0}, hazard_(std::make_unique<std::atomic<buffer*>[]>(max_threads))
  {
    OULY_ASSERT(cap && (cap & (cap - 1)) == 0);
    OULY_ASSERT(max_threads_ > 0);
    array_.store(new buffer(cap, 0), std::memory_order_release);
    for (auto& h : hazard_)
      h.store(nullptr, std::memory_order_relaxed);
    retired_head_.store(nullptr, std::memory_order_relaxed);
  }

  ~chase_lev_queue()
  {
    reclaim(true);
    delete array_.load();
    // Delete any leftovers in retire list
    buffer* node = retired_head_.load();
    while (node)
    {
      buffer* next = node->next;
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      delete node;
      node = next;
    }
  }

  chase_lev_queue(const chase_lev_queue&)                        = delete;
  auto operator=(const chase_lev_queue&) -> chase_lev_queue&     = delete;
  chase_lev_queue(chase_lev_queue&&) noexcept                    = delete;
  auto operator=(chase_lev_queue&&) noexcept -> chase_lev_queue& = delete;

  void push_back(std::size_t slot, const T& item) noexcept
  {
    guard g{hazard_[slot], array_};
    auto* a = g.buf;
    auto  b = bottom_.load(std::memory_order_relaxed);
    auto  t = top_.load(std::memory_order_acquire);

    if (static_cast<std::size_t>(b - t) >= a->capacity() - 1)
    {
      grow(a, t, b);
      a = array_.load(std::memory_order_acquire);
    }
    a->put(b, item);
    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + 1, std::memory_order_relaxed);
  }

  auto pop_front(std::size_t slot, T& out) noexcept -> bool
  {
    guard g{hazard_[slot], array_};
    auto  b = bottom_.load(std::memory_order_relaxed) - 1;
    auto* a = g.buf;
    bottom_.store(b, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto t = top_.load(std::memory_order_relaxed);

    if (t <= b)
    {
      out = a->get(b);
      if (t == b && !top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
      {
        bottom_.store(t + 1, std::memory_order_relaxed);
        return false;
      }
      return true;
    }
    bottom_.store(t, std::memory_order_relaxed);
    return false;
  }

  auto steal(std::size_t slot, T& out) noexcept -> bool
  {
    T           tmp[block_tasks];
    std::size_t n = steal_block(slot, tmp);
    if (n == 0)
    {
      return false;
    }
    out = std::move(tmp[0]);
    return true;
  }

  auto steal_block(std::size_t slot, T* dst) noexcept -> std::size_t
  {
    guard g{hazard_[slot], array_};
    auto* a = g.buf;
    auto  t = top_.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto b = bottom_.load(std::memory_order_acquire);

    auto avail = static_cast<std::size_t>(b - t);
    if (avail == 0)
    {
      return 0;
    }
    std::size_t take  = (avail >= block_tasks ? block_tasks : avail);
    auto        new_t = t + static_cast<std::int64_t>(take);
    if (!top_.compare_exchange_strong(t, new_t, std::memory_order_seq_cst, std::memory_order_relaxed))
    {
      return 0;
    }
    for (std::size_t i = 0; i < take; ++i)
    {
      dst[i] = a->get(t + static_cast<std::int64_t>(i));
    }
    return take;
  }

private:
  /*============================= BUFFER TYPE ============================*/
  struct buffer
  {
    struct slot_t
    {
      alignas(T) unsigned char bytes[sizeof(T)];
    };
    buffer(std::size_t cap, std::uint64_t ep) : capacity_{cap}, mask_{cap - 1}, epoch_{ep},
    {
      data_ = static_cast<slot_t*>(operator new[](cap * sizeof(slot_t), std::align_val_t{alignof(slot_t)}));
    }
    ~buffer() noexcept
    {
      operator delete[](data_, std::align_val_t{alignof(slot_t)});
    }

    void put(std::int64_t idx, const T& v) noexcept
    {
      new (&slot(idx)) T(v);
    }
    T& get(std::int64_t idx) noexcept
    {
      return *std::launder(reinterpret_cast<T*>(&slot(idx)));
    }
    const T& get(std::int64_t idx) const noexcept
    {
      return *std::launder(reinterpret_cast<const T*>(&slot(idx)));
    }
    std::size_t capacity() const noexcept
    {
      return capacity_;
    }
    slot_t& slot(std::int64_t idx) const noexcept
    {
      return data_[idx & mask_];
    }

    std::size_t   capacity_;
    std::size_t   mask_;
    slot_t*       data_;
    std::uint64_t epoch_;
    buffer*       next{}; // Treiber link
  };

  /*========================== HAZARD GUARD ==============================*/
  struct guard
  {
    guard(std::atomic<buffer*>& h, const std::atomic<buffer*>& global) : haz{h}
    {
      buf = global.load(std::memory_order_acquire);
      haz.store(buf, std::memory_order_release);
    }
    ~guard()
    {
      haz.store(nullptr, std::memory_order_release);
    }
    buffer*               buf;
    std::atomic<buffer*>& haz;
  };

  /*============================ GROW / RECLAIM ==========================*/
  void grow(buffer* old, std::int64_t t, std::int64_t b)
  {
    auto* bigger = new buffer(old->capacity_ * 2, epoch_.fetch_add(1, std::memory_order_relaxed) + 1);
    for (std::int64_t i = t; i < b; ++i)
      bigger->put(i, old->get(i));
    array_.store(bigger, std::memory_order_release);
    push_retired(old);
    reclaim(false);
  }

  void push_retired(buffer* node) noexcept
  {
    buffer* head = retired_head_.load(std::memory_order_relaxed);
    do
    {
      node->next = head;
    }
    while (!retired_head_.compare_exchange_weak(head, node, std::memory_order_release, std::memory_order_relaxed));
  }

  void reclaim(bool final)
  {
    std::uint64_t min_ep = epoch_.load(std::memory_order_relaxed);
    for (std::size_t i = 0; i < max_threads_; ++i)
      if (auto* h = hazard_[i].load(std::memory_order_acquire); h)
        min_ep = std::min(min_ep, h->epoch_);

    buffer* prev = nullptr;
    buffer* curr = retired_head_.load(std::memory_order_acquire);
    while (curr)
    {
      buffer* next = curr->next;
      if (final || curr->epoch_ < min_ep)
      {
        if (!prev)
        {
          retired_head_.compare_exchange_strong(curr, next, std::memory_order_acquire, std::memory_order_relaxed);
        }
        else
        {
          prev->next = next;
        }
        delete curr;
        curr = next;
        continue;
      }
      prev = curr;
      curr = next;
    }
  }

  /*=========================== DATA MEMBERS ============================*/
  alignas(cache_line) std::atomic<std::int64_t> top_{0};
  alignas(cache_line) std::atomic<std::int64_t> bottom_{0};
  alignas(cache_line) std::atomic<buffer*> array_{nullptr};

  const std::size_t                       max_threads_;
  std::atomic<std::uint64_t>              epoch_;
  std::unique_ptr<std::atomic<buffer*>[]> hazard_;
  std::atomic<buffer*>                    retired_head_; // Treiber stack
};
} // namespace ouly::detail

#ifdef _MSC_VER
#pragma warning(pop)
#endif