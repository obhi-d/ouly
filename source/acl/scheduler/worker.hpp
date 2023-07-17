

#pragma once

#include "task.hpp"
#include <acl/allocators/default_allocator.hpp>
#include <acl/containers/basic_queue.hpp>
#include <acl/utils/tagged_ptr.hpp>
#include <cstdint>
#include <limits>
#include <semaphore>
#include <string>

namespace acl
{

namespace detail
{
static constexpr uint8_t work_type_coroutine    = 0;
static constexpr uint8_t work_type_task_functor = 1;
static constexpr uint8_t work_type_free_functor = 2;

using work_context = acl::tagged_ptr<task_context>;
struct work_item
{
  work_context  item        = nullptr;
  task_delegate delegate_fn = nullptr;

  inline explicit operator bool() noexcept
  {
    return delegate_fn != nullptr;
  }

  inline work_item() noexcept = default;
  inline work_item(task_context* ctx, task_delegate td, uint8_t h) : item(ctx, h), delegate_fn(td) {}

  inline work_item(work_item&& other) noexcept : item(other.item), delegate_fn(other.delegate_fn)
  {
    other.delegate_fn = nullptr;
  }

  inline work_item& operator=(work_item&& other) noexcept
  {
    item              = other.item;
    delegate_fn       = other.delegate_fn;
    other.delegate_fn = nullptr;
    return *this;
  }

  auto unpack() const noexcept
  {
    return std::make_tuple(delegate_fn, item.get_ptr(), item.get_tag());
  }
};

struct work_queue_traits
{
  static constexpr uint32_t pool_size_v = 2048;
  using allocator_t                     = acl::default_allocator<>;
};
using work_queue_t       = acl::basic_queue<work_item, work_queue_traits>;
using concurrent_queue_t = std::pair<spin_lock, work_queue_t>;

struct work_group
{
  // Global queues, one per thread group
  std::string          name;
  uint32_t             start_thread_idx = 0;
  uint32_t             end_thread_idx   = 0;
  uint32_t             thread_count     = 0;
  std::atomic_uint32_t thread_selection = 0;
  std::atomic_uint32_t work_count       = 0;

  inline uint32_t create_group(std::string gname, uint32_t start, uint32_t count) noexcept
  {
    name             = std::move(gname);
    start_thread_idx = start;
    end_thread_idx   = start + count;
    thread_count     = count;
    thread_selection = start;
    queues           = std::make_unique<work_queue_t[]>(thread_count);
    locks            = std::make_unique<spin_lock[]>(thread_count);
    return end_thread_idx;
  }
  // Local queue
  concurrent_queue_t              shared_queue;
  std::unique_ptr<work_queue_t[]> queues;
  std::unique_ptr<spin_lock[]>    locks;

  work_group() noexcept = default;
};

struct wake_event
{
  inline wake_event() noexcept : semaphore(0) {}

  inline void acquire() noexcept
  {
    semaphore.acquire();
  }

  inline void release() noexcept
  {
    semaphore.release();
  }

  std::binary_semaphore semaphore;
};

struct worker_group_ids
{
  uint32_t group_count = 0;
  uint32_t group_ids[32];
};

} // namespace detail

} // namespace acl