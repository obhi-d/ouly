
#pragma once
#include "task.hpp"
#include "worker.hpp"
#include <acl/utils/config.hpp>
#include <acl/utils/type_traits.hpp>
#include <array>
#include <thread>

namespace acl
{

using scheduler_worker_entry = std::function<void(worker_desc)>;

class scheduler
{

public:
  static constexpr uint32_t work_scale = 4;

  ACL_API scheduler() noexcept = default;
  ACL_API ~scheduler() noexcept;

  template <CoroutineTask C>
  inline void submit(worker_id src, workgroup_id dst, C const& task_obj) noexcept
  {
    submit(src, dst,
           detail::work_item(
             [](task_data const& data, worker_context const&)
             {
               std::coroutine_handle<>::from_address(data.get<void*>()).resume();
             },
             task_data(dst, task_obj.address())));
  }

  template <typename... Args>
  inline void submit(worker_id src, workgroup_id dst, task_delegate task_obj, Args... data) noexcept
  {
    submit(src, dst, detail::work_item(task_obj, task_data(dst, data...)));
  }

  template <auto M, typename Class, typename... Args>
  inline void submit(worker_id src, workgroup_id dst, Class* ctx, Args... data) noexcept
  {
    submit(src, dst,
           detail::work_item(
             [](task_data const& data, worker_context const& wid)
             {
               auto const& tup = data.get<Class*, Args...>();
               if constexpr (sizeof...(Args) == 0)
                 std::invoke(M, tup, std::cref(wid));
               else
                 std::invoke(M, std::get<0>(tup), std::cref(wid), std::get<Args>(tup)...);
             },
             task_data(dst, ctx, data...)));
  }

  template <CoroutineTask C>
  inline void submit(worker_id src, worker_id dst, workgroup_id group, C const& task_obj) noexcept
  {
    submit(src, dst,
           detail::work_item(
             [](task_data const& data, worker_context const&)
             {
               std::coroutine_handle<>::from_address(data.get<void*>()).resume();
             },
             task_data(group, task_obj.address())));
  }

  template <typename... Args>
  inline void submit(worker_id src, worker_id dst, workgroup_id group, task_delegate task_obj, Args... data) noexcept
  {
    submit(src, dst, detail::work_item(task_obj, task_data(group, data...)));
  }

  template <auto M, typename Class, typename... Args>
  inline void submit(worker_id src, worker_id dst, workgroup_id group, Class* ctx, Args... data) noexcept
  {
    submit(src, dst,
           detail::work_item(
             [](task_data const& data, worker_context const& wid)
             {
               auto const& tup = data.get<Class*, Args...>();
               if constexpr (sizeof...(Args) == 0)
                 std::invoke(M, tup, std::cref(wid));
               else
                 std::invoke(M, std::get<0>(tup), std::cref(wid), std::get<Args>(tup)...);
             },
             task_data(group, ctx, data...)));
  }

  /**
   * @brief Submit a work for execution in the exclusive worker thread
   */
  ACL_API void submit(worker_id src, worker_id dst, detail::work_item const& work);

  /**
   * @brief Submit a work for execution
   */
  ACL_API void submit(worker_id src, workgroup_id dst, detail::work_item const& work);

  /**
   * @brief Begin scheduler execution, group creation is frozen after this call.
   * @param entry An entry function can be provided that will be executed on all worker threads upon entry.
   */
  ACL_API void begin_execution(scheduler_worker_entry&& entry = {});
  /**
   * @brief Wait for threads to finish executing and end scheduler execution. Scheduler execution can be restarted
   * using begin_execution. Unlocks scheduler and makes it mutable.
   */
  ACL_API void end_execution();

  /**
   * @brief Get worker count in the scheduler
   */
  inline uint32_t get_worker_count() const noexcept
  {
    return worker_count;
  }

  /**
   * @brief Ensure a work-group by id and set a name
   */
  ACL_API void create_group(workgroup_id group, uint32_t thread_offset, uint32_t thread_count, uint32_t priority = 0);
  /**
   * @brief Get the next available group. Group priority controls if a thread is shared between multiple groups, which
   * group is executed first by the thread
   */
  ACL_API workgroup_id create_group(uint32_t thread_offset, uint32_t thread_count, uint32_t priority = 0);
  /**
   * @brief Clear a group, and re-create it
   */
  ACL_API void clear_group(workgroup_id group);
  /**
   * @brief Get worker count in this group
   */
  inline uint32_t get_worker_count(workgroup_id g) const noexcept
  {
    return workgroups[g.get_index()].thread_count;
  }

  /**
   * @brief Get worker start index
   */
  inline uint32_t get_worker_start_idx(workgroup_id g) const noexcept
  {
    return workgroups[g.get_index()].start_thread_idx;
  }

  /**
   * @brief Get worker d
   */
  inline uint32_t get_logical_divisor(workgroup_id g) const noexcept
  {
    return workgroups[g.get_index()].thread_count * work_scale;
  }

  worker_context const& get_context(worker_id worker, workgroup_id group)
  {
    return workers[worker.get_index()].contexts[group.get_index()];
  }

  /**
   * @brief If multiple schedulers are active, this function should be called from main thread before using the
   * scheduler
   */
  ACL_API void take_ownership() noexcept;
  ACL_API void busy_work(worker_id) noexcept;

private:
  void              finish_pending_tasks() noexcept;
  inline void       do_work(worker_id, detail::work_item const&) noexcept;
  void              wake_up(worker_id) noexcept;
  void              run(worker_id);
  detail::work_item get_work(worker_id) noexcept;

  bool work(worker_id) noexcept;

  scheduler_worker_entry entry_fn;
  // Work groups
  std::vector<detail::workgroup> workgroups;
  // Workers present in the scheduler
  std::unique_ptr<detail::worker[]> workers;
  // Local cache for work items, until they are pushed into global queue
  std::unique_ptr<detail::work_item[]> local_work;
  // Global work items
  std::unique_ptr<detail::group_range[]> group_ranges;
  std::unique_ptr<std::atomic_bool[]>    wake_status;
  std::unique_ptr<detail::wake_event[]>  wake_events;
  std::vector<std::thread>               threads;

  uint32_t         worker_count         = 0;
  uint32_t         logical_task_divisor = 32;
  std::atomic_bool stop                 = false;
};

template <typename... Args>
void async(worker_context const& current, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().submit(current.get_worker(), submit_group, std::forward<Args>(args)...);
}

template <auto M, typename Class, typename... Args>
void async(worker_context const& current, workgroup_id submit_group, Class* obj, Args... data)
{
  current.get_scheduler().submit<M>(current.get_worker(), submit_group, obj, data...);
}

template <typename... Args>
void async(worker_context const& current, worker_id dst, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().submit(current.get_worker(), dst, submit_group, std::forward<Args>(args)...);
}

template <auto M, typename Class, typename... Args>
void async(worker_context const& current, worker_id dst, workgroup_id submit_group, Class* obj, Args... data)
{
  current.get_scheduler().submit<M>(current.get_worker(), dst, submit_group, obj, data...);
}

} // namespace acl
