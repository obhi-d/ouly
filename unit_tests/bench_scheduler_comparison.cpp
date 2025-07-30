// SPDX-License-Identifier: MIT
#define ANKERL_NANOBENCH_IMPLEMENT
#define GLM_ENABLE_EXPERIMENTAL

#include "nanobench.h"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"

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

// Constants for benchmark configuration
namespace benchmark_config
{
constexpr float    VECTOR_INCREMENT    = 0.1F;
constexpr float    SCALE_FACTOR        = 1.01F;
constexpr float    ROTATION_ANGLE      = 1.0F;
constexpr float    SCALE_MATRIX_FACTOR = 1.001F;
constexpr float    TRANSLATE_OFFSET    = 0.01F;
constexpr uint32_t HASH_MULTIPLIER     = 31U;
constexpr uint32_t HASH_ADDITIVE       = 17U;
constexpr uint32_t SHIFT_AMOUNT        = 16U;
constexpr float    SINE_MULTIPLIER     = 2.0F;
constexpr float    SCALAR_INCREMENT    = 0.001F;
constexpr float    LENGTH_MULTIPLIER   = 1000.0F;
constexpr uint32_t MATRIX_OP_TASKS     = 25000;
constexpr uint32_t MAX_SAMPLES         = 1;
} // namespace benchmark_config

// Comprehensive benchmark data structures
struct BenchmarkData
{
  std::vector<glm::vec3> vectors;
  std::vector<glm::mat4> matrices;
  std::vector<float>     scalar_data;
  std::vector<uint32_t>  integer_data;
  std::vector<double>    double_data;

  explicit BenchmarkData(size_t size)
      : vectors(size), matrices(size), scalar_data(size), integer_data(size), double_data(size)
  {
    // Initialize with meaningful data for realistic benchmarks
    std::random_device                      rd;
    std::mt19937                            gen(rd());
    std::uniform_real_distribution<float>   float_dist(0.0F, 100.0F);
    std::uniform_int_distribution<uint32_t> int_dist(1, 1000000);

    for (size_t i = 0; i < size; ++i)
    {
      const auto fi   = static_cast<float>(i);
      vectors[i]      = glm::vec3(fi, fi + 1.0F, fi + 2.0F);
      matrices[i]     = glm::translate(glm::mat4(1.0F), glm::vec3(fi));
      scalar_data[i]  = fi * benchmark_config::VECTOR_INCREMENT;
      integer_data[i] = static_cast<uint32_t>(i);
      double_data[i]  = static_cast<double>(i) * 0.01;
    }
  }
};

// Performance-critical computation kernels
struct ComputationKernels
{
  static void vector_operations(glm::vec3& vec)
  {
    vec = glm::normalize(vec);
    vec = glm::cross(vec, glm::vec3(1.0F, 0.0F, 0.0F));
    vec += glm::vec3(benchmark_config::VECTOR_INCREMENT);
    vec *= benchmark_config::SCALE_FACTOR;
  }

  static void matrix_operations(glm::mat4& matrix)
  {
    matrix = glm::rotate(matrix, glm::radians(benchmark_config::ROTATION_ANGLE), glm::vec3(0.0F, 1.0F, 0.0F));
    matrix = glm::scale(matrix, glm::vec3(benchmark_config::SCALE_MATRIX_FACTOR));
    matrix = glm::translate(matrix, glm::vec3(benchmark_config::TRANSLATE_OFFSET));
  }

  static void mixed_computation(uint32_t& integer, float& scalar, glm::vec3& vec)
  {
    // Integer hash operations
    integer = integer * benchmark_config::HASH_MULTIPLIER + benchmark_config::HASH_ADDITIVE;
    integer ^= (integer >> benchmark_config::SHIFT_AMOUNT);

    // Scalar trigonometric operations
    scalar = std::sin(scalar) * std::cos(scalar * benchmark_config::SINE_MULTIPLIER);
    scalar += benchmark_config::SCALAR_INCREMENT;

    // Vector operations
    vector_operations(vec);
  }

  static double parallel_reduction(const std::vector<double>& data)
  {
    return std::accumulate(data.begin(), data.end(), 0.0,
                           [](double acc, double val)
                           {
                             return acc + std::sqrt(val * val + 1.0);
                           });
  }
};

