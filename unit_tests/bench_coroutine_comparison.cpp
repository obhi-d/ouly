// SPDX-License-Identifier: MIT
#define ANKERL_NANOBENCH_IMPLEMENT
#define GLM_ENABLE_EXPERIMENTAL

#include "nanobench.h"
#include "ouly/scheduler/co_task.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include "ouly/scheduler/v1/scheduler.hpp"
#include "ouly/scheduler/v1/task_context.hpp"
#include "ouly/scheduler/v2/scheduler.hpp"
#include "ouly/scheduler/v2/task_context.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Include GLM for mathematical operations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Include TBB for comparison
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_arena.h>
#include <tbb/tbb.h>

// NOLINTBEGIN
// Constants for coroutine benchmark configuration
namespace coroutine_benchmark_config
{
constexpr uint32_t TASK_COUNT_SMALL    = 1000U;
constexpr uint32_t TASK_COUNT_MEDIUM   = 10000U;
constexpr uint32_t TASK_COUNT_LARGE    = 100000U;
constexpr uint32_t WORK_INTENSITY_LOW  = 10U;
constexpr uint32_t WORK_INTENSITY_HIGH = 1000U;
constexpr uint32_t CHAIN_LENGTH_SHORT  = 5U;
constexpr uint32_t CHAIN_LENGTH_MEDIUM = 20U;
constexpr uint32_t CHAIN_LENGTH_LONG   = 100U;
constexpr float    VECTOR_INCREMENT    = 0.1F;
constexpr float    SCALE_FACTOR        = 1.01F;
constexpr uint32_t NESTED_DEPTH        = 5U;
constexpr uint32_t BATCH_SIZE          = 100U;
} // namespace coroutine_benchmark_config

// Benchmark data structures for coroutine tests
struct CoroutineBenchmarkData
{
  std::vector<glm::vec3> vectors;
  std::vector<glm::mat4> matrices;
  std::vector<float>     scalar_data;
  std::vector<uint32_t>  integer_data;
  std::atomic<uint64_t>  result{0};

  explicit CoroutineBenchmarkData(size_t size) : vectors(size), matrices(size), scalar_data(size), integer_data(size)
  {
    std::random_device                      rd;
    std::mt19937                            gen(rd());
    std::uniform_real_distribution<float>   float_dist(0.0F, 100.0F);
    std::uniform_int_distribution<uint32_t> int_dist(1, 1000000);

    for (size_t i = 0; i < size; ++i)
    {
      const auto fi   = static_cast<float>(i);
      vectors[i]      = glm::vec3(fi, fi + 1.0F, fi + 2.0F);
      matrices[i]     = glm::translate(glm::mat4(1.0F), glm::vec3(fi));
      scalar_data[i]  = fi * coroutine_benchmark_config::VECTOR_INCREMENT;
      integer_data[i] = static_cast<uint32_t>(i);
    }
  }
};

// Computation kernels for coroutine benchmarks
struct CoroutineComputationKernels
{
  static void vector_operations(glm::vec3& vec)
  {
    vec = glm::normalize(vec);
    vec = glm::cross(vec, glm::vec3(1.0F, 0.0F, 0.0F));
    vec += glm::vec3(coroutine_benchmark_config::VECTOR_INCREMENT);
    vec *= coroutine_benchmark_config::SCALE_FACTOR;
  }

  static void intensive_computation(uint32_t iterations, float& result)
  {
    glm::vec3 vec(result, result + 1.0F, result + 2.0F);
    for (uint32_t i = 0; i < iterations; ++i)
    {
      vector_operations(vec);
    }
    result = glm::length(vec);
  }

  static void minimal_work(uint32_t& value)
  {
    value = value * 31U + 17U;
    value ^= (value >> 16U);
  }
};

