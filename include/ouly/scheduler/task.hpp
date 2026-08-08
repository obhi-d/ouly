// SPDX-License-Identifier: MIT
#pragma once

#include "ouly/scheduler/awaiters.hpp"
#include "ouly/scheduler/config.hpp"
#include "ouly/scheduler/detail/allocation.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/user_config.hpp"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <mutex>
#include <optional>
#include <ranges>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ouly
{

template <typename T, TaskContext WC>
class basic_task;

template <TaskContext WC>
class basic_task_scope;

namespace detail
{

template <TaskContext WC>
class task_state_base;

template <typename T, TaskContext WC>
class basic_task_awaiter;

template <TaskContext WC>
struct continuation_base
{
  continuation_base() noexcept                                      = default;
  virtual ~continuation_base() noexcept                             = default;
  continuation_base(continuation_base const&)                       = delete;
  auto operator=(continuation_base const&) -> continuation_base&    = delete;
  continuation_base(continuation_base&&)                            = delete;
  auto         operator=(continuation_base&&) -> continuation_base& = delete;
  virtual void schedule(WC const& ctx) noexcept                     = 0;

  continuation_base* next_ = nullptr;
};

template <TaskContext WC>
struct task_execution_slot
{
  inline static thread_local task_state_base<WC>* current = nullptr;
};

template <TaskContext WC>
class task_execution_guard
{
public:
  explicit task_execution_guard(task_state_base<WC>* state) noexcept
      : previous_(std::exchange(task_execution_slot<WC>::current, state))
  {
    state->executing_parent_ = previous_;
  }

  ~task_execution_guard() noexcept
  {
    task_execution_slot<WC>::current = previous_;
  }

  task_execution_guard(task_execution_guard const&)                    = delete;
  auto operator=(task_execution_guard const&) -> task_execution_guard& = delete;
  task_execution_guard(task_execution_guard&&)                         = delete;
  auto operator=(task_execution_guard&&) -> task_execution_guard&      = delete;

private:
  task_state_base<WC>* previous_ = nullptr;
};

template <TaskContext WC>
class task_state_base
{
public:
  using destroy_fn = void (*)(task_state_base*) noexcept;

  task_state_base(scheduler_allocator allocator, destroy_fn destroy) noexcept : allocator_(allocator), destroy_(destroy)
  {}

  task_state_base(task_state_base const&)                    = delete;
  auto operator=(task_state_base const&) -> task_state_base& = delete;
  task_state_base(task_state_base&&)                         = delete;
  auto operator=(task_state_base&&) -> task_state_base&      = delete;

  void add_ref() noexcept
  {
    references_.fetch_add(1, std::memory_order_relaxed);
  }

  void release() noexcept
  {
    if (references_.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
      destroy_(this);
    }
  }

  [[nodiscard]] auto is_complete() const noexcept -> bool
  {
    return continuations_.load(std::memory_order_acquire) == finished();
  }

  void wait() const noexcept
  {
    auto* head = continuations_.load(std::memory_order_acquire);
    while (head != finished())
    {
      continuations_.wait(head, std::memory_order_relaxed);
      head = continuations_.load(std::memory_order_acquire);
    }
  }

  void cooperative_wait(WC const& ctx) const noexcept
  {
    assert_not_waiting_on_ancestor();
    while (!is_complete())
    {
      ctx.get_scheduler().busy_work(ctx);
      std::this_thread::yield();
    }
  }

  /**
   * @brief Links `continuation` into the completion list, or schedules it immediately if the task
   * has already finished.
   *
   * The list head doubles as the completion flag, so there is no window between observing
   * "not complete" and publishing the link, and no lock is needed to close it.
   */
  void add_continuation(continuation_base<WC>* continuation, WC const& ctx) noexcept
  {
    auto* head = continuations_.load(std::memory_order_acquire);
    while (head != finished())
    {
      continuation->next_ = head;
      if (continuations_.compare_exchange_weak(head, continuation, std::memory_order_release,
                                               std::memory_order_acquire))
      {
        return;
      }
    }
    continuation->next_ = nullptr;
    continuation->schedule(ctx);
  }

  [[nodiscard]] auto get_exception() const noexcept -> std::exception_ptr
  {
    // finish() writes exception_ before its releasing exchange, so acquire-observing the
    // completion tag is exactly what makes this read safe.
    if (continuations_.load(std::memory_order_acquire) != finished())
    {
      return {};
    }
    return exception_;
  }

  [[nodiscard]] auto get_allocator() const noexcept -> scheduler_allocator
  {
    return allocator_;
  }

protected:
  ~task_state_base() noexcept
  {
    auto* head = continuations_.load(std::memory_order_relaxed);
    OULY_ASSERT(head == nullptr || head == finished());
  }

  void finish(WC const& ctx, std::exception_ptr exception = {}) noexcept
  {
    exception_ = std::move(exception);
    // Release: publishes exception_ and the derived state's value to every thread that later
    // acquire-observes the completion tag.
    auto* continuations = continuations_.exchange(finished(), std::memory_order_acq_rel);
    OULY_ASSERT(continuations != finished());
    continuations_.notify_all();

    while (continuations != nullptr)
    {
      auto* next           = continuations->next_;
      continuations->next_ = nullptr;
      continuations->schedule(ctx);
      continuations = next;
    }
  }

private:
  struct completion_tag final : continuation_base<WC>
  {
    void schedule(WC const& /*ctx*/) noexcept final {}
  };

  /**
   * @brief Distinct non-null address parked in continuations_ once the task is done.
   *
   * A real object rather than a fabricated pointer keeps the comparison well defined; it is only
   * ever compared against, never scheduled through.
   */
  static auto finished() noexcept -> continuation_base<WC>*
  {
    return &completed;
  }

  void assert_not_waiting_on_ancestor() const noexcept
  {
    auto* current = task_execution_slot<WC>::current;
    while (current != nullptr)
    {
      if (current == this)
      {
        OULY_ASSERT(false && "A task cannot wait on itself or an executing ancestor");
        std::terminate();
      }
      current = current->executing_parent_;
    }
  }

  template <TaskContext>
  friend class task_execution_guard;

  inline static completion_tag completed{};

  std::atomic<uint32_t>               references_{1};
  std::atomic<continuation_base<WC>*> continuations_{nullptr};
  scheduler_allocator                 allocator_;
  destroy_fn                          destroy_ = nullptr;
  std::exception_ptr                  exception_;
  task_state_base*                    executing_parent_ = nullptr;
};

template <typename T, TaskContext WC>
class task_state final : public task_state_base<WC>
{
public:
  explicit task_state(scheduler_allocator allocator) noexcept : task_state_base<WC>(allocator, &destroy_state) {}

  template <typename V>
  void set_value(WC const& ctx, V&& value) noexcept(std::is_nothrow_constructible_v<T, V&&>)
  {
    value_.emplace(std::forward<V>(value));
    this->finish(ctx);
  }

  void set_exception(WC const& ctx, std::exception_ptr exception) noexcept
  {
    this->finish(ctx, std::move(exception));
  }

  [[nodiscard]] auto value() const& noexcept -> T const&
  {
    OULY_ASSERT(value_.has_value());
    return *value_;
  }

private:
  static void destroy_state(task_state_base<WC>* state) noexcept
  {
    scheduler_allocator::destroy(static_cast<task_state*>(state));
  }

  std::optional<T> value_;
};

template <TaskContext WC>
class task_state<void, WC> final : public task_state_base<WC>
{
public:
  explicit task_state(scheduler_allocator allocator) noexcept : task_state_base<WC>(allocator, &destroy_state) {}

  void set_value(WC const& ctx) noexcept
  {
    this->finish(ctx);
  }

  void set_exception(WC const& ctx, std::exception_ptr exception) noexcept
  {
    this->finish(ctx, std::move(exception));
  }

private:
  static void destroy_state(task_state_base<WC>* state) noexcept
  {
    scheduler_allocator::destroy(static_cast<task_state*>(state));
  }
};

template <typename F, TaskContext WC>
auto invoke_task(F& function, WC const& ctx) -> decltype(auto)
{
  if constexpr (std::invocable<F&, WC const&>)
  {
    return std::invoke(function, ctx);
  }
  else
  {
    return std::invoke(function);
  }
}

template <typename F, typename T, TaskContext WC>
auto invoke_continuation(F& function, task_state<T, WC> const& predecessor, WC const& ctx) -> decltype(auto)
{
  if constexpr (std::is_void_v<T>)
  {
    if constexpr (std::invocable<F&, WC const&>)
    {
      return std::invoke(function, ctx);
    }
    else
    {
      return std::invoke(function);
    }
  }
  else if constexpr (std::invocable<F&, T const&, WC const&>)
  {
    return std::invoke(function, predecessor.value(), ctx);
  }
  else if constexpr (std::invocable<F&, WC const&, T const&>)
  {
    return std::invoke(function, ctx, predecessor.value());
  }
  else if constexpr (std::invocable<F&, T const&>)
  {
    return std::invoke(function, predecessor.value());
  }
  else
  {
    return std::invoke(function, ctx);
  }
}

template <typename F, TaskContext WC>
using task_result_t = std::remove_cvref_t<decltype(invoke_task(std::declval<F&>(), std::declval<WC const&>()))>;

template <typename F, typename T, TaskContext WC>
using continuation_result_t = std::remove_cvref_t<decltype(invoke_continuation(
 std::declval<F&>(), std::declval<task_state<T, WC> const&>(), std::declval<WC const&>()))>;

template <typename F, typename R, TaskContext WC>
class producer_node
{
public:
  producer_node(task_state<R, WC>* state, F function) : state_(state), function_(std::move(function))
  {
    state_->add_ref();
  }

  void run(WC const& ctx) noexcept
  {
    task_execution_guard guard(state_);
    try
    {
      if constexpr (std::is_void_v<R>)
      {
        invoke_task(function_, ctx);
        state_->set_value(ctx);
      }
      else
      {
        state_->set_value(ctx, invoke_task(function_, ctx));
      }
    }
    catch (...)
    {
      state_->set_exception(ctx, std::current_exception());
    }

    state_->release();
    scheduler_allocator::destroy(this);
  }

private:
  task_state<R, WC>* state_ = nullptr;
  F                  function_;
};

template <typename F, typename T, typename R, TaskContext WC>
class continuation_node final : public continuation_base<WC>
{
public:
  continuation_node(task_state<T, WC>* predecessor, task_state<R, WC>* result, workgroup_id group, F function)
      : predecessor_(predecessor), result_(result), group_(group), function_(std::move(function))
  {
    predecessor_->add_ref();
    result_->add_ref();
  }

  void schedule(WC const& ctx) noexcept final
  {
    ctx.get_scheduler().submit(ctx, group_,
                               [self = this](WC const& run_ctx) noexcept -> void
                               {
                                 self->run(run_ctx);
                               });
  }

private:
  void run(WC const& ctx) noexcept
  {
    task_execution_guard guard(result_);
    try
    {
      if (auto exception = predecessor_->get_exception())
      {
        result_->set_exception(ctx, std::move(exception));
      }
      else if constexpr (std::is_void_v<R>)
      {
        invoke_continuation(function_, *predecessor_, ctx);
        result_->set_value(ctx);
      }
      else
      {
        result_->set_value(ctx, invoke_continuation(function_, *predecessor_, ctx));
      }
    }
    catch (...)
    {
      result_->set_exception(ctx, std::current_exception());
    }

    predecessor_->release();
    result_->release();
    scheduler_allocator::destroy(this);
  }

  task_state<T, WC>* predecessor_ = nullptr;
  task_state<R, WC>* result_      = nullptr;
  workgroup_id       group_;
  F                  function_;
};

struct task_access
{
  template <typename T, TaskContext WC>
  static auto state(basic_task<T, WC> const& value) noexcept -> task_state<T, WC>*
  {
    return value.state_;
  }

  template <typename T, TaskContext WC>
  static auto make(task_state<T, WC>* state) noexcept -> basic_task<T, WC>
  {
    return basic_task<T, WC>(state);
  }
};

template <TaskContext WC>
class when_all_control
{
public:
  when_all_control(task_state<void, WC>* result, uint32_t count) noexcept : result_(result), remaining_(count)
  {
    result_->add_ref();
  }

  void arrive(WC const& ctx, std::exception_ptr exception) noexcept
  {
    // First failure wins. Claiming the slot makes writers exclusive without a mutex, and the
    // releasing fetch_sub below publishes the write to whichever thread closes the group.
    if (exception && !exception_claimed_.exchange(true, std::memory_order_relaxed))
    {
      exception_ = std::move(exception);
    }

    if (remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
      if (exception_)
      {
        result_->set_exception(ctx, std::move(exception_));
      }
      else
      {
        result_->set_value(ctx);
      }
      result_->release();
      scheduler_allocator::destroy(this);
    }
  }

private:
  task_state<void, WC>* result_ = nullptr;
  std::atomic<uint32_t> remaining_{0};
  std::atomic_bool      exception_claimed_{false};
  std::exception_ptr    exception_;
};

template <typename T, TaskContext WC>
class when_all_node final : public continuation_base<WC>
{
public:
  when_all_node(task_state<T, WC>* predecessor, when_all_control<WC>* control, workgroup_id group) noexcept
      : predecessor_(predecessor), control_(control), group_(group)
  {
    predecessor_->add_ref();
  }

  void schedule(WC const& ctx) noexcept final
  {
    ctx.get_scheduler().submit(ctx, group_,
                               [self = this](WC const& run_ctx) noexcept -> void
                               {
                                 self->run(run_ctx);
                               });
  }

private:
  void run(WC const& ctx) noexcept
  {
    auto exception = predecessor_->get_exception();
    predecessor_->release();
    control_->arrive(ctx, std::move(exception));
    scheduler_allocator::destroy(this);
  }

  task_state<T, WC>*    predecessor_ = nullptr;
  when_all_control<WC>* control_     = nullptr;
  workgroup_id          group_;
};

template <typename Promise, typename T, TaskContext WC>
class coroutine_task_continuation_node final : public continuation_base<WC>
{
public:
  coroutine_task_continuation_node(task_state<T, WC>* predecessor, std::coroutine_handle<Promise> coroutine,
                                   workgroup_id group) noexcept
      : predecessor_(predecessor), coroutine_(coroutine), group_(group)
  {
    predecessor_->add_ref();
  }

  void schedule(WC const& ctx) noexcept final
  {
    ctx.get_scheduler().submit(ctx, group_,
                               [self = this](WC const& run_ctx) noexcept -> void
                               {
                                 self->run(run_ctx);
                               });
  }

private:
  void run(WC const& ctx) noexcept
  {
    auto coroutine = coroutine_;
    predecessor_->release();
    scheduler_allocator::destroy(this);
    resume_coroutine(coroutine, ctx);
  }

  task_state<T, WC>*             predecessor_ = nullptr;
  std::coroutine_handle<Promise> coroutine_;
  workgroup_id                   group_;
};

template <TaskContext WC>
struct scope_execution_slot
{
  inline static thread_local basic_task_scope<WC>* current = nullptr;
};

template <TaskContext WC>
class scope_node_base
{
public:
  using execute_fn = void (*)(scope_node_base*, WC const&) noexcept;
  using destroy_fn = void (*)(scope_node_base*) noexcept;

  scope_node_base(workgroup_id group, execute_fn execute, destroy_fn destroy) noexcept
      : group_(group), execute_(execute), destroy_(destroy)
  {}

  void execute(WC const& ctx) noexcept
  {
    if (!claimed_.exchange(true, std::memory_order_acq_rel))
    {
      execute_(this, ctx);
    }
  }

  void release() noexcept
  {
    if (references_.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
      destroy_(this);
    }
  }

  [[nodiscard]] auto can_execute(WC const& ctx) const noexcept -> bool
  {
    return group_ == ctx.get_workgroup();
  }

  [[nodiscard]] auto next() const -> scope_node_base*
  {
    return next_;
  }

  void set_next(scope_node_base* next) noexcept
  {
    next_ = next;
  }

private:
  scope_node_base*      next_ = nullptr;
  std::atomic<uint32_t> references_{2};
  std::atomic_bool      claimed_{false};
  workgroup_id          group_;
  execute_fn            execute_ = nullptr;
  destroy_fn            destroy_ = nullptr;
};

template <typename F, typename R, TaskContext WC>
class scope_node final : public scope_node_base<WC>
{
public:
  using complete_fn = void (*)(void*, std::exception_ptr) noexcept;

  scope_node(task_state<R, WC>* state, void* scope, complete_fn complete, workgroup_id group, F function)
      : scope_node_base<WC>(group, &execute_node, &destroy_node), state_(state), scope_(scope), complete_(complete),
        function_(std::move(function))
  {
    state_->add_ref();
  }

private:
  static void execute_node(scope_node_base<WC>* base, WC const& ctx) noexcept
  {
    auto*                self = static_cast<scope_node*>(base);
    task_execution_guard task_guard(self->state_);
    auto* previous = std::exchange(scope_execution_slot<WC>::current, static_cast<basic_task_scope<WC>*>(self->scope_));
    std::exception_ptr exception;
    try
    {
      if constexpr (std::is_void_v<R>)
      {
        invoke_task(self->function_, ctx);
        self->state_->set_value(ctx);
      }
      else
      {
        self->state_->set_value(ctx, invoke_task(self->function_, ctx));
      }
    }
    catch (...)
    {
      exception = std::current_exception();
      self->state_->set_exception(ctx, exception);
    }
    scope_execution_slot<WC>::current = previous;
    self->state_->release();
    self->complete_(self->scope_, std::move(exception));
  }

  static void destroy_node(scope_node_base<WC>* base) noexcept
  {
    scheduler_allocator::destroy(static_cast<scope_node*>(base));
  }

  task_state<R, WC>* state_    = nullptr;
  void*              scope_    = nullptr;
  complete_fn        complete_ = nullptr;
  F                  function_;
};

} // namespace detail

template <typename T, TaskContext WC>
class basic_task
{
public:
  using value_type   = T;
  using context_type = WC;

  basic_task() noexcept = default;

  basic_task(basic_task const& other) noexcept : state_(other.state_)
  {
    add_ref();
  }

  basic_task(basic_task&& other) noexcept : state_(std::exchange(other.state_, nullptr)) {}

  ~basic_task() noexcept
  {
    release();
  }

  auto operator=(basic_task const& other) noexcept -> basic_task&
  {
    if (this != &other)
    {
      release();
      state_ = other.state_;
      add_ref();
    }
    return *this;
  }

  auto operator=(basic_task&& other) noexcept -> basic_task&
  {
    if (this != &other)
    {
      release();
      state_ = std::exchange(other.state_, nullptr);
    }
    return *this;
  }

  [[nodiscard]] explicit operator bool() const noexcept
  {
    return state_ != nullptr;
  }

  [[nodiscard]] auto is_ready() const noexcept -> bool
  {
    return state_ != nullptr && state_->is_complete();
  }

  void wait() const noexcept
  {
    OULY_ASSERT(state_ != nullptr);
    state_->wait();
  }

  void wait(WC const& ctx) const noexcept
  {
    OULY_ASSERT(state_ != nullptr);
    state_->cooperative_wait(ctx);
  }

  auto get() const -> decltype(auto)
  {
    wait();
    if (auto exception = state_->get_exception())
    {
      std::rethrow_exception(exception);
    }
    if constexpr (!std::is_void_v<T>)
    {
      return state_->value();
    }
  }

  auto get(WC const& ctx) const -> decltype(auto)
  {
    wait(ctx);
    if (auto exception = state_->get_exception())
    {
      std::rethrow_exception(exception);
    }
    if constexpr (!std::is_void_v<T>)
    {
      return state_->value();
    }
  }

  template <typename F>
  auto then(WC const& ctx, F&& function) const -> basic_task<detail::continuation_result_t<std::decay_t<F>, T, WC>, WC>
  {
    return then(ctx, ctx.get_workgroup(), state_->get_allocator(), std::forward<F>(function));
  }

  template <typename F>
  auto then(WC const& ctx, workgroup_id group, F&& function) const
   -> basic_task<detail::continuation_result_t<std::decay_t<F>, T, WC>, WC>
  {
    return then(ctx, group, state_->get_allocator(), std::forward<F>(function));
  }

  template <typename F>
  auto then(WC const& ctx, workgroup_id group, scheduler_allocator allocator, F&& function) const
   -> basic_task<detail::continuation_result_t<std::decay_t<F>, T, WC>, WC>
  {
    OULY_ASSERT(state_ != nullptr);
    using function_type = std::decay_t<F>;
    using result_type   = detail::continuation_result_t<function_type, T, WC>;
    using node_type     = detail::continuation_node<function_type, T, result_type, WC>;

    auto* result = allocator.make<detail::task_state<result_type, WC>>(allocator);
    try
    {
      auto* node = allocator.make<node_type>(state_, result, group, std::forward<F>(function));
      state_->add_continuation(node, ctx);
    }
    catch (...)
    {
      result->release();
      throw;
    }
    return basic_task<result_type, WC>(result);
  }

  auto operator co_await() const noexcept -> detail::basic_task_awaiter<T, WC>;

private:
  explicit basic_task(detail::task_state<T, WC>* state) noexcept : state_(state) {}

  void add_ref() noexcept
  {
    if (state_ != nullptr)
    {
      state_->add_ref();
    }
  }

  void release() noexcept
  {
    if (state_ != nullptr)
    {
      state_->release();
      state_ = nullptr;
    }
  }

  template <typename, TaskContext>
  friend class basic_task;
  friend struct detail::task_access;

  template <TaskContext Context, typename F>
  friend auto submit_task(Context const&, workgroup_id, scheduler_allocator, F&&)
   -> basic_task<detail::task_result_t<std::decay_t<F>, Context>, Context>;

  template <TaskContext Context, typename... Tasks>
  friend auto when_all(Context const&, workgroup_id, scheduler_allocator, Tasks const&...) -> basic_task<void, Context>;

  detail::task_state<T, WC>* state_ = nullptr;
};

namespace detail
{

template <typename T, TaskContext WC>
class basic_task_awaiter
{
public:
  explicit basic_task_awaiter(basic_task<T, WC> task) noexcept : task_(std::move(task)) {}

  [[nodiscard]] auto await_ready() const noexcept -> bool
  {
    return task_.is_ready();
  }

  template <typename Promise>
    requires std::is_same_v<typename Promise::context_type, WC>
  auto await_suspend(std::coroutine_handle<Promise> coroutine) -> bool
  {
    auto* ctx = coroutine_context_slot<WC>::current_;
    OULY_ASSERT(ctx != nullptr && "co_await task<T> requires a scheduler-resumed coroutine");
    if (ctx == nullptr)
    {
      task_access::state(task_)->wait();
      return false;
    }

    auto* state = task_access::state(task_);
    OULY_ASSERT(state != nullptr);
    auto group = coroutine.promise().resume_group_;
    if (!group)
    {
      group = ctx->get_workgroup();
    }
    auto  allocator = state->get_allocator();
    auto* node = allocator.template make<coroutine_task_continuation_node<Promise, T, WC>>(state, coroutine, group);
    state->add_continuation(node, *ctx);
    return true;
  }

  auto await_resume() const -> decltype(auto)
  {
    if (auto exception = task_access::state(task_)->get_exception())
    {
      std::rethrow_exception(exception);
    }
    if constexpr (!std::is_void_v<T>)
    {
      return task_access::state(task_)->value();
    }
  }

private:
  basic_task<T, WC> task_;
};

} // namespace detail

template <typename T, TaskContext WC>
auto basic_task<T, WC>::operator co_await() const noexcept -> detail::basic_task_awaiter<T, WC>
{
  return detail::basic_task_awaiter<T, WC>(*this);
}

template <TaskContext WC, typename F>
auto submit_task(WC const& ctx, workgroup_id group, scheduler_allocator allocator, F&& function)
 -> basic_task<detail::task_result_t<std::decay_t<F>, WC>, WC>
{
  using function_type = std::decay_t<F>;
  using result_type   = detail::task_result_t<function_type, WC>;
  using node_type     = detail::producer_node<function_type, result_type, WC>;

  auto* state = allocator.template make<detail::task_state<result_type, WC>>(allocator);
  try
  {
    auto* node = allocator.template make<node_type>(state, std::forward<F>(function));
    ctx.get_scheduler().submit(ctx, group,
                               [node](WC const& run_ctx) noexcept -> void
                               {
                                 node->run(run_ctx);
                               });
  }
  catch (...)
  {
    state->release();
    throw;
  }
  return basic_task<result_type, WC>(state);
}

template <TaskContext WC, typename F>
auto submit_task(WC const& ctx, scheduler_allocator allocator, F&& function)
 -> basic_task<detail::task_result_t<std::decay_t<F>, WC>, WC>
{
  return submit_task(ctx, ctx.get_workgroup(), allocator, std::forward<F>(function));
}

template <TaskContext WC, typename F>
auto submit_task(WC const& ctx, workgroup_id group, F&& function)
 -> basic_task<detail::task_result_t<std::decay_t<F>, WC>, WC>
{
  return submit_task(ctx, group, scheduler_allocator{}, std::forward<F>(function));
}

template <TaskContext WC, typename F>
auto submit_task(WC const& ctx, F&& function) -> basic_task<detail::task_result_t<std::decay_t<F>, WC>, WC>
{
  return submit_task(ctx, ctx.get_workgroup(), scheduler_allocator{}, std::forward<F>(function));
}

template <TaskContext WC, typename... Tasks>
auto when_all(WC const& ctx, workgroup_id group, scheduler_allocator allocator, Tasks const&... tasks)
 -> basic_task<void, WC>
{
  static_assert((std::is_same_v<typename Tasks::context_type, WC> && ...));
  constexpr auto count = static_cast<uint32_t>(sizeof...(Tasks));

  auto* result = allocator.make<detail::task_state<void, WC>>(allocator);
  if constexpr (count == 0)
  {
    result->set_value(ctx);
    return basic_task<void, WC>(result);
  }
  else
  {
    detail::when_all_control<WC>* control = nullptr;
    try
    {
      control = allocator.make<detail::when_all_control<WC>>(result, count);
    }
    catch (...)
    {
      result->release();
      throw;
    }
    uint32_t attached = 0;
    try
    {
      auto attach = [&]<typename T>(basic_task<T, WC> const& task) -> auto
      {
        auto* state = detail::task_access::state(task);
        OULY_ASSERT(state != nullptr);
        auto* node = allocator.make<detail::when_all_node<T, WC>>(state, control, group);
        state->add_continuation(node, ctx);
        ++attached;
      };
      (attach(tasks), ...);
    }
    catch (...)
    {
      auto exception = std::current_exception();
      for (uint32_t index = attached; index < count; ++index)
      {
        control->arrive(ctx, exception);
      }
    }
    return basic_task<void, WC>(result);
  }
}

template <TaskContext WC, typename... Tasks>
auto when_all(WC const& ctx, Tasks const&... tasks) -> basic_task<void, WC>
{
  return when_all(ctx, ctx.get_workgroup(), scheduler_allocator{}, tasks...);
}

template <TaskContext WC, std::ranges::sized_range Range>
  requires requires(std::ranges::range_value_t<Range> const& value) {
    typename std::ranges::range_value_t<Range>::value_type;
    requires std::is_same_v<typename std::ranges::range_value_t<Range>::context_type, WC>;
    { detail::task_access::state(value) };
  }
auto when_all(WC const& ctx, workgroup_id group, scheduler_allocator allocator, Range const& tasks)
 -> basic_task<void, WC>
{
  using task_type  = std::ranges::range_value_t<Range>;
  using value_type = typename task_type::value_type;

  auto const size = std::ranges::size(tasks);
  OULY_ASSERT(size <= std::numeric_limits<uint32_t>::max());
  auto const count  = static_cast<uint32_t>(size);
  auto*      result = allocator.make<detail::task_state<void, WC>>(allocator);
  if (count == 0)
  {
    result->set_value(ctx);
    return detail::task_access::make(result);
  }

  detail::when_all_control<WC>* control = nullptr;
  try
  {
    control = allocator.make<detail::when_all_control<WC>>(result, count);
  }
  catch (...)
  {
    result->release();
    throw;
  }
  uint32_t attached = 0;
  try
  {
    for (auto const& task : tasks)
    {
      auto* state = detail::task_access::state(task);
      OULY_ASSERT(state != nullptr);
      auto* node = allocator.make<detail::when_all_node<value_type, WC>>(state, control, group);
      state->add_continuation(node, ctx);
      ++attached;
    }
  }
  catch (...)
  {
    auto exception = std::current_exception();
    for (uint32_t index = attached; index < count; ++index)
    {
      control->arrive(ctx, exception);
    }
  }
  return detail::task_access::make(result);
}

template <TaskContext WC, std::ranges::sized_range Range>
  requires requires(std::ranges::range_value_t<Range> const& value) {
    typename std::ranges::range_value_t<Range>::value_type;
    requires std::is_same_v<typename std::ranges::range_value_t<Range>::context_type, WC>;
    { detail::task_access::state(value) };
  }
auto when_all(WC const& ctx, Range const& tasks) -> basic_task<void, WC>
{
  return when_all(ctx, ctx.get_workgroup(), scheduler_allocator{}, tasks);
}

template <TaskContext WC>
class basic_task_scope
{
public:
  explicit basic_task_scope(scheduler_allocator allocator = {}) noexcept : allocator_(allocator) {}

  basic_task_scope(basic_task_scope const&)                        = delete;
  auto operator=(basic_task_scope const&) -> basic_task_scope&     = delete;
  basic_task_scope(basic_task_scope&&) noexcept                    = delete;
  auto operator=(basic_task_scope&&) noexcept -> basic_task_scope& = delete;

  ~basic_task_scope() noexcept
  {
    auto const outstanding = outstanding_.load(std::memory_order_acquire);
    OULY_ASSERT(outstanding == 0 && "task_scope must be joined before destruction");
    if (outstanding != 0)
    {
      std::terminate();
    }
    release_nodes(take_nodes());
  }

  template <typename F>
  auto run(WC const& ctx, F&& function) -> basic_task<detail::task_result_t<std::decay_t<F>, WC>, WC>
  {
    return run(ctx, ctx.get_workgroup(), std::forward<F>(function));
  }

  template <typename F>
  auto run(WC const& ctx, workgroup_id group, F&& function)
   -> basic_task<detail::task_result_t<std::decay_t<F>, WC>, WC>
  {
    using function_type = std::decay_t<F>;
    using result_type   = detail::task_result_t<function_type, WC>;
    using node_type     = detail::scope_node<function_type, result_type, WC>;

    detail::task_state<result_type, WC>* state       = nullptr;
    node_type*                           node        = nullptr;
    bool                                 wake_joiner = false;
    {
      std::scoped_lock lock(mutex_);
      auto const       is_descendant = detail::scope_execution_slot<WC>::current == this;
      OULY_ASSERT((!closed_ || is_descendant) && "Cannot submit external work after joining a task_scope");
      if (closed_ && !is_descendant)
      {
        std::terminate();
      }

      auto const allocator = allocator_;
      state                = allocator.make<detail::task_state<result_type, WC>>(allocator);
      try
      {
        node = allocator.make<node_type>(state, this, &complete_one, group, std::forward<F>(function));
      }
      catch (...)
      {
        state->release();
        throw;
      }
      node->set_next(nodes_);
      nodes_ = node;
      outstanding_.fetch_add(1, std::memory_order_relaxed);
      // Only a joiner parks on outstanding_, and only after setting closed_ under this same lock.
      // Skipping the wake otherwise keeps the common submit path free of futex syscalls.
      wake_joiner = closed_;
    }

    // Both of these must stay outside `mutex_`. submit() is not a pure enqueue: when the target
    // workgroup's mailbox is full it drains queued work inline on this thread, which can re-enter
    // run() or complete_one() on this same scope and self-deadlock a non-recursive mutex.
    // The node is already published and counted above, so a concurrent joiner may claim and run it
    // before we get here; that is fine, the pending lambda still owns a reference to it.
    if (wake_joiner)
    {
      outstanding_.notify_all();
    }
    ctx.get_scheduler().submit(ctx, group,
                               [node](WC const& run_ctx) noexcept -> void
                               {
                                 node->execute(run_ctx);
                                 node->release();
                               });
    return detail::task_access::make(state);
  }

  void join(WC const& ctx)
  {
    verify_not_current();
    {
      std::scoped_lock lock(mutex_);
      closed_ = true;
    }
    while (true)
    {
      auto*      nodes     = take_nodes();
      auto const had_nodes = nodes != nullptr;
      for (auto* node = nodes; node != nullptr; node = node->next())
      {
        if (node->can_execute(ctx))
        {
          node->execute(ctx);
        }
      }
      release_nodes(nodes);

      auto const count = outstanding_.load(std::memory_order_acquire);
      if (count == 0)
      {
        break;
      }
      if (!had_nodes)
      {
        outstanding_.wait(count, std::memory_order_relaxed);
      }
    }
    rethrow_exception();
  }

  void join()
  {
    verify_not_current();
    {
      std::scoped_lock lock(mutex_);
      closed_ = true;
    }
    auto count = outstanding_.load(std::memory_order_acquire);
    while (count != 0)
    {
      outstanding_.wait(count, std::memory_order_relaxed);
      count = outstanding_.load(std::memory_order_acquire);
    }
    release_nodes(take_nodes());
    rethrow_exception();
  }

  [[nodiscard]] auto is_complete() const noexcept -> bool
  {
    return outstanding_.load(std::memory_order_acquire) == 0;
  }

  void reset() noexcept
  {
    reset(allocator_);
  }

  void reset(scheduler_allocator allocator) noexcept
  {
    auto const outstanding = outstanding_.load(std::memory_order_acquire);
    OULY_ASSERT(outstanding == 0 && "task_scope cannot be reset while work is outstanding");
    if (outstanding != 0)
    {
      std::terminate();
    }
    release_nodes(take_nodes());
    allocator_ = allocator;
    std::scoped_lock lock(mutex_);
    exception_ = {};
    closed_    = false;
  }

private:
  void verify_not_current() const noexcept
  {
    if (detail::scope_execution_slot<WC>::current == this)
    {
      OULY_ASSERT(false && "A task cannot join its own scope");
      std::terminate();
    }
  }

  static void complete_one(void* scope, std::exception_ptr exception) noexcept
  {
    auto* self = static_cast<basic_task_scope*>(scope);
    // The decrement and the notify are both done under `mutex_` on purpose. A waiter that is
    // parked on `outstanding_.wait(n)` returns as soon as the value differs from `n`, without
    // needing the notify; without this lock it could therefore observe zero, leave join() and let
    // the scope be destroyed while this thread is still about to touch `outstanding_`. Every path
    // that may destroy the scope (join, reset, destructor) re-acquires `mutex_` after seeing the
    // count reach zero, so taking it here orders that destruction after the notify below.
    std::scoped_lock lock(self->mutex_);
    if (exception && !self->exception_)
    {
      self->exception_ = std::move(exception);
    }
    if (self->outstanding_.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
      self->outstanding_.notify_all();
    }
  }

  void rethrow_exception() const
  {
    std::exception_ptr exception;
    {
      std::scoped_lock lock(mutex_);
      exception = exception_;
    }
    if (exception)
    {
      std::rethrow_exception(exception);
    }
  }

  auto take_nodes() noexcept -> detail::scope_node_base<WC>*
  {
    std::scoped_lock lock(mutex_);
    return std::exchange(nodes_, nullptr);
  }

  static void release_nodes(detail::scope_node_base<WC>* nodes) noexcept
  {
    while (nodes != nullptr)
    {
      auto* next = nodes->next();
      nodes->release();
      nodes = next;
    }
  }

  scheduler_allocator          allocator_;
  std::atomic<uint32_t>        outstanding_{0};
  mutable std::mutex           mutex_;
  detail::scope_node_base<WC>* nodes_ = nullptr;
  std::exception_ptr           exception_;
  bool                         closed_ = false;
};

} // namespace ouly