// Scheduler benchmark framework
template <typename SchedulerType, typename TaskContextType>
class ComprehensiveSchedulerBenchmark
{
public:
  using scheduler_type    = SchedulerType;
  using task_context_type = TaskContextType;

  // Task submission performance using parallel_for
  static void run_task_submission(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    constexpr uint32_t    TASK_COUNT = 10000U;
    std::vector<uint32_t> task_data(TASK_COUNT);
    std::iota(task_data.begin(), task_data.end(), 0);

    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    bench.run(std::string("TaskSubmission_") + name_suffix,
              [&task_data, &main_ctx]()
              {
                std::atomic<uint32_t> counter{0};

                ouly::auto_parallel_for(
                 [&counter](uint32_t, const task_context_type&)
                 {
                   counter.fetch_add(1, std::memory_order_relaxed);
                 },
                 task_data, main_ctx);

                ankerl::nanobench::doNotOptimizeAway(counter.load());
              });

    teardown_scheduler(scheduler);
  }

  // Parallel for vector operations
  static void run_parallel_for_vectors(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    constexpr size_t DATA_SIZE = 100000;
    BenchmarkData    data(DATA_SIZE);

    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    bench.run(std::string("ParallelFor_VectorOps_") + name_suffix,
              [&data, &main_ctx]()
              {
                ouly::auto_parallel_for(
                 [](glm::vec3& vec, const task_context_type&)
                 {
                   ComputationKernels::vector_operations(vec);
                 },
                 data.vectors, main_ctx);

                ankerl::nanobench::doNotOptimizeAway(data.vectors.data());
              });

    teardown_scheduler(scheduler);
  }

  // Matrix operations using parallel_for
  static void run_matrix_operations(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    constexpr size_t DATA_SIZE = benchmark_config::MATRIX_OP_TASKS;
    BenchmarkData    data(DATA_SIZE);
    auto             scheduler = setup_scheduler();
    const auto&      main_ctx  = get_main_context();

    bench.run(std::string("MatrixOps_") + name_suffix,
              [&]()
              {
                for (uint32_t i = 0; i < benchmark_config::MAX_SAMPLES; ++i)
                {
                  ouly::auto_parallel_for(
                   [](glm::mat4& matrix, const task_context_type&)
                   {
                     ComputationKernels::matrix_operations(matrix);
                   },
                   data.matrices, main_ctx);
                }
                ankerl::nanobench::doNotOptimizeAway(data.matrices.data());
              });

    teardown_scheduler(scheduler);
  }

  // Mixed workload benchmark
  static void run_mixed_workload(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    constexpr size_t DATA_SIZE = 50000;
    BenchmarkData    data(DATA_SIZE);

    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    bench.run(std::string("MixedWorkload_") + name_suffix,
              [&data, &main_ctx]()
              {
                ouly::auto_parallel_for(
                 [&data](auto begin, auto end, const task_context_type&)
                 {
                   for (auto it = begin; it != end; ++it)
                   {
                     const auto idx = static_cast<size_t>(std::distance(begin, it));
                     if (idx < data.vectors.size() && idx < data.scalar_data.size())
                     {
                       ComputationKernels::mixed_computation(*it, data.scalar_data[idx], data.vectors[idx]);
                     }
                   }
                 },
                 data.integer_data, main_ctx);

                ankerl::nanobench::doNotOptimizeAway(data.integer_data.data());
              });

    teardown_scheduler(scheduler);
  }

