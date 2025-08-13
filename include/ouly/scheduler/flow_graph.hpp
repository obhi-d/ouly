// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/containers/small_vector.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include <atomic>
#include <cstdint>
#include <semaphore>
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
  enum class status : uint8_t
  {
    all_done,
    more_tasks
  };

  class task_node
  {
  public:
    task_node()  = default;
    ~task_node() = default;

    // Custom move constructor to handle atomic members
    task_node(task_node&& other) noexcept
        : workgroup_(other.workgroup_), tasks_(std::move(other.tasks_)), next_nodes_(std::move(other.next_nodes_)),
          pending_dependencies_(other.pending_dependencies_.load(std::memory_order_relaxed))
    {}

    // Custom move assignment operator
    auto operator=(task_node&& other) noexcept -> task_node&
    {
      if (this != &other)
      {
        workgroup_  = other.workgroup_;
        tasks_      = std::move(other.tasks_);
        next_nodes_ = std::move(other.next_nodes_);
        pending_dependencies_.store(other.pending_dependencies_.load(std::memory_order_relaxed),
                                    std::memory_order_relaxed);
      }
      return *this;
    }

    // Delete copy operations
    task_node(const task_node&)                    = delete;
    auto operator=(const task_node&) -> task_node& = delete;
    void add(delegate_type&& task) noexcept
    {
      tasks_.emplace_back(std::move(task));
    }

    void set_workgroup(workgroup_id group) noexcept
    {
      workgroup_ = group;
    }

    [[nodiscard]] auto get_workgroup() const noexcept -> workgroup_id
    {
      return workgroup_;
    }

    void add_successor(uint32_t node_id) noexcept
    {
      next_nodes_.emplace_back(node_id);
    }

    [[nodiscard]] auto get_successors() const noexcept -> std::span<const uint32_t>
    {
      return {next_nodes_.data(), next_nodes_.size()};
    }

    [[nodiscard]] auto get_tasks() const noexcept -> std::span<const delegate_type>
    {
      return {tasks_.data(), tasks_.size()};
    }

    void reset_dependencies(uint32_t count) noexcept
    {
      pending_dependencies_.store(count, std::memory_order_relaxed);
    }

    void reset_run_count() noexcept
    {
      run_count_.store(0, std::memory_order_relaxed);
    }

    auto decrement_dependencies() noexcept -> uint32_t
    {
      return pending_dependencies_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    }

    [[nodiscard]] auto has_pending_dependencies() const noexcept -> bool
    {
      return pending_dependencies_.load(std::memory_order_acquire) > 0;
    }

    auto execute_task(uint32_t index, context_type const& ctx) noexcept -> status
    {
      tasks_[index](ctx);
      // Check if this is the last task in this node to complete
      auto completed_count = run_count_.fetch_add(1, std::memory_order_acq_rel) + 1;
      if (completed_count == static_cast<uint32_t>(tasks_.size()))
      {
        return status::all_done;
      }
      return status::more_tasks;
    }

  private:
    workgroup_id                               workgroup_{default_workgroup_id};
    std::vector<delegate_type>                 tasks_;
    ouly::small_vector<uint32_t, AvgNodeCount> next_nodes_;
    std::atomic<uint32_t>                      pending_dependencies_{0};
    std::atomic_uint32_t                       run_count_{0}; // For tracking how many times this node has been executed
  };

  ouly::small_vector<task_node, AvgNodeCount> nodes_;
  ouly::small_vector<uint32_t, AvgNodeCount>  dependency_counts_;
  uint32_t                                    total_tasks_{0};
  std::atomic<uint32_t>                       remaining_tasks_{0};
  std::atomic_bool                            started_{false};
  std::binary_semaphore                       done_{0};

  void execute_node(uint32_t node_id, context_type const& ctx);
  void notify_successors(uint32_t node_id, context_type const& ctx);
};

//
// Implementation
//

template <typename SchedulerType, size_t AvgNodeCount>
auto flow_graph<SchedulerType, AvgNodeCount>::create_node() -> node_id
{
  auto id = static_cast<node_id>(nodes_.size());
  nodes_.emplace_back();
  dependency_counts_.emplace_back(0);
  return id;
}

