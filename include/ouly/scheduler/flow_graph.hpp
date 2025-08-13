// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/containers/small_vector.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include <atomic>
#include <span>
#include <vector>
namespace ouly
{

/**
 * @brief Flow Graph
 *
 * `task_node` is a class that represents a node in the flow graph. You can add tasks to a task node to be scheduled for
 * exection when a flow graph is started. `task_node::then(&next_task_node)` can be used to specify the next task to be
 * executed after the current task. A task_node can only be executed in a single workgroup
 */

template <typename SchedulerType, size_t AvgNodeCount = 4>
class flow_graph
{
public:
  using node_id       = uint32_t;
  using delegate_type = typename SchedulerType::delegate_type;
  using context_type  = typename SchedulerType::context_type;

  auto create_node() -> node_id;

  void add(node_id id, delegate_type&& exec_delegate) noexcept;
  void connect(node_id from, node_id to) noexcept;

  void start(context_type const& ctx);

  // possibly calls ctx.get_scheduler().busy_wait(ctx) based on done_.try_wait
  void cooperative_wait(context_type const& ctx);

  // waits for done_
  void wait(context_type const& ctx);

private:
  class task_node
  {
  public:
    void add(delegate_type&& task) noexcept
    {
      tasks_.emplace_back(std::move(task));
    }

  private:
    workgroup_id                               workgroup_;
    std::vector<delegate_type>                 tasks_;
    ouly::small_vector<uint32_t, AvgNodeCount> next_nodes_;
  };

  ouly::small_vector<task_node, AvgNodeCount> nodes_;
  std::atomic_bool                            started_{false};
  std::binary_semaphore                       done_{0};
};

} // namespace ouly