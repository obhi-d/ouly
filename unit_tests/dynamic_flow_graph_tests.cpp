#ifndef OULY_ASSERT
#define OULY_ASSERT(expr)
#endif

#include "ouly/scheduler/dynamic_flow_graph.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <array>
#include <atomic>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
// NOLINTBEGIN
using namespace ouly;

class dynamic_flow_graph_progress_listener final : public Catch::EventListenerBase
{
public:
  using Catch::EventListenerBase::EventListenerBase;

  void testCaseStarting(Catch::TestCaseInfo const& test_info) override
  {
    std::cerr << "[dynamic_flow_graph] starting: " << test_info.name << '\n';
  }
};

CATCH_REGISTER_LISTENER(dynamic_flow_graph_progress_listener)

TEST_CASE("dynamic_flow_graph linear one-shot ordering", "[dynamic_flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::atomic<int> order{0};
  std::atomic<int> a_order{-1};
  std::atomic<int> b_order{-1};
  std::atomic<int> c_order{-1};

  auto a = graph.create_node();
  auto b = graph.create_node();
  auto c = graph.create_node();

  graph.connect(a, b);
  graph.connect(b, c);

  graph.add(a,
            [&](auto const&)
            {
              a_order.store(order.fetch_add(1));
              std::this_thread::sleep_for(std::chrono::milliseconds(5));
            });
  graph.add(b,
            [&](auto const&)
            {
              b_order.store(order.fetch_add(1));
              std::this_thread::sleep_for(std::chrono::milliseconds(5));
            });
  graph.add(c,
            [&](auto const&)
            {
              c_order.store(order.fetch_add(1));
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(a, ctx); // kick the root
  graph.cooperative_wait(ctx);

  REQUIRE(a_order.load() == 0);
  REQUIRE(b_order.load() == 1);
  REQUIRE(c_order.load() == 2);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph parallel tasks within a node", "[dynamic_flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::atomic<int> node1_count{0};
  std::atomic<int> node2_count{0};
  std::atomic<int> ordering_fail{0};

  auto node1 = graph.create_node();
  auto node2 = graph.create_node();
  graph.connect(node1, node2);

  for (int i = 0; i < 5; ++i)
  {
    graph.add(node1,
              [&](auto const&)
              {
                node1_count.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
              });
  }
  for (int i = 0; i < 3; ++i)
  {
    graph.add(node2,
              [&](auto const&)
              {
                if (node1_count.load() != 5)
                {
                  ordering_fail.fetch_add(1);
                }
                node2_count.fetch_add(1);
              });
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(node1, ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(node1_count.load() == 5);
  REQUIRE(node2_count.load() == 3);
  REQUIRE(ordering_fail.load() == 0);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph diamond converges correctly", "[dynamic_flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;
  using node_type = typename dynamic_flow_graph<SchedulerType>::node_id;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::atomic<int>                order{0};
  std::array<std::atomic<int>, 4> node_order;
  for (auto& o : node_order)
  {
    o.store(-1);
  }

  auto top    = graph.create_node();
  auto left   = graph.create_node();
  auto right  = graph.create_node();
  auto bottom = graph.create_node();

  graph.connect(top, left);
  graph.connect(top, right);
  graph.connect(left, bottom);
  graph.connect(right, bottom);

  for (uint32_t i = 0; i < 4; ++i)
  {
    graph.add(node_type{i},
              [&, i](auto const&)
              {
                node_order[i].store(order.fetch_add(1));
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
              });
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(top, ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(node_order[0].load() < node_order[1].load());
  REQUIRE(node_order[0].load() < node_order[2].load());
  REQUIRE(node_order[1].load() < node_order[3].load());
  REQUIRE(node_order[2].load() < node_order[3].load());
  // bottom must observe both predecessors (fired exactly once, last)
  REQUIRE(node_order[3].load() == 3);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph self-loop runs a fixed number of iterations", "[dynamic_flow_graph][loop]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  constexpr int    iterations = 100;
  std::atomic<int> counter{0};

  auto tick = graph.create_node();
  graph.connect(tick, tick); // self loop

  graph.add(tick,
            [&](auto const&)
            {
              int v = counter.fetch_add(1) + 1;
              if (v == iterations)
              {
                graph.request_stop();
              }
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(tick, ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(counter.load() == iterations);
  REQUIRE(graph.is_idle());
  REQUIRE(graph.stop_requested());

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph multi-node cycle loops correctly", "[dynamic_flow_graph][loop]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  constexpr int    frames = 50;
  std::atomic<int> update_count{0};
  std::atomic<int> render_count{0};
  std::atomic<int> present_count{0};
  std::atomic<int> ordering_fail{0};

  auto update  = graph.create_node();
  auto render  = graph.create_node();
  auto present = graph.create_node();

  // Steady cycle: update -> render -> present -> update
  graph.connect(update, render);
  graph.connect(render, present);
  graph.connect(present, update);

  graph.add(update,
            [&](auto const&)
            {
              update_count.fetch_add(1);
            });
  graph.add(render,
            [&](auto const&)
            {
              // render must never run ahead of update for the frame
              if (render_count.load() >= update_count.load())
              {
                ordering_fail.fetch_add(1);
              }
              render_count.fetch_add(1);
            });
  graph.add(present,
            [&](auto const&)
            {
              int v = present_count.fetch_add(1) + 1;
              if (v == frames)
              {
                graph.request_stop();
              }
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(update, ctx); // kick the loop once
  graph.cooperative_wait(ctx);

  REQUIRE(present_count.load() == frames);
  REQUIRE(update_count.load() == frames);
  REQUIRE(render_count.load() == frames);
  REQUIRE(ordering_fail.load() == 0);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph add nodes and edges mid-execution", "[dynamic_flow_graph][dynamic]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  constexpr int     total_iterations = 40;
  constexpr int     inject_at        = 10;
  std::atomic<int>  tick_count{0};
  std::atomic<int>  injected_count{0};
  std::atomic<bool> injected{false};

  auto tick = graph.create_node();
  graph.connect(tick, tick);

  graph.add(tick,
            [&](auto const& task_ctx)
            {
              int v = tick_count.fetch_add(1) + 1;

              // Halfway through, grow the graph from inside a running task.
              if (v == inject_at && !injected.exchange(true))
              {
                auto extra = graph.create_node();
                graph.add(extra,
                          [&](auto const&)
                          {
                            injected_count.fetch_add(1);
                          });
                graph.connect(tick, extra); // tick now also fires `extra` each iteration
              }

              if (v == total_iterations)
              {
                graph.request_stop();
              }
              (void)task_ctx;
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(tick, ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(tick_count.load() == total_iterations);
  // The injected node fires from the iteration after it was wired in, until stop.
  REQUIRE(injected_count.load() > 0);
  REQUIRE(injected_count.load() <= total_iterations - inject_at);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph main-thread node runs on the waiting thread", "[dynamic_flow_graph][main_thread]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::thread::id  waiting_thread;
  std::thread::id  main_node_thread;
  std::atomic<int> order{0};
  int              a_order{-1};
  int              b_order{-1};
  int              c_order{-1};

  auto a = graph.create_node();             // worker
  auto b = graph.create_main_thread_node(); // must run on the waiting thread
  auto c = graph.create_node();             // worker

  graph.connect(a, b);
  graph.connect(b, c);

  graph.add(a,
            [&](auto const&)
            {
              a_order = order.fetch_add(1);
            });
  graph.add(b,
            [&](auto const&)
            {
              main_node_thread = std::this_thread::get_id();
              b_order          = order.fetch_add(1);
            });
  graph.add(c,
            [&](auto const&)
            {
              c_order = order.fetch_add(1);
            });

  auto ctx       = SchedulerType::context_type::this_context::get();
  waiting_thread = std::this_thread::get_id();
  graph.signal(a, ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(a_order == 0);
  REQUIRE(b_order == 1);
  REQUIRE(c_order == 2);
  REQUIRE(main_node_thread == waiting_thread);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph task removal before signal", "[dynamic_flow_graph][task_removal]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<int> kept{0};
  std::atomic<int> removed{0};

  auto node = graph.create_node();

  auto t1 = graph.add(node,
                      [&](auto const&)
                      {
                        kept.fetch_add(1);
                      });
  auto t2 = graph.add(node,
                      [&](auto const&)
                      {
                        removed.fetch_add(1);
                      });
  auto t3 = graph.add(node,
                      [&](auto const&)
                      {
                        kept.fetch_add(1);
                      });
  (void)t1;
  (void)t3;

  graph.remove(node, t2);
  // double remove and invalid removes must be safe
  graph.remove(node, t2);
  graph.remove(typename dynamic_flow_graph<SchedulerType>::node_id{999}, t2);

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(node, ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(kept.load() == 2);
  REQUIRE(removed.load() == 0);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph empty nodes still propagate", "[dynamic_flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<bool> final_ran{false};

  auto e1    = graph.create_node(); // no tasks
  auto e2    = graph.create_node(); // no tasks
  auto final = graph.create_node();

  graph.connect(e1, e2);
  graph.connect(e2, final);

  graph.add(final,
            [&](auto const&)
            {
              final_ran.store(true);
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(e1, ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(final_ran.load());

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph independent roots run in parallel", "[dynamic_flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  std::chrono::high_resolution_clock::time_point t1;
  std::chrono::high_resolution_clock::time_point t2;

  auto branch1 = graph.create_node();
  auto branch2 = graph.create_node();

  graph.add(branch1,
            [&](auto const&)
            {
              t1 = std::chrono::high_resolution_clock::now();
              std::this_thread::sleep_for(std::chrono::milliseconds(50));
            });
  graph.add(branch2,
            [&](auto const&)
            {
              t2 = std::chrono::high_resolution_clock::now();
              std::this_thread::sleep_for(std::chrono::milliseconds(50));
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(branch1, ctx);
  graph.signal(branch2, ctx);
  graph.cooperative_wait(ctx);

  auto diff = std::abs(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t2).count());
  REQUIRE(diff <= 25);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph stress linear chain", "[dynamic_flow_graph][scheduler]")
{
  using SchedulerType = ouly::v2::scheduler;
  dynamic_flow_graph<SchedulerType> graph;
  using node_type = typename dynamic_flow_graph<SchedulerType>::node_id;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  constexpr int                            num_nodes = 200; // spans several storage chunks
  std::array<std::atomic<bool>, num_nodes> ran;
  std::atomic<int>                         fails{0};
  for (auto& r : ran)
  {
    r.store(false);
  }

  std::vector<node_type> nodes;
  nodes.reserve(num_nodes);
  for (int i = 0; i < num_nodes; ++i)
  {
    nodes.push_back(graph.create_node());
  }
  for (int i = 0; i < num_nodes - 1; ++i)
  {
    graph.connect(nodes[i], nodes[i + 1]);
  }
  for (int i = 0; i < num_nodes; ++i)
  {
    graph.add(nodes[i],
              [&, i](auto const&)
              {
                ran[i].store(true);
                if (i > 0 && !ran[i - 1].load())
                {
                  fails.fetch_add(1);
                }
              });
  }

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(nodes[0], ctx);
  graph.cooperative_wait(ctx);

  for (int i = 0; i < num_nodes; ++i)
  {
    REQUIRE(ran[i].load());
  }
  REQUIRE(fails.load() == 0);

  scheduler.end_execution();
}

TEST_CASE("dynamic_flow_graph works with v1 scheduler", "[dynamic_flow_graph][scheduler]")
{
  using SchedulerType = ouly::v1::scheduler;
  dynamic_flow_graph<SchedulerType> graph;

  SchedulerType scheduler;
  scheduler.create_group(default_workgroup_id, 0, 2);
  scheduler.begin_execution();

  std::atomic<bool> ran{false};

  auto node = graph.create_node();
  graph.add(node,
            [&](auto const&)
            {
              ran.store(true);
            });

  auto ctx = SchedulerType::context_type::this_context::get();
  graph.signal(node, ctx);
  graph.cooperative_wait(ctx);

  REQUIRE(ran.load());

  scheduler.end_execution();
}
// NOLINTEND
