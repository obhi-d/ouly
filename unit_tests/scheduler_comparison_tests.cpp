// SPDX-License-Identifier: MIT
#define GLM_ENABLE_EXPERIMENTAL
#include "catch2/catch_all.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
#include <chrono>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

// Test data structures
struct TestCounter
{
  std::atomic<uint32_t> count{0};
  std::atomic<uint32_t> total_count{0};
};

// Template wrapper to test both scheduler versions
template <typename SchedulerType, typename TaskContextType>
class SchedulerTestWrapper
{
public:
  using scheduler_type    = SchedulerType;
  using task_context_type = TaskContextType;

  static void run_basic_task_test(TestCounter& counter)
  {
    SchedulerType scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 4);
    scheduler.create_group(ouly::workgroup_id(1), 4, 2);

    scheduler.begin_execution();

    // Submit 1000 tasks across workgroups
    for (uint32_t i = 0; i < 1000; ++i)
    {
      auto wg = ouly::workgroup_id(i % 2);
      ouly::async(TaskContextType::get(wg), wg,
                  [&counter](TaskContextType const&)
                  {
                    counter.count.fetch_add(1, std::memory_order_relaxed);
                    counter.total_count.fetch_add(1, std::memory_order_relaxed);
                  });
    }

    scheduler.end_execution();
  }

  static void run_heavy_computation_test(TestCounter& counter, uint32_t computation_intensity = 1000)
  {
    SchedulerType scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());

    scheduler.begin_execution();

    std::vector<uint32_t> data(10000);
    std::iota(data.begin(), data.end(), 0);

    auto start_time = std::chrono::high_resolution_clock::now();

    // Heavy computation using parallel_for equivalent
    std::atomic<uint32_t> processed_count{0};

    for (size_t i = 0; i < data.size(); ++i)
    {
      ouly::async(TaskContextType::get(ouly::workgroup_id(0)), ouly::workgroup_id(0),
                  [&data, &counter, &processed_count, computation_intensity, i](TaskContextType const&)
                  {
                    // Heavy computation
                    volatile uint32_t result = data[i];
                    for (uint32_t j = 0; j < computation_intensity; ++j)
                    {
                      result = result * 31 + j;
                      result = result ^ (result >> 16);
                    }
                    data[i] = result;

                    processed_count.fetch_add(1, std::memory_order_relaxed);
                    counter.count.fetch_add(1, std::memory_order_relaxed);
                    counter.total_count.fetch_add(1, std::memory_order_relaxed);
                  });
    }

    scheduler.end_execution();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    INFO("Scheduler type: " << (std::is_same_v<SchedulerType, ouly::v1::scheduler> ? "v1" : "v2"));
    INFO("Processing time: " << duration.count() << " microseconds");
    INFO("Processed items: " << processed_count.load());

    REQUIRE(processed_count.load() == data.size());
  }

  static void run_work_stealing_test(TestCounter& counter)
  {
    SchedulerType scheduler;
    // Create imbalanced workgroups to encourage work stealing
    scheduler.create_group(ouly::workgroup_id(0), 0, 1); // Single thread group
    scheduler.create_group(ouly::workgroup_id(1), 1, 3); // Three thread group

    scheduler.begin_execution();

    std::atomic<uint32_t> group0_tasks{0};
    std::atomic<uint32_t> group1_tasks{0};

    // Submit many tasks to the single-thread group to encourage stealing
    for (uint32_t i = 0; i < 100; ++i)
    {
      ouly::async(TaskContextType::get(ouly::workgroup_id(0)), ouly::workgroup_id(0),
                  [&counter, &group0_tasks](TaskContextType const&)
                  {
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    group0_tasks.fetch_add(1, std::memory_order_relaxed);
                    counter.count.fetch_add(1, std::memory_order_relaxed);
                    counter.total_count.fetch_add(1, std::memory_order_relaxed);
                  });
    }

    // Submit some tasks to group 1 as well
    for (uint32_t i = 0; i < 50; ++i)
    {
      ouly::async(TaskContextType::get(ouly::workgroup_id(1)), ouly::workgroup_id(1),
                  [&counter, &group1_tasks](TaskContextType const&)
                  {
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                    group1_tasks.fetch_add(1, std::memory_order_relaxed);
                    counter.v1_count.fetch_add(1, std::memory_order_relaxed);
                    counter.total_count.fetch_add(1, std::memory_order_relaxed);
                  });
    }

    scheduler.end_execution();

    INFO("Group 0 tasks executed: " << group0_tasks.load());
    INFO("Group 1 tasks executed: " << group1_tasks.load());

    REQUIRE(group0_tasks.load() == 100);
    REQUIRE(group1_tasks.load() == 50);
  }
};