  // High-throughput task processing using parallel_for
  static void run_task_throughput(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    constexpr uint32_t    TASK_COUNT     = 25000U;
    constexpr uint32_t    WORK_INTENSITY = 500U;
    std::vector<uint32_t> task_indices(TASK_COUNT);
    std::iota(task_indices.begin(), task_indices.end(), 0);

    auto        scheduler = setup_scheduler();
    const auto& main_ctx  = get_main_context();

    bench.run(std::string("TaskThroughput_") + name_suffix,
              [&task_indices, &main_ctx, WORK_INTENSITY]()
              {
                std::atomic<uint64_t> result{0};

                ouly::auto_parallel_for(
                 [&result, WORK_INTENSITY](uint32_t& i, const task_context_type&)
                 {
                   glm::vec3 vec(static_cast<float>(i));
                   for (uint32_t j = 0; j < WORK_INTENSITY; ++j)
                   {
                     ComputationKernels::vector_operations(vec);
                   }
                   result.fetch_add(static_cast<uint64_t>(glm::length(vec) * benchmark_config::LENGTH_MULTIPLIER),
                                    std::memory_order_relaxed);
                 },
                 task_indices, main_ctx);

                ankerl::nanobench::doNotOptimizeAway(result.load());
              });

    teardown_scheduler(scheduler);
  }

  // Nested parallel workload using parallel_for
  static void run_nested_parallel(ankerl::nanobench::Bench& bench, const std::string& name_suffix)
  {
    constexpr size_t DATA_SIZE = 10000;
    BenchmarkData    data(DATA_SIZE);

    auto        scheduler = setup_scheduler_with_two_groups();
    const auto& main_ctx  = get_main_context();

    bench.run(std::string("NestedParallel_") + name_suffix,
              [&data, &main_ctx]()
              {
                // Outer parallel loop with nested parallel work
                ouly::auto_parallel_for(
                 [&data](glm::vec3& vec, const task_context_type& ctx)
                 {
                   const auto idx = static_cast<size_t>(&vec - data.vectors.data());

                   // Submit nested work to different workgroup using parallel_for on matrices
                   if (idx < data.matrices.size())
                   {
                     std::vector<glm::mat4*> matrix_refs = {&data.matrices[idx]};
                     ouly::auto_parallel_for(
                      [](glm::mat4*& matrix_ptr, const task_context_type&)
                      {
                        ComputationKernels::matrix_operations(*matrix_ptr);
                      },
                      matrix_refs, ctx);
                   }

                   ComputationKernels::vector_operations(vec);
                 },
                 data.vectors, main_ctx);

                ankerl::nanobench::doNotOptimizeAway(data.matrices.data());
              });

    teardown_scheduler(scheduler);
  }

private:
  // Helper functions to manage scheduler setup and teardown
  static auto setup_scheduler() -> scheduler_type
  {
    scheduler_type scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());
    scheduler.begin_execution();
    return scheduler;
  }

  static auto setup_scheduler_with_two_groups() -> scheduler_type
  {
    scheduler_type scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, std::thread::hardware_concurrency());
    scheduler.create_group(ouly::workgroup_id(1), 0, std::thread::hardware_concurrency());
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

