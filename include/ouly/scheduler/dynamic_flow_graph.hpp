// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/containers/small_vector.hpp"
#include "ouly/scheduler/spin_lock.hpp"
#include "ouly/scheduler/worker_structs.hpp"
#include "ouly/utility/tagged_int.hpp"
#include "ouly/utility/user_config.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>

namespace ouly
{

/**
 * @file dynamic_flow_graph.hpp
 * @brief Mutable, looping Task Flow Graph for long-running parallel execution.
 *
 * While ouly::flow_graph models a static Directed Acyclic Graph that is built once and then
 * executed (and optionally re-executed) as a single shot, dynamic_flow_graph models a *living*
 * task graph that is started once and is expected to keep running while it is mutated.
 *
 * It is designed for the "persistent game loop" pattern: a top level loop task is pushed and is
 * sub-divided into many sub-tasks (initialization, simulation, render, present, ...). Those
 * sub-tasks can form cycles (a frame loops back onto itself), can be added or removed while the
 * graph is live, and can simultaneously be dependencies and dependents of one another.
 *
 * Compared to flow_graph the important differences are:
 *
 *  - **Mutable while running**: create_node(), add(), remove() and connect() are thread-safe and
 *    may be called at any time, including from inside running tasks. Structural changes take effect
 *    on the *next* firing of the affected node.
 *  - **Cycles / loops are first class**: a node fires every time it accumulates `in_degree` triggers
 *    (a marked-graph / dataflow firing rule) rather than exactly once. Feeding an edge back to an
 *    ancestor produces a repeating loop.
 *  - **Run once, never restarted**: there is no reset/reuse machinery. The graph is kicked with
 *    signal(), runs (potentially forever) and is drained to completion with request_stop().
 *
 * ## Firing model
 *
 * Every node has a *threshold* equal to `max(in_degree, 1)` — the number of triggers it must
 * receive before it fires. When a node finishes executing all of its tasks it delivers one trigger
 * to each of its successors. `signal()` injects an external trigger and is how loops are seeded and
 * how root nodes (in_degree == 0) are started.
 *
 * Because the firing rule is "fire once per `threshold` triggers", a node placed inside a cycle
 * fires once per loop iteration. Wire your cycle so that, in steady state, each node receives
 * exactly `threshold` triggers per iteration.
 *
 * ## Completion / quiescence
 *
 * The graph keeps an atomic count of in-flight work (pending fires and running tasks). The graph is
 * "idle" when that count reaches zero. cooperative_wait()/wait() return once the graph becomes idle.
 * A perpetual loop never becomes idle on its own; call request_stop() (typically from within a task)
 * to stop trigger propagation and let the currently running work drain, after which wait() returns.
 *
 * ## Usage example (game loop)
 *
 * ```cpp
 * ouly::dynamic_flow_graph<ouly::v2::scheduler> graph;
 *
 * auto update  = graph.create_node();
 * auto render  = graph.create_node();
 * auto present = graph.create_main_thread_node(); // runs on the waiting thread
 *
 * // Steady cycle: update -> render -> present -> update
 * graph.connect(update, render);
 * graph.connect(render, present);
 * graph.connect(present, update);
 *
 * graph.add(update,  [&](auto const&) { simulate(); });
 * graph.add(render,  [&](auto const&) { record_render_commands(); });
 * graph.add(present, [&](auto const&) { if (++frame == max_frames) graph.request_stop(); });
 *
 * auto ctx = ouly::v2::scheduler::context_type::this_context::get();
 * graph.signal(update, ctx); // kick the loop
 * graph.cooperative_wait(ctx);
 * ```
 *
 * @tparam SchedulerType  The scheduler type (v1::scheduler or v2::scheduler) that executes tasks.
 * @tparam NodeChunkSize  Number of nodes per stable storage chunk (default 64).
 * @tparam EdgeChunkSize  Number of edges per stable storage chunk (default 256).
 */
constexpr uint32_t default_chunk_size      = 32;
constexpr uint32_t default_edge_chunk_size = 256;
template <typename SchedulerType, uint32_t NodeChunkSize = default_chunk_size,
          uint32_t EdgeChunkSize = default_edge_chunk_size>
class dynamic_flow_graph
{
public:
  struct node_tag
  {};
  struct task_tag
  {};
  using node_id       = ouly::tagged_int<node_tag, uint32_t, std::numeric_limits<uint32_t>::max()>;
  using task_id       = ouly::tagged_int<task_tag, uint32_t, std::numeric_limits<uint32_t>::max()>;
  using delegate_type = typename SchedulerType::delegate_type; ///< Task delegate type from scheduler
  using context_type  = typename SchedulerType::context_type;  ///< Scheduler context type