// Simple coroutine task implementations
namespace simple_coroutines
{

// Basic computation coroutine
ouly::co_task<float> compute_task(float input, uint32_t work_intensity)
{
  float result = input;
  CoroutineComputationKernels::intensive_computation(work_intensity, result);
  co_return result;
}

// Chain of dependent coroutines
ouly::co_task<float> chain_task(float input, uint32_t remaining_depth, uint32_t work_intensity)
{
  float result = input;
  CoroutineComputationKernels::intensive_computation(work_intensity, result);

  if (remaining_depth > 0)
  {
    auto next_task = chain_task(result, remaining_depth - 1, work_intensity);
    result         = co_await next_task;
  }

  co_return result;
}

// Parallel coroutine execution
ouly::co_task<void> parallel_coroutine_batch(std::span<float> data, uint32_t work_intensity,
                                             std::atomic<uint64_t>& result_accumulator)
{
  for (auto& value : data)
  {
    CoroutineComputationKernels::intensive_computation(work_intensity, value);
    result_accumulator.fetch_add(static_cast<uint64_t>(value * 1000.0F), std::memory_order_relaxed);
  }
  co_return;
}

// Fan-out/fan-in pattern
ouly::co_task<float> fan_out_task(std::span<float> data, uint32_t work_intensity)
{
  std::vector<ouly::co_task<float>> tasks;
  tasks.reserve(data.size());

  // Fan out: create tasks for each element
  for (auto& value : data)
  {
    tasks.emplace_back(compute_task(value, work_intensity));
  }

  // Fan in: await all results
  float total = 0.0F;
  for (auto& task : tasks)
  {
    total += co_await task;
  }

  co_return total;
}

// Nested coroutine with suspension points
ouly::co_task<float> nested_suspend_task(float input, uint32_t nesting_level)
{
  float result = input;

  for (uint32_t i = 0; i < nesting_level; ++i)
  {
    // Simulate suspension point with nested coroutine
    auto nested = compute_task(result, coroutine_benchmark_config::WORK_INTENSITY_LOW);
    result      = co_await nested;

    // Additional computation after suspension
    CoroutineComputationKernels::intensive_computation(coroutine_benchmark_config::WORK_INTENSITY_LOW, result);
  }

  co_return result;
}

// Sequence-based coroutine (immediately starts execution)
ouly::co_sequence<float> sequence_compute(float input, uint32_t work_intensity)
{
  float result = input;
  CoroutineComputationKernels::intensive_computation(work_intensity, result);
  co_return result;
}

// Chain of sequence coroutines
ouly::co_sequence<float> sequence_chain(float input, uint32_t chain_length, uint32_t work_intensity)
{
  float result = input;

  for (uint32_t i = 0; i < chain_length; ++i)
  {
    auto seq = sequence_compute(result, work_intensity);
    result   = co_await seq;
  }

  co_return result;
}

} // namespace simple_coroutines

// Coroutine overhead benchmark framework
template <typename SchedulerType, typename TaskContextType>
class CoroutineOverheadBenchmark
{
public:
  using scheduler_type    = SchedulerType;
  using task_context_type = TaskContextType;

  // Measure coroutine creation/destruction overhead
  static void run_coroutine_creation_overhead(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    bench.run(std::string("CoroutineCreation_") + name_suffix,
              [=]()
              {
                std::vector<ouly::co_task<float>> tasks;
                tasks.reserve(coroutine_benchmark_config::TASK_COUNT_MEDIUM);

                for (uint32_t i = 0; i < coroutine_benchmark_config::TASK_COUNT_MEDIUM; ++i)
                {
                  tasks.emplace_back(simple_coroutines::compute_task(static_cast<float>(i),
                                                                     coroutine_benchmark_config::WORK_INTENSITY_LOW));
                }

                // Let tasks destruct naturally
                ankerl::nanobench::doNotOptimizeAway(tasks.data());
              });
  }

