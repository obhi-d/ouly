// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <coroutine>

namespace ouly::detail
{

inline auto completed_sentinel() noexcept -> std::coroutine_handle<>
{
  return std::noop_coroutine();
}

struct coro_state
{
  std::atomic<std::coroutine_handle<>> continuation_{nullptr};
};
} // namespace ouly::detail