
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
  inline void submit(C const& task_obj, workgroup_id submit_group, worker_id current) noexcept
  {
    task_data data;
    data.reserved_0 = detail::work_type_coroutine;
    data.reserved_1 = static_cast<uint8_t>(submit_group.get_index());
    submit(detail::work_item(reinterpret_cast<task_delegate>(task_obj.address()), data), current);
  }

  inline void submit(task* task_obj, task_data data, workgroup_id submit_group, worker_id current) noexcept
  {
    data.reserved_0 = detail::work_type_task_functor;
    data.reserved_1 = static_cast<uint8_t>(submit_group.get_index());
    submit(detail::work_item(reinterpret_cast<task_delegate>(task_obj), data), current);
  }

  inline void submit(task_delegate task_obj, task_data data, workgroup_id submit_group, worker_id current) noexcept
  {
    data.reserved_0 = detail::work_type_free_functor;
    data.reserved_1 = static_cast<uint8_t>(submit_group.get_index());
    submit(detail::work_item(task_obj, data), current);
  }

  template <auto M, typename Class>
  inline void submit(Class& ctx, workgroup_id submit_group, worker_id current) noexcept
  {
    task_data data;
    data.context    = reinterpret_cast<task_context*>(&ctx);
    data.reserved_0 = detail::work_type_free_functor;
    data.reserved_1 = static_cast<uint8_t>(submit_group.get_index());

    submit(detail::work_item(
             [](task_data cctx, worker_context const& wid)
             {
               std::invoke(M, *reinterpret_cast<Class*>(cctx.context), std::cref(wid));
             },
             data),
           current);
  }

  template <auto M, typename Class>
  inline void submit(Class& ctx, uint32_t additional_data, workgroup_id submit_group, worker_id current) noexcept
  {
    task_data data;
    data.context    = reinterpret_cast<task_context*>(&ctx);
    data.uint_data  = additional_data;
    data.reserved_0 = detail::work_type_free_functor;
    data.reserved_1 = static_cast<uint8_t>(submit_group.get_index());

    submit(detail::work_item(
             [](task_data cctx, worker_context const& wid)
             {
               std::invoke(M, *reinterpret_cast<Class*>(cctx.context), std::cref(wid), cctx.uint_data);
             },
             data),
           current);
  }

  template <CoroutineTask C>
  inline void submit_to(C const& task_obj, worker_id to, worker_id current) noexcept
  {
    task_data data;
    data.reserved_0 = detail::work_type_coroutine;
    data.reserved_1 = static_cast<uint8_t>(default_workgroup_id.get_index());
    submit_to(detail::work_item(reinterpret_cast<task_delegate>(task_obj.address()), data), to, current);
  }

  inline void submit_to(task* task_obj, task_data data, worker_id to, worker_id current) noexcept
  {
    data.reserved_0 = detail::work_type_task_functor;
    data.reserved_1 = static_cast<uint8_t>(default_workgroup_id.get_index());
    submit_to(detail::work_item(reinterpret_cast<task_delegate>(task_obj), data), to, current);
  }

  inline void submit_to(task_delegate task_obj, task_data data, worker_id to, worker_id current) noexcept
  {
    data.reserved_0 = detail::work_type_free_functor;
    data.reserved_1 = static_cast<uint8_t>(default_workgroup_id.get_index());
    submit_to(detail::work_item(task_obj, data), to, current);
  }

  template <auto M, typename Class>
  inline void submit_to(Class& ctx, worker_id to, worker_id current) noexcept
  {
    task_data data;
    data.context    = reinterpret_cast<task_context*>(&ctx);
    data.reserved_0 = detail::work_type_free_functor;
    data.reserved_1 = static_cast<uint8_t>(default_workgroup_id.get_index());

    submit_to(detail::work_item(
                [](task_data cctx, worker_context const& wid)
                {
                  std::invoke(M, *reinterpret_cast<Class*>(cctx.context), std::cref(wid));
                },
                data),
              to, current);
  }

  template <auto M, typename Class>
  inline void submit_to(Class& ctx, uint32_t additional_data, worker_id to, worker_id current) noexcept
  {
    task_data data;
    data.context    = reinterpret_cast<task_context*>(&ctx);
    data.uint_data  = additional_data;
    data.reserved_0 = detail::work_type_free_functor;
    data.reserved_1 = static_cast<uint8_t>(default_workgroup_id.get_index());

    submit_to(detail::work_item(
                [](task_data cctx, worker_context const& wid)
                {
                  std::invoke(M, *reinterpret_cast<Class*>(cctx.context), std::cref(wid), cctx.uint_data);
                },
                data),
              to, current);
  }

  /**
   * @brief Submit a work for execution in the exclusive worker thread
   */
  ACL_API void submit_to(detail::work_item work, worker_id to, worker_id current);

  /**
   * @brief Submit a work for execution
   */
  ACL_API void submit(detail::work_item work, worker_id current);

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
  ACL_API void create_group(workgroup_id group, std::string name, uint32_t thread_offset, uint32_t thread_count);
  /**
   * @brief Get the next available group.
   */
  ACL_API workgroup_id create_group(std::string name, uint32_t thread_offset, uint32_t thread_count);
  /**
   * @brief Clear a group, and re-create it
   */
  ACL_API void clear_group(workgroup_id group);
  /**
   * @brief  Find an existing work group by name
   */
  ACL_API workgroup_id find_group(std::string const& name);
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
    return workers[worker.get_index()].contexts[group.get_index()].get();
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
  detail::work_item try_get_work(worker_id) noexcept;
  detail::work_item get_work(worker_id) noexcept;

  bool work(worker_id) noexcept;

  scheduler_worker_entry entry_fn;
  // Work groups
  std::array<detail::workgroup, 32> workgroups;
  // Workers present in the scheduler
  std::unique_ptr<detail::worker[]> workers;
  // Local cache for work items, until they are pushed into global queue
  std::unique_ptr<detail::work_item[]> local_work;
  // Global work items
  std::unique_ptr<detail::global_work_queue[]> global_work;
  std::unique_ptr<uint32_t[]>                  group_masks;
  std::unique_ptr<std::atomic_bool[]>          wake_status;
  std::unique_ptr<detail::wake_event[]>        wake_events;
  std::vector<std::thread>                     threads;
  uint32_t                                     worker_count         = 0;
  uint32_t                                     logical_task_divisor = 32;
  std::atomic_bool                             stop                 = false;
};


template <typename... Args>
void async(worker_context const& current, Args&&... args)
{
  current.get_scheduler().submit(std::forward<Args>(args)..., current.get_worker());
}

} // namespace acl
