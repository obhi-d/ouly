#define ANKERL_NANOBENCH_IMPLEMENT
#define GLM_ENABLE_EXPERIMENTAL
#include "nanobench.h"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler_v1.hpp"
#include "ouly/scheduler/scheduler_v2.hpp"
#include <atomic>
#include <fstream>
#include <iostream>
#include <numeric>
#include <span>
#include <thread>
#include <vector>

// Include oneTBB headers
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_arena.h>
#include <tbb/tbb.h>

// Include GLM for math operations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>

// Test data structures
struct WorkItem
{
  int              id;
  double           data;
  std::atomic<int> counter{0};

  WorkItem() : id(0), data(0.0) {}
  WorkItem(int i, double d) : id(i), data(d) {}

  // Non-copyable due to atomic member
  WorkItem(const WorkItem& other) : id(other.id), data(other.data), counter(other.counter.load()) {}
  WorkItem& operator=(const WorkItem& other)
  {
    if (this != &other)
    {
      id   = other.id;
      data = other.data;
      counter.store(other.counter.load());
    }
    return *this;
  }
};

// Mathematical computation structure for GLM benchmarks
struct MathWorkItem
{
  glm::vec3 position;
  glm::vec3 velocity;
  glm::vec3 acceleration;
  glm::mat4 transform;
  float     scalar_data;

  MathWorkItem() : position(0.0f), velocity(0.0f), acceleration(0.0f), transform(1.0f), scalar_data(0.0f) {}

  MathWorkItem(float x, float y, float z)
      : position(x, y, z), velocity(x * 0.1f, y * 0.1f, z * 0.1f), acceleration(0.0f, -9.81f, 0.0f), transform(1.0f),
        scalar_data(x + y + z)
  {}
};

// Computational kernels for benchmarks
inline void compute_intensive_task(int& value, int iterations = 100)
{
  volatile int temp = value;
  for (int i = 0; i < iterations; ++i)
  {
    temp = temp * 31 + i;
    temp = temp ^ (temp >> 16);
  }
  value = temp;
}

inline void compute_math_intensive_task(MathWorkItem& item, float dt = 0.016f)
{
  // Physics simulation step
  item.velocity += item.acceleration * dt;
  item.position += item.velocity * dt;

  // Matrix transformations
  item.transform = glm::translate(glm::mat4(1.0f), item.position);
  item.transform = glm::rotate(item.transform, glm::length(item.velocity) * dt, glm::vec3(0.0f, 1.0f, 0.0f));

  // Vector operations
  glm::vec3 normalized_vel = glm::normalize(item.velocity);
  item.scalar_data         = glm::dot(normalized_vel, glm::vec3(1.0f, 0.0f, 0.0f));

  // More complex math operations
  float distance = glm::distance(item.position, glm::vec3(0.0f));
  item.scalar_data += sin(distance) * cos(item.scalar_data);

  // Ensure the item isn't optimized away
  volatile float result = item.scalar_data;
  (void)result;
}

// Benchmark configuration
struct BenchmarkConfig
{
  size_t   data_size             = 10000;
  uint32_t computation_intensity = 1000;
  uint32_t num_workers           = std::thread::hardware_concurrency();
  uint32_t epochs                = 3;
  uint32_t warmup_epochs         = 1;
};

class SchedulerBenchmarkSuite
{
public:
  static void benchmark_task_throughput(ankerl::nanobench::Bench& bench, const BenchmarkConfig& config)
  {
    std::cout << "=== Task Throughput Benchmark ===" << std::endl;

    // V1 Scheduler benchmark
    bench.run("OULY v1 - Task Throughput",
              [&]
              {
                ouly::v1::scheduler scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, config.num_workers);

                std::vector<std::atomic<int>> data(config.data_size);
                std::iota(data.begin(), data.end(), 0);

                scheduler.begin_execution();

                for (size_t i = 0; i < data.size(); ++i)
                {
                  ouly::v1::async(ouly::v1::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(0),
                                  [&data, &config, i](ouly::v1::task_context const&)
                                  {
                                    int value = data[i].load();
                                    compute_intensive_task(value, config.computation_intensity);
                                    data[i].store(value);
                                  });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data);
              });

