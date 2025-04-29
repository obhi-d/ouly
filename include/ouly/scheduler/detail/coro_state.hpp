// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <coroutine>

namespace ouly::detail
{
struct coro_state
{
  std::coroutine_handle<> continuation_       = nullptr;
  std::atomic_bool        continuation_state_ = false;
};
} // namespace ouly::detail