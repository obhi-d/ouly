// SPDX-License-Identifier: MIT
#define GLM_ENABLE_EXPERIMENTAL
#include "catch2/catch_all.hpp"
#include "ouly/scheduler/auto_parallel_for.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include "ouly/utility/subrange.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
#include <ranges>
#include <thread>
#include <vector>

// NOLINTBEGIN
// Include GLM for mathematical operations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Test utilities
struct TestCounter
{
  std::atomic<uint32_t> task_count{0};
  std::atomic<uint32_t> sub_task_count_0{0};
  std::atomic<uint32_t> sub_task_count_1{0};
  std::atomic<uint32_t> total_operations{0};
  std::atomic<uint64_t> computation_result{0};
};

// Template wrapper for testing both scheduler versions
template <typename SchedulerType, typename TaskContextType>
struct SchedulerTestRunner
{
  using scheduler_type    = SchedulerType;
  using task_context_type = TaskContextType;

  static auto setup_scheduler(uint32_t worker_count = std::thread::hardware_concurrency()) -> SchedulerType
  {
    SchedulerType scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, worker_count);
    return scheduler;
  }

  static auto get_main_context() -> TaskContextType const&
  {
    return task_context_type::this_context::get();
  }
};

// Test basic task submission
TEMPLATE_TEST_CASE("Basic Task Submission", "[scheduler][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner = TestType;
  // using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  TestCounter counter;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  // Submit 1000 simple tasks
  for (uint32_t i = 0; i < 1000; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&counter](TaskContextType const&)
                     {
                       counter.task_count.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(counter.task_count.load() == 1000);
}

struct small_loop_task_traits
{
  /**
   * Relevant for ranged executers, this value determines the number of batches dispatched per worker on average.
   * Higher value means the individual task batches are smaller.
   */
  static constexpr uint32_t batches_per_worker = 1;
  /**
   * This value is used as the minimum task count that will fire the parallel executer, if the task count is less than
   * this value, a for loop is executed instead.
   */
  static constexpr uint32_t parallel_execution_threshold = 1;
  /**
   * This value, if set to non-zero, would override the `batches_per_worker` value and instead be used as the batch
   * size for the tasks.
   */
  static constexpr uint32_t fixed_batch_size = 1;
};

// Test parallel_for functionality
TEMPLATE_TEST_CASE("Parallel For Small Loop", "[scheduler][parallel_for][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{

  using TestRunner = TestType;
  // using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  TestCounter counter;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  // Create test data
  std::vector<uint32_t> data(10);
  std::iota(data.begin(), data.end(), 0);

  // Execute parallel_for with element-wise processing
  ouly::parallel_for(
   [&counter](uint32_t& element, TaskContextType const&)
   {
     element *= 2; // Simple operation
     counter.task_count.fetch_add(1, std::memory_order_relaxed);
   },
   data, main_ctx, small_loop_task_traits{});

  scheduler.end_execution();

  // Verify all elements were processed
  REQUIRE(counter.task_count.load() == 10);

  // Verify data transformation
  for (size_t i = 0; i < data.size(); ++i)
  {
    REQUIRE(data[i] == i * 2);
  }
}

// Test parallel_for functionality
TEMPLATE_TEST_CASE("Parallel For Execution", "[scheduler][parallel_for][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner = TestType;
  // using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  TestCounter counter;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  // Create test data
  std::vector<uint32_t> data(10000);
  std::iota(data.begin(), data.end(), 0);

  // Execute parallel_for with element-wise processing
  ouly::parallel_for(
   [&counter](uint32_t& element, TaskContextType const&)
   {
     element *= 2; // Simple operation
     counter.task_count.fetch_add(1, std::memory_order_relaxed);
   },
   data, main_ctx);

  scheduler.end_execution();

  // Verify all elements were processed
  REQUIRE(counter.task_count.load() == 10000);

  // Verify data transformation
  for (size_t i = 0; i < data.size(); ++i)
  {
    REQUIRE(data[i] == i * 2);
  }
}

// Test GLM mathematical operations for task throughput
TEMPLATE_TEST_CASE("GLM Mathematical Operations", "[scheduler][glm][math][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner = TestType;
  // using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  TestCounter counter;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  // Create test vectors for mathematical operations
  static constexpr size_t vector_count = 50000;
  struct data_holder_t
  {
    std::vector<glm::vec3> vectors;
    std::vector<glm::mat4> matrices;
    std::vector<float>     results;

    data_holder_t(size_t count) : vectors(count), matrices(count), results(count) {}
  };

  data_holder_t data_holder(vector_count);

  // Initialize test data
  for (size_t i = 0; i < vector_count; ++i)
  {
    data_holder.vectors[i]  = glm::vec3(static_cast<float>(i), static_cast<float>(i + 1), static_cast<float>(i + 2));
    data_holder.matrices[i] = glm::translate(glm::mat4(1.0f), glm::vec3(static_cast<float>(i)));
  }

  // Test vector operations using parallel_for
  ouly::parallel_for(
   [&counter](auto begin, auto end, TaskContextType const&)
   {
     for (auto it = begin; it != end; ++it)
     {
       auto& vec = *it;
       // Perform multiple vector operations
       vec = glm::normalize(vec);
       vec = glm::cross(vec, glm::vec3(1.0f, 0.0f, 0.0f));
       vec += glm::vec3(0.1f);

       counter.total_operations.fetch_add(3, std::memory_order_relaxed);
       counter.sub_task_count_0.fetch_add(1, std::memory_order_relaxed);
     }
     counter.task_count.fetch_add(1, std::memory_order_relaxed);
   },
   data_holder.vectors, main_ctx);

  // Test matrix operations using tasks
  for (size_t i = 0; i < data_holder.matrices.size(); ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&data_holder, &counter, i](TaskContextType const&)
                     {
                       // Complex matrix operations
                       auto& matrix = data_holder.matrices[i];
                       matrix       = glm::rotate(matrix, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                       matrix       = glm::scale(matrix, glm::vec3(1.1f));

                       // Extract some result for verification
                       glm::vec4 transformed  = matrix * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                       data_holder.results[i] = glm::length(glm::vec3(transformed));
                       if (data_holder.results[i] < 0.1f)
                         data_holder.results[i] = 1.0f; // Prevent zero results

                       counter.total_operations.fetch_add(3, std::memory_order_relaxed);
                       counter.sub_task_count_1.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(counter.sub_task_count_0.load() == data_holder.vectors.size());
  REQUIRE(counter.sub_task_count_1.load() == data_holder.matrices.size());

  // Verify operations were performed
  REQUIRE(counter.total_operations.load() >= vector_count * 3 + data_holder.matrices.size() * 3);

  // Verify some results are non-zero (indicating computation occurred)
  uint32_t non_zero_count = 0;
  for (const auto& result : data_holder.results)
  {
    if (result > 0.0f)
      non_zero_count++;
  }
  REQUIRE(non_zero_count == data_holder.results.size());
}

// Test heavy computational workload
TEMPLATE_TEST_CASE("Heavy Computation Stress Test", "[scheduler][stress][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner = TestType;
  // using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  TestCounter counter;

  auto scheduler = TestRunner::setup_scheduler(std::thread::hardware_concurrency());

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t     task_count            = 100;
  constexpr uint32_t computation_intensity = 1000;

  // Submit computationally intensive tasks
  for (uint32_t i = 0; i < task_count; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&counter, i](TaskContextType const&)
                     {
                       // Heavy computation with GLM operations
                       glm::mat4 result(1.0f);
                       glm::vec3 vector(static_cast<float>(i), static_cast<float>(i + 1), static_cast<float>(i + 2));

                       for (uint32_t j = 0; j < computation_intensity; ++j)
                       {
                         result = glm::rotate(result, 0.01f, vector);
                         vector = glm::normalize(vector + glm::vec3(0.001f));

                         // Add some integer computation
                         volatile uint32_t temp = i * j;
                         temp                   = temp ^ (temp >> 16);
                         temp                   = temp * 31 + j;
                       }

                       // Store some result to prevent optimization
                       counter.computation_result.fetch_add(static_cast<uint64_t>(glm::length(vector)),
                                                            std::memory_order_relaxed);
                       counter.task_count.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(counter.task_count.load() == task_count);
  REQUIRE(counter.computation_result.load() > 0);
}

// Test cross-workgroup task submission
TEMPLATE_TEST_CASE("Cross-Workgroup Task Submission", "[scheduler][workgroup][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  TestCounter counter;

  SchedulerType scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.create_group(ouly::workgroup_id(1), 2, 2);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  // Submit tasks to different workgroups
  for (uint32_t i = 0; i < 500; ++i)
  {
    auto target_group = ouly::workgroup_id(i % 2);
    scheduler.submit(main_ctx, target_group,
                     [&counter](TaskContextType const&)
                     {
                       // Note: We can't verify workgroup for v1 since get_workgroup is private
                       // This test just verifies task execution across workgroups
                       counter.task_count.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(counter.task_count.load() == 500);
}

// Test async helper functions
TEMPLATE_TEST_CASE("Async Helper Functions", "[scheduler][async][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner = TestType;
  // using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  TestCounter counter;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  // Test async function with current workgroup
  scheduler.submit(main_ctx,
                   [&counter](TaskContextType const&)
                   {
                     counter.task_count.fetch_add(1, std::memory_order_relaxed);
                   });

  // Test async function with explicit workgroup
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&counter](TaskContextType const&)
                   {
                     counter.task_count.fetch_add(1, std::memory_order_relaxed);
                   });

  scheduler.end_execution();

  REQUIRE(counter.task_count.load() == 2);
}

// Test multiple workgroups with different worker counts
TEMPLATE_TEST_CASE("Multiple Workgroups Different Sizes", "[scheduler][workgroup][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  [[maybe_unused]] TestCounter counter;

  SchedulerType scheduler;
  // Create workgroups with different worker counts
  scheduler.create_group(ouly::workgroup_id(0), 0, 1); // Single worker
  scheduler.create_group(ouly::workgroup_id(1), 1, 2); // Two workers
  scheduler.create_group(ouly::workgroup_id(2), 3, 3); // Three workers

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> group0_tasks{0};
  std::atomic<uint32_t> group1_tasks{0};
  std::atomic<uint32_t> group2_tasks{0};

  // Submit tasks to each workgroup
  const uint32_t tasks_per_group = 100;

  for (uint32_t i = 0; i < tasks_per_group; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&group0_tasks](TaskContextType const&)
                     {
                       std::this_thread::sleep_for(std::chrono::microseconds(10));
                       group0_tasks.fetch_add(1, std::memory_order_relaxed);
                     });

    scheduler.submit(main_ctx, ouly::workgroup_id(1),
                     [&group1_tasks](TaskContextType const&)
                     {
                       std::this_thread::sleep_for(std::chrono::microseconds(10));
                       group1_tasks.fetch_add(1, std::memory_order_relaxed);
                     });

    scheduler.submit(main_ctx, ouly::workgroup_id(2),
                     [&group2_tasks](TaskContextType const&)
                     {
                       std::this_thread::sleep_for(std::chrono::microseconds(10));
                       group2_tasks.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(group0_tasks.load() == tasks_per_group);
  REQUIRE(group1_tasks.load() == tasks_per_group);
  REQUIRE(group2_tasks.load() == tasks_per_group);
}

// Test parallel_for within async tasks
TEMPLATE_TEST_CASE("Parallel For Within Async Tasks", "[scheduler][parallel_for][async][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(6);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t outer_task_count   = 10;
  const uint32_t data_size_per_task = 1000;

  std::atomic<uint32_t> completed_outer_tasks{0};
  std::atomic<uint32_t> total_inner_operations{0};

  for (uint32_t task_id = 0; task_id < outer_task_count; ++task_id)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&completed_outer_tasks, &total_inner_operations, task_id](TaskContextType const& ctx)
                     {
                       // Create data within the async task
                       const uint32_t        data_size_per_task = 1000;
                       std::vector<uint32_t> task_data(data_size_per_task);
                       std::iota(task_data.begin(), task_data.end(), task_id * data_size_per_task);

                       std::atomic<uint32_t> inner_ops{0};

                       // Execute parallel_for within the async task
                       ouly::parallel_for(
                        [&inner_ops](uint32_t& element, TaskContextType const&)
                        {
                          element = element * 2 + 1; // Some computation
                          inner_ops.fetch_add(1, std::memory_order_relaxed);
                        },
                        task_data, ctx);

                       // Verify all elements were processed (safe since we're in single async task)
                       uint32_t expected_ops = data_size_per_task;
                       if (inner_ops.load() == expected_ops)
                       {
                         total_inner_operations.fetch_add(expected_ops, std::memory_order_relaxed);
                         completed_outer_tasks.fetch_add(1, std::memory_order_relaxed);
                       }
                     });
  }

  scheduler.end_execution();

  REQUIRE(completed_outer_tasks.load() == outer_task_count);
  REQUIRE(total_inner_operations.load() == outer_task_count * data_size_per_task);
}

// Test nested async tasks (async tasks creating other async tasks) - simplified
TEMPLATE_TEST_CASE("Nested Async Tasks Simple", "[scheduler][async][nested][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  SchedulerType scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.create_group(ouly::workgroup_id(1), 2, 2);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> parent_tasks_completed{0};
  std::atomic<uint32_t> child_tasks_completed{0};

  // Submit a parent task that creates child tasks
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&scheduler, &parent_tasks_completed, &child_tasks_completed](TaskContextType const& parent_ctx)
                   {
                     // Create child tasks from within parent task
                     for (uint32_t child_id = 0; child_id < 3; ++child_id)
                     {
                       scheduler.submit(parent_ctx, ouly::workgroup_id(1),
                                        [&child_tasks_completed](TaskContextType const&)
                                        {
                                          child_tasks_completed.fetch_add(1, std::memory_order_relaxed);
                                        });
                     }
                     parent_tasks_completed.fetch_add(1, std::memory_order_relaxed);
                   });

  scheduler.end_execution();

  REQUIRE(parent_tasks_completed.load() == 1);
  REQUIRE(child_tasks_completed.load() == 3);
}

// Test simple parallel_for within async tasks
TEMPLATE_TEST_CASE("Simple Parallel For Within Async", "[scheduler][parallel_for][async][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> completed_tasks{0};

  // Submit task that uses parallel_for internally
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&completed_tasks](TaskContextType const& ctx)
                   {
                     std::vector<uint32_t> data(100);
                     std::iota(data.begin(), data.end(), 0);

                     ouly::parallel_for(
                      [](uint32_t& element, TaskContextType const&)
                      {
                        element *= 2;
                      },
                      data, ctx);

                     completed_tasks.fetch_add(1, std::memory_order_relaxed);
                   });

  scheduler.end_execution();

  REQUIRE(completed_tasks.load() == 1);
}

