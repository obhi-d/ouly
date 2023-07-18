
#pragma once
#include "spin_lock.hpp"
#include "task.hpp"
#include "worker.hpp"
#include <acl/utils/type_traits.hpp>
#include <array>
#include <thread>

namespace acl
{

using scheduler_worker_entry = std::function<void(worker_id)>;

class scheduler
{

public:
  static constexpr uint32_t work_scale = 4;

  scheduler() noexcept = default;
  ~scheduler() noexcept;

  template <CoroutineTask C>
  void submit(C const& task_obj, work_group_id submit_group, worker_id current) noexcept
  {
    submit(detail::work_item(nullptr, reinterpret_cast<task_delegate>(task_obj.address()), detail::work_type_coroutine),
           submit_group, current);
  }

  void submit(task* task_obj, task_context* ctx, work_group_id submit_group, worker_id current) noexcept
  {
    submit(detail::work_item(ctx, reinterpret_cast<task_delegate>(task_obj), detail::work_type_task_functor),
           submit_group, current);
  }

  template <auto M, typename Class>
  void submit(Class& ctx, work_group_id submit_group, worker_id current) noexcept
  {
    submit(detail::work_item(
             reinterpret_cast<task_context*>(&ctx),
             [](task_context* cctx, worker_context const& wid)
             {
               std::invoke(M, *reinterpret_cast<Class*>(cctx), std::cref(wid));
             },
             detail::work_type_free_functor),
           submit_group, current);
  }

  /// @brief Submit a work for execution
  void submit(detail::work_item work, work_group_id submit_group, worker_id current);

  /// @brief Begin scheduler execution, group creation is frozen after this call.
  /// @param entry An entry function can be provided that will be executed on all worker threads upon entry.
  void begin_execution(scheduler_worker_entry&& entry = {});
  /// @brief Wait for threads to finish executing and end scheduler execution. Scheduler execution can be restarted
  /// using begin_execution. Unlocks scheduler and makes it mutable.
  void end_execution();

  /// @brief Get worker count in the scheduler
  inline uint32_t get_worker_count() const noexcept
  {
    return worker_count;
  }

  /// @brief Ensure a work-group by id and set a name
  void create_group(work_group_id group, std::string name, uint32_t thread_offset, uint32_t thread_count);
  /// @brief Get the next available group.
  work_group_id create_group(std::string name, uint32_t thread_offset, uint32_t thread_count);
  /// @brief Clear a group, and re-create it
  void clear_group(work_group_id group);
  /// @brief  Find an existing work group by name
  work_group_id find_group(std::string const& name);
  /// @brief Get worker count in this group
  inline uint32_t get_worker_count(work_group_id g) const noexcept
  {
    return work_groups[g.get_index()].thread_count;
  }

  /// @brief Get worker start index
  inline uint32_t get_worker_start_idx(work_group_id g) const noexcept
  {
    return work_groups[g.get_index()].start_thread_idx;
  }

  /// @brief Get worker d
  inline uint32_t get_logical_divisor(work_group_id g) const noexcept
  {
    return work_groups[g.get_index()].thread_count * work_scale;
  }

  worker_context const& get_context(worker_id worker, work_group_id group)
  {
    return workers[worker.get_index()].contexts[group.get_index()].get();
  }

  /// @brief If multiple schedulers are active, this function should be called from main thread before using the
  /// scheduler
  void take_ownership() noexcept;

private:
  void         finish_pending_tasks() noexcept;
  void         wake_up(worker_id) noexcept;
  void         run(worker_id);
  bool         should_we_sleep(worker_id) noexcept;
  detail::work get_work(worker_id) noexcept;

  bool work(worker_id) noexcept;

  scheduler_worker_entry entry_fn;
  // Global queues, one per thread group
  std::array<detail::work_group, 32> work_groups;

  std::unique_ptr<detail::worker[]>     workers;
  std::unique_ptr<detail::work_item[]>  immediate_work;
  std::unique_ptr<work_group_id[]>      immediate_work_group;
  std::unique_ptr<uint32_t[]>           group_masks;
  std::unique_ptr<std::atomic_bool[]>   sleep_status;
  std::unique_ptr<detail::wake_event[]> wake_events;
  std::vector<std::thread>              threads;
  uint32_t                              worker_count         = 0;
  uint32_t                              logical_task_divisor = 32;
  std::atomic_bool                      stop                 = false;
};

} // namespace acl