  // Compare coroutine vs regular task submission overhead
  static void run_submission_overhead_comparison(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    // Coroutine submission benchmark
    bench.run(std::string("CoroutineSubmission_") + name_suffix,
              [&main_ctx]()
              {
                std::atomic<uint32_t> counter{0};

                for (uint32_t i = 0; i < coroutine_benchmark_config::TASK_COUNT_SMALL; ++i)
                {
                  auto task = simple_coroutines::compute_task(static_cast<float>(i),
                                                              coroutine_benchmark_config::WORK_INTENSITY_LOW);
                  ouly::async(main_ctx, ouly::workgroup_id(0), std::move(task));
                  counter.fetch_add(1, std::memory_order_relaxed);
                }

                // Wait for completion
                main_ctx.get_scheduler().wait_for_tasks();
              });

    // Regular lambda submission benchmark for comparison
    bench.run(std::string("LambdaSubmission_") + name_suffix,
              [&main_ctx]()
              {
                std::atomic<uint32_t> counter{0};

                for (uint32_t i = 0; i < coroutine_benchmark_config::TASK_COUNT_SMALL; ++i)
                {
                  ouly::async(main_ctx, ouly::workgroup_id(0),
                              [i, &counter](const task_context_type&)
                              {
                                auto result = static_cast<float>(i);
                                CoroutineComputationKernels::intensive_computation(
                                 coroutine_benchmark_config::WORK_INTENSITY_LOW, result);
                                counter.fetch_add(1, std::memory_order_relaxed);
                                ankerl::nanobench::doNotOptimizeAway(result);
                              });
                }

                // Wait for completion
                main_ctx.get_scheduler().wait_for_tasks();
              });

    teardown_scheduler(scheduler);
  }

  // Measure suspension/resumption overhead
  static void run_suspension_overhead(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    bench.run(std::string("SuspensionOverhead_") + name_suffix,
              [&main_ctx]()
              {
                auto task = simple_coroutines::chain_task(1.0F, coroutine_benchmark_config::CHAIN_LENGTH_MEDIUM,
                                                          coroutine_benchmark_config::WORK_INTENSITY_LOW);

                ouly::async(main_ctx, ouly::workgroup_id(0), std::move(task));

                // Wait for chain completion
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
              });

    teardown_scheduler(scheduler);
  }

  // Coroutine memory overhead
  static void run_memory_overhead(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    bench.run(std::string("CoroutineMemory_") + name_suffix,
              [=]()
              {
                std::vector<ouly::co_task<float>> tasks;
                tasks.reserve(coroutine_benchmark_config::TASK_COUNT_LARGE);

                // Create many coroutines to measure memory overhead
                for (uint32_t i = 0; i < coroutine_benchmark_config::TASK_COUNT_LARGE; ++i)
                {
                  tasks.emplace_back(simple_coroutines::nested_suspend_task(static_cast<float>(i),
                                                                            coroutine_benchmark_config::NESTED_DEPTH));
                }

                ankerl::nanobench::doNotOptimizeAway(tasks.data());
              });
  }

private:
  static auto setup_scheduler() -> scheduler_type
  {
    scheduler_type scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());
    scheduler.begin_execution();
    return scheduler;
  }

  static void teardown_scheduler(scheduler_type& scheduler)
  {
    scheduler.end_execution();
  }

  static auto get_main_context() -> const task_context_type&
  {
    return task_context_type::this_context::get();
  }
};

// Performance comparison benchmarks (coroutines vs regular tasks)
template <typename SchedulerType, typename TaskContextType>
class CoroutinePerformanceBenchmark
{
public:
  using scheduler_type    = SchedulerType;
  using task_context_type = TaskContextType;

  // Parallel computation: coroutines vs regular tasks
  static void run_parallel_computation_comparison(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    constexpr size_t       DATA_SIZE = 10000;
    CoroutineBenchmarkData data(DATA_SIZE);

    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    // Coroutine-based parallel computation
    bench.run(std::string("ParallelCompute_Coroutines_") + name_suffix,
              [&data, &main_ctx]()
              {
                std::vector<ouly::co_task<void>> tasks;
                tasks.reserve(data.vectors.size() / coroutine_benchmark_config::BATCH_SIZE);

                data.result.store(0, std::memory_order_relaxed);

                // Batch work into coroutines
                for (size_t i = 0; i < data.scalar_data.size(); i += coroutine_benchmark_config::BATCH_SIZE)
                {
                  const size_t end_idx = std::min(i + coroutine_benchmark_config::BATCH_SIZE, data.scalar_data.size());
                  auto         batch_span = std::span<float>(data.scalar_data.data() + i, end_idx - i);

                  tasks.emplace_back(simple_coroutines::parallel_coroutine_batch(
                   batch_span, coroutine_benchmark_config::WORK_INTENSITY_HIGH, data.result));

                  ouly::async(main_ctx, ouly::workgroup_id(0), std::move(tasks.back()));
                }

                // Wait for all coroutines to complete
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                ankerl::nanobench::doNotOptimizeAway(data.result.load());
              });

    // Regular task-based parallel computation for comparison
    bench.run(std::string("ParallelCompute_RegularTasks_") + name_suffix,
              [&data, &main_ctx]()
              {
                data.result.store(0, std::memory_order_relaxed);

                ouly::auto_parallel_for(
                 [&data](float& value, const task_context_type&)
                 {
                   CoroutineComputationKernels::intensive_computation(coroutine_benchmark_config::WORK_INTENSITY_HIGH,
                                                                      value);
                   data.result.fetch_add(static_cast<uint64_t>(value * 1000.0F), std::memory_order_relaxed);
                 },
                 data.scalar_data, main_ctx);

                ankerl::nanobench::doNotOptimizeAway(data.result.load());
              });

    teardown_scheduler(scheduler);
  }