// Test simple task chain
TEMPLATE_TEST_CASE("Simple Task Chain", "[scheduler][chain][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> chain_step{0};

  // Create a simple task chain
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&scheduler, &chain_step](TaskContextType const& ctx)
                   {
                     chain_step.fetch_add(1, std::memory_order_relaxed);

                     scheduler.submit(ctx, ouly::workgroup_id(0),
                                      [&scheduler, &chain_step](TaskContextType const& ctx2)
                                      {
                                        chain_step.fetch_add(1, std::memory_order_relaxed);

                                        scheduler.submit(ctx2, ouly::workgroup_id(0),
                                                         [&chain_step](TaskContextType const&)
                                                         {
                                                           chain_step.fetch_add(1, std::memory_order_relaxed);
                                                         });
                                      });
                   });

  scheduler.end_execution();

  REQUIRE(chain_step.load() == 3);
}

// Test task dependencies and synchronization patterns
TEMPLATE_TEST_CASE("Task Dependencies and Synchronization", "[scheduler][sync][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t pipeline_stages = 5;
  const uint32_t items_per_stage = 100;

  std::vector<std::atomic<uint32_t>> stage_counters(pipeline_stages);
  std::vector<std::vector<uint32_t>> stage_data(pipeline_stages);

  // Initialize data for first stage
  stage_data[0].resize(items_per_stage);
  std::iota(stage_data[0].begin(), stage_data[0].end(), 1);

  // Stage 0: Initial processing
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&scheduler, &stage_counters, &stage_data](TaskContextType const& ctx)
                   {
                     ouly::parallel_for(
                      [&stage_counters](uint32_t& item, TaskContextType const&)
                      {
                        item *= 2; // First transformation
                        stage_counters[0].fetch_add(1, std::memory_order_relaxed);
                      },
                      stage_data[0], ctx);

                     // Prepare data for next stage
                     stage_data[1] = stage_data[0];

                     // Submit next stage
                     scheduler.submit(ctx, ouly::workgroup_id(0),
                                      [&scheduler, &stage_counters, &stage_data](TaskContextType const& ctx2)
                                      {
                                        ouly::parallel_for(
                                         [&stage_counters](uint32_t& item, TaskContextType const&)
                                         {
                                           item += 10; // Second transformation
                                           stage_counters[1].fetch_add(1, std::memory_order_relaxed);
                                         },
                                         stage_data[1], ctx2);

                                        // Continue pipeline
                                        stage_data[2] = stage_data[1];

                                        scheduler.submit(
                                         ctx2, ouly::workgroup_id(0),
                                         [&scheduler, &stage_counters, &stage_data](TaskContextType const& ctx3)
                                         {
                                           for (auto& item : stage_data[2])
                                           {
                                             item = item * item % 1000; // Third transformation
                                             stage_counters[2].fetch_add(1, std::memory_order_relaxed);
                                           }

                                           stage_data[3] = stage_data[2];

                                           scheduler.submit(
                                            ctx3, ouly::workgroup_id(0),
                                            [&stage_counters, &stage_data](TaskContextType const&)
                                            {
                                              for (auto& item : stage_data[3])
                                              {
                                                item += 1; // Fourth transformation
                                                stage_counters[3].fetch_add(1, std::memory_order_relaxed);
                                              }

                                              stage_data[4] = stage_data[3];

                                              for (auto& item : stage_data[4])
                                              {
                                                item = item % 100; // Final transformation
                                                stage_counters[4].fetch_add(1, std::memory_order_relaxed);
                                              }
                                            });
                                         });
                                      });
                   });

  scheduler.end_execution();

  // Verify all stages completed
  for (uint32_t stage = 0; stage < pipeline_stages; ++stage)
  {
    REQUIRE(stage_counters[stage].load() == items_per_stage);
  }
}

