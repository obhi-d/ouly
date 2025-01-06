#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>
#include <cstdint>

namespace acl
{
namespace detail
{
struct coro_state
{
  std::coroutine_handle<> continuation_       = nullptr;
  std::atomic_bool        continuation_state_ = false;
};
} // namespace detail

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
    detail::coro_state& state = awaiting_coro.promise();
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
    detail::coro_state& state = coro_.promise();
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

namespace detail
{
template <typename Awaiter>
concept HasMemberCoAwait = requires(Awaiter w) {
  { w.operator co_await() };
};

template <typename Awaiter>
concept HasFreeCoAwait = requires(Awaiter w) {
  { operator co_await(w) };
};

template <typename Awaiter>
concept HasCoAwait = HasFreeCoAwait<Awaiter> || HasFreeCoAwait<Awaiter>;

template <typename Awaiter>
using awaiter_result_t = decltype(std::declval<Awaiter&>().await_resume());

template <HasMemberCoAwait Awaitable>
auto get_awaiter(Awaitable&& awaitable) noexcept -> decltype(auto)
{
  return static_cast<Awaitable&&>(std::forward<Awaitable>(awaitable)).operator co_await();
}

template <HasFreeCoAwait Awaitable>
auto get_awaiter(Awaitable&& awaitable) noexcept -> decltype(auto)
{
  return operator co_await(static_cast<Awaitable&&>(std::forward<Awaitable>(awaitable)));
}

template <typename Awaitable>
using awaiter_t = decltype(get_awaiter(std::declval<Awaitable>()));

} // namespace detail

} // namespace acl