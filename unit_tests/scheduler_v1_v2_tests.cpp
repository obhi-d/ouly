// SPDX-License-Identifier: MIT
#define GLM_ENABLE_EXPERIMENTAL
#include "catch2/catch_all.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include "ouly/scheduler/scheduler_v1.hpp"
#include "ouly/scheduler/scheduler_v2.hpp"
#include "ouly/scheduler/task_context_v1.hpp"
#include "ouly/scheduler/task_context_v2.hpp"
#include <atomic>
#include <chrono>
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
  std::atomic<uint32_t> total_operations{0};
  std::atomic<uint64_t> computation_result{0};
};

// Helper to create a main context for v1 scheduler
namespace ouly::v1
{
inline auto make_main_context(scheduler& sched, workgroup_id group = workgroup_id(0)) -> task_context
{
  return task_context(sched, nullptr, worker_id(0), group, 0xFFFFFFFF, 0);
}
} // namespace ouly::v1

// Helper to create a main context for v2 scheduler
namespace ouly::v2
{
inline auto make_main_context(scheduler& sched, [[maybe_unused]] workgroup_id group = workgroup_id(0)) -> task_context
{
  auto ctx = task_context(sched, nullptr, 0, worker_id(0));
  // v2 task_context doesn't have set_workgroup, the group is set internally
  return ctx;
}
} // namespace ouly::v2

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

  static auto get_main_context(SchedulerType& scheduler, ouly::workgroup_id group = ouly::workgroup_id(0))
   -> TaskContextType
  {
    if constexpr (std::is_same_v<SchedulerType, ouly::v1::scheduler>)
    {
      return ouly::v1::make_main_context(scheduler, group);
    }
    else
    {
      return ouly::v2::make_main_context(scheduler, group);
    }
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
  auto main_ctx  = TestRunner::get_main_context(scheduler);

  scheduler.begin_execution();

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
  auto main_ctx  = TestRunner::get_main_context(scheduler);

  scheduler.begin_execution();

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
  auto main_ctx  = TestRunner::get_main_context(scheduler);

  scheduler.begin_execution();

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

                       counter.total_operations.fetch_add(3, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

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
  auto main_ctx  = TestRunner::get_main_context(scheduler);

  scheduler.begin_execution();

  const uint32_t     task_count            = 1000;
  constexpr uint32_t computation_intensity = 10000;

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

  auto main_ctx = TestRunner::get_main_context(scheduler, ouly::workgroup_id(0));

  scheduler.begin_execution();

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
  auto main_ctx  = TestRunner::get_main_context(scheduler);

  scheduler.begin_execution();

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

// NOLINTEND