// Test work-stealing behavior with uneven workloads
TEMPLATE_TEST_CASE("Work Stealing Uneven Workloads", "[scheduler][work-stealing][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t        total_tasks = 1000;
  std::atomic<uint32_t> fast_tasks{0};
  std::atomic<uint32_t> slow_tasks{0};

  // Submit mix of fast and slow tasks
  for (uint32_t i = 0; i < total_tasks; ++i)
  {
    if (i % 10 == 0) // Every 10th task is slow
    {
      scheduler.submit(main_ctx, ouly::workgroup_id(0),
                       [&slow_tasks](TaskContextType const&)
                       {
                         // Slow task - CPU intensive work
                         volatile uint64_t result = 1;
                         for (uint32_t j = 0; j < 10000; ++j)
                         {
                           result = (result * 31 + j) ^ (result >> 16);
                         }
                         slow_tasks.fetch_add(1, std::memory_order_relaxed);
                       });
    }
    else
    {
      scheduler.submit(main_ctx, ouly::workgroup_id(0),
                       [&fast_tasks](TaskContextType const&)
                       {
                         // Fast task - minimal work
                         volatile uint32_t temp = 42;
                         temp                   = temp * 3 + 1;
                         fast_tasks.fetch_add(1, std::memory_order_relaxed);
                       });
    }
  }

  scheduler.end_execution();

  uint32_t expected_slow = total_tasks / 10;
  uint32_t expected_fast = total_tasks - expected_slow;

  REQUIRE(slow_tasks.load() == expected_slow);
  REQUIRE(fast_tasks.load() == expected_fast);
}

