// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/scheduler/detail/get_awaiter.hpp"
#include "ouly/scheduler/detail/promise_type.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include <concepts>
#include <semaphore>

namespace ouly::detail
{
template <typename R, typename Promise>
class co_task
{
public:
  using promise_type = Promise;
  using handle       = std::coroutine_handle<promise_type>;

  ~co_task() noexcept
  {
    if (coro_)
    {
      coro_.destroy();
    }
  }

  co_task() noexcept      = default;
  co_task(const co_task&) = delete;
  co_task(handle h) : coro_(h) {}
  co_task(co_task&& other) noexcept : coro_(std::move(other.coro_))
  {
    other.coro_ = nullptr;
  }
  auto operator=(co_task const&) -> co_task& = delete;
  auto operator=(co_task&& other) noexcept -> co_task&
  {
    if (coro_)
    {
      coro_.destroy();
    }
    coro_       = std::move(other.coro_);
    other.coro_ = nullptr;
    return *this;
  }

  auto operator co_await() const& noexcept
  {
    return awaiter<promise_type>(coro_);
  }

  [[nodiscard]] auto is_done() const noexcept -> bool
  {
    return !coro_ || coro_.done();
  }

  [[nodiscard]] explicit operator bool() const noexcept
  {
    return !!coro_;
  }

  auto address() const -> decltype(auto)
  {
    return coro_.address();
  }

  auto result() -> R
  {
    if constexpr (std::is_same_v<R, void>)
    {
      coro_.promise().result();
    }
    else
    {
      return coro_.promise().result();
    }
  }

  void resume() noexcept
  {
    coro_.resume();
  }

  /**
   * @brief Returns result after waiting for the task to finish, blocks the current thread until work is done
   */
  auto wait() -> R
  {
    std::binary_semaphore event{0};
    ouly::detail::wait(&event, this);
    event.acquire();
    if constexpr (std::is_same_v<R, void>)
    {
      coro_.promise().result();
    }
    else
    {
      return coro_.promise().result();
    }
  }

  /**
   * @brief Returns result after waiting for the task to finish, with a non-blocking event, that tries to do work when
   * this coro is not available
   */
  template <TaskContext WC>
    requires std::same_as<WC, typename promise_type::context_type>
  auto cooperative_wait(WC const& ctx) -> R
  {
    std::binary_semaphore event{0};
    ouly::detail::coroutine_context_guard<WC> guard(ctx);
    ouly::detail::wait(&event, this);
    ctx.cooperative_wait(event);
    if constexpr (std::is_same_v<R, void>)
    {
      coro_.promise().result();
    }
    else
    {
      return coro_.promise().result();
    }
  }

protected:
  template <typename C>
  friend struct co_lambda_executor;

  auto release() noexcept -> handle
  {
    auto h = coro_;
    coro_  = nullptr;
    return h;
  }

private:
  handle coro_ = {};
};

template <typename C>
struct co_lambda_executor
{
  using handle_type = C::handle;
  // NOLINTNEXTLINE
  co_lambda_executor(C&& c) : coro_(c.release()) {}

  template <typename TC>
  void operator()(TC const& ctx)
  {
    auto& promise = coro_.promise();
    promise.detached_     = true;
    promise.resume_group_ = ctx.get_workgroup();
    auto const was_started = promise.started_.exchange(true, std::memory_order_acq_rel);
    OULY_ASSERT(!was_started && "A coroutine task can only be submitted once");
    if (!was_started)
    {
      if constexpr (std::is_same_v<TC, typename C::promise_type::context_type>)
      {
        ouly::detail::resume_coroutine(coro_, ctx);
      }
      else
      {
        coro_.resume();
        if (coro_.done())
        {
          coro_.destroy();
        }
      }
    }
  }

  handle_type coro_ = {};
};

template <typename C>
struct co_borrowed_executor
{
  using handle_type = typename C::handle;

  explicit co_borrowed_executor(C const& coroutine)
      : coro_(handle_type::from_address(coroutine.address()))
  {}

  template <typename TC>
  void operator()(TC const& ctx)
  {
    auto& promise         = coro_.promise();
    promise.resume_group_ = ctx.get_workgroup();
    if (!promise.started_.exchange(true, std::memory_order_acq_rel))
    {
      if constexpr (std::is_same_v<TC, typename C::promise_type::context_type>)
      {
        ouly::detail::resume_coroutine(coro_, ctx);
      }
      else
      {
        coro_.resume();
      }
    }
  }

  handle_type coro_ = {};
};

} // namespace ouly::detail