// TBB benchmark implementations for comparison
class TBBBenchmarks
{
public:
  static void run_task_submission(ankerl::nanobench::Bench& bench)
  {
    constexpr uint32_t TASK_COUNT = 10000U;

    bench.run("TaskSubmission_TBB",
              [=]()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                std::atomic<uint32_t> counter{0};

                tbb::parallel_for(0U, TASK_COUNT, 1U,
                                  [&counter](uint32_t)
                                  {
                                    counter.fetch_add(1, std::memory_order_relaxed);
                                  });

                ankerl::nanobench::doNotOptimizeAway(counter.load());
              });
  }

  static void run_parallel_for_vectors(ankerl::nanobench::Bench& bench)
  {
    constexpr size_t DATA_SIZE = 100000;
    BenchmarkData    data(DATA_SIZE);

    bench.run("ParallelFor_VectorOps_TBB",
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

  static void run_matrix_operations(ankerl::nanobench::Bench& bench)
  {
    constexpr size_t DATA_SIZE = benchmark_config::MATRIX_OP_TASKS;
    BenchmarkData    data(DATA_SIZE);

    tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, std::thread::hardware_concurrency());

    bench.run("MatrixOps_TBB",
              [&data]()
              {
                for (uint32_t i = 0; i < benchmark_config::MAX_SAMPLES; ++i)
                {
                  tbb::parallel_for(static_cast<size_t>(0), data.matrices.size(),
                                    [&data](size_t i)
                                    {
                                      ComputationKernels::matrix_operations(data.matrices[i]);
                                    });
                }
                ankerl::nanobench::doNotOptimizeAway(data.matrices.data());
              });
  }

  static void run_mixed_workload(ankerl::nanobench::Bench& bench)
  {
    constexpr size_t DATA_SIZE = 50000;
    BenchmarkData    data(DATA_SIZE);

    bench.run("MixedWorkload_TBB",
              [&data]()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                tbb::parallel_for(static_cast<size_t>(0), data.integer_data.size(),
                                  [&data](size_t i)
                                  {
                                    if (i < data.vectors.size() && i < data.scalar_data.size())
                                    {
                                      ComputationKernels::mixed_computation(data.integer_data[i], data.scalar_data[i],
                                                                            data.vectors[i]);
                                    }
                                  });

                ankerl::nanobench::doNotOptimizeAway(data.integer_data.data());
              });
  }

  static void run_task_throughput(ankerl::nanobench::Bench& bench)
  {
    constexpr uint32_t TASK_COUNT     = 25000U;
    constexpr uint32_t WORK_INTENSITY = 500U;

    bench.run("TaskThroughput_TBB",
              [=]()
              {
                tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                                 std::thread::hardware_concurrency());

                std::atomic<uint64_t> result{0};

                tbb::parallel_for(0U, TASK_COUNT, 1U,
                                  [&result, WORK_INTENSITY](uint32_t i)
                                  {
                                    glm::vec3 vec(static_cast<float>(i));
                                    for (uint32_t j = 0; j < WORK_INTENSITY; ++j)
                                    {
                                      ComputationKernels::vector_operations(vec);
                                    }
                                    result.fetch_add(
                                     static_cast<uint64_t>(glm::length(vec) * benchmark_config::LENGTH_MULTIPLIER),
                                     std::memory_order_relaxed);
                                  });

                ankerl::nanobench::doNotOptimizeAway(result.load());
              });
  }
};