// Test scheduler API completeness - scheduler creation and destruction
TEMPLATE_TEST_CASE("Scheduler Lifecycle Management", "[scheduler][lifecycle][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  // Test multiple scheduler creation/destruction cycles
  for (uint32_t cycle = 0; cycle < 3; ++cycle)
  {
    TestCounter counter;

    {
      SchedulerType scheduler;
      scheduler.create_group(ouly::workgroup_id(0), 0, 2);

      scheduler.begin_execution();
      auto const& main_ctx = TestRunner::get_main_context();

      // Submit some tasks
      for (uint32_t i = 0; i < 50; ++i)
      {
        scheduler.submit(main_ctx, ouly::workgroup_id(0),
                         [&counter](TaskContextType const&)
                         {
                           counter.task_count.fetch_add(1, std::memory_order_relaxed);
                         });
      }

      scheduler.end_execution();

      REQUIRE(counter.task_count.load() == 50);
    } // Scheduler destroyed here

    // Brief pause between cycles
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// Test edge cases and error conditions
TEMPLATE_TEST_CASE("Edge Cases and Boundary Conditions", "[scheduler][edge-cases][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  TestCounter counter;

  SchedulerType scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 1); // Single worker

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  // Test empty parallel_for
  std::vector<uint32_t> empty_data;
  ouly::parallel_for(
   [&counter](uint32_t& /*element*/, TaskContextType const&)
   {
     counter.task_count.fetch_add(1, std::memory_order_relaxed);
   },
   empty_data, main_ctx);

  // Test single element parallel_for
  std::vector<uint32_t> single_data{42};
  ouly::parallel_for(
   [&counter](uint32_t& element, TaskContextType const&)
   {
     element *= 2;
     counter.sub_task_count_0.fetch_add(1, std::memory_order_relaxed);
   },
   single_data, main_ctx);

  // Test task that does nothing
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&counter](TaskContextType const&)
                   {
                     // Intentionally do minimal work
                     counter.sub_task_count_1.fetch_add(1, std::memory_order_relaxed);
                   });

  scheduler.end_execution();

  REQUIRE(counter.task_count.load() == 0);       // Empty parallel_for should do nothing
  REQUIRE(counter.sub_task_count_0.load() == 1); // Single element processed
  REQUIRE(counter.sub_task_count_1.load() == 1); // Empty task executed
  REQUIRE(single_data[0] == 84);                 // Verify single element was processed
}

