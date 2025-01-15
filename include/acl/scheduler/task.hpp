#pragma once

#include <acl/scheduler/detail/co_task.hpp>
#include <acl/scheduler/event_types.hpp>
#include <acl/utility/delegate.hpp>
#include <concepts>

namespace acl
{
class scheduler;
class worker_context;
constexpr uint32_t max_task_data_size = 20;
constexpr uint32_t max_task_base_size = 24;

using task_delegate = acl::basic_delegate<max_task_base_size, void(worker_context const&)>;

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
struct co_task : public acl::detail::co_task<R, acl::detail::promise_type<co_task, R>>
{
  using super = acl::detail::co_task<R, acl::detail::promise_type<co_task, R>>;
  using typename super::handle;

  co_task() noexcept      = default;
  co_task(const co_task&) = delete;
  co_task(handle h) : super(h) {}
  co_task(co_task&& other) noexcept : super(other.release()) {}
  ~co_task() noexcept                        = default;
  auto operator=(co_task const&) -> co_task& = delete;
  auto operator=(co_task&& other) noexcept -> co_task&
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
struct co_sequence : public acl::detail::co_task<R, acl::detail::sequence_promise<co_sequence, R>>
{
  using super = acl::detail::co_task<R, acl::detail::sequence_promise<co_sequence, R>>;
  using typename super::handle;

  co_sequence() noexcept          = default;
  co_sequence(const co_sequence&) = delete;
  co_sequence(handle h) : super(h) {}
  co_sequence(co_sequence&& other) noexcept : super(std::move<super>((super&&)other)) {}
  ~co_sequence() noexcept                            = default;
  auto operator=(co_sequence const&) -> co_sequence& = delete;
  auto operator=(co_sequence&& other) noexcept -> co_sequence&
  {
    (super&)(*this) = std::move<super>((super&&)other);
    return *this;
  }
};

} // namespace acl
