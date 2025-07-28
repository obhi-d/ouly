// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/scheduler/detail/get_awaiter.hpp"
#include "ouly/scheduler/detail/promise_type.hpp"
#include "ouly/scheduler/worker_structs.hpp"
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

  auto result() noexcept -> R
  {
    if constexpr (!std::is_same_v<R, void>)
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
  auto sync_wait_result() noexcept -> R
  {
    std::binary_semaphore event{0};
    ouly::detail::wait(&event, this);
    event.acquire();
    if constexpr (!std::is_same_v<R, void>)
    {
      return coro_.promise().result();
    }
  }

  /**
   * @brief Returns result after waiting for the task to finish, with a non-blocking event, that tries to do work when
   * this coro is not available
   */
  template <TaskContext WC>
  auto busy_wait_result(WC const& ctx) noexcept -> R
  {
    std::binary_semaphore event{0};
    ouly::detail::wait(&event, this);
    ctx.busy_wait(event);
    if constexpr (!std::is_same_v<R, void>)
    {
      return coro_.promise().result();
    }
  }

protected:
  auto release() noexcept -> handle
  {
    auto h = coro_;
    coro_  = nullptr;
    return h;
  }

private:
  handle coro_ = {};
};
} // namespace ouly::detail
