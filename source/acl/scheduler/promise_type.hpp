
#pragma once

#include "awaiters.hpp"
#include <array>
#include <concepts>
#include <semaphore>

namespace acl::detail
{

class base_promise : public detail::coro_state
{
public:
  auto initial_suspend() noexcept
  {
    return std::suspend_always();
  }

  auto final_suspend() noexcept
  {
    return final_awaiter{};
  }

  void unhandled_exception() noexcept
  {
    assert(0 && "Coroutine throwing! Terminate!");
  }
};

template <template <typename R> class task_class, typename Ty>
class promise_type : public base_promise
{
public:
  using rvalue_type = std::conditional_t<std::is_arithmetic_v<Ty> || std::is_pointer_v<Ty>, Ty, Ty&&>;

  inline ~promise_type() noexcept
  {
    if (!std::is_trivially_destructible_v<Ty>)
      result().~Ty();
  }

  template <std::convertible_to<Ty> V>
  void return_value(V&& value) noexcept(std::is_nothrow_constructible_v<Ty, V&&>)
  {
    ::new (static_cast<void*>(std::addressof(data))) Ty(std::forward<V>(value));
  }

  Ty& result() & noexcept
  {
    return reinterpret_cast<Ty&>(data);
  }

  rvalue_type result() &&
  {
    return std::move(reinterpret_cast<Ty&>(data));
  }

  task_class<Ty> get_return_object() noexcept
  {
    return task_class<Ty>(std::coroutine_handle<promise_type<task_class, Ty>>::from_promise(*this));
  }

private:
  alignas(alignof(Ty)) std::array<uint8_t, sizeof(Ty)> data;
};

template <template <typename R> class task_class, typename Ty>
class promise_type<task_class, Ty&> : public base_promise
{
public:
  inline ~promise_type() noexcept = default;

  void return_value(Ty& value) noexcept
  {
    data = &value;
  }

  Ty& result() noexcept
  {
    return *data;
  }

  task_class<Ty&> get_return_object() noexcept
  {
    return task_class<Ty&>(std::coroutine_handle<promise_type<task_class, Ty&>>::from_promise(*this));
  }

private:
  Ty* data = nullptr;
};

template <template <typename R> class task_class>
class promise_type<task_class, void> : public base_promise
{
public:
  inline ~promise_type() noexcept {}

  void result() & noexcept {}
  void return_void() noexcept {}

  task_class<void> get_return_object() noexcept
  {
    return task_class<void>(std::coroutine_handle<promise_type<task_class, void>>::from_promise(*this));
  }
};

template <template <typename R> class task_class, typename Ty>
class sequence_promise : public promise_type<task_class, Ty>
{
public:
  auto initial_suspend() noexcept
  {
    return std::suspend_never();
  }

  task_class<Ty> get_return_object() noexcept
  {
    return task_class<Ty>(std::coroutine_handle<sequence_promise<task_class, Ty>>::from_promise(*this));
  }
};

struct sync_waiter
{
  class promise_type
  {
  public:
    inline auto initial_suspend() noexcept
    {
      return std::suspend_never();
    }
    inline auto final_suspend() noexcept
    {
      return std::suspend_never();
    }
    inline void unhandled_exception() noexcept
    {
      assert(0 && "Coroutine throwing! Terminate!");
    }
    void return_void() noexcept {}

    sync_waiter get_return_object() const
    {
      return sync_waiter{};
    }
  };
};

template <typename Awaiter>
sync_waiter wait(std::binary_semaphore& event, Awaiter&& task)
{
  co_await task;
  event.release();
  co_return;
}

} // namespace acl::detail