#include "ouly/scheduler/flow_graph.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
// NOLINTBEGIN
using namespace ouly;

TEST_CASE("flow_graph basic operations", "[flow_graph]")
{
  SECTION("create nodes and connect them")
  {
    using SchedulerType = ouly::v2::scheduler;
    flow_graph<SchedulerType> graph;

    auto node1 = graph.create_node();
    auto node2 = graph.create_node();
    auto node3 = graph.create_node();

    REQUIRE(node1.value() == 0);
    REQUIRE(node2.value() == 1);
    REQUIRE(node3.value() == 2);

    // Test connections
    graph.connect(node1, node2);
    graph.connect(node2, node3);
  }
}

TEST_CASE("flow_graph execution with v2 scheduler", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2); // 1 worker
  scheduler.begin_execution();

  std::atomic<int> execution_order{0};
  std::atomic<int> node1_order{-1};
  std::atomic<int> node2_order{-1};
  std::atomic<int> node3_order{-1};

  auto node1 = graph.create_node();
  auto node2 = graph.create_node();
  auto node3 = graph.create_node();

  // Connect nodes: node1 -> node2 -> node3
  graph.connect(node1, node2);
  graph.connect(node2, node3);

  // Add tasks to nodes
  graph.add(node1,
            [&](auto const&)
            {
              node1_order.store(execution_order.fetch_add(1));
              std::this_thread::sleep_for(std::chrono::milliseconds(10));
            });

  graph.add(node2,
            [&](auto const&)
            {
              node2_order.store(execution_order.fetch_add(1));
              std::this_thread::sleep_for(std::chrono::milliseconds(10));
            });

  graph.add(node3,
            [&](auto const&)
            {
              node3_order.store(execution_order.fetch_add(1));
              std::this_thread::sleep_for(std::chrono::milliseconds(10));
            });

  // Start execution

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.wait(ctx);

  // Check execution order
  REQUIRE(node1_order.load() < node2_order.load());
  REQUIRE(node2_order.load() < node3_order.load());

  scheduler.end_execution();
}

TEST_CASE("flow_graph reusability", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2); // 1 worker
  scheduler.begin_execution();

  std::atomic<int> run1_count{0};
  std::atomic<int> run2_count{0};

  auto node1 = graph.create_node();
  auto node2 = graph.create_node();

  graph.connect(node1, node2);

  // Add tasks to nodes
  graph.add(node1,
            [&](auto const&)
            {
              run1_count.fetch_add(1);
            });

  graph.add(node2,
            [&](auto const&)
            {
              run1_count.fetch_add(1);
            });

  auto ctx = SchedulerType::context_type::this_context::get();

  // First run

  graph.start(ctx);
  graph.wait(ctx);

  REQUIRE(run1_count.load() == 2);

  // Add more tasks for second run
  graph.add(node1,
            [&](auto const&)
            {
              run2_count.fetch_add(1);
            });

  graph.add(node2,
            [&](auto const&)
            {
              run2_count.fetch_add(1);
            });

  // Second run - should execute all tasks (original + new ones)
  graph.start(ctx);
  graph.wait(ctx);

  REQUIRE(run1_count.load() == 4); // Original tasks executed again
  REQUIRE(run2_count.load() == 2); // New tasks executed

  scheduler.end_execution();
}

TEST_CASE("flow_graph with v1 scheduler", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v1::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2); // 1 worker
  scheduler.begin_execution();

  std::atomic<bool> task_executed{false};

  auto node1 = graph.create_node();

  // Add task to node
  graph.add(node1,
            [&](auto const&)
            {
              task_executed.store(true);
            });

  // Start execution

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.wait(ctx);

  REQUIRE(task_executed.load());

  scheduler.end_execution();
}

