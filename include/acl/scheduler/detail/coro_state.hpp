#pragma once

#include <atomic>
#include <coroutine>

namespace acl::detail
{
struct coro_state
{
  std::coroutine_handle<> continuation_       = nullptr;
  std::atomic_bool        continuation_state_ = false;
};
} // namespace acl::detail