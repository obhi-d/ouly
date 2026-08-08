// SPDX-License-Identifier: MIT
#pragma once
#include "ouly/utility/user_config.hpp"

#include "ouly/scheduler/detail/coro_state.hpp"

#include <exception>

namespace ouly
{

namespace detail
{

template <typename Promise>
void resume_coroutine(std::coroutine_handle<Promise> coroutine, typename Promise::context_type const& ctx) noexcept
{
  using context_type = Promise::context_type;
  coroutine_context_guard<context_type> guard(ctx);
  // Whether the frame is ours has to be read before resuming: a borrowed coroutine can be waited
  // on and destroyed by its owner the moment it completes, so the handle may already be dangling
  // by the time resume() returns
  bool const owns_frame = coroutine.promise().detached_;
  coroutine.resume();
  if (owns_frame && coroutine.done())
  {
    coroutine.destroy();
  }
}

template <typename Promise>
auto dispatch_coroutine(std::coroutine_handle<> coroutine) noexcept -> std::coroutine_handle<>
{
  using context_type = Promise::context_type;
  auto* ctx          = coroutine_context_slot<context_type>::current;
  if (ctx == nullptr)
  {
    return coroutine;
  }

  auto typed = std::coroutine_handle<Promise>::from_address(coroutine.address());
  auto group = typed.promise().resume_group_;
  if (!group)
  {
    group = ctx->get_workgroup();
  }
  ctx->get_scheduler().submit(*ctx, group,
                              [typed](context_type const& run_ctx) noexcept -> void
                              {
                                resume_coroutine(typed, run_ctx);
                              });
  return std::noop_coroutine();
}

template <typename Promise>
auto start_coroutine(std::coroutine_handle<Promise> coroutine) noexcept -> std::coroutine_handle<>
{
  auto& state = coroutine.promise();
  if (state.started_.exchange(true, std::memory_order_acq_rel))
  {
    return std::noop_coroutine();
  }
  return dispatch_coroutine<Promise>(coroutine);
}

} // namespace detail

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

    // Scheduler-bound continuations are enqueued as fresh work. A coroutine resumed manually
    // outside a scheduler falls back to symmetric transfer so it still makes progress.
    if (prev && prev != ouly::detail::completed_sentinel())
    {
      auto dispatch = state.continuation_dispatch_.load(std::memory_order_acquire);
      return dispatch != nullptr ? dispatch(prev) : prev;
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

  template <typename AwaitingPromise>
  auto await_suspend(std::coroutine_handle<AwaitingPromise> awaiting_coro) noexcept -> std::coroutine_handle<>
  {
    OULY_ASSERT(awaiting_coro);
    ouly::detail::coro_state& state = coro_.promise();
    if constexpr (requires { typename AwaitingPromise::context_type; })
    {
      state.continuation_dispatch_.store(&ouly::detail::dispatch_coroutine<AwaitingPromise>, std::memory_order_release);
    }
    else
    {
      state.continuation_dispatch_.store(nullptr, std::memory_order_release);
    }
    // Try to install ourselves as the continuation.
    std::coroutine_handle<> expected = nullptr;
    if (state.continuation_.compare_exchange_strong(expected, awaiting_coro, std::memory_order_acq_rel,
                                                    std::memory_order_acquire))
    {
      if (auto* ctx = ouly::detail::coroutine_context_slot<typename PromiseArg::context_type>::current)
      {
        coro_.promise().resume_group_ = ctx->get_workgroup();
      }
      return ouly::detail::start_coroutine(coro_);
    }

    // Failed to install:
    // - If we observed "completed", run inline (do not suspend).
    // - If we observed some *other* non-null, that's a logic error
    //   (multiple awaiters for a single task).
    if (expected != ouly::detail::completed_sentinel())
    {
      OULY_ASSERT(false && "A coroutine task supports only one awaiter");
      std::terminate();
    }
    return awaiting_coro;
  }

  auto await_resume() -> decltype(auto)
  {
    if (!coro_)
    {
      OULY_ASSERT(false && "Invalid state!");
      std::terminate();
    }
    return coro_.promise().result();
  }

private:
  std::coroutine_handle<PromiseArg> coro_;
};

} // namespace ouly
