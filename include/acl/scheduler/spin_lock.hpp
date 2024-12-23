
#pragma once

#include <atomic>

namespace acl
{
struct spin_lock
{
	void lock() noexcept
	{
		while (!try_lock())
			flag.wait(true, std::memory_order_relaxed);
	}

	bool try_lock() noexcept
	{
		return !flag.test_and_set(std::memory_order_acquire);
	}

	template <typename Notify = std::true_type>
	void unlock() noexcept
	{
		flag.clear(std::memory_order_release);
		if constexpr (Notify::value)
			flag.notify_one();
	}

private:
	std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

} // namespace acl