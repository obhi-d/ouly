// SPDX-License-Identifier: MIT
#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include "ouly/scheduler/scheduler_v1.hpp"
#include "ouly/scheduler/scheduler_v2.hpp"
#include "ouly/scheduler/task_context_v1.hpp"
#include "ouly/scheduler/task_context_v2.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
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

// Helper to create main contexts
namespace ouly::v1
{
inline auto make_main_context(scheduler& sched, workgroup_id group = workgroup_id(0)) -> task_context
{
  return task_context(sched, nullptr, worker_id(0), group, 0xFFFFFFFF, 0);
}
} // namespace ouly::v1

namespace ouly::v2
{
inline auto make_main_context(scheduler& sched, [[maybe_unused]] workgroup_id group = workgroup_id(0)) -> task_context
{
  auto ctx = task_context(sched, nullptr, 0, worker_id(0));
  return ctx;
}
} // namespace ouly::v2

// Benchmark data structures
struct BenchmarkData
{
  std::vector<glm::vec3> vectors;
  std::vector<glm::mat4> matrices;
  std::vector<float>     scalar_data;
  std::vector<uint32_t>  integer_data;

  BenchmarkData(size_t size) : vectors(size), matrices(size), scalar_data(size), integer_data(size)
  {
    // Initialize with meaningful data
    for (size_t i = 0; i < size; ++i)
    {
      vectors[i]      = glm::vec3(static_cast<float>(i), static_cast<float>(i + 1), static_cast<float>(i + 2));
      matrices[i]     = glm::translate(glm::mat4(1.0f), glm::vec3(static_cast<float>(i)));
      scalar_data[i]  = static_cast<float>(i * 0.1f);
      integer_data[i] = static_cast<uint32_t>(i);
    }
  }
};

// Simple computation kernels for consistent benchmarking
struct ComputationKernels
{
  static void vector_operations(glm::vec3& vec)
  {
    vec = glm::normalize(vec);
    vec = glm::cross(vec, glm::vec3(1.0f, 0.0f, 0.0f));
    vec += glm::vec3(0.1f);
    vec *= 1.01f;
  }

