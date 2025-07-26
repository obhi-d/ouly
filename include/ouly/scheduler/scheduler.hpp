// SPDX-License-Identifier: MIT
#pragma once

// Include the v2 implementation which uses inline v2 namespace, making it available as ouly::scheduler
#include "ouly/scheduler/scheduler_v2.hpp"
#include "ouly/scheduler/task_context_v2.hpp"

namespace ouly
{
/**
 * @brief Asynchronously submits a task to the scheduler
 *
 * This function forwards the given arguments to the scheduler's submit function,
 * allowing tasks to be queued for asynchronous execution in the specified workgroup.
 *
 * @tparam Args Variadic template parameter pack for forwarded arguments
 * @param current The current worker context from which the task is being submitted
 * @param args Arguments to be forwarded to the task
 */
template <TaskContext WC, typename... Args>
void async(WC const& current, Args&&... args)
{
  current.get_scheduler().submit(current, std::forward<Args>(args)...);
}

/**
 * @brief Submits a task asynchronously to the scheduler
 *
 * @tparam M Work item type to be submitted
 * @tparam Args Variable template parameter pack for work item arguments
 *
 * @param current Current worker context that will submit the work
 * @param args Arguments to be forwarded to the work item constructor
 *
 * @note This is a helper function that forwards the submission request to the scheduler
 *       associated with the current worker context
 */
template <auto M, TaskContext WC, typename... Args>
void async(WC const& current, Args&&... args)
{
  current.get_scheduler().template submit<M>(current, std::forward<Args>(args)...);
}

/**
 * @brief Asynchronously submits a task to the scheduler
 *
 * This function forwards the given arguments to the scheduler's submit function,
 * allowing tasks to be queued for asynchronous execution in the specified workgroup.
 *
 * @tparam Args Variadic template parameter pack for forwarded arguments
 * @param current The current worker context from which the task is being submitted
 * @param submit_group The workgroup identifier where the task should be scheduled
 * @param args Arguments to be forwarded to the task
 *
 * @note This is a convenience wrapper around scheduler::submit()
 */
template <TaskContext WC, typename... Args>
void async(WC const& current, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().submit(current, submit_group, std::forward<Args>(args)...);
}

/**
 * @brief Submits a task asynchronously to the scheduler
 *
 * @tparam M Work item type to be submitted
 * @tparam Args Variable template parameter pack for work item arguments
 *
 * @param current Current worker context that will submit the work
 * @param submit_group Target workgroup ID where the work will be submitted
 * @param args Arguments to be forwarded to the work item constructor
 *
 * @note This is a helper function that forwards the submission request to the scheduler
 *       associated with the current worker context
 */
template <auto M, TaskContext WC, typename... Args>
void async(WC const& current, workgroup_id submit_group, Args&&... args)
{
  current.get_scheduler().template submit<M>(current, submit_group, std::forward<Args>(args)...);
}

} // namespace ouly