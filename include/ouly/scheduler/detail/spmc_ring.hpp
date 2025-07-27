// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/detail/cache_optimized_data.hpp"
#include "ouly/utility/user_config.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

namespace ouly::detail
{

static constexpr size_t spmc_default_capacity = 256;
// NOLINTNEXTLINE
#define LOCK_FREE_QUEUE
#ifndef LOCK_FREE_QUEUE

template <typename T, std::size_t Capacity = spmc_default_capacity>
class spmc_ring
{
  static_assert((Capacity > 0) && (Capacity & (Capacity - 1)) == 0, "Capacity must be a power‑of‑two > 0");
  static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
                "T must be trivially copyable & trivially destructible");

public:
  void validate_thread()
  {
    static thread_local auto thread_id = std::this_thread::get_id();
    assert(thread_id == std::this_thread::get_id());
  }

  /*===============================  PRODUCER  ===============================*/
  auto push_back(const T& item) noexcept -> bool
  {
    validate_thread();
    auto lock = std::scoped_lock(lock_);
    auto b    = bottom_;
    auto t    = top_;
    if (static_cast<std::size_t>(b - t) >= Capacity)
    {
      return false; // full
    }

    // memcpy is fine
    auto& data = slot(b);
    std::memcpy(&data, &item, sizeof(T));
    bottom_ += 1;
    return true;
  }

  auto pop_back(T& out) noexcept -> bool // owner only
  {
    validate_thread();
    auto lock = std::scoped_lock(lock_);
    bottom_ -= 1;
    auto b = bottom_;
    auto t = top_;

    if (t <= b)
    { // non‑empty
      // NOLINTNEXTLINE
      auto& data = slot(b);
      std::memcpy(&out, &data, sizeof(T));
      return true;
    }
    // queue empty – restore bottom
    bottom_ = t;
    return false;
  }

  /*==============================  CONSUMER  ==============================*/
  /**
   * Steal one item; returns actual count (0 ⇒ empty or CAS lost).
   * Caller must have space in `dst` for one item.
   */
  auto steal(T& dst) noexcept -> bool
  {
    auto lock = std::scoped_lock(lock_);
    auto t    = top_;
    auto b    = bottom_;

    if (b <= t)
    {
      return false; // empty
    }

    auto new_t = t + 1;
    top_       = new_t;
    auto& data = slot(t);
    std::memcpy(&dst, &data, sizeof(T));
    return true;
  }

private:
  auto slot(std::int64_t idx) noexcept -> T&
  {
    return buffer_[static_cast<uint32_t>(idx) & (Capacity - 1)];
  }
  std::mutex lock_;
  int64_t    top_    = 0;
  int64_t    bottom_ = 0; // producer only writes
  alignas(cache_line_size) T buffer_[Capacity];
};

#else

template <typename T, std::size_t Capacity = spmc_default_capacity>
class spmc_ring
{
  static_assert((Capacity > 0) && (Capacity & (Capacity - 1)) == 0, "Capacity must be a power‑of‑two > 0");
  static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
                "T must be trivially copyable & trivially destructible");

public:
  spmc_ring() noexcept  = default;
  ~spmc_ring() noexcept = default;

  spmc_ring(const spmc_ring&)                        = delete;
  auto operator=(const spmc_ring&) -> spmc_ring&     = delete;
  spmc_ring(spmc_ring&&) noexcept                    = default;
  auto operator=(spmc_ring&&) noexcept -> spmc_ring& = default;

  /*===============================  PRODUCER  ===============================*/
  auto push_back(const T& item) noexcept -> bool
  {
    auto b = bottom_.load(std::memory_order_relaxed);
    auto t = top_.load(std::memory_order_acquire);
    if (static_cast<std::size_t>(b - t) >= Capacity)
    {
      return false; // full
    }

    // memcpy is fine
    auto& data = slot(b);
    std::memcpy(&data, &item, sizeof(T));
    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + 1, std::memory_order_relaxed);
    return true;
  }

  auto pop_back(T& out) noexcept -> bool // owner only
  {
    
    auto b = bottom_.load(std::memory_order_relaxed) - 1;
    bottom_.store(b, std::memory_order_relaxed);

    std::atomic_thread_fence(std::memory_order_seq_cst);

    auto t = top_.load(std::memory_order_relaxed);

    auto size = b - t;
    if (size < 0)
    {
      // queue empty – restore bottom
      bottom_.store(b + 1, std::memory_order_relaxed);
      return false;
    }

    auto& data = slot(b);
    std::memcpy(&out, &data, sizeof(T));

    if (size > 0)
      return true;
    
    if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
    {
      // lost race – revert and fail
      bottom_.store(b + 1, std::memory_order_relaxed);
      return false;
    }
    
    // queue empty – restore bottom
    bottom_.store(b + 1, std::memory_order_relaxed);
    return true;
  }

  /*==============================  CONSUMER  ==============================*/
  /**
   * Steal one item; returns true if found.
   */
  auto steal(T& dst) noexcept -> bool
  {
    auto t = top_.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto b = bottom_.load(std::memory_order_acquire);

    if (b <= t)
    {
      return false; // empty
    }

    if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
    {
      return false;
    }

    
    auto& data = slot(t);
    std::memcpy(&dst, &data, sizeof(T));

    return true;
  }

private:
  /*-------------------------------- slot helper ------------------------------*/

  auto slot(std::int64_t idx) noexcept -> T&
  {
    return buffer_[static_cast<uint32_t>(idx) & (Capacity - 1)];
  }

  /*-------------------------------- data members -----------------------------*/
  alignas(cache_line_size) std::atomic<std::int64_t> top_;    // thieves CAS on this
  alignas(cache_line_size) std::atomic<std::int64_t> bottom_; // producer only writes
  alignas(cache_line_size) T buffer_[Capacity];
};
#endif
} // namespace ouly::detail

#ifdef _MSC_VER
#pragma warning(pop)
#endif