  // Task chaining: coroutines vs callback-style
  static void run_task_chaining_comparison(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    // Coroutine-based chaining
    bench.run(std::string("TaskChaining_Coroutines_") + name_suffix,
              [&main_ctx]()
              {
                std::vector<ouly::co_task<float>> chains;
                chains.reserve(coroutine_benchmark_config::TASK_COUNT_SMALL);

                for (uint32_t i = 0; i < coroutine_benchmark_config::TASK_COUNT_SMALL; ++i)
                {
                  chains.emplace_back(simple_coroutines::chain_task(static_cast<float>(i),
                                                                    coroutine_benchmark_config::CHAIN_LENGTH_SHORT,
                                                                    coroutine_benchmark_config::WORK_INTENSITY_LOW));

                  ouly::async(main_ctx, ouly::workgroup_id(0), std::move(chains.back()));
                }

                // Wait for all chains to complete
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
              });

    // Regular nested task submission for comparison
    bench.run(
     std::string("TaskChaining_NestedSubmission_") + name_suffix,
     [&main_ctx]()
     {
       std::atomic<uint32_t> completed_chains{0};

       for (uint32_t i = 0; i < coroutine_benchmark_config::TASK_COUNT_SMALL; ++i)
       {
         // Simulate chaining with nested lambda submissions
         auto chain_lambda = [&main_ctx, &completed_chains, i](auto&& self, float value, uint32_t remaining) -> void
         {
           float result = value;
           CoroutineComputationKernels::intensive_computation(coroutine_benchmark_config::WORK_INTENSITY_LOW, result);

           if (remaining > 0)
           {
             ouly::async(main_ctx, ouly::workgroup_id(0),
                         [self, result, remaining, &completed_chains](const task_context_type&)
                         {
                           self(self, result, remaining - 1);
                         });
           }
           else
           {
             completed_chains.fetch_add(1, std::memory_order_relaxed);
           }
         };

         ouly::async(main_ctx, ouly::workgroup_id(0),
                     [chain_lambda, i](const task_context_type&)
                     {
                       chain_lambda(chain_lambda, static_cast<float>(i),
                                    coroutine_benchmark_config::CHAIN_LENGTH_SHORT);
                     });
       }

       // Wait for all chains to complete
       while (completed_chains.load() < coroutine_benchmark_config::TASK_COUNT_SMALL)
       {
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
       }
     });

    teardown_scheduler(scheduler);
  }

  // Fan-out/fan-in pattern comparison
  static void run_fan_out_in_comparison(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    constexpr size_t   FAN_OUT_SIZE = 100;
    std::vector<float> data(FAN_OUT_SIZE);
    std::iota(data.begin(), data.end(), 1.0F);

    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    // Coroutine-based fan-out/fan-in
    bench.run(std::string("FanOutIn_Coroutines_") + name_suffix,
              [&data, &main_ctx]()
              {
                auto fan_out_task = simple_coroutines::fan_out_task(std::span<float>(data),
                                                                    coroutine_benchmark_config::WORK_INTENSITY_LOW);

                ouly::async(main_ctx, ouly::workgroup_id(0), std::move(fan_out_task));

                // Wait for fan-out/fan-in completion
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
              });

    // Regular parallel_for equivalent
    bench.run(std::string("FanOutIn_ParallelFor_") + name_suffix,
              [&data, &main_ctx]()
              {
                std::atomic<float> total{0.0F};

                ouly::auto_parallel_for(
                 [&total](float& value, const task_context_type&)
                 {
                   CoroutineComputationKernels::intensive_computation(coroutine_benchmark_config::WORK_INTENSITY_LOW,
                                                                      value);
                   float current_total = total.load(std::memory_order_relaxed);
                   while (!total.compare_exchange_weak(current_total, current_total + value, std::memory_order_relaxed))
                   {
                   }
                 },
                 data, main_ctx);

                ankerl::nanobench::doNotOptimizeAway(total.load());
              });

    teardown_scheduler(scheduler);
  }

private:
  static auto setup_scheduler() -> scheduler_type
  {
    scheduler_type scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());
    scheduler.begin_execution();
    return scheduler;
  }

  static void teardown_scheduler(scheduler_type& scheduler)
  {
    scheduler.end_execution();
  }

  static auto get_main_context() -> const task_context_type&
  {
    return task_context_type::this_context::get();
  }
};

