// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/utility/user_config.hpp"

#include "ouly/scheduler/detail/coro_state.hpp"

namespace ouly
{

class final_awaiter
{
public:
  [[nodiscard]] static auto await_ready() noexcept -> bool
  {
    return false;
  }

  template <typename AwaiterPromise>
  auto await_suspend(std::coroutine_handle<AwaiterPromise> awaiting_coro) noexcept -> std::coroutine_handle<>
  {
    ouly::detail::coro_state& state = awaiting_coro.promise();

    std::coroutine_handle<> prev =
     state.continuation_.exchange(ouly::detail::completed_sentinel(), std::memory_order_acq_rel);

    // If a continuation was installed, *symmetrically transfer* into it.
    if (prev && prev != ouly::detail::completed_sentinel())
    {
      return prev; // tail-call jump to continuation
    }

    // No continuation yet: return noop; the runtime will “resume” it (no-op)
    // and control will unwind to the resumer.
    return ouly::detail::completed_sentinel();
  }

  void await_resume() noexcept {}
};

template <typename PromiseArg>
class awaiter
{
public:
  awaiter(std::coroutine_handle<PromiseArg> handle) : coro_(handle) {}

  [[nodiscard]] auto await_ready() const noexcept -> bool
  {
    auto& state = static_cast<ouly::detail::coro_state&>(coro_.promise());
    return state.continuation_.load(std::memory_order_acquire) == ouly::detail::completed_sentinel();
  }

  auto await_suspend(std::coroutine_handle<> awaiting_coro) noexcept -> bool
  {
    OULY_ASSERT(awaiting_coro);
    ouly::detail::coro_state& state = coro_.promise();
    // Try to install ourselves as the continuation.
    std::coroutine_handle<> expected = nullptr;
    if (state.continuation_.compare_exchange_strong(expected, awaiting_coro, std::memory_order_acq_rel,
                                                    std::memory_order_acquire))
    {
      // Successfully installed continuation: suspend.
      return true;
    }

    // Failed to install:
    // - If we observed "completed", run inline (do not suspend).
    // - If we observed some *other* non-null, that's a logic error
    //   (multiple awaiters for a single task).
    OULY_ASSERT(expected == ouly::detail::completed_sentinel() && "Illegal state: multiple awaiters?");
    (void)expected;
    return false; // continue inline
  }

  auto await_resume() noexcept -> decltype(auto)
  {
    if (!coro_)
    {
      // .. terminate ?
      OULY_ASSERT(false && "Invalid state!");
    }
    return coro_.promise().result();
  }

private:
  std::coroutine_handle<PromiseArg> coro_;
};

} // namespace ouly