// Utility functions for output and reporting
class BenchmarkReporter
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
  static void save_results(ankerl::nanobench::Bench& bench, const std::string& test_id,
                           const std::string& commit_hash = "", const std::string& build_number = "")
  {
    const std::string compiler = get_compiler_info();

    // Generate short commit hash (8 chars) and build number for filename
    std::string short_commit = commit_hash.empty() ? "local" : commit_hash.substr(0, 8);
    std::string build_num    = build_number.empty() ? "0" : build_number;

    // New filename format: <compiler-id>-<small-commit-hash>-<build-number>-<test_id>.json
    const std::string json_filename = compiler + "-" + short_commit + "-" + build_num + "-" + test_id + ".json";

    // Output test_id to console for CI tracking
    std::cout << "TEST_ID: " << test_id << std::endl;

    std::ofstream json_file(json_filename);
    if (json_file.is_open())
    {
      bench.render(ankerl::nanobench::templates::json(), json_file);
      json_file.close();
      std::cout << "✅ JSON results saved to: " << json_filename << std::endl;
    }

    // Save human-readable results with same naming pattern
    const std::string txt_filename = compiler + "-" + short_commit + "-" + build_num + "-" + test_id + ".txt";
    std::ofstream     txt_file(txt_filename);
    if (txt_file.is_open())
    {
      bench.render(
       "{{#result}}{{name}}: {{median(elapsed)}} seconds ({{median(instructions)}} instructions)\n{{/result}}",
       txt_file);
      txt_file.close();
      std::cout << "📄 Text results saved to: " << txt_filename << std::endl;
    }
  }

  // Legacy function for backward compatibility
  static void save_results(ankerl::nanobench::Bench& bench, const std::string& test_type)
  {
    save_results(bench, test_type, "", "");
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

// Main benchmark execution
void run_comprehensive_scheduler_benchmarks(int run_only = -1)
{
  std::cout << "🚀 OULY Comprehensive Scheduler Comparison Benchmarks" << std::endl;
  std::cout << "=======================================================" << std::endl;

  BenchmarkReporter::print_system_info();

  // Configure nanobench for high precision
  ankerl::nanobench::Bench bench;
  bench.title("Scheduler Performance Comparison")
   .unit("operation")
   .warmup(3)
   .epochIterations(10)
   .minEpochIterations(5)
   .relative(true);

  if (run_only < 0 || run_only == 0)
  {
    std::cout << "📊 Running Task Submission Benchmarks..." << std::endl;
    ComprehensiveSchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_task_submission(bench, "V1");
    ComprehensiveSchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_task_submission(bench, "V2");
    TBBBenchmarks::run_task_submission(bench);
  }

  if (run_only < 0 || run_only == 1)
  {
    std::cout << "🔄 Running Parallel For Vector Operations..." << std::endl;
    ComprehensiveSchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_parallel_for_vectors(bench, "V1");
    ComprehensiveSchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_parallel_for_vectors(bench, "V2");
    TBBBenchmarks::run_parallel_for_vectors(bench);
  }

  if (run_only < 0 || run_only == 2)
  {
    std::cout << "🧮 Running Matrix Operations..." << std::endl;
    ComprehensiveSchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_matrix_operations(bench, "V1");
    ComprehensiveSchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_matrix_operations(bench, "V2");
    TBBBenchmarks::run_matrix_operations(bench);
  }

  if (run_only < 0 || run_only == 3)
  {
    std::cout << "🔀 Running Mixed Workload Benchmarks..." << std::endl;
    ComprehensiveSchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_mixed_workload(bench, "V1");
    ComprehensiveSchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_mixed_workload(bench, "V2");
    TBBBenchmarks::run_mixed_workload(bench);
  }

  if (run_only < 0 || run_only == 4)
  {
    std::cout << "⚡ Running Task Throughput Benchmarks..." << std::endl;
    ComprehensiveSchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_task_throughput(bench, "V1");
    ComprehensiveSchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_task_throughput(bench, "V2");
    TBBBenchmarks::run_task_throughput(bench);
  }

  if (run_only < 0 || run_only == 5)
  {
    std::cout << "🔗 Running Nested Parallel Workloads..." << std::endl;
    ComprehensiveSchedulerBenchmark<ouly::v1::scheduler, ouly::v1::task_context>::run_nested_parallel(bench, "V1");
    ComprehensiveSchedulerBenchmark<ouly::v2::scheduler, ouly::v2::task_context>::run_nested_parallel(bench, "V2");
  }

  std::cout << " Saving benchmark results...\n";

  // Get environment variables for CI integration
  const char* commit_hash_env  = std::getenv("GITHUB_SHA");
  const char* build_number_env = std::getenv("GITHUB_RUN_NUMBER");

  std::string commit_hash  = (commit_hash_env != nullptr) ? commit_hash_env : "";
  std::string build_number = (build_number_env != nullptr) ? build_number_env : "";

  BenchmarkReporter::save_results(bench, "scheduler_comparison", commit_hash, build_number);

  std::cout << std::endl;
  std::cout << "✅ Comprehensive benchmark suite completed successfully!" << std::endl;
  std::cout << "📁 Results saved in JSON format for performance tracking integration." << std::endl;
}

int main(int argc, char** argv)
{
  try
  {
    // Handle command line arguments for CI integration
    if (argc > 1)
    {
      const std::string arg = argv[1];
      if (arg == "--help" || arg == "-h")
      {
        std::cout << "OULY Scheduler Benchmark Suite" << std::endl;
        std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --help, -h    Show this help message" << std::endl;
        std::cout << "  --quick  [test_index]  Run quick benchmark subset" << std::endl;
        return 0;
      }
      if (arg == "--quick")
      {
        std::cout << "Running quick benchmark subset..." << std::endl;
        int test_index = -1;
        if (argc > 2)
        {
          try
          {
            test_index = std::stoi(argv[2]);
          }
          catch (const std::exception&)
          {
            std::cerr << "Invalid test index provided. Running comprehensive benchmarks." << std::endl;
          }
        }

        // Could implement a reduced benchmark set here for CI
        run_comprehensive_scheduler_benchmarks(test_index);
        return 0;
      }
    }

    run_comprehensive_scheduler_benchmarks(-1);
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "❌ Benchmark failed with exception: " << e.what() << std::endl;
    return 1;
  }
  catch (...)
  {
    std::cerr << "❌ Benchmark failed with unknown exception" << std::endl;
    return 1;
  }
}
