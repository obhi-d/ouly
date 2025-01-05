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
constexpr uint32_t max_task_base_size = 24;

using task_delegate = acl::basic_delegate<max_task_base_size, void(worker_context const&)>;

namespace detail
{
template <typename R, typename Promise>
class co_task
{
public:
	using promise_type = Promise;
	using handle			 = std::coroutine_handle<promise_type>;

	~co_task() noexcept
	{
		if (coro_)
		{
			coro_.destroy();
		}
	}

	co_task() noexcept			= default;
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
		coro_				= std::move(other.coro_);
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
		blocking_event event;
		detail::wait(&event, this);
		event.wait();
		if constexpr (!std::is_same_v<R, void>)
		{
			return coro_.promise().result();
		}
	}

	/**
	 * @brief Returns result after waiting for the task to finish, with a non-blocking event, that tries to do work when
	 * this coro is not available
	 */
	auto sync_wait_result(worker_id worker, scheduler& s) noexcept -> R
	{
		busywork_event event;
		detail::wait(&event, this);
		event.wait(worker, s);
		if constexpr (!std::is_same_v<R, void>)
		{
			return coro_.promise().result();
		}
	}

protected:
	auto release() noexcept -> handle
	{
		auto h = coro_;
		coro_	 = nullptr;
		return h;
	}

private:
	handle coro_ = {};
};
} // namespace detail

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
struct co_task : public detail::co_task<R, detail::promise_type<co_task, R>>
{
	using super = detail::co_task<R, detail::promise_type<co_task, R>>;
	using typename super::handle;

	co_task() noexcept			= default;
	co_task(const co_task&) = delete;
	co_task(handle h) : super(h) {}
	co_task(co_task&& other) noexcept : super(other.release()) {}
	~co_task() noexcept												 = default;
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
struct co_sequence : public detail::co_task<R, detail::sequence_promise<co_sequence, R>>
{
	using super = detail::co_task<R, detail::sequence_promise<co_sequence, R>>;
	using typename super::handle;

	co_sequence() noexcept					= default;
	co_sequence(const co_sequence&) = delete;
	co_sequence(handle h) : super(h) {}
	co_sequence(co_sequence&& other) noexcept : super(std::move<super>((super&&)other)) {}
	~co_sequence() noexcept														 = default;
	auto operator=(co_sequence const&) -> co_sequence& = delete;
	auto operator=(co_sequence&& other) noexcept -> co_sequence&
	{
		(super&)(*this) = std::move<super>((super&&)other);
		return *this;
	}
};

} // namespace acl