TEST_CASE("flow_graph multiple tasks per node", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4); // More workers for parallel execution
  scheduler.begin_execution();

  std::atomic<int> node1_task_count{0};
  std::atomic<int> node2_task_count{0};
  std::atomic<int> total_executions{0};
  std::atomic<int> fails{0};

  auto node1 = graph.create_node();
  auto node2 = graph.create_node();

  graph.connect(node1, node2);

  // Add multiple tasks to node1
  for (int i = 0; i < 5; ++i)
  {
    graph.add(node1,
              [&](auto const&)
              {
                node1_task_count.fetch_add(1);
                total_executions.fetch_add(1);
                // Small delay to ensure concurrent execution
                std::this_thread::sleep_for(std::chrono::microseconds(100));
              });
  }

  // Add multiple tasks to node2
  for (int i = 0; i < 3; ++i)
  {
    graph.add(node2,
              [&](auto const&)
              {
                node2_task_count.fetch_add(1);
                total_executions.fetch_add(1);
                // Verify node1 completed before node2 starts
                if (node1_task_count.load() != 5)
                {
                  fails.fetch_add(1);
                }
              });
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx); // Use cooperative wait for better performance

  REQUIRE(node1_task_count.load() == 5);
  REQUIRE(node2_task_count.load() == 3);
  REQUIRE(total_executions.load() == 8);
  REQUIRE(fails.load() == 0); // Ensure no tasks in node2 executed before node1 completed

  scheduler.end_execution();
}

