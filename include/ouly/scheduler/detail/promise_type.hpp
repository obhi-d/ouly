// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/scheduler/config.hpp"
#include "ouly/scheduler/detail/allocation.hpp"
#include "ouly/scheduler/detail/get_awaiter.hpp"
#include <array>
#include <concepts>
#include <cstddef>
#include <exception>
#include <semaphore>

namespace ouly::detail
{

inline auto coroutine_allocator_from() noexcept -> scheduler_allocator
{
  return {};
}

template <typename First, typename... Rest>
auto coroutine_allocator_from([[maybe_unused]] First&& first, Rest&&... rest) noexcept -> scheduler_allocator
{
  if constexpr (std::is_same_v<std::remove_cvref_t<First>, scheduler_allocator>)
  {
    return first;
  }
  else
  {
    return coroutine_allocator_from(std::forward<Rest>(rest)...);
  }
}

class base_promise : public ouly::detail::coro_state
{
public:
  using context_type = ouly::task_context;

  static auto operator new(std::size_t size) -> void*
  {
    return scheduler_allocator{}.allocate_bytes(size);
  }

  template <typename... Args>
  static auto operator new(std::size_t size, Args&&... args) -> void*
  {
    return coroutine_allocator_from(std::forward<Args>(args)...).allocate_bytes(size);
  }

  static void operator delete(void* ptr, std::size_t /*size*/) noexcept
  {
    scheduler_allocator::deallocate_bytes(ptr);
  }

  template <typename... Args>
  static void operator delete(void* ptr, std::size_t /*size*/, Args&&... /*args*/) noexcept // NOLINT
  {
    scheduler_allocator::deallocate_bytes(ptr);
  }

  static auto initial_suspend() noexcept
  {
    return std::suspend_always();
  }

  static auto final_suspend() noexcept
  {
    return final_awaiter{};
  }

  void unhandled_exception() noexcept
  {
    exception_ = std::current_exception();
  }

  void rethrow_if_exception() const
  {
    if (exception_)
    {
      std::rethrow_exception(exception_);
    }
  }

private:
  std::exception_ptr exception_;
};

template <template <typename R> class TaskClass, typename Ty>
class promise_type : public base_promise
{
public:
  promise_type() noexcept                              = default;
  promise_type(const promise_type&)                    = default;
  promise_type(promise_type&&)                         = default;
  auto operator=(const promise_type&) -> promise_type& = default;
  auto operator=(promise_type&&) -> promise_type&      = default;
  using rvalue_type = std::conditional_t<std::is_arithmetic_v<Ty> || std::is_pointer_v<Ty>, Ty, Ty&&>;

  ~promise_type() noexcept
  {
    if (has_value_ && !std::is_trivially_destructible_v<Ty>)
    {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      reinterpret_cast<Ty*>(data_)->~Ty();
    }
  }

  template <std::convertible_to<Ty> V>
  void return_value(V&& value) noexcept(std::is_nothrow_constructible_v<Ty, V&&>)
  {
    ::new (data_) Ty(std::forward<V>(value));
    has_value_ = true;
  }

  auto result() & -> Ty&
  {
    this->rethrow_if_exception();
    OULY_ASSERT(has_value_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<Ty*>(data_);
  }

  auto result() && -> rvalue_type
  {
    this->rethrow_if_exception();
    OULY_ASSERT(has_value_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::move(*reinterpret_cast<Ty*>(data_));
  }

  auto get_return_object() noexcept -> TaskClass<Ty>
  {
    return TaskClass<Ty>(std::coroutine_handle<promise_type<TaskClass, Ty>>::from_promise(*this));
  }

private:
  alignas(alignof(Ty)) std::byte data_[sizeof(Ty)]{};
  bool has_value_ = false;
};

template <template <typename R> class TaskClass, typename Ty>
class promise_type<TaskClass, Ty&> : public base_promise
{
public:
  promise_type() noexcept                              = default;
  promise_type(const promise_type&)                    = default;
  promise_type(promise_type&&)                         = default;
  auto operator=(const promise_type&) -> promise_type& = default;
  auto operator=(promise_type&&) -> promise_type&      = default;

  ~promise_type() noexcept = default;

  void return_value(Ty& value) noexcept
  {
    data_ = &value;
  }

  auto result() -> Ty&
  {
    this->rethrow_if_exception();
    OULY_ASSERT(data_ != nullptr);
    return *data_;
  }

  auto get_return_object() noexcept -> TaskClass<Ty&>
  {
    return TaskClass<Ty&>(std::coroutine_handle<promise_type<TaskClass, Ty&>>::from_promise(*this));
  }

private:
  Ty* data_ = nullptr;
};

template <template <typename R> class TaskClass>
class promise_type<TaskClass, void> : public base_promise
{
public:
  promise_type() noexcept                              = default;
  promise_type(const promise_type&)                    = default;
  promise_type(promise_type&&)                         = default;
  auto operator=(const promise_type&) -> promise_type& = default;
  auto operator=(promise_type&&) -> promise_type&      = default;

  ~promise_type() noexcept = default;

  void result() &
  {
    this->rethrow_if_exception();
  }
  void return_void() noexcept {}

  auto get_return_object() noexcept -> TaskClass<void>
  {
    return TaskClass<void>(std::coroutine_handle<promise_type<TaskClass, void>>::from_promise(*this));
  }
};

template <template <typename R> class TaskClass, typename Ty>
class sequence_promise : public promise_type<TaskClass, Ty>
{
public:
  auto initial_suspend() noexcept
  {
    return std::suspend_never();
  }

  auto get_return_object() noexcept -> TaskClass<Ty>
  {
    return TaskClass<Ty>(std::coroutine_handle<sequence_promise<TaskClass, Ty>>::from_promise(*this));
  }
};

struct sync_waiter
{
  class promise_type
  {
  public:
    static auto initial_suspend() noexcept
    {
      return std::suspend_never();
    }
    static auto final_suspend() noexcept
    {
      return std::suspend_never();
    }
    static void unhandled_exception() noexcept
    {
      OULY_ASSERT(0 && "Coroutine throwing! Terminate!");
    }
    void return_void() noexcept {}

    [[nodiscard]] static auto get_return_object() -> sync_waiter
    {
      return sync_waiter{};
    }
  };
};

template <typename EventType, typename Awaiter>
auto wait(EventType* event, Awaiter* task) -> sync_waiter
{
  std::exception_ptr exception;
  try
  {
    co_await *task;
  }
  catch (...)
  {
    exception = std::current_exception();
  }
  event->release();
  if (exception)
  {
    std::rethrow_exception(exception);
  }
  co_return;
}

} // namespace ouly::detail