  static constexpr uint32_t nil = std::numeric_limits<uint32_t>::max();

  dynamic_flow_graph() noexcept                                    = default;
  dynamic_flow_graph(const dynamic_flow_graph&)                    = delete;
  dynamic_flow_graph(dynamic_flow_graph&&)                         = delete;
  auto operator=(const dynamic_flow_graph&) -> dynamic_flow_graph& = delete;
  auto operator=(dynamic_flow_graph&&) -> dynamic_flow_graph&      = delete;
  ~dynamic_flow_graph() noexcept                                   = default;

  /**
   * @brief Create a new node that executes its tasks on the scheduler.
   *
   * @param workgroup The workgroup whose workers run this node's tasks.
   * @return The new node's identifier.
   *
   * @note Thread-safe; may be called while the graph is running.
   */
  auto create_node(workgroup_id workgroup = default_workgroup_id) -> node_id
  {
    uint32_t idx    = nodes_.allocate();
    auto&    node   = nodes_[idx];
    node.workgroup_ = workgroup;
    return node_id{idx};
  }

  /**
   * @brief Create a node whose tasks run inline on the thread driving wait()/cooperative_wait().
   *
   * Useful for work that must happen on a specific (e.g. main/render) thread such as a present/swap.
   * The node's tasks run sequentially on the waiting thread once its dependencies are satisfied.
   *
   * @note A main-thread node only makes progress while a thread is inside wait()/cooperative_wait().
   */
  auto create_main_thread_node() -> node_id
  {
    uint32_t idx      = nodes_.allocate();
    auto&    node     = nodes_[idx];
    node.main_thread_ = true;
    return node_id{idx};
  }

  /**
   * @brief Add a task to a node.
   *
   * Multiple tasks added to the same node run in parallel (on worker nodes) once the node fires.
   * Tasks added while the graph is running take effect on the node's next firing.
   *
   * @return The task's identifier (used by remove()).
   * @note Thread-safe.
   */
  template <typename Func>
  auto add(node_id id, Func&& exec_delegate) -> task_id
  {
    OULY_ASSERT(id.value() < nodes_.size());
    auto&                      node = nodes_[id.value()];
    std::lock_guard<spin_lock> lk(node.task_lock_);
    return node.add(delegate_type::bind(std::forward<Func>(exec_delegate)));
  }

  /**
   * @brief Remove a previously added task from a node.
   *
   * Removing takes effect on the node's next firing. Removing an already removed or invalid task is
   * a no-op. The slot is reused by a subsequent add().
   *
   * @note Thread-safe.
   */
  void remove(node_id id, task_id task)
  {
    if (id.value() >= nodes_.size())
    {
      return;
    }
    auto&                      node = nodes_[id.value()];
    std::lock_guard<spin_lock> lk(node.task_lock_);
    node.remove(task);
  }

  /**
   * @brief Create a dependency edge: `to` receives a trigger every time `from` finishes.
   *
   * Increases `to`'s firing threshold by one. May be called while the graph is running, but avoid
   * changing a node's incoming edges while that same node is actively accumulating triggers.
   *
   * @note Thread-safe. Self loops and cycles are allowed.
   */
  void connect(node_id from, node_id to)
  {
    OULY_ASSERT(from.value() < nodes_.size() && to.value() < nodes_.size());
    nodes_[to.value()].in_degree_.fetch_add(1, std::memory_order_acq_rel);

    // Allocate a stable edge record and atomically prepend it to `from`'s successor list.
    uint32_t edge_idx  = edges_.allocate();
    auto&    edge      = edges_[edge_idx];
    edge.target_       = to.value();
    auto&    from_node = nodes_[from.value()];
    uint32_t head      = from_node.first_edge_.load(std::memory_order_relaxed);
    // NOLINTNEXTLINE
    do
    {
      edge.next_.store(head, std::memory_order_relaxed);
    }
    while (!from_node.first_edge_.compare_exchange_weak(head, edge_idx, std::memory_order_acq_rel,
                                                        std::memory_order_relaxed));
  }

