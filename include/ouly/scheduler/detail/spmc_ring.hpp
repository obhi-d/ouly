// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/utility/user_config.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly::detail
{

static constexpr size_t spmc_default_capacity = 256;
// NOLINTNEXTLINE

template <typename T, std::size_t Capacity = spmc_default_capacity>
  requires(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
class spmc_ring
{
  static_assert((Capacity > 0) && (Capacity & (Capacity - 1)) == 0, "Capacity must be a power-of-two > 0");

  static constexpr size_t module_mask = Capacity - 1; // For modulo operation with power-of-two capacity

public:
  spmc_ring() noexcept  = default;
  ~spmc_ring() noexcept = default;

  spmc_ring(const spmc_ring&)                        = delete;
  auto operator=(const spmc_ring&) -> spmc_ring&     = delete;
  spmc_ring(spmc_ring&&) noexcept                    = default;
  auto operator=(spmc_ring&&) noexcept -> spmc_ring& = default;

  /*===============================  PRODUCER  ===============================*/
  /** Push item only from a single thread */
  auto push_back(const T& item) noexcept -> bool
  {
    const size_t b = bottom_.load(std::memory_order_relaxed);
    const size_t t = top_.load(std::memory_order_acquire);

    // Check if the deque is full.
    if (b - t >= Capacity)
    {
      return false;
    }

    buffer_[b & module_mask] = item;

    // A release store ensures that the write to the buffer is visible
    // to other threads before the update to 'bottom'.
    bottom_.store(b + 1, std::memory_order_release);

    return true;
  }

  /** Pop item only from a single thread, same thread as `push_back` */
  auto pop_back(T& out) noexcept -> bool // owner only
  {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_relaxed);

    // Quick check if the deque is empty.
    if (t >= b)
    {
      return false;
    }

    // Reserve an item from the bottom.
    b -= 1;
    bottom_.store(b, std::memory_order_relaxed);

    // This fence is crucial. It prevents the CPU from reordering the store to 'bottom'
    // with the subsequent load of 'top'. This ensures we race fairly with stealers.
    std::atomic_thread_fence(std::memory_order_seq_cst);

    t = top_.load(std::memory_order_relaxed);
    if (t <= b)
    {
      out = buffer_[b & module_mask];
      if (t == b)
      {
        // last element: compete with thieves
        std::size_t expected = t;
        if (!top_.compare_exchange_strong(expected, t + 1, std::memory_order_seq_cst, std::memory_order_seq_cst))
        {
          // lost the race, restore
          bottom_.store(t + 1, std::memory_order_relaxed);
          return false;
        }
        bottom_.store(t + 1, std::memory_order_relaxed);
      }
      return true;
    }
    // deque became empty
    bottom_.store(t, std::memory_order_relaxed);
    return false;
  }

  /*==============================  CONSUMER  ==============================*/
  /**
   * Steal one item; returns true if found. Can be stolen from any thread.
   */
  auto steal(T& dst) noexcept -> bool
  {
    size_t t = top_.load(std::memory_order_acquire);

    // The fence prevents reordering of the 'top' load with the 'bottom' load,
    // ensuring we get a consistent (though not necessarily current) view of the deque's state.
    std::atomic_thread_fence(std::memory_order_seq_cst);

    size_t b = bottom_.load(std::memory_order_acquire);
    if (t < b)
    {
      dst                  = buffer_[t & module_mask];
      std::size_t expected = t;
      if (top_.compare_exchange_strong(expected, t + 1, std::memory_order_seq_cst, std::memory_order_seq_cst))
      {
        return true;
      }
    }
    return false;
  }

  void clear()
  {
    // Reset the ring buffer by clearing the top and bottom indices.
    top_.store(0, std::memory_order_relaxed);
    bottom_.store(0, std::memory_order_relaxed);
  }

  [[nodiscard]] auto size() const noexcept -> size_t
  {
    return bottom_.load(std::memory_order_relaxed) - top_.load(std::memory_order_relaxed);
  }

private:
  /*-------------------------------- data members -----------------------------*/
  alignas(cache_line_size) std::atomic<size_t> top_{0};    // thieves CAS on this
  alignas(cache_line_size) std::atomic<size_t> bottom_{0}; // producer only writes
  alignas(cache_line_size) std::array<T, Capacity> buffer_ = {};
};

} // namespace ouly::detail

#ifdef _MSC_VER
#pragma warning(pop)
#endif
