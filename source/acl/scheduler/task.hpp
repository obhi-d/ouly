#pragma once

#include "awaiters.hpp"
#include "promise_type.hpp"

namespace acl
{
/// @brief A worker represents a specific thread. A worker can belong to any of maximum of 32 worker_groups allowed by
/// the scheduler.
class worker_id
{
public:
  worker_id() noexcept = default;
  worker_id(uint32_t id, uint32_t mask) noexcept : index(id), group_mask(mask) {}
  /// @brief Retruns a positive integer != std::numeric_limits<uint32_t>::max() when valid.
  /// @return
  uint32_t get_index() const noexcept
  {
    return index;
  }

  bool belongs_to(uint32_t group_idx) const noexcept
  {
    return group_mask & (1u << group_idx);
  }

private:
  uint32_t index      = 0;
  uint32_t group_mask = 0;
};

struct task_context
{};
using task_delegate = void (*)(task_context*, worker_id);

struct task
{
  virtual ~task() noexcept                                 = default;
  virtual void operator()(task_context*, worker_id worker) = 0;
};

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
  co_task(co_task&& other) : coro(std::move(other.coro))
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

/// @brief Use a coroutine task to defer execute a task, inital state is suspended. Coroutine is only resumed manually
/// and mostly by a scheduler. This task allows waiting on another task and be suspended during execution from any
/// thread. This task can only be waited from a single wait point.
/// @tparam R
template <typename R>
struct co_task : public detail::co_task<R, promise_type<co_task, R>>
{
  using super = detail::co_task<R, promise_type<co_task, R>>;
  using super::co_task;
  inline co_task& operator=(co_task&&) noexcept = default;
};
/// @brief Use a sequence task to immediately executed. The expectation is you will co_await this task on another task,
/// which will result in suspension of execution. This task allows waiting on another task and be suspended during
/// execution from any thread. This task can only be waited from a single wait point.
/// @tparam R
template <typename R>
struct co_sequence : public detail::co_task<R, sequence_promise<co_sequence, R>>
{
  using super = detail::co_task<R, sequence_promise<co_sequence, R>>;
  using super::co_task;
  inline co_sequence& operator=(co_sequence&&) noexcept = default;
};

} // namespace acl