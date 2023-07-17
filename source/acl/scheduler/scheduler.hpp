
#pragma once
#include "spin_lock.hpp"
#include "task.hpp"
#include "worker.hpp"
#include <acl/utils/type_traits.hpp>
#include <array>
#include <thread>

namespace acl
{

class scheduler
{

public:
  scheduler() noexcept = default;
  ~scheduler() noexcept;

  template <CoroutineTask C>
  void submit(C task_obj, uint32_t group, worker_id current) noexcept
  {
    submit(detail::work_item(nullptr, reinterpret_cast<task_delegate>(task_obj.address()), detail::work_type_coroutine),
           group, current);
  }

  void submit(task* task_obj, task_context* ctx, uint32_t group, worker_id current) noexcept
  {
    submit(detail::work_item(ctx, reinterpret_cast<task_delegate>(task_obj), detail::work_type_task_functor), group,
           current);
  }

  template <auto M, typename Class>
  void submit(Class& ctx, uint32_t group, worker_id current) noexcept
  {
    submit(detail::work_item(
             reinterpret_cast<task_context*>(&ctx),
             [](task_context* cctx, worker_id wid)
             {
               std::invoke(M, *reinterpret_cast<Class*>(cctx), wid);
             },
             detail::work_type_free_functor),
           group, current);
  }

  /// @brief Submit a work for execution
  void submit(detail::work_item work, uint32_t submit_group, worker_id current);

  /// @brief Begin scheduler execution, group creation is frozen after this call
  void begin_execution();
  /// @brief Wait for threads to finish executing and end scheduler execution. Scheduler execution can be restarted
  /// using begin_execution. Unlocks scheduler and makes it mutable.
  void end_execution();

  /// @brief Get worker count in the scheduler
  inline uint32_t get_worker_count() const noexcept
  {
    return worker_count;
  }

  /// @brief Ensure a work-group by id and set a name
  void create_group(uint32_t group, std::string name, uint32_t thread_offset, uint32_t thread_count);
  /// @brief Get the next available group.
  uint32_t create_group(std::string name, uint32_t thread_offset, uint32_t thread_count);
  /// @brief Clear a group, and re-create it
  void clear_group(uint32_t group);
  /// @brief  Find an existing work group by name
  uint32_t find_group(std::string const& name);
  /// @brief Get worker count in this group
  inline uint32_t get_worker_count(uint32_t g) const noexcept
  {
    return work_groups[g].thread_count;
  }

private:
  void              finish_pending_tasks() noexcept;
  void              wake_up(uint32_t) noexcept;
  void              run(worker_id);
  bool              should_we_sleep(uint32_t) noexcept;
  void              work(worker_id) noexcept;
  void              work_no_sleep(worker_id) noexcept;
  detail::work_item get_work(worker_id) noexcept;
  // Global queues, one per thread group
  std::array<detail::work_group, 32> work_groups;

  std::unique_ptr<detail::worker_group_ids[]> worker_groups;
  std::unique_ptr<detail::work_item[]>        immediate_work;
  std::unique_ptr<uint32_t[]>                 group_masks;
  std::unique_ptr<std::atomic_bool[]>         sleep_status;
  std::unique_ptr<detail::wake_event[]>       wake_events;
  std::vector<std::thread>                    threads;
  uint32_t                                    worker_count = 0;
  std::atomic_bool                            stop         = false;
};

} // namespace acl