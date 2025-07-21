
#pragma once

#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/utility/utils.hpp"
#include <array>

namespace ouly::detail
{
// ---------------------------------------------------------------------
// Lock-free multiple-producer / multiple-consumer ring buffer.
// Capacity must be a power-of-two. Algorithm based on Dmitry Vyukov's
// bounded MPMC queue (public domain).
// ---------------------------------------------------------------------
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
    node*       node_ptr = nullptr;
    std::size_t pos      = head_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask_];
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

    new (&node_ptr->storage_) T(std::move(value));
    node_ptr->sequence_.store(pos + 1, std::memory_order_release);
    return true;
  }

  template <class... Args>
  auto emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> bool
  {
    node*       node_ptr = nullptr;
    std::size_t pos      = head_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask_];
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

    new (&node_ptr->storage_) T(std::forward<Args>(args)...);
    node_ptr->sequence_.store(pos + 1, std::memory_order_release);
    return true;
  }

  auto pop(T& out) noexcept -> bool
  {
    node*       node_ptr = {};
    std::size_t pos      = tail_.load(std::memory_order_relaxed);
    for (;;)
    {
      node_ptr         = &buffer_[pos & mask_];
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

  auto empty() const noexcept -> bool
  {
    std::size_t pos      = tail_.load(std::memory_order_acquire);
    node const* node_ptr = &buffer_[pos & mask];
    return static_cast<intptr_t>(node_ptr->sequence_.load(std::memory_order_relaxed)) - static_cast<intptr_t>(pos + 1) <
           0;
  }

private:
  union node
  {
    std::atomic<std::size_t>                      sequence_;
    std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
  };

  using node_list_t = std::array<node, capacity_pow2>;

  alignas(cache_line_size) std::atomic<std::size_t> head_{0};
  cache_aligned_padding<std::atomic<std::size_t>> head_padding_; // Prevent false sharing
  alignas(cache_line_size) std::atomic<std::size_t> tail_{0};
  cache_aligned_padding<std::atomic<std::size_t>> tail_padding_; // Prevent false sharing
  alignas(cache_line_size) node_list_t buffer_;
};

} // namespace ouly::detail