  /**
   * @brief Inject an external trigger into a node.
   *
   * This is how the graph is started and how loops are seeded. A node with no predecessors fires on
   * the first signal; a node inside a cycle is kicked with a single signal to start the first
   * iteration.
   *
   * @param id  The node to trigger.
   * @param ctx The current scheduler context (used to submit work).
   *
   * @note Thread-safe; may be called from any worker or from the thread that will wait().
   */
  void signal(node_id id, context_type const& ctx)
  {
    OULY_ASSERT(id.value() < nodes_.size());
    deliver_trigger(id.value(), ctx);
  }

  /**
   * @brief Request that the graph stop propagating triggers.
   *
   * After this call no new node firings are scheduled; already running tasks finish and the graph
   * drains to idle, at which point wait()/cooperative_wait() returns. Typically called from inside a
   * task to terminate a loop.
   *
   * @note Thread-safe.
   */
  void request_stop() noexcept
  {
    stop_.store(true, std::memory_order_release);
  }

  /**
   * @brief Whether stop has been requested.
   */
  [[nodiscard]] auto stop_requested() const noexcept -> bool
  {
    return stop_.load(std::memory_order_acquire);
  }

  /**
   * @brief Whether the graph currently has no in-flight work.
   */
  [[nodiscard]] auto is_idle() const noexcept -> bool
  {
    return inflight_.load(std::memory_order_acquire) == 0;
  }

  /**
   * @brief Wait for the graph to become idle while cooperatively executing tasks.
   *
   * The calling thread participates as a worker (stealing and running scheduled tasks) and also
   * drives any main-thread nodes. Returns once all in-flight work has drained.
   *
   * @param ctx The scheduler context for cooperative execution.
   */
  void cooperative_wait(context_type const& ctx)
  {
    while (inflight_.load(std::memory_order_acquire) != 0)
    {
      drain_main_nodes(ctx);
      ctx.get_scheduler().busy_work(ctx);
    }
    drain_main_nodes(ctx);
  }

  /**
   * @brief Wait for the graph to become idle, driving work on the calling thread.
   *
   * Equivalent to cooperative_wait() using the calling thread's context. Use this when you started
   * the graph from a plain thread that holds a valid scheduler context.
   */
  void wait()
  {
    cooperative_wait(context_type::this_context::get());
  }

  /**
   * @brief Number of nodes created so far.
   */
  [[nodiscard]] auto node_count() const noexcept -> uint32_t
  {
    return nodes_.size();
  }

private:
  /**
   * @brief Stable, append-only storage with lock-free indexed reads.
   *
   * Elements never move once allocated, so node/edge references obtained on the hot path stay valid
   * even while other threads append new elements. Only chunk allocation takes a mutex; indexed reads
   * are lock-free.
   */
  template <typename T, uint32_t ChunkSize>
  class stable_pool
  {
  public:
    static constexpr uint32_t max_chunks = 4096;

    stable_pool() noexcept                             = default;
    stable_pool(const stable_pool&)                    = delete;
    stable_pool(stable_pool&&)                         = delete;
    auto operator=(const stable_pool&) -> stable_pool& = delete;
    auto operator=(stable_pool&&) -> stable_pool&      = delete;

    ~stable_pool() noexcept
    {
      for (auto& chunk : chunks_)
      {
        // NOLINTNEXTLINE
        delete[] chunk.load(std::memory_order_relaxed);
      }
    }

    /// Allocate the next element index, ensuring its backing chunk exists.
    auto allocate() -> uint32_t
    {
      uint32_t idx = size_.fetch_add(1, std::memory_order_acq_rel);
      ensure_chunk(idx / ChunkSize);
      return idx;
    }

    auto operator[](uint32_t idx) noexcept -> T&
    {
      return chunks_[idx / ChunkSize].load(std::memory_order_acquire)[idx % ChunkSize];
    }

    auto operator[](uint32_t idx) const noexcept -> const T&
    {
      return chunks_[idx / ChunkSize].load(std::memory_order_acquire)[idx % ChunkSize];
    }

    [[nodiscard]] auto size() const noexcept -> uint32_t
    {
      return size_.load(std::memory_order_acquire);
    }