// Test high-frequency task submission and completion
TEMPLATE_TEST_CASE("High Frequency Task Submission", "[scheduler][performance][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(std::thread::hardware_concurrency());

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t        high_task_count = 10000;
  std::atomic<uint32_t> completed_tasks{0};

  auto start_time = std::chrono::high_resolution_clock::now();

  // Submit many small tasks rapidly
  for (uint32_t i = 0; i < high_task_count; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&completed_tasks, i](TaskContextType const&)
                     {
                       // Minimal computation to avoid optimization
                       volatile uint32_t temp = i * 13 + 7;
                       temp                   = temp ^ (temp >> 8);
                       completed_tasks.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  REQUIRE(completed_tasks.load() == high_task_count);

  // Performance check - should complete reasonably quickly
  REQUIRE(duration.count() < 5000); // Less than 5 seconds
}

// Test scheduler with dynamic workgroup creation
TEMPLATE_TEST_CASE("Dynamic Workgroup Management", "[scheduler][workgroup][dynamic][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  SchedulerType scheduler;

  // Create workgroups with various configurations
  const uint32_t total_workers = std::min(8u, std::thread::hardware_concurrency());
  uint32_t       worker_offset = 0;

  std::vector<ouly::workgroup_id>               workgroups;
  const uint32_t                                max_groups = 4;
  std::array<std::atomic<uint32_t>, max_groups> group_task_counts{};

  // Create 4 workgroups with different worker counts
  uint32_t actual_groups = 0;
  for (uint32_t group_idx = 0; group_idx < max_groups && worker_offset < total_workers; ++group_idx)
  {
    uint32_t workers_for_group = std::min(2u, total_workers - worker_offset);
    if (workers_for_group == 0)
      break;

    workgroups.emplace_back(group_idx);
    group_task_counts[actual_groups].store(0);

    scheduler.create_group(ouly::workgroup_id(group_idx), worker_offset, workers_for_group);
    worker_offset += workers_for_group;
    actual_groups++;
  }

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t tasks_per_group = 50;

  // Submit tasks to all workgroups
  for (size_t group_idx = 0; group_idx < workgroups.size(); ++group_idx)
  {
    for (uint32_t task_idx = 0; task_idx < tasks_per_group; ++task_idx)
    {
      scheduler.submit(main_ctx, workgroups[group_idx],
                       [&group_task_counts, group_idx](TaskContextType const&)
                       {
                         // Some computation specific to this group
                         volatile uint32_t result = static_cast<uint32_t>(group_idx) * 100;
                         for (uint32_t i = 0; i < 50; ++i)
                         {
                           result = result * 7 + i;
                         }
                         group_task_counts[group_idx].fetch_add(1, std::memory_order_relaxed);
                       });
    }
  }

  scheduler.end_execution();

  // Verify all groups processed their tasks
  for (size_t group_idx = 0; group_idx < workgroups.size(); ++group_idx)
  {
    REQUIRE(group_task_counts[group_idx].load() == tasks_per_group);
  }
}

// Test complex nested parallel_for patterns
TEMPLATE_TEST_CASE("Complex Nested Parallel For Patterns", "[scheduler][parallel_for][complex][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(6);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t matrix_size  = 100;
  const uint32_t num_matrices = 10;

  std::vector<std::vector<std::vector<float>>> matrices(num_matrices);
  std::atomic<uint32_t>                        total_processed_elements{0};
  std::atomic<uint32_t>                        matrices_completed{0};

  // Initialize matrices
  for (auto& matrix : matrices)
  {
    matrix.resize(matrix_size);
    for (auto& row : matrix)
    {
      row.resize(matrix_size);
      std::iota(row.begin(), row.end(), 1.0f);
    }
  }

  // Process each matrix in parallel
  ouly::parallel_for(
   [&](std::vector<std::vector<float>>& matrix, TaskContextType const& ctx)
   {
     std::atomic<uint32_t> elements_in_matrix{0};

     // Process each row of the matrix in parallel
     ouly::parallel_for(
      [&elements_in_matrix](std::vector<float>& row, TaskContextType const&)
      {
        // Process each element in the row
        for (float& element : row)
        {
          element = std::sqrt(element * element + 1.0f); // Some computation
          elements_in_matrix.fetch_add(1, std::memory_order_relaxed);
        }
      },
      matrix, ctx);

     // Wait for row processing to complete and update totals
     while (elements_in_matrix.load() < matrix_size * matrix_size)
     {
       std::this_thread::yield();
     }

     total_processed_elements.fetch_add(matrix_size * matrix_size, std::memory_order_relaxed);
     matrices_completed.fetch_add(1, std::memory_order_relaxed);
   },
   matrices, main_ctx);

  scheduler.end_execution();

  REQUIRE(matrices_completed.load() == num_matrices);
  REQUIRE(total_processed_elements.load() == num_matrices * matrix_size * matrix_size);

  // Verify computation was actually performed
  for (const auto& matrix : matrices)
  {
    for (const auto& row : matrix)
    {
      for (float element : row)
      {
        REQUIRE(element > 1.0f); // Should be modified from original values
      }
    }
  }
}

// Test scheduler behavior under memory pressure
TEMPLATE_TEST_CASE("Memory Pressure and Large Task Queues", "[scheduler][memory][stress][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t large_task_count   = 50000;
  const uint32_t data_size_per_task = 1000;

  std::atomic<uint32_t> completed_tasks{0};
  std::atomic<uint64_t> total_memory_processed{0};

  // Submit many memory-intensive tasks
  for (uint32_t i = 0; i < large_task_count; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&total_memory_processed, &completed_tasks, i](TaskContextType const&)
                     {
                       // Allocate and process some memory
                       std::vector<uint32_t> local_data(data_size_per_task);
                       std::iota(local_data.begin(), local_data.end(), i);

                       uint64_t sum = 0;
                       for (uint32_t value : local_data)
                       {
                         sum += value * value;
                       }

                       total_memory_processed.fetch_add(sum % 1000000, std::memory_order_relaxed);
                       completed_tasks.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(completed_tasks.load() == large_task_count);
  REQUIRE(total_memory_processed.load() > 0);
}

// Test scheduler with simple computational patterns
TEMPLATE_TEST_CASE("Simple Computational Patterns", "[scheduler][mixed][computation][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  SchedulerType scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.create_group(ouly::workgroup_id(1), 2, 2);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> cpu_tasks{0};
  std::atomic<uint32_t> memory_tasks{0};

  const uint32_t tasks_per_type = 50;

  // CPU-intensive tasks
  for (uint32_t i = 0; i < tasks_per_type; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&cpu_tasks](TaskContextType const&)
                     {
                       // Heavy CPU computation
                       volatile uint64_t result = 1;
                       for (uint32_t j = 0; j < 1000; ++j)
                       {
                         result = (result * 31 + j) ^ (result >> 16);
                       }
                       cpu_tasks.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  // Memory-intensive tasks
  for (uint32_t i = 0; i < tasks_per_type; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(1),
                     [&memory_tasks](TaskContextType const&)
                     {
                       // Memory allocation and processing
                       std::vector<uint32_t> data(1000);
                       std::iota(data.begin(), data.end(), 0);

                       volatile uint64_t sum = 0;
                       for (auto value : data)
                       {
                         sum += value;
                       }

                       memory_tasks.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(cpu_tasks.load() == tasks_per_type);
  REQUIRE(memory_tasks.load() == tasks_per_type);
}

// Test scheduler API completeness - workgroup queries and worker information
TEMPLATE_TEST_CASE("Scheduler API Worker Information", "[scheduler][api][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  SchedulerType  scheduler;
  const uint32_t worker_count = 4;
  scheduler.create_group(ouly::workgroup_id(0), 0, worker_count);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> context_checks{0};

  // Test context availability within tasks
  for (uint32_t i = 0; i < 10; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&context_checks](TaskContextType const& ctx)
                     {
                       // Verify we have a valid context
                       (void)ctx; // Use the context parameter
                       context_checks.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(context_checks.load() == 10);
}

// Test scheduler with varying workgroup sizes
TEMPLATE_TEST_CASE("Scheduler Varying Workgroup Sizes", "[scheduler][workgroup][sizes][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  const uint32_t available_workers = std::min(6u, std::thread::hardware_concurrency());
  if (available_workers < 3)
    return; // Skip if not enough workers

  SchedulerType scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 1);                     // Single worker
  scheduler.create_group(ouly::workgroup_id(1), 1, 2);                     // Two workers
  scheduler.create_group(ouly::workgroup_id(2), 3, available_workers - 3); // Remaining workers

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::array<std::atomic<uint32_t>, 3> group_counters{};

  const uint32_t tasks_per_group = 20;

  // Submit tasks to each workgroup
  for (uint32_t group = 0; group < 3; ++group)
  {
    for (uint32_t task = 0; task < tasks_per_group; ++task)
    {
      scheduler.submit(main_ctx, ouly::workgroup_id(group),
                       [&group_counters, group](TaskContextType const&)
                       {
                         // Simulate some work
                         volatile uint32_t work = 0;
                         for (uint32_t i = 0; i < 100; ++i)
                         {
                           work += i;
                         }
                         group_counters[group].fetch_add(1, std::memory_order_relaxed);
                       });
    }
  }

  scheduler.end_execution();

  // Verify all groups completed their tasks
  for (uint32_t group = 0; group < 3; ++group)
  {
    REQUIRE(group_counters[group].load() == tasks_per_group);
  }
}

// Test scheduler error handling and edge cases
TEMPLATE_TEST_CASE("Scheduler Error Handling Edge Cases", "[scheduler][edge][error][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  SchedulerType scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 1);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> successful_tasks{0};

  // Test with tasks that do various edge case operations

  // 1. Task that does nothing
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&successful_tasks](TaskContextType const&)
                   {
                     successful_tasks.fetch_add(1, std::memory_order_relaxed);
                   });

  // 2. Task that only does local computation
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&successful_tasks](TaskContextType const&)
                   {
                     [[maybe_unused]] int local_var = 42;
                     local_var *= 2;
                     successful_tasks.fetch_add(1, std::memory_order_relaxed);
                   });

  // 3. Task that creates and destroys local objects
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&successful_tasks](TaskContextType const&)
                   {
                     {
                       std::vector<int> temp_vec(10, 1);
                       volatile int     sum = 0;
                       for (int val : temp_vec)
                       {
                         sum += val;
                       }
                     }
                     successful_tasks.fetch_add(1, std::memory_order_relaxed);
                   });

  scheduler.end_execution();

  REQUIRE(successful_tasks.load() == 3);
}

