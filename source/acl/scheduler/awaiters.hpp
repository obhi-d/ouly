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
  std::coroutine_handle<> continuation       = nullptr;
  std::atomic_bool        continuation_state = false;
};
} // namespace detail

class final_awaiter
{
public:
  bool await_ready() const noexcept
  {
    return false;
  }

  template <typename AwaiterPromise>
  void await_suspend(std::coroutine_handle<AwaiterPromise> awaiting_coro) noexcept
  {
    detail::coro_state& state = awaiting_coro.promise();
    if (state.continuation_state.exchange(true))
      state.continuation.resume();
  }

  void await_resume() noexcept {}
};

template <typename PromiseArg>
class awaiter
{
public:
  awaiter(std::coroutine_handle<PromiseArg> handle) : coro(handle) {}

  bool await_ready() const noexcept
  {
    return false;
  }

  bool await_suspend(std::coroutine_handle<> awaiting_coro) noexcept
  {
    assert(awaiting_coro);
    detail::coro_state& state = coro.promise();
    assert(!state.continuation);
    // set continuation
    state.continuation = awaiting_coro;
    return state.continuation_state.exchange(true) == false;
  }

  decltype(auto) await_resume() noexcept
  {
    if (!coro)
    {
      // .. terminate ?
      assert(false && "Invalid state!");
    }
    return coro.promise().result();
  }

private:
  std::coroutine_handle<PromiseArg> coro;
};

namespace detail
{
template <typename Awaiter>
concept HasMemberCoAwait = requires(Awaiter w) {
  {
    w.operator co_await()
  };
};

template <typename Awaiter>
concept HasFreeCoAwait = requires(Awaiter w) {
  {
    operator co_await(w)
  };
};

template <typename Awaiter>
concept HasCoAwait = HasFreeCoAwait<Awaiter> || HasFreeCoAwait<Awaiter>;

template <typename Awaiter>
using awaiter_result_t = decltype(std::declval<Awaiter&>().await_resume());

template <HasMemberCoAwait Awaitable>
decltype(auto) get_awaiter(Awaitable&& awaitable) noexcept
{
  return static_cast<Awaitable&&>(awaitable).operator co_await();
}

template <HasFreeCoAwait Awaitable>
decltype(auto) get_awaiter(Awaitable&& awaitable) noexcept
{
  return operator co_await(static_cast<Awaitable&&>(awaitable));
}

template <typename Awaitable>
using awaiter_t = decltype(get_awaiter(std::declval<Awaitable>()));

} // namespace detail

} // namespace acl