  private:
    void ensure_chunk(uint32_t chunk_idx)
    {
      OULY_ASSERT(chunk_idx < max_chunks);
      if (chunks_[chunk_idx].load(std::memory_order_acquire) != nullptr)
      {
        return;
      }
      std::lock_guard<std::mutex> lk(grow_mutex_);
      if (chunks_[chunk_idx].load(std::memory_order_relaxed) == nullptr)
      {
        chunks_[chunk_idx].store(new T[ChunkSize], std::memory_order_release);
      }
    }

    std::array<std::atomic<T*>, max_chunks> chunks_{};
    std::atomic<uint32_t>                   size_{0};
    std::mutex                              grow_mutex_;
  };

  /// Directed edge record (successor list node), stored in a stable pool.
  struct edge
  {
    uint32_t              target_{nil};
    std::atomic<uint32_t> next_{nil};
  };

  /// A node: a set of tasks plus dependency / successor bookkeeping.
  struct task_node
  {
    task_node() noexcept = default;

    /// Add a task, reusing a freed slot if available.
    auto add(delegate_type&& task) -> task_id
    {
      valid_task_count_++;
      for (auto& t : tasks_)
      {
        if (!t)
        {
          t = std::move(task);
          return task_id{static_cast<uint32_t>(&t - tasks_.data())};
        }
      }
      tasks_.emplace_back(std::move(task));
      return task_id{static_cast<uint32_t>(tasks_.size() - 1)};
    }

    void remove(task_id id) noexcept
    {
      auto index = id.value();
      if (index < tasks_.size() && tasks_[index])
      {
        valid_task_count_--;
        tasks_[index] = delegate_type();
      }
    }

    // Structural (rarely written, read on hot path through stable storage):
    std::atomic<uint32_t> first_edge_{nil}; ///< Head of the successor edge list.
    std::atomic<uint32_t> in_degree_{0};    ///< Number of incoming edges.
    workgroup_id          workgroup_{default_workgroup_id};
    bool                  main_thread_{false};

    // Firing state:
    std::atomic<uint32_t> arrived_{0}; ///< Triggers accumulated toward the next firing.

    // Task storage (guarded by task_lock_):
    spin_lock                  task_lock_;
    std::vector<delegate_type> tasks_; ///< Source task list (slots reused).
    uint32_t                   valid_task_count_{0};
  };

  /**
   * @brief An immutable snapshot of a node's tasks for a single firing.
   *
   * A batch is filled once when a node fires and is never mutated again until that firing fully
   * completes, so the delegates it holds can be read concurrently by the firing's tasks without any
   * synchronization. Completed batches are returned to a freelist and reused, giving zero
   * steady-state allocation while remaining robust against task-list growth.
   */
  struct fire_batch
  {
    std::atomic<uint32_t>      remaining_{0};       ///< Tasks left to complete in this firing.
    uint32_t                   node_idx_{nil};      ///< Owning node.
    fire_batch*                pool_next_{nullptr}; ///< Freelist link.
    std::vector<delegate_type> tasks_;              ///< Snapshot of the node's valid tasks.
  };

  /// Obtain a batch from the freelist, or allocate a new one.
  auto acquire_batch() -> fire_batch*
  {
    std::lock_guard<spin_lock> lk(batch_lock_);
    if (batch_free_ != nullptr)
    {
      fire_batch* fb = batch_free_;
      batch_free_    = fb->pool_next_;
      fb->pool_next_ = nullptr;
      return fb;
    }
    batch_storage_.push_back(std::make_unique<fire_batch>());
    return batch_storage_.back().get();
  }

  /// Return a batch to the freelist for reuse.
  void release_batch(fire_batch* fb)
  {
    std::lock_guard<spin_lock> lk(batch_lock_);
    fb->pool_next_ = batch_free_;
    batch_free_    = fb;
  }

  /// Deliver one trigger to a node; fire it if its threshold is reached.
  void deliver_trigger(uint32_t idx, context_type const& ctx)
  {
    auto&    node      = nodes_[idx];
    uint32_t threshold = std::max(node.in_degree_.load(std::memory_order_acquire), 1U);
    if (node.arrived_.fetch_add(1, std::memory_order_acq_rel) + 1 == threshold)
    {
      // Consume one threshold's worth of triggers, carrying any surplus to the next round.
      node.arrived_.fetch_sub(threshold, std::memory_order_acq_rel);
      fire_node(idx, ctx);
    }
  }

