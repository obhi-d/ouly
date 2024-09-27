#pragma once

#include "awaiters.hpp"
#include "event_types.hpp"
#include "promise_type.hpp"
#include "worker_context.hpp"
#include <acl/utils/delegate.hpp>
#include <cstddef>
#include <tuple>

namespace acl
{
class scheduler;
class worker_context;
constexpr uint32_t max_task_data_size = 20;

using task_delegate = acl::basic_delegate<24, void(worker_context const&)>;

namespace detail
{
template <typename R, typename promise>
class co_task
{
public:
  using promise_type = promise;
  using handle       = std::coroutine_handle<promise_type>;

  ~co_task() noexcept
  {
    if (coro)
      coro.destroy();
  }

  co_task(handle h) : coro(h) {}
  co_task(co_task&& other) noexcept : coro(std::move(other.coro))
  {
    other.coro = nullptr;
  }
  inline co_task& operator=(co_task const&) = delete;
  inline co_task& operator=(co_task&& other) noexcept
  {
    if (coro)
      coro.destroy();
    coro       = std::move(other.coro);
    other.coro = nullptr;
    return *this;
  }

  auto operator co_await() const& noexcept
  {
    return awaiter<promise_type>(coro);
  }

  [[nodiscard]] bool is_done() const noexcept
  {
    return !coro || coro.done();
  }

  [[nodiscard]] explicit operator bool() const noexcept
  {
    return !!coro;
  }

  decltype(auto) address() const
  {
    return coro.address();
  }

  R result() noexcept
  {
    if constexpr (!std::is_same_v<R, void>)
      return coro.promise().result();
  }

  void resume() noexcept
  {
    coro.resume();
  }

  /**
   * @brief Returns result after waiting for the task to finish, blocks the current thread until work is done
   */
  R sync_wait_result() noexcept
  {
    blocking_event event;
    detail::wait(event, *this);
    event.wait();
    if constexpr (!std::is_same_v<R, void>)
      return coro.promise().result();
  }

  /**
   * @brief Returns result after waiting for the task to finish, with a non-blocking event, that tries to do work when
   * this coro is not available
   */
  R sync_wait_result(worker_id worker, scheduler& s) noexcept
  {
    busywork_event event;
    detail::wait(event, *this);
    event.wait(worker, s);
    if constexpr (!std::is_same_v<R, void>)
      return coro.promise().result();
  }

protected:
  inline handle release() noexcept
  {
    auto h = coro;
    coro   = nullptr;
    return h;
  }

private:
  handle coro = {};
};
} // namespace detail

template <typename T>
concept CoroutineTask = requires(T a) {
  {
    a.address()
  } -> std::same_as<void*>;
};

/**
 * @brief Use a coroutine task to defer execute a task, inital state is suspended. Coroutine is only resumed manually
 * and mostly by a scheduler. This task allows waiting on another task and be suspended during execution from any
 * thread. This task can only be waited from a single wait point.
 * @tparam R
 */
template <typename R>
struct co_task : public detail::co_task<R, detail::promise_type<co_task, R>>
{
  using super = detail::co_task<R, detail::promise_type<co_task, R>>;
  using typename super::handle;

  co_task(handle h) : super(h) {}
  co_task(co_task&& other) noexcept : super(other.release()) {}
  inline co_task& operator=(co_task const&) = delete;
  inline co_task& operator=(co_task&& other) noexcept
  {
    (super&)(*this) = std::move<super>(other);
    return *this;
  }
};
/**
 * @brief Use a sequence task to immediately executed. The expectation is you will co_await this task on another task,
 * which will result in suspension of execution. This task allows waiting on another task and be suspended during
 * execution from any thread. This task can only be waited from a single wait point.
 * @tparam R
 */
template <typename R>
struct co_sequence : public detail::co_task<R, detail::sequence_promise<co_sequence, R>>
{
  using super = detail::co_task<R, detail::sequence_promise<co_sequence, R>>;
  using typename super::handle;

  co_sequence(handle h) : super(h) {}
  co_sequence(co_sequence&& other) noexcept : super(std::move<super>((super&&)other)) {}
  inline co_sequence& operator=(co_sequence const&) = delete;
  inline co_sequence& operator=(co_sequence&& other) noexcept
  {
    (super&)(*this) = std::move<super>((super&&)other);
    return *this;
  }
};

} // namespace acl
