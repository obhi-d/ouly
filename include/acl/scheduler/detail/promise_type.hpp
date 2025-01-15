
#pragma once

#include <acl/scheduler/detail/get_awaiter.hpp>
#include <array>
#include <concepts>
#include <cstddef>
#include <semaphore>

namespace acl::detail
{

class base_promise : public acl::detail::coro_state
{
public:
  static auto initial_suspend() noexcept
  {
    return std::suspend_always();
  }

  static auto final_suspend() noexcept
  {
    return final_awaiter{};
  }

  static void unhandled_exception() noexcept
  {
    assert(0 && "Coroutine throwing! Terminate!");
  }
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
    if (!std::is_trivially_destructible_v<Ty>)
    {
      result().~Ty();
    }
  }

  template <std::convertible_to<Ty> V>
  void return_value(V&& value) noexcept(std::is_nothrow_constructible_v<Ty, V&&>)
  {
    ::new (data_) Ty(std::forward<V>(value));
  }

  auto result() & noexcept -> Ty&
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return *reinterpret_cast<Ty*>(data_);
  }

  auto result() && -> rvalue_type
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::move(*reinterpret_cast<Ty*>(data_));
  }

  auto get_return_object() noexcept -> TaskClass<Ty>
  {
    return TaskClass<Ty>(std::coroutine_handle<promise_type<TaskClass, Ty>>::from_promise(*this));
  }

private:
  alignas(alignof(Ty)) std::byte data_[sizeof(Ty)]{};
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

  auto result() noexcept -> Ty&
  {
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

  void result() & noexcept {}
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
      assert(0 && "Coroutine throwing! Terminate!");
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
  co_await *task;
  event->notify();
  co_return;
}

} // namespace acl::detail