// co_task vs co_sequence comparison
template <typename SchedulerType, typename TaskContextType>
class CoTaskVsCoSequenceBenchmark
{
public:
  using scheduler_type    = SchedulerType;
  using task_context_type = TaskContextType;

  // Compare co_task (suspended start) vs co_sequence (immediate start)
  static void run_startup_behavior_comparison(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    // co_task (suspended start) benchmark
    bench.run(std::string("CoTask_SuspendedStart_") + name_suffix,
              [&main_ctx]()
              {
                std::vector<ouly::co_task<float>> tasks;
                tasks.reserve(coroutine_benchmark_config::TASK_COUNT_MEDIUM);

                for (uint32_t i = 0; i < coroutine_benchmark_config::TASK_COUNT_MEDIUM; ++i)
                {
                  tasks.emplace_back(simple_coroutines::compute_task(static_cast<float>(i),
                                                                     coroutine_benchmark_config::WORK_INTENSITY_LOW));

                  ouly::async(main_ctx, ouly::workgroup_id(0), std::move(tasks.back()));
                }

                // Wait for completion
                main_ctx.get_scheduler().wait_for_tasks();
              });

    // co_sequence (immediate start) benchmark
    bench.run(std::string("CoSequence_ImmediateStart_") + name_suffix,
              [&main_ctx]()
              {
                std::vector<ouly::co_sequence<float>> sequences;
                sequences.reserve(coroutine_benchmark_config::TASK_COUNT_MEDIUM);

                for (uint32_t i = 0; i < coroutine_benchmark_config::TASK_COUNT_MEDIUM; ++i)
                {
                  sequences.emplace_back(simple_coroutines::sequence_compute(
                   static_cast<float>(i), coroutine_benchmark_config::WORK_INTENSITY_LOW));

                  ouly::async(main_ctx, ouly::workgroup_id(0), std::move(sequences.back()));
                }

                // Wait for completion
                main_ctx.get_scheduler().wait_for_tasks();
              });

    teardown_scheduler(scheduler);
  }

  // Compare chaining behavior
  static void run_chaining_behavior_comparison(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    // co_task chaining
    bench.run(std::string("CoTask_Chaining_") + name_suffix,
              [&main_ctx]()
              {
                auto task = simple_coroutines::chain_task(1.0F, coroutine_benchmark_config::CHAIN_LENGTH_MEDIUM,
                                                          coroutine_benchmark_config::WORK_INTENSITY_LOW);

                ouly::async(main_ctx, ouly::workgroup_id(0), std::move(task));

                // Wait for chain completion
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
              });

    // co_sequence chaining
    bench.run(std::string("CoSequence_Chaining_") + name_suffix,
              [&main_ctx]()
              {
                auto sequence = simple_coroutines::sequence_chain(1.0F, coroutine_benchmark_config::CHAIN_LENGTH_MEDIUM,
                                                                  coroutine_benchmark_config::WORK_INTENSITY_LOW);

                ouly::async(main_ctx, ouly::workgroup_id(0), std::move(sequence));

                // Wait for chain completion
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
              });

    teardown_scheduler(scheduler);
  }

private:
  static auto setup_scheduler() -> scheduler_type
  {
    scheduler_type scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());
    scheduler.begin_execution();
    return scheduler;
  }

  static void teardown_scheduler(scheduler_type& scheduler)
  {
    scheduler.end_execution();
  }

  static auto get_main_context() -> const task_context_type&
  {
    return task_context_type::this_context::get();
  }
};

