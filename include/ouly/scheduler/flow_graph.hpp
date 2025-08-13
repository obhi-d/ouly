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
 * @file flow_graph.hpp
 * @brief Task Flow Graph for Parallel Execution
 *
 * This file contains the flow_graph class which provides a powerful framework for
 * orchestrating complex task dependencies and parallel execution patterns using
 * the OULY scheduler system.
 */

/**
 * @brief Flow Graph - A Directed Acyclic Graph (DAG) for Task Orchestration
 *
 * The flow_graph class enables the creation and execution of complex task dependency
 * graphs where tasks are organized into nodes, and dependencies between nodes control
 * execution order. This is particularly useful for:
 *
 * - Parallel processing pipelines
 * - Complex build systems
 * - Game update loops with dependencies
 * - Data processing workflows
 * - Any scenario requiring ordered parallel execution
 *
 * @tparam SchedulerType The scheduler type (v1::scheduler or v2::scheduler) that will execute tasks
 * @tparam AvgNodeCount Expected average number of nodes for optimization (default: 4)
 *
 * ## Key Features:
 *
 * - **Thread-Safe**: All operations are thread-safe and use atomic synchronization
 * - **Reusable**: Graphs can be executed multiple times with different data
 * - **Flexible**: Supports multiple tasks per node and complex dependency patterns
 * - **Efficient**: Uses work-stealing and cooperative waiting for optimal performance
 * - **Dynamic**: Tasks can be added after graph construction but before execution
 *
 * ## Usage Example:
 *
 * ```cpp
 * // Create a flow graph for the v2 scheduler
 * ouly::flow_graph<ouly::v2::scheduler> graph;
 *
 * // Create nodes
 * auto setup_node = graph.create_node();
 * auto process_node = graph.create_node();
 * auto finalize_node = graph.create_node();
 *
 * // Define dependencies: setup -> process -> finalize
 * graph.connect(setup_node, process_node);
 * graph.connect(process_node, finalize_node);
 *
 * // Add tasks to nodes
 * graph.add(setup_node,
 *     [](auto const& ctx) {
 *         // Setup work
 *         std::cout << "Setting up...\n";
 *     });
 *
 * graph.add(process_node,
 *     [](auto const& ctx) {
 *         // Main processing work
 *         std::cout << "Processing...\n";
 *     });
 *
 * graph.add(finalize_node,
 *     [](auto const& ctx) {
 *         // Cleanup work
 *         std::cout << "Finalizing...\n";
 *     });
 *
 * // Execute the graph
 * auto ctx = SchedulerType::context_type::this_context::get();
 * graph.start(ctx);
 * graph.cooperative_wait(ctx);  // Use cooperative_wait for main thread participation
 * ```
 *
 * ## Complex Dependency Patterns:
 *
 * The flow_graph supports complex patterns like:
 *
 * - **Fan-out**: One node feeding multiple parallel nodes
 * - **Fan-in**: Multiple nodes converging to one node
 * - **Diamond**: Parallel branches that reconverge
 * - **Tree structures**: Hierarchical dependencies
 *
 * ```cpp
 * // Diamond pattern example:
 * //     root
 * //    /    \\
 * //  left   right
 * //    \\    /
 * //     join
 *
 * auto root = graph.create_node();
 * auto left = graph.create_node();
 * auto right = graph.create_node();
 * auto join = graph.create_node();
 *
 * graph.connect(root, left);
 * graph.connect(root, right);
 * graph.connect(left, join);
 * graph.connect(right, join);
 * ```
 *
 * ## Thread Safety and Performance:
 *
 * - Uses atomic operations for dependency tracking and completion counting
 * - Employs work-stealing queues through the scheduler for load balancing
 * - Supports cooperative waiting to prevent main thread blocking
 * - Optimized for cache-friendly access patterns
 *
 * ## Important Notes:
 *
 * - When using a single workgroup, always use `cooperative_wait()` instead of `wait()`
 *   to prevent deadlocks, as the main thread participates as a worker
 * - Tasks can be added dynamically up until `start()` is called
 * - The graph can be reused by calling `start()` multiple times
 * - Empty nodes (nodes without tasks) are supported and will trigger their successors
 */

template <typename SchedulerType, size_t AvgNodeCount = 4>
class flow_graph
{
public:
  using node_id       = uint32_t;                              ///< Type for node identifiers
  using delegate_type = typename SchedulerType::delegate_type; ///< Task delegate type from scheduler
  using context_type  = typename SchedulerType::context_type;  ///< Scheduler context type

  /**
   * @brief Create a new node in the flow graph
   *
   * Creates a new task node and returns its unique identifier. Nodes are created
   * sequentially starting from 0.
   *
   * @return node_id The unique identifier for the newly created node
   *
   * @note This operation is not thread-safe and should be done during graph construction
   */
  auto create_node() -> node_id;

  /**
   * @brief Add a task to a specific node
   *
   * Adds a task lambda to the specified node. Multiple tasks can be added to the
   * same node, and they will execute in parallel when the node's dependencies are satisfied.
   *
   * @param id The node identifier to add the task to
   * @param exec_delegate The task lambda to execute
   *
   * @note Tasks can be added dynamically up until start() is called
   * @note This operation is not thread-safe during graph construction
   */
  template <typename Func>
  void add(node_id id, Func&& exec_delegate) noexcept
  {
    if (id < nodes_.size())
    {
      nodes_[id].add(delegate_type::bind(std::forward<Func>(exec_delegate)));
    }
  }

