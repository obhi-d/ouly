// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>

namespace ouly
{
struct spin_lock
{
  void lock() noexcept
  {
    while (!try_lock())
    {
      flag_.wait(true, std::memory_order_relaxed);
    }
  }

  auto try_lock() noexcept -> bool
  {
    return !flag_.test_and_set(std::memory_order_acquire);
  }

  template <typename Notify = std::true_type>
  void unlock() noexcept
  {
    flag_.clear(std::memory_order_release);
    if constexpr (Notify::value)
    {
      flag_.notify_one();
    }
  }

private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

} // namespace ouly