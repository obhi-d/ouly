#pragma once

#include <acl/scheduler/detail/coro_state.hpp>
#include <cassert>

namespace acl
{

class final_awaiter
{
public:
  [[nodiscard]] static auto await_ready() noexcept -> bool
  {
    return false;
  }

  template <typename AwaiterPromise>
  void await_suspend(std::coroutine_handle<AwaiterPromise> awaiting_coro) noexcept
  {
    acl::detail::coro_state& state = awaiting_coro.promise();
    if (state.continuation_state_.exchange(true))
    {
      state.continuation_.resume();
    }
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
    return false;
  }

  auto await_suspend(std::coroutine_handle<> awaiting_coro) noexcept -> bool
  {
    assert(awaiting_coro);
    acl::detail::coro_state& state = coro_.promise();
    assert(!state.continuation_);
    // set continuation
    state.continuation_ = awaiting_coro;
    return !state.continuation_state_.exchange(true);
  }

  auto await_resume() noexcept -> decltype(auto)
  {
    if (!coro_)
    {
      // .. terminate ?
      assert(false && "Invalid state!");
    }
    return coro_.promise().result();
  }

private:
  std::coroutine_handle<PromiseArg> coro_;
};

} // namespace acl