  static void matrix_operations(glm::mat4& matrix)
  {
    matrix = glm::rotate(matrix, glm::radians(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    matrix = glm::scale(matrix, glm::vec3(1.001f));
    matrix = glm::translate(matrix, glm::vec3(0.01f));
  }

  static void mixed_computation(uint32_t& integer, float& scalar, glm::vec3& vec)
  {
    // Integer operations
    integer = integer * 31 + 17;
    integer ^= (integer >> 16);

    // Scalar operations
    scalar = std::sin(scalar) * std::cos(scalar * 2.0f);
    scalar += 0.001f;

    // Vector operations
    vector_operations(vec);
  }
};

// Benchmark class template for scheduler testing
template <typename SchedulerType, typename TaskContextType>
class SchedulerBenchmark
{
public:
  using scheduler_type    = SchedulerType;
  using task_context_type = TaskContextType;

  static void run_task_submission_benchmark(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    const uint32_t task_count = 10000;

    bench.run(std::string("Task Submission - ") + name_suffix,
              [task_count]()
              {
                SchedulerType scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());

                auto                  main_ctx = get_main_context(scheduler);
                std::atomic<uint32_t> counter{0};

                scheduler.begin_execution();

                for (uint32_t i = 0; i < task_count; ++i)
                {
                  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                                   [&counter](TaskContextType const&)
                                   {
                                     counter.fetch_add(1, std::memory_order_relaxed);
                                   });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(counter.load());
              });
  }

  static void run_parallel_for_benchmark(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    const size_t  data_size = 100000;
    BenchmarkData data(data_size);

    bench.run(std::string("Parallel For Vector Ops - ") + name_suffix,
              [&data]()
              {
                SchedulerType scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());

                auto main_ctx = get_main_context(scheduler);

                scheduler.begin_execution();

                ouly::parallel_for(
                 [](glm::vec3& vec, TaskContextType const&)
                 {
                   ComputationKernels::vector_operations(vec);
                 },
                 data.vectors, main_ctx);

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data.vectors.data());
              });
  }

  static void run_glm_math_benchmark(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    const size_t  data_size = 50000;
    BenchmarkData data(data_size);

    bench.run(std::string("GLM Math Operations - ") + name_suffix,
              [&data]()
              {
                SchedulerType scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());

                auto main_ctx = get_main_context(scheduler);

                scheduler.begin_execution();

                // Matrix operations
                for (size_t i = 0; i < data.matrices.size(); ++i)
                {
                  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                                   [&data, i](TaskContextType const&)
                                   {
                                     ComputationKernels::matrix_operations(data.matrices[i]);
                                   });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data.matrices.data());
              });
  }

  static void run_mixed_workload_benchmark(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    const size_t  data_size = 30000;
    BenchmarkData data(data_size);

    bench.run(std::string("Mixed Workload - ") + name_suffix,
              [&data]()
              {
                SchedulerType scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());

                auto main_ctx = get_main_context(scheduler);

                scheduler.begin_execution();

                ouly::parallel_for(
                 [](auto begin, auto end, TaskContextType const&)
                 {
                   for (auto it = begin; it != end; ++it)
                   {
                     size_t idx = std::distance(begin, it);
                     // Access data arrays directly (assuming this is within BenchmarkData context)
                     // Note: This is a simplified version for the benchmark
                     glm::vec3 temp_vec = glm::vec3(static_cast<float>(idx));
                     ComputationKernels::vector_operations(temp_vec);
                     *it = static_cast<uint32_t>(glm::length(temp_vec) * 1000.0f);
                   }
                 },
                 data.integer_data, main_ctx);

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data.integer_data.data());
              });
  }

  static void run_throughput_benchmark(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    const uint32_t task_count            = 50000;
    const uint32_t computation_intensity = 1000;

    bench.run(std::string("Task Throughput - ") + name_suffix,
              [task_count, computation_intensity]()
              {
                SchedulerType scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());

                auto                  main_ctx = get_main_context(scheduler);
                std::atomic<uint64_t> result{0};

                scheduler.begin_execution();

                for (uint32_t i = 0; i < task_count; ++i)
                {
                  scheduler.submit(main_ctx, ouly::workgroup_id(0),
                                   [&result, computation_intensity, i](TaskContextType const&)
                                   {
                                     glm::vec3 vec(static_cast<float>(i));
                                     for (uint32_t j = 0; j < computation_intensity; ++j)
                                     {
                                       ComputationKernels::vector_operations(vec);
                                     }
                                     result.fetch_add(static_cast<uint64_t>(glm::length(vec) * 1000.0f),
                                                      std::memory_order_relaxed);
                                   });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(result.load());
              });
  }

private:
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

