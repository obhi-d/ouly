// SPDX-License-Identifier: MIT
#pragma once

// Include the v2 implementation which uses inline v2 namespace, making it available as ouly::scheduler
#include "ouly/scheduler/v1/scheduler.hpp"
#include "ouly/scheduler/v1/task_context.hpp"

#include "ouly/scheduler/v2/scheduler.hpp"
#include "ouly/scheduler/v2/task_context.hpp"

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

} // namespace ouly