    // V2 Scheduler benchmark
    bench.run("OULY v2 - Task Throughput",
              [&]
              {
                ouly::v2::scheduler scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, config.num_workers);

                std::vector<std::atomic<int>> data(config.data_size);
                std::iota(data.begin(), data.end(), 0);

                scheduler.begin_execution();

                for (size_t i = 0; i < data.size(); ++i)
                {
                  ouly::v2::async(ouly::v2::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(0),
                                  [&data, &config, i](ouly::v2::task_context const&)
                                  {
                                    int value = data[i].load();
                                    compute_intensive_task(value, config.computation_intensity);
                                    data[i].store(value);
                                  });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data);
              });

    // TBB benchmark
    bench.run("TBB - Task Throughput",
              [&]
              {
                std::vector<std::atomic<int>> data(config.data_size);
                std::iota(data.begin(), data.end(), 0);

                tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
                                  [&](const tbb::blocked_range<size_t>& range)
                                  {
                                    for (size_t i = range.begin(); i != range.end(); ++i)
                                    {
                                      int value = data[i].load();
                                      compute_intensive_task(value, config.computation_intensity);
                                      data[i].store(value);
                                    }
                                  });

                ankerl::nanobench::doNotOptimizeAway(data);
              });
  }

  static void benchmark_work_stealing_efficiency(ankerl::nanobench::Bench& bench, const BenchmarkConfig& config)
  {
    std::cout << "=== Work Stealing Efficiency Benchmark ===" << std::endl;

    const size_t   imbalanced_size = config.data_size;
    const uint32_t num_groups      = 4;

    // V1 Scheduler work stealing
    bench.run("OULY v1 - Work Stealing",
              [&]
              {
                ouly::v1::scheduler scheduler;
                // Create workgroups with different thread counts to encourage stealing
                scheduler.create_group(ouly::workgroup_id(0), 0, 1);                      // Single thread
                scheduler.create_group(ouly::workgroup_id(1), 1, 2);                      // Two threads
                scheduler.create_group(ouly::workgroup_id(2), 3, 3);                      // Three threads
                scheduler.create_group(ouly::workgroup_id(3), 6, config.num_workers - 6); // Remaining threads

                std::vector<std::atomic<int>> data(imbalanced_size);
                std::iota(data.begin(), data.end(), 0);

                scheduler.begin_execution();

                // Submit most work to the single-thread group to encourage stealing
                for (size_t i = 0; i < data.size(); ++i)
                {
                  uint32_t target_group = (i < data.size() * 0.7) ? 0 : (i % num_groups);
                  ouly::v1::async(ouly::v1::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(target_group),
                                  [&data, &config, i](ouly::v1::task_context const&)
                                  {
                                    int value = data[i].load();
                                    compute_intensive_task(value, config.computation_intensity);
                                    data[i].store(value);
                                  });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data);
              });

    // V2 Scheduler work stealing
    bench.run("OULY v2 - Work Stealing",
              [&]
              {
                ouly::v2::scheduler scheduler;
                // Create workgroups with different thread counts to encourage stealing
                scheduler.create_group(ouly::workgroup_id(0), 0, 1);                      // Single thread
                scheduler.create_group(ouly::workgroup_id(1), 1, 2);                      // Two threads
                scheduler.create_group(ouly::workgroup_id(2), 3, 3);                      // Three threads
                scheduler.create_group(ouly::workgroup_id(3), 6, config.num_workers - 6); // Remaining threads

                std::vector<std::atomic<int>> data(imbalanced_size);
                std::iota(data.begin(), data.end(), 0);

                scheduler.begin_execution();

                // Submit most work to the single-thread group to encourage stealing
                for (size_t i = 0; i < data.size(); ++i)
                {
                  uint32_t target_group = (i < data.size() * 0.7) ? 0 : (i % num_groups);
                  ouly::v2::async(ouly::v2::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(target_group),
                                  [&data, &config, i](ouly::v2::task_context const&)
                                  {
                                    int value = data[i].load();
                                    compute_intensive_task(value, config.computation_intensity);
                                    data[i].store(value);
                                  });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data);
              });

    // TBB work stealing (automatic load balancing)
    bench.run("TBB - Auto Load Balancing",
              [&]
              {
                std::vector<std::atomic<int>> data(imbalanced_size);
                std::iota(data.begin(), data.end(), 0);

                tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
                                  [&](const tbb::blocked_range<size_t>& range)
                                  {
                                    for (size_t i = range.begin(); i != range.end(); ++i)
                                    {
                                      int value = data[i].load();
                                      compute_intensive_task(value, config.computation_intensity);
                                      data[i].store(value);
                                    }
                                  });

                ankerl::nanobench::doNotOptimizeAway(data);
              });
  }

  static void benchmark_mathematical_computation(ankerl::nanobench::Bench& bench, const BenchmarkConfig& config)
  {
    std::cout << "=== Mathematical Computation Benchmark ===" << std::endl;

    // V1 Math computation
    bench.run("OULY v1 - Math Intensive",
              [&]
              {
                ouly::v1::scheduler scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, config.num_workers);

                std::vector<MathWorkItem> data(config.data_size);
                for (size_t i = 0; i < data.size(); ++i)
                {
                  data[i] = MathWorkItem(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
                }

                scheduler.begin_execution();

                for (size_t i = 0; i < data.size(); ++i)
                {
                  ouly::v1::async(ouly::v1::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(0),
                                  [&data, i](ouly::v1::task_context const&)
                                  {
                                    compute_math_intensive_task(data[i]);
                                  });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data);
              });

    // V2 Math computation
    bench.run("OULY v2 - Math Intensive",
              [&]
              {
                ouly::v2::scheduler scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, config.num_workers);

                std::vector<MathWorkItem> data(config.data_size);
                for (size_t i = 0; i < data.size(); ++i)
                {
                  data[i] = MathWorkItem(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
                }

                scheduler.begin_execution();

                for (size_t i = 0; i < data.size(); ++i)
                {
                  ouly::v2::async(ouly::v2::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(0),
                                  [&data, i](ouly::v2::task_context const&)
                                  {
                                    compute_math_intensive_task(data[i]);
                                  });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(data);
              });

    // TBB Math computation
    bench.run("TBB - Math Intensive",
              [&]
              {
                std::vector<MathWorkItem> data(config.data_size);
                for (size_t i = 0; i < data.size(); ++i)
                {
                  data[i] = MathWorkItem(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
                }

                tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
                                  [&](const tbb::blocked_range<size_t>& range)
                                  {
                                    for (size_t i = range.begin(); i != range.end(); ++i)
                                    {
                                      compute_math_intensive_task(data[i]);
                                    }
                                  });

                ankerl::nanobench::doNotOptimizeAway(data);
              });
  }

  static void benchmark_task_submission_overhead(ankerl::nanobench::Bench& bench, const BenchmarkConfig& config)
  {
    std::cout << "=== Task Submission Overhead Benchmark ===" << std::endl;

    constexpr size_t many_small_tasks = 100000;

    // V1 Small task overhead
    bench.run("OULY v1 - Small Task Overhead",
              [&]
              {
                ouly::v1::scheduler scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, config.num_workers);

                std::atomic<size_t> counter{0};

                scheduler.begin_execution();

                for (size_t i = 0; i < many_small_tasks; ++i)
                {
                  ouly::v1::async(ouly::v1::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(0),
                                  [&counter](ouly::v1::task_context const&)
                                  {
                                    counter.fetch_add(1, std::memory_order_relaxed);
                                  });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(counter.load());
              });

    // V2 Small task overhead
    bench.run("OULY v2 - Small Task Overhead",
              [&]
              {
                ouly::v2::scheduler scheduler;
                scheduler.create_group(ouly::workgroup_id(0), 0, config.num_workers);

                std::atomic<size_t> counter{0};

                scheduler.begin_execution();

                for (size_t i = 0; i < many_small_tasks; ++i)
                {
                  ouly::v2::async(ouly::v2::task_context::get(ouly::workgroup_id(0)), ouly::workgroup_id(0),
                                  [&counter](ouly::v2::task_context const&)
                                  {
                                    counter.fetch_add(1, std::memory_order_relaxed);
                                  });
                }

                scheduler.end_execution();
                ankerl::nanobench::doNotOptimizeAway(counter.load());
              });

    // TBB Small task overhead
    bench.run("TBB - Small Task Overhead",
              [&]
              {
                std::atomic<size_t> counter{0};

                tbb::parallel_for(tbb::blocked_range<size_t>(0, many_small_tasks, 1),
                                  [&](const tbb::blocked_range<size_t>& range)
                                  {
                                    for (size_t i = range.begin(); i != range.end(); ++i)
                                    {
                                      counter.fetch_add(1, std::memory_order_relaxed);
                                    }
                                  });

                ankerl::nanobench::doNotOptimizeAway(counter.load());
              });
  }

  static void benchmark_cross_workgroup_communication(ankerl::nanobench::Bench& bench, const BenchmarkConfig& config)
  {
    std::cout << "=== Cross-Workgroup Communication Benchmark ===" << std::endl;

    constexpr uint32_t num_workgroups  = 4;
    constexpr size_t   tasks_per_group = 1000;

    // V1 Cross-workgroup communication
    bench.run("OULY v1 - Cross-WG Communication",
              [&]
              {
                ouly::v1::scheduler scheduler;
                uint32_t            threads_per_group = config.num_workers / num_workgroups;
                if (threads_per_group == 0)
                  threads_per_group = 1;

                for (uint32_t i = 0; i < num_workgroups; ++i)
                {
                  scheduler.create_group(ouly::workgroup_id(i), i * threads_per_group, threads_per_group);
                }

                std::array<std::atomic<size_t>, num_workgroups> counters{};

                scheduler.begin_execution();

                // Each workgroup submits work to all other workgroups
                for (uint32_t src_group = 0; src_group < num_workgroups; ++src_group)
                {
                  for (uint32_t dst_group = 0; dst_group < num_workgroups; ++dst_group)
                  {
                    if (src_group == dst_group)
                      continue;

                    for (size_t task = 0; task < tasks_per_group; ++task)
                    {
                      ouly::v1::async(ouly::v1::task_context::get(ouly::workgroup_id(src_group)),
                                      ouly::workgroup_id(dst_group),
                                      [&counters, dst_group](ouly::v1::task_context const&)
                                      {
                                        counters[dst_group].fetch_add(1, std::memory_order_relaxed);
                                      });
                    }
                  }
                }

                scheduler.end_execution();

                size_t total = 0;
                for (const auto& counter : counters)
                {
                  total += counter.load();
                }
                ankerl::nanobench::doNotOptimizeAway(total);
              });

    // V2 Cross-workgroup communication
    bench.run("OULY v2 - Cross-WG Communication",
              [&]
              {
                ouly::v2::scheduler scheduler;
                uint32_t            threads_per_group = config.num_workers / num_workgroups;
                if (threads_per_group == 0)
                  threads_per_group = 1;

                for (uint32_t i = 0; i < num_workgroups; ++i)
                {
                  scheduler.create_group(ouly::workgroup_id(i), i * threads_per_group, threads_per_group);
                }

                std::array<std::atomic<size_t>, num_workgroups> counters{};

                scheduler.begin_execution();

                // Each workgroup submits work to all other workgroups
                for (uint32_t src_group = 0; src_group < num_workgroups; ++src_group)
                {
                  for (uint32_t dst_group = 0; dst_group < num_workgroups; ++dst_group)
                  {
                    if (src_group == dst_group)
                      continue;

                    for (size_t task = 0; task < tasks_per_group; ++task)
                    {
                      ouly::v2::async(ouly::v2::task_context::get(ouly::workgroup_id(src_group)),
                                      ouly::workgroup_id(dst_group),
                                      [&counters, dst_group](ouly::v2::task_context const&)
                                      {
                                        counters[dst_group].fetch_add(1, std::memory_order_relaxed);
                                      });
                    }
                  }
                }

                scheduler.end_execution();

                size_t total = 0;
                for (const auto& counter : counters)
                {
                  total += counter.load();
                }
                ankerl::nanobench::doNotOptimizeAway(total);
              });
  }
};