// TBB Benchmark implementations
class TBBBenchmark
{
public:
  static void run_task_submission_benchmark(ankerl::nanobench::Bench& bench)
  {
    const uint32_t task_count = 10000;

    bench.run("Task Submission - TBB",
              [task_count]()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                std::atomic<uint32_t> counter{0};

                tbb::parallel_for(0u, task_count, 1u,
                                  [&counter](uint32_t)
                                  {
                                    counter.fetch_add(1, std::memory_order_relaxed);
                                  });

                ankerl::nanobench::doNotOptimizeAway(counter.load());
              });
  }

  static void run_parallel_for_benchmark(ankerl::nanobench::Bench& bench)
  {
    const size_t  data_size = 100000;
    BenchmarkData data(data_size);

    bench.run("Parallel For Vector Ops - TBB",
              [&data]()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                tbb::parallel_for(static_cast<size_t>(0), data.vectors.size(),
                                  [&data](size_t i)
                                  {
                                    ComputationKernels::vector_operations(data.vectors[i]);
                                  });

                ankerl::nanobench::doNotOptimizeAway(data.vectors.data());
              });
  }

  static void run_glm_math_benchmark(ankerl::nanobench::Bench& bench)
  {
    const size_t  data_size = 50000;
    BenchmarkData data(data_size);

    bench.run("GLM Math Operations - TBB",
              [&data]()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                tbb::parallel_for(static_cast<size_t>(0), data.matrices.size(),
                                  [&data](size_t i)
                                  {
                                    ComputationKernels::matrix_operations(data.matrices[i]);
                                  });

                ankerl::nanobench::doNotOptimizeAway(data.matrices.data());
              });
  }

  static void run_mixed_workload_benchmark(ankerl::nanobench::Bench& bench)
  {
    const size_t  data_size = 30000;
    BenchmarkData data(data_size);

    bench.run("Mixed Workload - TBB",
              [&data]()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                tbb::parallel_for(static_cast<size_t>(0), data.integer_data.size(),
                                  [&data](size_t i)
                                  {
                                    glm::vec3 temp_vec = glm::vec3(static_cast<float>(i));
                                    ComputationKernels::vector_operations(temp_vec);
                                    data.integer_data[i] = static_cast<uint32_t>(glm::length(temp_vec) * 1000.0f);
                                  });

                ankerl::nanobench::doNotOptimizeAway(data.integer_data.data());
              });
  }
};

// Main benchmark runner
void run_scheduler_comparison_benchmarks()
{
  std::cout << "Starting Scheduler Comparison Benchmarks (v1 vs v2 vs TBB)\n";
  std::cout << "Hardware Concurrency: " << std::thread::hardware_concurrency() << " threads\n\n";

  ankerl::nanobench::Bench bench;
  bench.title("Scheduler Performance Comparison").unit("operation").warmup(3).epochIterations(10).minEpochIterations(5);

  // Task Submission Benchmarks
  std::cout << "Running Task Submission Benchmarks...\n";
  SchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_task_submission_benchmark(bench, "V1");
  SchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_task_submission_benchmark(bench, "V2");
  TBBBenchmark::run_task_submission_benchmark(bench);

  // Parallel For Benchmarks
  std::cout << "Running Parallel For Benchmarks...\n";
  SchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_parallel_for_benchmark(bench, "V1");
  SchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_parallel_for_benchmark(bench, "V2");
  TBBBenchmark::run_parallel_for_benchmark(bench);

  // GLM Math Benchmarks
  std::cout << "Running GLM Mathematical Operation Benchmarks...\n";
  SchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_glm_math_benchmark(bench, "V1");
  SchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_glm_math_benchmark(bench, "V2");
  TBBBenchmark::run_glm_math_benchmark(bench);

  // Mixed Workload Benchmarks
  std::cout << "Running Mixed Workload Benchmarks...\n";
  SchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_mixed_workload_benchmark(bench, "V1");
  SchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_mixed_workload_benchmark(bench, "V2");
  TBBBenchmark::run_mixed_workload_benchmark(bench);

  // Throughput Benchmarks
  std::cout << "Running Task Throughput Benchmarks...\n";
  SchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_throughput_benchmark(bench, "V1");
  SchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_throughput_benchmark(bench, "V2");

  std::cout << "\nBenchmarks completed!\n";

  // Save results to file
  std::ofstream results_file("scheduler_comparison_results.json");
  if (results_file.is_open())
  {
    results_file << bench.results();
    results_file.close();
    std::cout << "Results saved to scheduler_comparison_results.json\n";
  }
}

int main()
{
  try
  {
    run_scheduler_comparison_benchmarks();
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Benchmark failed with exception: " << e.what() << std::endl;
    return 1;
  }
  catch (...)
  {
    std::cerr << "Benchmark failed with unknown exception" << std::endl;
    return 1;
  }
}