// Test task submission patterns and timing
TEMPLATE_TEST_CASE("Task Submission Patterns", "[scheduler][submission][patterns][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using SchedulerType   = typename TestRunner::scheduler_type;
  using TaskContextType = typename TestRunner::task_context_type;

  SchedulerType scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> immediate_tasks{0};
  std::atomic<uint32_t> delayed_tasks{0};

  // Submit immediate tasks
  for (uint32_t i = 0; i < 50; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&immediate_tasks](TaskContextType const&)
                     {
                       immediate_tasks.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  // Submit tasks that schedule more tasks
  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                   [&scheduler, &delayed_tasks](TaskContextType const& ctx)
                   {
                     for (uint32_t i = 0; i < 25; ++i)
                     {
                       scheduler.submit(ctx, ouly::workgroup_id(0),
                                        [&delayed_tasks](TaskContextType const&)
                                        {
                                          delayed_tasks.fetch_add(1, std::memory_order_relaxed);
                                        });
                     }
                   });

  scheduler.end_execution();

  REQUIRE(immediate_tasks.load() == 50);
  REQUIRE(delayed_tasks.load() == 25);
}

// Test auto_parallel_for with ouly::subrange<uint32_t> - Basic functionality
TEMPLATE_TEST_CASE("Auto Parallel For with Subrange Basic", "[scheduler][auto_parallel_for][subrange][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> processed_elements{0};
  std::atomic<uint32_t> sum{0};

  // Create a subrange from 0 to 99
  ouly::subrange<uint32_t> test_range{0, 100};

  // Test range-based lambda with subrange
  ouly::auto_parallel_for(
   [&](uint32_t begin, uint32_t end, TaskContextType const&)
   {
     for (uint32_t i = begin; i < end; ++i)
     {
       sum.fetch_add(i, std::memory_order_relaxed);
       processed_elements.fetch_add(1, std::memory_order_relaxed);
     }
   },
   test_range, main_ctx);

  scheduler.end_execution();

  REQUIRE(processed_elements.load() == 100);
  // Sum of 0 to 99 = 99 * 100 / 2 = 4950
  REQUIRE(sum.load() == 4950);
}