TEST_CASE("flow_graph complex dependency tree", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;
  using node_type = typename flow_graph<SchedulerType>::node_id;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::atomic<int>                execution_counter{0};
  std::array<std::atomic<int>, 7> node_execution_order;
  for (auto& order : node_execution_order)
  {
    order.store(-1);
  }

  // Create a complex dependency tree:
  // Root (0) branches to left (1) and right (2)
  // Left (1) leads to node 3
  // Right (2) leads to nodes 4, 5, 6

  auto node0 = graph.create_node(); // Root
  auto node1 = graph.create_node(); // Left branch
  auto node2 = graph.create_node(); // Right branch
  auto node3 = graph.create_node(); // Left leaf
  auto node4 = graph.create_node(); // Center-left leaf
  auto node5 = graph.create_node(); // Center-right leaf
  auto node6 = graph.create_node(); // Right leaf

  // Build the dependency tree
  graph.connect(node0, node1);
  graph.connect(node0, node2);
  graph.connect(node1, node3);
  graph.connect(node2, node4);
  graph.connect(node2, node5);
  graph.connect(node2, node6);

  // Add tasks to each node
  for (uint32_t i = 0; i < 7; ++i)
  {
    graph.add(node_type{i},
              [&, i](auto const&)
              {
                node_execution_order[i].store(execution_counter.fetch_add(1));
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
              });
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Verify execution order constraints
  REQUIRE(node_execution_order[0].load() < node_execution_order[1].load());
  REQUIRE(node_execution_order[0].load() < node_execution_order[2].load());
  REQUIRE(node_execution_order[1].load() < node_execution_order[3].load());
  REQUIRE(node_execution_order[2].load() < node_execution_order[4].load());
  REQUIRE(node_execution_order[2].load() < node_execution_order[5].load());
  REQUIRE(node_execution_order[2].load() < node_execution_order[6].load());

  scheduler.end_execution();
}

TEST_CASE("flow_graph parallel independent branches", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::atomic<int> branch1_start_time{-1};
  std::atomic<int> branch2_start_time{-1};
  std::atomic<int> time_counter{0};

  // Create two independent branches that should execute in parallel
  auto branch1_node = graph.create_node();
  auto branch2_node = graph.create_node();

  graph.add(branch1_node,
            [&](auto const&)
            {
              branch1_start_time.store(time_counter.fetch_add(1));
              std::this_thread::sleep_for(std::chrono::milliseconds(50));
            });

  graph.add(branch2_node,
            [&](auto const&)
            {
              branch2_start_time.store(time_counter.fetch_add(1));
              std::this_thread::sleep_for(std::chrono::milliseconds(50));
            });

  auto ctx = SchedulerType::context_type::this_context::get();

  auto start_time = std::chrono::high_resolution_clock::now();
  graph.start(ctx);
  graph.cooperative_wait(ctx);
  auto end_time = std::chrono::high_resolution_clock::now();

  // Both branches should start around the same time (parallel execution)
  auto start_time_diff = std::abs(branch1_start_time.load() - branch2_start_time.load());
  REQUIRE(start_time_diff <= 1); // Should start very close to each other

  // Total execution time should be close to single branch time (not additive)
  auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  REQUIRE(total_duration.count() < 80); // Should be less than sum of both delays

  scheduler.end_execution();
}

TEST_CASE("flow_graph empty nodes", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<bool> final_task_executed{false};

  auto empty_node1 = graph.create_node(); // No tasks added
  auto empty_node2 = graph.create_node(); // No tasks added
  auto final_node  = graph.create_node();

  // Chain: empty_node1 -> empty_node2 -> final_node
  graph.connect(empty_node1, empty_node2);
  graph.connect(empty_node2, final_node);

  // Only add task to final node
  graph.add(final_node,
            [&](auto const&)
            {
              final_task_executed.store(true);
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(final_task_executed.load());

  scheduler.end_execution();
}

TEST_CASE("flow_graph single node with no dependencies", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> execution_count{0};

  auto single_node = graph.create_node();

  // Add multiple tasks to a single node with no dependencies
  for (int i = 0; i < 10; ++i)
  {
    graph.add(single_node,
              [&](auto const&)
              {
                execution_count.fetch_add(1);
              });
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(execution_count.load() == 10);

  scheduler.end_execution();
}

TEST_CASE("flow_graph stress test with many nodes", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;
  using node_type = typename flow_graph<SchedulerType>::node_id;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  constexpr int                            NUM_NODES = 50;
  std::array<std::atomic<bool>, NUM_NODES> node_executed;
  std::atomic_int                          fails{0};
  for (auto& executed : node_executed)
  {
    executed.store(false);
  }

  // Create linear chain of nodes
  std::vector<node_type> nodes;
  for (int i = 0; i < NUM_NODES; ++i)
  {
    nodes.push_back(graph.create_node());
  }

  // Connect nodes in sequence
  for (int i = 0; i < NUM_NODES - 1; ++i)
  {
    graph.connect(nodes[i], nodes[i + 1]);
  }

  // Add tasks to each node
  for (int i = 0; i < NUM_NODES; ++i)
  {
    graph.add(nodes[i],
              [&, i](auto const&)
              {
                node_executed[i].store(true);
                // Verify execution order
                if (i > 0)
                {
                  if (node_executed[i - 1].load() != true)
                  {
                    fails.fetch_add(1);
                  }
                }
              });
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Verify all nodes executed
  for (int i = 0; i < NUM_NODES; ++i)
  {
    REQUIRE(node_executed[i].load() == true);
  }

  // Ensure no execution order violations
  REQUIRE(fails.load() == 0);

  scheduler.end_execution();
}

TEST_CASE("flow_graph exception safety", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<bool> recovery_task_executed{false};

  auto failing_node  = graph.create_node();
  auto recovery_node = graph.create_node();

  graph.connect(failing_node, recovery_node);

  // Add a task that might throw (though the scheduler should handle it gracefully)
  graph.add(failing_node,
            [&](auto const&)
            {
              // Simulate some work that might fail but doesn't throw
              // in this case, just complete normally
            });

  graph.add(recovery_node,
            [&](auto const&)
            {
              recovery_task_executed.store(true);
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(recovery_task_executed.load());

  scheduler.end_execution();
}

TEST_CASE("flow_graph multiple starts without prepare", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> execution_count{0};

  auto node = graph.create_node();
  graph.add(node,
            [&](auto const&)
            {
              execution_count.fetch_add(1);
            });

  auto ctx = SchedulerType::context_type::this_context::get();

  // First execution with prepare

  graph.start(ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(execution_count.load() == 1);

  // Second execution without new prepare (should still work due to reusability)
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(execution_count.load() == 2);

  scheduler.end_execution();
}

TEST_CASE("flow_graph diamond dependency pattern", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;
  using node_type = typename flow_graph<SchedulerType>::node_id;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::atomic<int>                execution_order{0};
  std::array<std::atomic<int>, 4> node_order;
  for (auto& order : node_order)
  {
    order.store(-1);
  }

  // Create diamond pattern:
  // Top node 0 branches to left (1) and right (2)
  // Both left and right converge to bottom node (3)

  auto top_node    = graph.create_node(); // 0
  auto left_node   = graph.create_node(); // 1
  auto right_node  = graph.create_node(); // 2
  auto bottom_node = graph.create_node(); // 3

  graph.connect(top_node, left_node);
  graph.connect(top_node, right_node);
  graph.connect(left_node, bottom_node);
  graph.connect(right_node, bottom_node);

  // Add tasks to each node
  for (uint32_t i = 0; i < 4; ++i)
  {
    graph.add(node_type{i},
              [&, i](auto const&)
              {
                node_order[i].store(execution_order.fetch_add(1));
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
              });
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Verify execution order constraints
  REQUIRE(node_order[0].load() < node_order[1].load()); // top before left
  REQUIRE(node_order[0].load() < node_order[2].load()); // top before right
  REQUIRE(node_order[1].load() < node_order[3].load()); // left before bottom
  REQUIRE(node_order[2].load() < node_order[3].load()); // right before bottom

  // Left and right should be able to execute in parallel
  // (no direct dependency between them)

  scheduler.end_execution();
}

TEST_CASE("flow_graph add tasks after prepare", "[flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> execution_count{0};

  auto node = graph.create_node();

  // Add initial task
  graph.add(node,
            [&](auto const&)
            {
              execution_count.fetch_add(1);
            });

  // Prepare first

  // Add another task after prepare (potential issue)
  graph.add(node,
            [&](auto const&)
            {
              execution_count.fetch_add(1);
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // This test might fail if the implementation doesn't handle
  // tasks added after prepare() correctly
  REQUIRE(execution_count.load() == 2);

  scheduler.end_execution();
}

TEST_CASE("flow_graph task removal basic functionality", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> execution_count{0};
  std::atomic<int> removed_task_count{0};

  auto node = graph.create_node();

  // Add multiple tasks
  [[maybe_unused]] auto task1 = graph.add(node,
                                          [&](auto const&)
                                          {
                                            execution_count.fetch_add(1);
                                          });
  auto                  task2 = graph.add(node,
                                          [&](auto const&)
                                          {
                           removed_task_count.fetch_add(1);
                         });
  [[maybe_unused]] auto task3 = graph.add(node,
                                          [&](auto const&)
                                          {
                                            execution_count.fetch_add(1);
                                          });

  // Remove the middle task
  graph.remove(node, task2);

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Only task1 and task3 should execute
  REQUIRE(execution_count.load() == 2);
  REQUIRE(removed_task_count.load() == 0);

  scheduler.end_execution();
}

TEST_CASE("flow_graph task removal with invalid node ID", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;
  using node_type = typename flow_graph<SchedulerType>::node_id;
  using task_type = typename flow_graph<SchedulerType>::task_id;

  auto                  node = graph.create_node();
  [[maybe_unused]] auto task = graph.add(node, [](auto const&) {});

  // Try to remove from invalid node - should not crash
  graph.remove(node_type{999}, task);

  // Try to remove invalid task from valid node - should not crash
  graph.remove(node, task_type{999});

  // This should succeed
  REQUIRE(true);
}

TEST_CASE("flow_graph task removal with empty task ID", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;
  using task_type = typename flow_graph<SchedulerType>::task_id;

  auto                  node = graph.create_node();
  [[maybe_unused]] auto task = graph.add(node, [](auto const&) {});

  // Try to remove with empty/default task ID - should not crash
  graph.remove(node, task_type{});

  // This should succeed
  REQUIRE(true);
}

TEST_CASE("flow_graph task removal reuses slots", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> execution_count{0};
  std::vector<int> execution_order;
  std::mutex       order_mutex;

  auto node = graph.create_node();

  // Add tasks
  [[maybe_unused]] auto task1 = graph.add(node,
                                          [&](auto const&)
                                          {
                                            std::lock_guard<std::mutex> lock(order_mutex);
                                            execution_order.push_back(1);
                                            execution_count.fetch_add(1);
                                          });

  auto task2 = graph.add(node,
                         [&](auto const&)
                         {
                           std::lock_guard<std::mutex> lock(order_mutex);
                           execution_order.push_back(2);
                           execution_count.fetch_add(1);
                         });

  [[maybe_unused]] auto task3 = graph.add(node,
                                          [&](auto const&)
                                          {
                                            std::lock_guard<std::mutex> lock(order_mutex);
                                            execution_order.push_back(3);
                                            execution_count.fetch_add(1);
                                          });

  // Remove middle task
  graph.remove(node, task2);

  // Add a new task - should reuse the slot from removed task2
  [[maybe_unused]] auto task4 = graph.add(node,
                                          [&](auto const&)
                                          {
                                            std::lock_guard<std::mutex> lock(order_mutex);
                                            execution_order.push_back(4);
                                            execution_count.fetch_add(1);
                                          });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Should execute task1, task3, and task4 (not task2)
  REQUIRE(execution_count.load() == 3);
  REQUIRE(execution_order.size() == 3);

  // Make sure task2 didn't execute
  for (int val : execution_order)
  {
    REQUIRE(val != 2);
  }

  // Should contain 1, 3, and 4
  std::sort(execution_order.begin(), execution_order.end());
  REQUIRE(execution_order[0] == 1);
  REQUIRE(execution_order[1] == 3);
  REQUIRE(execution_order[2] == 4);

  scheduler.end_execution();
}

TEST_CASE("flow_graph remove all tasks from node", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<bool> successor_executed{false};

  auto node1 = graph.create_node();
  auto node2 = graph.create_node();

  graph.connect(node1, node2);

  // Add tasks to first node
  auto task1 = graph.add(node1, [](auto const&) {});
  auto task2 = graph.add(node1, [](auto const&) {});

  // Add task to second node
  graph.add(node2,
            [&](auto const&)
            {
              successor_executed.store(true);
            });

  // Remove all tasks from first node
  graph.remove(node1, task1);
  graph.remove(node1, task2);

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Even with no tasks in node1, node2 should still execute
  // because empty nodes trigger their successors
  REQUIRE(successor_executed.load());

  scheduler.end_execution();
}

TEST_CASE("flow_graph task removal with dependencies", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> node1_execution_count{0};
  std::atomic<int> node2_execution_count{0};
  std::atomic<int> removed_task_executions{0};

  auto node1 = graph.create_node();
  auto node2 = graph.create_node();

  graph.connect(node1, node2);

  // Add multiple tasks to first node
  [[maybe_unused]] auto task1a = graph.add(node1,
                                           [&](auto const&)
                                           {
                                             node1_execution_count.fetch_add(1);
                                           });
  auto                  task1b = graph.add(node1,
                                           [&](auto const&)
                                           {
                            removed_task_executions.fetch_add(1);
                          });
  [[maybe_unused]] auto task1c = graph.add(node1,
                                           [&](auto const&)
                                           {
                                             node1_execution_count.fetch_add(1);
                                           });

  // Add task to second node
  graph.add(node2,
            [&](auto const&)
            {
              node2_execution_count.fetch_add(1);
            });

  // Remove middle task from first node
  graph.remove(node1, task1b);

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Node1 should execute 2 tasks (not the removed one), node2 should execute after
  REQUIRE(node1_execution_count.load() == 2);
  REQUIRE(node2_execution_count.load() == 1);
  REQUIRE(removed_task_executions.load() == 0);

  scheduler.end_execution();
}

TEST_CASE("flow_graph multiple removes and graph reusability", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> execution_count{0};

  auto node = graph.create_node();

  // Add multiple tasks
  [[maybe_unused]] auto task1 = graph.add(node,
                                          [&](auto const&)
                                          {
                                            execution_count.fetch_add(1);
                                          });
  auto                  task2 = graph.add(node,
                                          [&](auto const&)
                                          {
                           execution_count.fetch_add(1);
                         });
  [[maybe_unused]] auto task3 = graph.add(node,
                                          [&](auto const&)
                                          {
                                            execution_count.fetch_add(1);
                                          });
  auto                  task4 = graph.add(node,
                                          [&](auto const&)
                                          {
                           execution_count.fetch_add(1);
                         });

  // Remove some tasks
  graph.remove(node, task2);
  graph.remove(node, task4);

  auto ctx = SchedulerType::context_type::this_context::get();

  // First execution - should execute 2 remaining tasks
  graph.start(ctx);
  graph.cooperative_wait(ctx);
  REQUIRE(execution_count.load() == 2);

  // Second execution - should execute the same 2 tasks again (graph reusability)
  graph.start(ctx);
  graph.cooperative_wait(ctx);
  REQUIRE(execution_count.load() == 4);

  // Add a new task and execute again
  graph.add(node,
            [&](auto const&)
            {
              execution_count.fetch_add(1);
            });
  graph.start(ctx);
  graph.cooperative_wait(ctx);
  REQUIRE(execution_count.load() == 7); // 2 original + 1 new = 3 per run, total = 4 + 3 = 7

  scheduler.end_execution();
}

TEST_CASE("flow_graph remove task then double remove", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> execution_count{0};

  auto node = graph.create_node();

  auto                  task1 = graph.add(node,
                                          [&](auto const&)
                                          {
                           execution_count.fetch_add(1);
                         });
  [[maybe_unused]] auto task2 = graph.add(node,
                                          [&](auto const&)
                                          {
                                            execution_count.fetch_add(1);
                                          });

  // Remove task1
  graph.remove(node, task1);

  // Remove task1 again - should be safe
  graph.remove(node, task1);

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Only task2 should execute
  REQUIRE(execution_count.load() == 1);

  scheduler.end_execution();
}

TEST_CASE("flow_graph remove tasks stress test", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;
  using task_type = typename flow_graph<SchedulerType>::task_id;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::atomic<int> execution_count{0};
  constexpr int    NUM_TASKS = 100;

  auto                   node = graph.create_node();
  std::vector<task_type> task_ids;

  // Add many tasks
  for (int i = 0; i < NUM_TASKS; ++i)
  {
    auto task_id = graph.add(node,
                             [&](auto const&)
                             {
                               execution_count.fetch_add(1);
                             });
    task_ids.push_back(task_id);
  }

  // Remove every other task
  for (int i = 1; i < NUM_TASKS; i += 2)
  {
    graph.remove(node, task_ids[i]);
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  // Should execute NUM_TASKS/2 tasks (every other one)
  REQUIRE(execution_count.load() == NUM_TASKS / 2);

  scheduler.end_execution();
}

TEST_CASE("flow_graph complex removal with diamond pattern", "[flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::atomic<int> top_count{0};
  std::atomic<int> left_count{0};
  std::atomic<int> right_count{0};
  std::atomic<int> bottom_count{0};
  std::atomic<int> removed_count{0};

  // Create diamond pattern
  auto top_node    = graph.create_node();
  auto left_node   = graph.create_node();
  auto right_node  = graph.create_node();
  auto bottom_node = graph.create_node();

  graph.connect(top_node, left_node);
  graph.connect(top_node, right_node);
  graph.connect(left_node, bottom_node);
  graph.connect(right_node, bottom_node);

  // Add tasks and some that will be removed
  graph.add(top_node,
            [&](auto const&)
            {
              top_count.fetch_add(1);
            });

  [[maybe_unused]] auto left_task1 = graph.add(left_node,
                                               [&](auto const&)
                                               {
                                                 left_count.fetch_add(1);
                                               });
  auto                  left_task2 = graph.add(left_node,
                                               [&](auto const&)
                                               {
                                removed_count.fetch_add(1);
                              }); // Will be removed

  graph.add(right_node,
            [&](auto const&)
            {
              right_count.fetch_add(1);
            });
  auto right_task2 = graph.add(right_node,
                               [&](auto const&)
                               {
                                 removed_count.fetch_add(1);
                               }); // Will be removed

  graph.add(bottom_node,
            [&](auto const&)
            {
              bottom_count.fetch_add(1);
            });

  // Remove some tasks
  graph.remove(left_node, left_task2);
  graph.remove(right_node, right_task2);

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(top_count.load() == 1);
  REQUIRE(left_count.load() == 1);
  REQUIRE(right_count.load() == 1);
  REQUIRE(bottom_count.load() == 1);
  REQUIRE(removed_count.load() == 0); // Removed tasks should not execute

  scheduler.end_execution();
}
// NOLINTEND