  /**
   * @brief Create a dependency between two nodes
   *
   * Establishes a dependency where the 'to' node will only execute after the 'from'
   * node has completed all its tasks.
   *
   * @param from The predecessor node that must complete first
   * @param to The successor node that will wait for the predecessor
   *
   * @note This creates a directed edge in the dependency graph
   * @note Circular dependencies are not detected and will cause deadlocks
   * @note This operation is not thread-safe during graph construction
   */
  void connect(node_id from, node_id to) noexcept;

  /**
   * @brief Start execution of the flow graph
   *
   * Begins execution of the flow graph by submitting all ready nodes (nodes with no
   * dependencies) to the scheduler. The graph will execute asynchronously according
   * to the defined dependencies.
   *
   * @param ctx The scheduler context for task submission
   *
   * @note This method can be called multiple times to re-execute the graph
   * @note All tasks added to nodes up to this point will be executed
   * @note This method calculates total task count dynamically to handle late additions
   */
  void start(context_type const& ctx);

  /**
   * @brief Wait for graph completion with cooperative multitasking
   *
   * Waits for the flow graph to complete execution while allowing the calling thread
   * to participate in task execution. This is the preferred waiting method when the
   * main thread is part of the worker pool.
   *
   * @param ctx The scheduler context for cooperative task execution
   *
   * @warning Always use this method instead of wait() when using a single workgroup
   *          to prevent deadlocks, as the main thread is considered a worker
   */
  void cooperative_wait(context_type const& ctx);

  /**
   * @brief Wait for graph completion (blocking)
   *
   * Blocks the calling thread until the flow graph execution is complete.
   *
   * @param ctx The scheduler context (unused in this implementation)
   *
   * @warning Do not use this method when the main thread is part of a single workgroup
   *          as it can cause deadlocks. Use cooperative_wait() instead.
   */
  void wait(context_type const& ctx);

private:
  /// @brief Status returned by task execution indicating whether all tasks in a node are complete
  enum class status : uint8_t
  {
    all_done,  ///< All tasks in the node have completed
    more_tasks ///< More tasks in the node are still running
  };

  /**
   * @brief Internal task node representation
   *
   * Represents a single node in the flow graph containing tasks and dependency information.
   * Each node can contain multiple tasks that execute in parallel once dependencies are satisfied.
   */
  class task_node
  {
  public:
    task_node()  = default;
    ~task_node() = default;

    /// Custom move constructor to handle atomic members properly
    task_node(task_node&& other) noexcept
        : workgroup_(other.workgroup_), tasks_(std::move(other.tasks_)), next_nodes_(std::move(other.next_nodes_)),
          pending_dependencies_(other.pending_dependencies_.load(std::memory_order_relaxed))
    {}

    /// Custom move assignment operator for atomic member handling
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

    // Delete copy operations - nodes should not be copied
    task_node(const task_node&)                    = delete;
    auto operator=(const task_node&) -> task_node& = delete;

    /// Add a task delegate to this node
    void add(delegate_type&& task) noexcept
    {
      tasks_.emplace_back(std::move(task));
    }

    /// Set the workgroup for task execution (currently unused but reserved for future multi-workgroup support)
    void set_workgroup(workgroup_id group) noexcept
    {
      workgroup_ = group;
    }

    /// Get the assigned workgroup ID
    [[nodiscard]] auto get_workgroup() const noexcept -> workgroup_id
    {
      return workgroup_;
    }

    /// Add a successor node ID that depends on this node
    void add_successor(uint32_t node_id) noexcept
    {
      next_nodes_.emplace_back(node_id);
    }

    /// Get all successor node IDs
    [[nodiscard]] auto get_successors() const noexcept -> std::span<const uint32_t>
    {
      return {next_nodes_.data(), next_nodes_.size()};
    }

    /// Get all tasks in this node
    [[nodiscard]] auto get_tasks() const noexcept -> std::span<const delegate_type>
    {
      return {tasks_.data(), tasks_.size()};
    }

    /// Reset dependency count for graph reuse
    void reset_dependencies(uint32_t count) noexcept
    {
      pending_dependencies_.store(count, std::memory_order_relaxed);
    }

    /// Reset task execution count for graph reuse
    void reset_run_count() noexcept
    {
      run_count_.store(0, std::memory_order_relaxed);
    }

    /// Atomically decrement dependency count and return new value
    auto decrement_dependencies() noexcept -> uint32_t
    {
      return pending_dependencies_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    }

    /// Check if this node still has pending dependencies
    [[nodiscard]] auto has_pending_dependencies() const noexcept -> bool
    {
      return pending_dependencies_.load(std::memory_order_acquire) > 0;
    }

    /// Execute a specific task by index and return completion status
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
    workgroup_id                               workgroup_{default_workgroup_id}; ///< Workgroup for task execution
    std::vector<delegate_type>                 tasks_;                           ///< Tasks to execute in this node
    ouly::small_vector<uint32_t, AvgNodeCount> next_nodes_;                      ///< Successor node IDs
    std::atomic<uint32_t>                      pending_dependencies_{0};         ///< Number of unfinished dependencies
    std::atomic_uint32_t                       run_count_{0};                    ///< Completed task count in this node
  };

  // Graph state
  ouly::small_vector<task_node, AvgNodeCount> nodes_;              ///< All nodes in the graph
  ouly::small_vector<uint32_t, AvgNodeCount>  dependency_counts_;  ///< Initial dependency count per node
  uint32_t                                    total_tasks_{0};     ///< Total number of tasks across all nodes
  std::atomic<uint32_t>                       remaining_tasks_{0}; ///< Remaining unfinished tasks
  std::atomic_bool                            started_{false};     ///< Whether graph execution has started
  std::binary_semaphore                       done_{0};            ///< Signaled when all tasks complete

  /// Execute all tasks in a specific node
  void execute_node(uint32_t node_id, context_type const& ctx);

  /// Notify successor nodes when a node completes
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
