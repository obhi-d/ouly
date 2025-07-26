// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/detail/co_task.hpp"
#include <concepts>

namespace ouly
{

template <typename T>
concept CoroutineTask = requires(T a) {
  { a.address() } -> std::same_as<void*>;
};

/**
 * @brief Use a coroutine task to defer execute a task, inital state is suspended. Coroutine is only resumed manually
 * and mostly by a scheduler. This task allows waiting on another task and be suspended during execution from any
 * thread. This task can only be waited from a single wait point.
 * @tparam R
 */
template <typename R>
struct co_task : public ouly::detail::co_task<R, ouly::detail::promise_type<co_task, R>>
{
  using super = ouly::detail::co_task<R, ouly::detail::promise_type<co_task, R>>;
  using typename super::handle;

  co_task() noexcept      = default;
  co_task(const co_task&) = delete;
  co_task(handle h) : super(h) {}
  co_task(co_task&& other) noexcept : super(other.release()) {}
  ~co_task() noexcept                        = default;
  auto operator=(co_task const&) -> co_task& = delete;
  auto operator=(co_task&& other) noexcept -> co_task&
  {
    static_cast<super&>(*this) = std::move<super>(other);
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
struct co_sequence : public ouly::detail::co_task<R, ouly::detail::sequence_promise<co_sequence, R>>
{
  using super = ouly::detail::co_task<R, ouly::detail::sequence_promise<co_sequence, R>>;
  using typename super::handle;

  co_sequence() noexcept          = default;
  co_sequence(const co_sequence&) = delete;
  co_sequence(handle h) : super(h) {}
  co_sequence(co_sequence&& other) noexcept : super(std::move<super>(static_cast<super&&>(other))) {}
  ~co_sequence() noexcept                            = default;
  auto operator=(co_sequence const&) -> co_sequence& = delete;
  auto operator=(co_sequence&& other) noexcept -> co_sequence&
  {
    static_cast<super&>(*this) = std::move<super>(static_cast<super&&>(other));
    return *this;
  }
};

} // namespace ouly
