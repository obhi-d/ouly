#include "ouly/scheduler/flow_graph.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

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

    REQUIRE(node1 == 0);
    REQUIRE(node2 == 1);
    REQUIRE(node3 == 2);

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
  scheduler.create_group(default_workgroup_id, 0, 1); // 1 worker
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
  graph.add(node1, SchedulerType::delegate_type::bind(
                    [&](auto const&)
                    {
                      node1_order.store(execution_order.fetch_add(1));
                      std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }));

  graph.add(node2, SchedulerType::delegate_type::bind(
                    [&](auto const&)
                    {
                      node2_order.store(execution_order.fetch_add(1));
                      std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }));

  graph.add(node3, SchedulerType::delegate_type::bind(
                    [&](auto const&)
                    {
                      node3_order.store(execution_order.fetch_add(1));
                      std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }));

  // Start execution
  graph.prepare();
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
  scheduler.create_group(default_workgroup_id, 0, 1); // 1 worker
  scheduler.begin_execution();

  std::atomic<int> run1_count{0};
  std::atomic<int> run2_count{0};

  auto node1 = graph.create_node();
  auto node2 = graph.create_node();

  graph.connect(node1, node2);

  // Add tasks to nodes
  graph.add(node1, SchedulerType::delegate_type::bind(
                    [&](auto const&)
                    {
                      run1_count.fetch_add(1);
                    }));

  graph.add(node2, SchedulerType::delegate_type::bind(
                    [&](auto const&)
                    {
                      run1_count.fetch_add(1);
                    }));

  auto ctx = SchedulerType::context_type::this_context::get();

  // First run
  graph.prepare();
  graph.start(ctx);
  graph.wait(ctx);

  REQUIRE(run1_count.load() == 2);

  // Add more tasks for second run
  graph.add(node1, SchedulerType::delegate_type::bind(
                    [&](auto const&)
                    {
                      run2_count.fetch_add(1);
                    }));

  graph.add(node2, SchedulerType::delegate_type::bind(
                    [&](auto const&)
                    {
                      run2_count.fetch_add(1);
                    }));

  // Second run - should execute all tasks (original + new ones)
  graph.prepare();
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
  scheduler.create_group(default_workgroup_id, 0, 1); // 1 worker
  scheduler.begin_execution();

  std::atomic<bool> task_executed{false};

  auto node1 = graph.create_node();

  // Add task to node
  graph.add(node1, SchedulerType::delegate_type::bind(
                    [&](auto const&)
                    {
                      task_executed.store(true);
                    }));

  // Start execution
  graph.prepare();
  auto ctx = SchedulerType::context_type::this_context::get();
  graph.start(ctx);
  graph.wait(ctx);

  REQUIRE(task_executed.load());

  scheduler.end_execution();
}