// Test auto_parallel_for with ouly::subrange<uint32_t> - Element-based processing
TEMPLATE_TEST_CASE("Auto Parallel For with Subrange Element Based",
                   "[scheduler][auto_parallel_for][subrange][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> processed_elements{0};
  std::atomic<uint32_t> doubled_sum{0};

  // Create a subrange from 1 to 50
  ouly::subrange<uint32_t> test_range{1, 51};

  // Test element-based lambda with subrange
  ouly::auto_parallel_for(
   [&](uint32_t value, TaskContextType const&)
   {
     doubled_sum.fetch_add(value * 2, std::memory_order_relaxed);
     processed_elements.fetch_add(1, std::memory_order_relaxed);
   },
   test_range, main_ctx);

  scheduler.end_execution();

  REQUIRE(processed_elements.load() == 50);
  // Sum of 1 to 50 doubled = 2 * (50 * 51 / 2) = 2550
  REQUIRE(doubled_sum.load() == 2550);
}

// Test auto_parallel_for with ouly::subrange<uint32_t> - Large range stress test
TEMPLATE_TEST_CASE("Auto Parallel For with Subrange Stress Test",
                   "[scheduler][auto_parallel_for][subrange][stress][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(std::thread::hardware_concurrency());

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  constexpr uint32_t    range_size = 100000;
  std::atomic<uint32_t> processed_elements{0};
  std::atomic<uint64_t> computation_result{0};

  // Create a large subrange
  ouly::subrange<uint32_t> large_range{0, range_size};

  // Test with computationally intensive operations
  ouly::auto_parallel_for(
   [&](uint32_t begin, uint32_t end, TaskContextType const&)
   {
     uint64_t local_result = 0;
     uint32_t local_count  = 0;

     for (uint32_t i = begin; i < end; ++i)
     {
       // Some computation to make work meaningful
       volatile uint32_t temp = i * 13 + 7;
       temp                   = temp ^ (temp >> 8);
       local_result += temp % 1000;
       local_count++;
     }

     computation_result.fetch_add(local_result, std::memory_order_relaxed);
     processed_elements.fetch_add(local_count, std::memory_order_relaxed);
   },
   large_range, main_ctx);

  scheduler.end_execution();

  REQUIRE(processed_elements.load() == range_size);
  REQUIRE(computation_result.load() > 0);
}

