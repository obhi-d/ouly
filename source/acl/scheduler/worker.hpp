

#pragma once

#include "spin_lock.hpp"
#include "task.hpp"
#include "worker_context.hpp"
#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/basic_queue.hpp>
#include <acl/utils/tagged_ptr.hpp>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <semaphore>
#include <string>
#include <tuple>

namespace acl
{

namespace detail
{
static constexpr uint8_t  work_type_coroutine    = 0;
static constexpr uint8_t  work_type_task_functor = 1;
static constexpr uint8_t  work_type_free_functor = 2;
static constexpr uint32_t max_worker_groups      = 32;
static constexpr uint32_t max_local_work_item    = 32; // 2 cache lines

struct work_item
{
  task_delegate delegate_fn = nullptr; // or a coro
  task_data     data;

  inline explicit operator bool() noexcept
  {
    return delegate_fn != nullptr;
  }

  inline work_item() noexcept = default;
  inline work_item(task_delegate tfn, task_data tdata) : delegate_fn(tfn), data(tdata) {}

  inline work_item(work_item&& other) noexcept : delegate_fn(other.delegate_fn), data(other.data) {}

  inline work_item& operator=(work_item&& other) noexcept
  {
    delegate_fn = other.delegate_fn;
    data        = other.data;
    return *this;
  }

  auto unpack() const noexcept
  {
    return std::make_tuple((void*)delegate_fn, data);
  }
};

struct work_queue_traits
{
  static constexpr uint32_t pool_size_v = 2048;
  using allocator_t                     = acl::default_allocator<>;
};

using work_queue        = acl::basic_queue<work_item, work_queue_traits>;
using global_work_queue = std::pair<std::mutex, work_queue>;

struct workgroup
{
  // Global queues, one per thread group
  uint32_t start_thread_idx = 0;
  uint32_t end_thread_idx   = 0;
  uint32_t thread_count     = 0;

  std::string name;

  inline uint32_t create_group(std::string gname, uint32_t start, uint32_t count) noexcept
  {
    start_thread_idx = start;
    end_thread_idx   = start + count;
    thread_count     = count;
    name             = std::move(gname);
    return end_thread_idx;
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
  std::atomic_uint32_t                       head;
  std::atomic_uint32_t                       tail;
  std::array<work_item, max_local_work_item> queue;
};

struct worker
{
  // Contexts
  std::array<worker_context_opt, max_worker_groups> contexts;
  // This thread can steal from any of the workers in the range [friend_worker_start, friend_worker_end)
  uint32_t friend_worker_count = 0;
  uint32_t friend_worker_start = std::numeric_limits<uint32_t>::max();
  uint32_t push_offset         = 0;
  // Thread from which this worker should steal work
  uint32_t stealing_source;
  // worker id
  worker_id id;
  // quit event
  std::atomic_bool quitting = false;
};

} // namespace detail

} // namespace acl