// TBB comparison for coroutine-style workflows
class TBBCoroutineStyleBenchmarks
{
public:
  // TBB equivalent of coroutine chaining (using nested parallel_for)
  static void run_tbb_chaining_equivalent(ankerl::nanobench::Bench& bench)
  {
    bench.run("TaskChaining_TBB_Equivalent",
              []()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                std::vector<float> chain_results(coroutine_benchmark_config::TASK_COUNT_SMALL);
                std::iota(chain_results.begin(), chain_results.end(), 1.0F);

                // Simulate chaining with multiple sequential parallel_for calls
                for (uint32_t depth = 0; depth < coroutine_benchmark_config::CHAIN_LENGTH_SHORT; ++depth)
                {
                  tbb::parallel_for(static_cast<size_t>(0), chain_results.size(),
                                    [&chain_results](size_t i)
                                    {
                                      CoroutineComputationKernels::intensive_computation(
                                       coroutine_benchmark_config::WORK_INTENSITY_LOW, chain_results[i]);
                                    });
                }

                ankerl::nanobench::doNotOptimizeAway(chain_results.data());
              });
  }

  // TBB equivalent of fan-out/fan-in
  static void run_tbb_fan_out_in_equivalent(ankerl::nanobench::Bench& bench)
  {
    constexpr size_t   FAN_OUT_SIZE = 100;
    std::vector<float> data(FAN_OUT_SIZE);
    std::iota(data.begin(), data.end(), 1.0F);

    bench.run("FanOutIn_TBB_Equivalent",
              [&data]()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                std::atomic<float> total{0.0F};

                tbb::parallel_for(
                 static_cast<size_t>(0), data.size(),
                 [&data, &total](size_t i)
                 {
                   float value = data[i];
                   CoroutineComputationKernels::intensive_computation(coroutine_benchmark_config::WORK_INTENSITY_LOW,
                                                                      value);

                   float current_total = total.load(std::memory_order_relaxed);
                   while (!total.compare_exchange_weak(current_total, current_total + value, std::memory_order_relaxed))
                   {
                   }
                 });

                ankerl::nanobench::doNotOptimizeAway(total.load());
              });
  }
};

// Utility functions for coroutine benchmark reporting
class CoroutineBenchmarkReporter
{
private:
  static std::string get_compiler_info()
  {
    std::string compiler = "unknown";
    std::string version  = "unknown";

#ifdef __GNUC__
    compiler = "gcc";
    version  = std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#elif defined(__clang__)
    compiler = "clang";
    version  = std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif defined(_MSC_VER)
    compiler = "msvc";
    version  = std::to_string(_MSC_VER);
#endif

    return compiler + "-" + version;
  }

  static std::string get_timestamp()
  {
    const auto now    = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return oss.str();
  }

public:
  static void save_results(ankerl::nanobench::Bench& bench, const std::string& test_id)
  {
    const std::string compiler      = get_compiler_info();
    const std::string timestamp     = get_timestamp();
    const std::string json_filename = "coroutine_" + test_id + "_" + compiler + "_" + timestamp + ".json";
    const std::string txt_filename  = "coroutine_" + test_id + "_" + compiler + "_" + timestamp + ".txt";

    std::cout << "TEST_ID: " << test_id << std::endl;

    std::ofstream json_file(json_filename);
    if (json_file.is_open())
    {
      bench.render(ankerl::nanobench::templates::json(), json_file);
      json_file.close();
      std::cout << "✅ JSON results saved to: " << json_filename << std::endl;
    }

    std::ofstream txt_file(txt_filename);
    if (txt_file.is_open())
    {
      bench.render(
       "{{#result}}{{name}}: {{median(elapsed)}} seconds ({{median(instructions)}} instructions)\n{{/result}}",
       txt_file);
      txt_file.close();
      std::cout << "📄 Text results saved to: " << txt_filename << std::endl;
    }
  }

  static void print_system_info()
  {
    std::cout << "🖥️  System Information:" << std::endl;
    std::cout << "   Hardware Concurrency: " << std::thread::hardware_concurrency() << " threads" << std::endl;
    std::cout << "   Compiler: " << get_compiler_info() << std::endl;
    std::cout << "   Timestamp: " << get_timestamp() << std::endl;
    std::cout << std::endl;
  }
};