// Test auto_parallel_for with ouly::subrange<uint32_t> - Empty range edge case
TEMPLATE_TEST_CASE("Auto Parallel For with Subrange Empty Range",
                   "[scheduler][auto_parallel_for][subrange][edge][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(2);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> processed_elements{0};

  // Create an empty subrange
  ouly::subrange<uint32_t> empty_range{10, 10};

  // Test with empty range
  ouly::auto_parallel_for(
   [&](uint32_t begin, uint32_t end, TaskContextType const&)
   {
     for (uint32_t i = begin; i < end; ++i)
     {
       processed_elements.fetch_add(1, std::memory_order_relaxed);
     }
   },
   empty_range, main_ctx);

  scheduler.end_execution();

  REQUIRE(processed_elements.load() == 0);
}

// Test auto_parallel_for with ouly::subrange<uint32_t> - Single element range
TEMPLATE_TEST_CASE("Auto Parallel For with Subrange Single Element",
                   "[scheduler][auto_parallel_for][subrange][edge][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(2);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  std::atomic<uint32_t> processed_elements{0};
  std::atomic<uint32_t> result_value{0};

  // Create a single element subrange
  ouly::subrange<uint32_t> single_range{42, 43};

  // Test with single element range
  ouly::auto_parallel_for(
   [&](uint32_t value, TaskContextType const&)
   {
     result_value.store(value, std::memory_order_relaxed);
     processed_elements.fetch_add(1, std::memory_order_relaxed);
   },
   single_range, main_ctx);

  scheduler.end_execution();

  REQUIRE(processed_elements.load() == 1);
  REQUIRE(result_value.load() == 42);
}

// Test auto_parallel_for with ouly::subrange<uint32_t> - Nested in async tasks
TEMPLATE_TEST_CASE("Auto Parallel For with Subrange in Async Tasks",
                   "[scheduler][auto_parallel_for][subrange][async][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner      = TestType;
  using TaskContextType = typename TestRunner::task_context_type;

  auto scheduler = TestRunner::setup_scheduler(6);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  const uint32_t outer_task_count    = 5;
  const uint32_t range_size_per_task = 1000;

  std::atomic<uint32_t> completed_outer_tasks{0};
  std::atomic<uint32_t> total_processed_elements{0};

  for (uint32_t task_id = 0; task_id < outer_task_count; ++task_id)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&, task_id](TaskContextType const& ctx)
                     {
                       // Create a subrange for this task
                       uint32_t                 start = task_id * range_size_per_task;
                       uint32_t                 end   = start + range_size_per_task;
                       ouly::subrange<uint32_t> task_range{start, end};

                       std::atomic<uint32_t> task_processed{0};

                       // Execute auto_parallel_for within the async task
                       ouly::auto_parallel_for(
                        [&task_processed](uint32_t begin, uint32_t end_val, TaskContextType const&)
                        {
                          for (uint32_t i = begin; i < end_val; ++i)
                          {
                            volatile uint32_t computation = i * 2 + 1;
                            (void)computation; // Prevent optimization
                            task_processed.fetch_add(1, std::memory_order_relaxed);
                          }
                        },
                        task_range, ctx);

                       // Verify all elements were processed in this task
                       if (task_processed.load() == range_size_per_task)
                       {
                         total_processed_elements.fetch_add(range_size_per_task, std::memory_order_relaxed);
                         completed_outer_tasks.fetch_add(1, std::memory_order_relaxed);
                       }
                     });
  }

  scheduler.end_execution();

  REQUIRE(completed_outer_tasks.load() == outer_task_count);
  REQUIRE(total_processed_elements.load() == outer_task_count * range_size_per_task);
}

// NOLINTEND