  /// Begin a firing of a node: snapshot its tasks into a fresh batch and dispatch them.
  void fire_node(uint32_t idx, context_type const& ctx)
  {
    if (stop_.load(std::memory_order_acquire))
    {
      return;
    }

    auto& node = nodes_[idx];

    // Snapshot the current valid tasks into a private, immutable firing batch under the task lock.
    fire_batch* fb = acquire_batch();
    fb->node_idx_  = idx;
    fb->tasks_.clear();
    {
      std::lock_guard<spin_lock> lk(node.task_lock_);
      for (auto& t : node.tasks_)
      {
        if (t)
        {
          fb->tasks_.push_back(t);
        }
      }
    }
    auto valid = static_cast<uint32_t>(fb->tasks_.size());

    // Hold a "fire token" until tasks complete and successors are notified, so the graph does not
    // appear idle mid hand-off.
    inflight_.fetch_add(1, std::memory_order_acq_rel);

    if (node.main_thread_)
    {
      enqueue_main_node(fb);
      return;
    }

    if (valid == 0)
    {
      finish_fire(fb, ctx);
      return;
    }

    fb->remaining_.store(valid, std::memory_order_release);
    auto* graph = this;
    for (uint32_t i = 0; i < valid; ++i)
    {
      ctx.get_scheduler().submit(ctx, node.workgroup_,
                                 [graph, fb, i](context_type const& task_ctx) -> void
                                 {
                                   fb->tasks_[i](task_ctx);
                                   if (fb->remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1)
                                   {
                                     graph->finish_fire(fb, task_ctx);
                                   }
                                 });
    }
  }

  /// Run a main-thread node's batch inline (called from the waiting thread).
  void run_main_node(fire_batch* fb, context_type const& ctx)
  {
    for (auto& task : fb->tasks_)
    {
      task(ctx);
    }
    finish_fire(fb, ctx);
  }

  /// Complete a node's firing: notify successors, release the fire token, recycle the batch.
  void finish_fire(fire_batch* fb, context_type const& ctx)
  {
    notify_successors(fb->node_idx_, ctx);
    release_batch(fb);
    inflight_.fetch_sub(1, std::memory_order_acq_rel);
  }

  /// Deliver a trigger to every successor of a node.
  void notify_successors(uint32_t idx, context_type const& ctx)
  {
    if (stop_.load(std::memory_order_acquire))
    {
      return;
    }
    uint32_t e = nodes_[idx].first_edge_.load(std::memory_order_acquire);
    while (e != nil)
    {
      auto& edge = edges_[e];
      deliver_trigger(edge.target_, ctx);
      e = edge.next_.load(std::memory_order_acquire);
    }
  }

  /// Queue a ready main-thread firing for the waiting thread to run.
  void enqueue_main_node(fire_batch* fb)
  {
    std::lock_guard<spin_lock> lk(main_lock_);
    main_ready_.push_back(fb);
  }

  /// Run all currently ready main-thread firings on the calling thread.
  void drain_main_nodes(context_type const& ctx)
  {
    for (;;)
    {
      fire_batch* fb = nullptr;
      {
        std::lock_guard<spin_lock> lk(main_lock_);
        if (main_ready_.empty())
        {
          break;
        }
        fb = main_ready_.back();
        main_ready_.pop_back();
      }
      run_main_node(fb, ctx);
    }
  }

  stable_pool<task_node, NodeChunkSize> nodes_;
  stable_pool<edge, EdgeChunkSize>      edges_;

  std::atomic<uint32_t> inflight_{0}; ///< In-flight fires + running tasks; zero means idle.
  std::atomic_bool      stop_{false}; ///< When set, trigger propagation halts and the graph drains.

  // Batch pool (guarded by batch_lock_):
  spin_lock                                batch_lock_;
  fire_batch*                              batch_free_{nullptr}; ///< Freelist head of reusable batches.
  std::vector<std::unique_ptr<fire_batch>> batch_storage_;       ///< Owns all allocated batches.

  spin_lock                                      main_lock_;
  ouly::small_vector<fire_batch*, NodeChunkSize> main_ready_; ///< Ready main-thread firings.
};

} // namespace ouly