// Main coroutine benchmark execution
void run_coroutine_benchmarks(int benchmark_set = -1)
{
  std::cout << "🚀 OULY Coroutine Performance Benchmarks" << std::endl;
  std::cout << "=========================================" << std::endl;

  CoroutineBenchmarkReporter::print_system_info();

  // Configure nanobench for high precision
  ankerl::nanobench::Bench bench;
  bench.title("Coroutine Performance Comparison")
   .unit("operation")
   .warmup(3)
   .epochIterations(10)
   .minEpochIterations(5)
   .relative(true);

  if (benchmark_set < 0 || benchmark_set == 0)
  {
    std::cout << "📊 Running Coroutine Overhead Benchmarks..." << std::endl;

    CoroutineOverheadBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_coroutine_creation_overhead(bench,
                                                                                                             "V1");
    CoroutineOverheadBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_coroutine_creation_overhead(bench,
                                                                                                             "V2");

    CoroutineOverheadBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_submission_overhead_comparison(bench,
                                                                                                                "V1");
    CoroutineOverheadBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_submission_overhead_comparison(bench,
                                                                                                                "V2");

    CoroutineOverheadBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_suspension_overhead(bench, "V1");
    CoroutineOverheadBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_suspension_overhead(bench, "V2");

    CoroutineOverheadBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_memory_overhead(bench, "V1");
    CoroutineOverheadBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_memory_overhead(bench, "V2");

    CoroutineBenchmarkReporter::save_results(bench, "overhead_comparison");
    bench = ankerl::nanobench::Bench{};
    bench.title("Coroutine Performance Comparison")
     .unit("operation")
     .warmup(3)
     .epochIterations(10)
     .minEpochIterations(5)
     .relative(true);
  }

  if (benchmark_set < 0 || benchmark_set == 1)
  {
    std::cout << "🔄 Running Coroutine vs Regular Task Performance..." << std::endl;

    CoroutinePerformanceBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_parallel_computation_comparison(
     bench, "V1");
    CoroutinePerformanceBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_parallel_computation_comparison(
     bench, "V2");

    CoroutinePerformanceBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_task_chaining_comparison(bench,
                                                                                                             "V1");
    CoroutinePerformanceBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_task_chaining_comparison(bench,
                                                                                                             "V2");

    CoroutinePerformanceBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_fan_out_in_comparison(bench, "V1");
    CoroutinePerformanceBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_fan_out_in_comparison(bench, "V2");

    TBBCoroutineStyleBenchmarks::run_tbb_chaining_equivalent(bench);
    TBBCoroutineStyleBenchmarks::run_tbb_fan_out_in_equivalent(bench);

    CoroutineBenchmarkReporter::save_results(bench, "performance_comparison");
    bench = ankerl::nanobench::Bench{};
    bench.title("Coroutine Performance Comparison")
     .unit("operation")
     .warmup(3)
     .epochIterations(10)
     .minEpochIterations(5)
     .relative(true);
  }

  if (benchmark_set < 0 || benchmark_set == 2)
  {
    std::cout << "⚡ Running co_task vs co_sequence Comparison..." << std::endl;

    CoTaskVsCoSequenceBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_startup_behavior_comparison(bench,
                                                                                                              "V1");
    CoTaskVsCoSequenceBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_startup_behavior_comparison(bench,
                                                                                                              "V2");

    CoTaskVsCoSequenceBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_chaining_behavior_comparison(bench,
                                                                                                               "V1");
    CoTaskVsCoSequenceBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_chaining_behavior_comparison(bench,
                                                                                                               "V2");

    CoroutineBenchmarkReporter::save_results(bench, "cotask_vs_cosequence");
  }

  std::cout << "✅ All coroutine benchmarks completed!" << std::endl;
}

// Main function to run all benchmarks
int main(int argc, char* argv[])
{
  int benchmark_set = -1; // Run all by default

  if (argc > 1)
  {
    benchmark_set = std::stoi(argv[1]);
  }

  try
  {
    run_coroutine_benchmarks(benchmark_set);
  }
  catch (const std::exception& e)
  {
    std::cerr << "❌ Benchmark failed with exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

// NOLINTEND