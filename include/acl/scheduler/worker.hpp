

#pragma once

#include "spin_lock.hpp"
#include "task.hpp"
#include "worker_context.hpp"
#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/basic_queue.hpp>
#include <acl/utils/tagged_ptr.hpp>
#include <cstdint>
#include <limits>
#include <mutex>
#include <semaphore>
#include <tuple>

namespace acl
{

namespace detail
{

static constexpr uint32_t max_worker_groups		= 32;
static constexpr uint32_t max_local_work_item = 32; // 2 cache lines

using work_item = task_delegate;

struct work_queue_traits
{
	static constexpr uint32_t pool_size_v = 2048;
	using allocator_t											= acl::default_allocator<>;
};

using work_queue			 = acl::basic_queue<work_item, work_queue_traits>;
using async_work_queue = std::pair<acl::spin_lock, work_queue>;

struct workgroup
{
	// Global queues, one per thread group
	std::unique_ptr<detail::async_work_queue[]> work_queues;
	uint32_t																		thread_count		 = 0;
	uint32_t																		start_thread_idx = 0;
	uint32_t																		push_offset			 = 0;
	uint32_t																		priority				 = 0;

	inline uint32_t create_group(uint32_t start, uint32_t count, uint32_t priority) noexcept
	{
		work_queues			 = std::make_unique<detail::async_work_queue[]>(count);
		thread_count		 = count;
		start_thread_idx = start;
		this->priority	 = priority;
		return start + count;
	}

	workgroup() noexcept = default;
};

struct wake_event
{
	inline wake_event() noexcept : semaphore(0) {}

	inline void wait() noexcept
	{
		semaphore.acquire();
	}

	inline void notify() noexcept
	{
		semaphore.release();
	}

	std::binary_semaphore semaphore;
};

struct local_queue
{
	std::atomic_uint32_t											 head;
	std::atomic_uint32_t											 tail;
	std::array<work_item, max_local_work_item> queue;
};

struct group_range
{
	std::array<uint8_t, max_worker_groups> priority_order;
	uint32_t															 count = 0;
	uint32_t															 mask	 = 0;

	group_range() noexcept
	{
		priority_order.fill(std::numeric_limits<uint8_t>::max());
	}
};

struct worker
{
	// Context per work group
	std::unique_ptr<worker_context[]> contexts;
	// Worker specific item
	async_work_queue exlusive_items;
	// worker id
	worker_id id;
	// quit event
	std::atomic_bool quitting = false;
};

} // namespace detail

} // namespace acl