// Main benchmark execution
int main()
{
  std::cout << "OULY Scheduler v1 vs v2 vs TBB Benchmark Suite" << std::endl;
  std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;

  BenchmarkConfig config;
  config.num_workers           = std::min(8u, std::thread::hardware_concurrency()); // Limit for consistent testing
  config.data_size             = 5000;                                              // Reduced for faster benchmarking
  config.computation_intensity = 500;

  // Initialize TBB with the same number of workers
  tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, config.num_workers);

  // Create benchmark instance
  ankerl::nanobench::Bench bench;
  bench.epochs(config.epochs);
  bench.warmup(config.warmup_epochs);
  bench.relative(true);
  bench.performanceCounters(true);

  // Run all benchmark suites
  SchedulerBenchmarkSuite::benchmark_task_throughput(bench, config);
  SchedulerBenchmarkSuite::benchmark_work_stealing_efficiency(bench, config);
  SchedulerBenchmarkSuite::benchmark_mathematical_computation(bench, config);
  SchedulerBenchmarkSuite::benchmark_task_submission_overhead(bench, config);
  SchedulerBenchmarkSuite::benchmark_cross_workgroup_communication(bench, config);

  // Generate performance report
  std::ofstream report("scheduler_benchmark_results.json");
  report << bench.render();
  report.close();

  std::cout << "\nBenchmark results saved to scheduler_benchmark_results.json" << std::endl;
  std::cout << "Summary table:" << std::endl;
  std::cout << bench.render() << std::endl;

  return 0;
}