TEMPLATE_TEST_CASE("scheduler_comparison: Basic task execution",
                   (SchedulerTestWrapper<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestWrapper<ouly::v2::scheduler, ouly::v2::task_context>))
{
  TestCounter counter;

  TestType::run_basic_task_test(counter);
  REQUIRE(counter.count.load() == 1000);
  REQUIRE(counter.count.load() == 0);
}

TEMPLATE_TEST_CASE("scheduler_comparison: Heavy computation workload",
                   (SchedulerTestWrapper<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestWrapper<ouly::v2::scheduler, ouly::v2::task_context>))
{
  TestCounter counter;

  TestType::run_heavy_computation_test(counter_v1, 500);
  REQUIRE(counter.count.load() == 10000);
}

TEMPLATE_TEST_CASE("scheduler_comparison: Work stealing behavior",
                   (SchedulerTestWrapper<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestWrapper<ouly::v2::scheduler, ouly::v2::task_context>))
{
  TestCounter counter;
  TestType::run_work_stealing_test(counter_v1);
  REQUIRE(counter.total_count.load() == 150);
}

TEMPLATE_TEST_CASE("scheduler_comparison: Coroutine support",
                   (SchedulerTestWrapper<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestWrapper<ouly::v2::scheduler, ouly::v2::task_context>))
{
  typename TestType::scheduler_type scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);

  std::atomic<bool> coroutine_executed{false};

  auto simple_coroutine = [&coroutine_executed]() -> ouly::co_task<void>
  {
    coroutine_executed.store(true, std::memory_order_relaxed);
    co_return;
  };

  scheduler.begin_execution();

  auto task = simple_coroutine();
  scheduler.submit(ouly::v1::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(0), task);

  scheduler.end_execution();

  REQUIRE(coroutine_executed.load());
}

TEMPLATE_TEST_CASE("scheduler_comparison: Multi-workgroup cross communication",
                   (SchedulerTestWrapper<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestWrapper<ouly::v2::scheduler, ouly::v2::task_context>))
{
  constexpr uint32_t num_workgroups  = 4;
  constexpr uint32_t tasks_per_group = 100;

  typename TestType::scheduler_type scheduler;
  for (uint32_t i = 0; i < num_workgroups; ++i)
  {
    scheduler.create_group(ouly::workgroup_id(i), i * 2, 2);
  }

  std::array<std::atomic<uint32_t>, num_workgroups> group_counters{};

  scheduler.begin_execution();

  // Each workgroup submits work to the next workgroup in a round-robin fashion
  for (uint32_t group = 0; group < num_workgroups; ++group)
  {
    for (uint32_t task = 0; task < tasks_per_group; ++task)
    {
      uint32_t target_group = (group + 1) % num_workgroups;
      ouly::v1::async(ouly::v1::task_context::get(ouly::workgroup_id(group)), ouly::workgroup_id(target_group),
                      [&group_counters, target_group](ouly::v1::task_context const&)
                      {
                        group_counters[target_group].fetch_add(1, std::memory_order_relaxed);
                      });
    }
  }

  scheduler.end_execution();

  for (uint32_t i = 0; i < num_workgroups; ++i)
  {
    REQUIRE(group_counters[i].load() == tasks_per_group);
  }
}