template <typename SchedulerType, size_t AvgNodeCount>
void flow_graph<SchedulerType, AvgNodeCount>::add(node_id id, delegate_type&& exec_delegate) noexcept
{
  if (id < nodes_.size())
  {
    nodes_[id].add(std::move(exec_delegate));
  }
}

template <typename SchedulerType, size_t AvgNodeCount>
void flow_graph<SchedulerType, AvgNodeCount>::connect(node_id from, node_id to) noexcept
{
  if (from < nodes_.size() && to < nodes_.size())
  {
    nodes_[from].add_successor(to);
    dependency_counts_[to]++;
  }
}

template <typename SchedulerType, size_t AvgNodeCount>
void flow_graph<SchedulerType, AvgNodeCount>::start(context_type const& ctx)
{
  // Reset state for reusability
  started_.store(false, std::memory_order_relaxed);
  remaining_tasks_.store(0, std::memory_order_relaxed);

  for (uint32_t node = 0; node < nodes_.size(); ++node)
  {
    nodes_[node].reset_dependencies(dependency_counts_[node]);
    nodes_[node].reset_run_count();
  }

  uint32_t total_tasks = 0;
  for (uint32_t i = 0; i < nodes_.size(); ++i)
  {
    total_tasks += static_cast<uint32_t>(nodes_[i].get_tasks().size());
  }
  total_tasks_ = total_tasks;

  // Initialize dependency counts and find ready nodes
  remaining_tasks_.store(total_tasks_, std::memory_order_relaxed);
  started_.store(true, std::memory_order_release);

  // Submit all ready nodes (nodes with 0 dependencies)
  for (uint32_t i = 0; i < nodes_.size(); ++i)
  {
    if (dependency_counts_[i] == 0)
    {
      execute_node(i, ctx);
    }
  }
}

template <typename SchedulerType, size_t AvgNodeCount>
void flow_graph<SchedulerType, AvgNodeCount>::execute_node(uint32_t node_id, context_type const& ctx)
{
  auto& node      = nodes_[node_id];
  auto  tasks     = node.get_tasks();
  auto  workgroup = node.get_workgroup();

  if (tasks.empty())
  {
    // Node has no tasks, just notify successors
    notify_successors(node_id, ctx);
    return;
  }

  // Submit all tasks for this node
  for (size_t i = 0; i < tasks.size(); ++i)
  {
    // Create a simple lambda that captures minimal data
    auto* graph_ptr = this;
    ctx.get_scheduler().submit(ctx, workgroup,
                               [graph_ptr, node_id, i](context_type const& task_ctx) mutable
                               {
                                 // Execute the actual task
                                 if (graph_ptr->nodes_[node_id].execute_task(static_cast<uint32_t>(i), task_ctx) ==
                                     status::all_done)
                                 {
                                   // Last task in this node, notify successors
                                   graph_ptr->notify_successors(node_id, task_ctx);
                                 }
                                 // Each task decrements the global task count
                                 if (graph_ptr->remaining_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1)
                                 {
                                   graph_ptr->done_.release();
                                 }
                               });
  }
}

template <typename SchedulerType, size_t AvgNodeCount>
void flow_graph<SchedulerType, AvgNodeCount>::notify_successors(uint32_t node_id, context_type const& ctx)
{
  if (node_id >= nodes_.size())
  {
    return;
  }

  auto& node       = nodes_[node_id];
  auto  successors = node.get_successors();

  // Notify all successor nodes
  for (uint32_t successor_id : successors)
  {
    if (successor_id < nodes_.size())
    {
      auto& successor = nodes_[successor_id];
      if (successor.decrement_dependencies() == 0)
      {
        // All dependencies satisfied, execute this node
        execute_node(successor_id, ctx);
      }
    }
  }
}

template <typename SchedulerType, size_t AvgNodeCount>
void flow_graph<SchedulerType, AvgNodeCount>::cooperative_wait(context_type const& ctx)
{
  if (!started_.load(std::memory_order_acquire))
  {
    return;
  }

  ctx.busy_wait(done_);
}

template <typename SchedulerType, size_t AvgNodeCount>
void flow_graph<SchedulerType, AvgNodeCount>::wait(context_type const& /* ctx */)
{
  if (!started_.load(std::memory_order_acquire))
  {
    return;
  }

  done_.acquire();
}

} // namespace ouly