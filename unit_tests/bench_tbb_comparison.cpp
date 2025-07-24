#define ANKERL_NANOBENCH_IMPLEMENT
#define GLM_ENABLE_EXPERIMENTAL
#include "nanobench.h"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
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

// Test data structure for benchmarks
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

// Computational kernel for benchmarks
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

// GLM-based mathematical computation kernel
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
  volatile float temp = item.scalar_data;
  item.scalar_data    = temp;
}

// Benchmark: Basic task submission comparison
void bench_task_submission()
{
  std::cout << "Benchmarking task submission performance...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Task Submission Comparison").unit("task").warmup(5).epochIterations(50);

  constexpr int num_tasks   = 1000;
  constexpr int num_threads = 4;

  // TBB task submission benchmark
  bench.run("TBB task_arena submit",
            [&]
            {
              tbb::task_arena  arena(num_threads);
              std::atomic<int> counter{0};

              arena.execute(
               [&]
               {
                 for (int i = 0; i < num_tasks; ++i)
                 {
                   arena.enqueue(
                    [&counter]()
                    {
                      counter.fetch_add(1, std::memory_order_relaxed);
                    });
                 }
               });

              // Wait for completion
              arena.execute(
               [&]
               {
                 while (counter.load() < num_tasks)
                 {
                   std::this_thread::yield();
                 }
               });

              ankerl::nanobench::doNotOptimizeAway(counter.load());
            });

  // ouly::scheduler task submission benchmark
  bench.run("ouly::scheduler async",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, num_threads);
              scheduler.begin_execution();

              std::atomic<int> counter{0};
              auto             ctx = ouly::worker_context::get(ouly::workgroup_id(0));

              for (int i = 0; i < num_tasks; ++i)
              {
                ouly::async(ctx, ouly::workgroup_id(0),
                            [&counter](ouly::worker_context const&)
                            {
                              counter.fetch_add(1, std::memory_order_relaxed);
                            });
              }

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(counter.load());
            });
}

// Benchmark: Parallel for comparison
void bench_parallel_for()
{
  std::cout << "Benchmarking parallel_for performance...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Parallel For Comparison").unit("operation").warmup(5).epochIterations(20);

  constexpr size_t data_size = 100000;

  // TBB parallel_for benchmark
  bench.run("TBB parallel_for",
            [&]
            {
              std::vector<int> data(data_size);
              std::iota(data.begin(), data.end(), 0);

              tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
                                [&data](const tbb::blocked_range<size_t>& range)
                                {
                                  for (size_t i = range.begin(); i != range.end(); ++i)
                                  {
                                    compute_intensive_task(data[i], 50);
                                  }
                                });

              ankerl::nanobench::doNotOptimizeAway(data[0]);
            });

  // ouly parallel_for benchmark
  bench.run("ouly parallel_for",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::vector<int> data(data_size);
              std::iota(data.begin(), data.end(), 0);

              ouly::parallel_for(
               [](int& value, ouly::worker_context const&)
               {
                 compute_intensive_task(value, 50);
               },
               std::span(data), ouly::workgroup_id(0));

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(data[0]);
            });
}

// Benchmark: Work stealing efficiency
void bench_work_stealing()
{
  std::cout << "Benchmarking work stealing efficiency...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Work Stealing Comparison").unit("task").warmup(3).epochIterations(10);

  constexpr int num_tasks = 1000;

  // TBB work stealing benchmark
  bench.run("TBB work_stealing",
            [&]
            {
              tbb::task_arena        arena(4);
              std::atomic<long long> result{0};

              arena.execute(
               [&]
               {
                 tbb::parallel_for(0, num_tasks,
                                   [&result](int i)
                                   {
                                     // Variable work amount to trigger work stealing
                                     long long sum  = 0;
                                     int       work = 100 + (i % 500);
                                     for (int j = 0; j < work; ++j)
                                     {
                                       sum += j * j;
                                     }
                                     result.fetch_add(sum, std::memory_order_relaxed);
                                   });
               });

              ankerl::nanobench::doNotOptimizeAway(result.load());
            });

  // ouly scheduler work stealing benchmark
  bench.run("ouly work_stealing",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::atomic<long long> result{0};
              auto                   ctx = ouly::worker_context::get(ouly::workgroup_id(0));

              // Submit compute-intensive tasks that vary in duration
              for (int i = 0; i < num_tasks; ++i)
              {
                ouly::async(ctx, ouly::workgroup_id(0),
                            [&result, i](ouly::worker_context const&)
                            {
                              long long sum = 0;
                              // Variable work amount to trigger work stealing
                              int work = 100 + (i % 500);
                              for (int j = 0; j < work; ++j)
                              {
                                sum += j * j;
                              }
                              result.fetch_add(sum, std::memory_order_relaxed);
                            });
              }

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(result.load());
            });
}

// Benchmark: Multi-workgroup vs TBB task arena
void bench_multi_workgroup()
{
  std::cout << "Benchmarking multi-workgroup vs TBB task arena...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Multi-Group Scheduling").unit("task").warmup(3).epochIterations(10);

  constexpr int tasks_per_group = 250;

  // TBB multiple task arenas
  bench.run("TBB multi_arena",
            [&]
            {
              tbb::task_arena  arena1(2);
              tbb::task_arena  arena2(2);
              std::atomic<int> counter{0};

              std::thread t1(
               [&]
               {
                 arena1.execute(
                  [&]
                  {
                    for (int i = 0; i < tasks_per_group; ++i)
                    {
                      arena1.enqueue(
                       [&counter]()
                       {
                         counter.fetch_add(1, std::memory_order_relaxed);
                       });
                    }
                  });
               });

              std::thread t2(
               [&]
               {
                 arena2.execute(
                  [&]
                  {
                    for (int i = 0; i < tasks_per_group; ++i)
                    {
                      arena2.enqueue(
                       [&counter]()
                       {
                         counter.fetch_add(1, std::memory_order_relaxed);
                       });
                    }
                  });
               });

              t1.join();
              t2.join();

              // Wait for completion
              while (counter.load() < (tasks_per_group * 2))
              {
                std::this_thread::yield();
              }

              ankerl::nanobench::doNotOptimizeAway(counter.load());
            });

  // ouly multi-workgroup
  bench.run("ouly multi_workgroup",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 2);
              scheduler.create_group(ouly::workgroup_id(1), 2, 2);
              scheduler.begin_execution();

              std::atomic<int> counter{0};
              auto             ctx = ouly::worker_context::get(ouly::workgroup_id(0));

              // Submit tasks to both workgroups
              for (int i = 0; i < tasks_per_group; ++i)
              {
                ouly::async(ctx, ouly::workgroup_id(0),
                            [&counter](ouly::worker_context const&)
                            {
                              counter.fetch_add(1, std::memory_order_relaxed);
                            });

                ouly::async(ctx, ouly::workgroup_id(1),
                            [&counter](ouly::worker_context const&)
                            {
                              counter.fetch_add(1, std::memory_order_relaxed);
                            });
              }

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(counter.load());
            });
}

// Benchmark: Memory allocation patterns
void bench_memory_allocation()
{
  std::cout << "Benchmarking memory allocation patterns...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Memory Allocation Patterns").unit("allocation").warmup(5).epochIterations(50);

  constexpr int num_allocations = 1000;

  // TBB standard allocator (scalable_allocator may not be available in all versions)
  bench.run("TBB with_std_allocator",
            [&]
            {
              std::vector<std::vector<int>> vectors;
              vectors.reserve(num_allocations);

              tbb::parallel_for(0, num_allocations,
                                [&vectors](int i)
                                {
                                  std::vector<int> vec(100 + (i % 200));
                                  std::iota(vec.begin(), vec.end(), i);
                                  // Simulate some work with the vector
                                  volatile int sum = 0;
                                  for (int val : vec)
                                  {
                                    sum += val;
                                  }
                                  ankerl::nanobench::doNotOptimizeAway(sum);
                                });
            });

  // Standard allocator with ouly scheduler
  bench.run("ouly with std_allocator",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::atomic<int> completed{0};
              auto             ctx = ouly::worker_context::get(ouly::workgroup_id(0));

              for (int i = 0; i < num_allocations; ++i)
              {
                ouly::async(ctx, ouly::workgroup_id(0),
                            [&completed, i](ouly::worker_context const&)
                            {
                              std::vector<int> vec(100 + (i % 200));
                              std::iota(vec.begin(), vec.end(), i);
                              // Simulate some work with the vector
                              volatile int sum = 0;
                              for (int val : vec)
                              {
                                sum += val;
                              }
                              ankerl::nanobench::doNotOptimizeAway(sum);
                              completed.fetch_add(1);
                            });
              }

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(completed.load());
            });
}

// Benchmark: GLM Math Operations - Vector Math
void bench_glm_vector_math()
{
  std::cout << "Benchmarking GLM vector mathematics...\n";

  ankerl::nanobench::Bench bench;
  bench.title("GLM Vector Math Operations").unit("operation").warmup(3).epochIterations(20);

  constexpr size_t data_size = 50000;

  // TBB parallel vector math operations
  bench.run("TBB_glm_vector_math",
            [&]
            {
              std::vector<MathWorkItem> data(data_size);
              for (size_t i = 0; i < data_size; ++i)
              {
                data[i] = MathWorkItem(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
              }

              tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
                                [&data](const tbb::blocked_range<size_t>& range)
                                {
                                  for (size_t i = range.begin(); i != range.end(); ++i)
                                  {
                                    compute_math_intensive_task(data[i]);
                                  }
                                });

              ankerl::nanobench::doNotOptimizeAway(data[0].scalar_data);
            });

  // ouly parallel vector math operations
  bench.run("ouly_glm_vector_math",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::vector<MathWorkItem> data(data_size);
              for (size_t i = 0; i < data_size; ++i)
              {
                data[i] = MathWorkItem(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
              }

              ouly::parallel_for(
               [](MathWorkItem& item, ouly::worker_context const&)
               {
                 compute_math_intensive_task(item);
               },
               std::span(data), ouly::workgroup_id(0));

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(data[0].scalar_data);
            });
}

// Benchmark: GLM Math Operations - Matrix Transformations
void bench_glm_matrix_math()
{
  std::cout << "Benchmarking GLM matrix transformations...\n";

  ankerl::nanobench::Bench bench;
  bench.title("GLM Matrix Transform Operations").unit("operation").warmup(3).epochIterations(15);

  constexpr size_t matrix_count = 10000;

  // TBB parallel matrix transformations
  bench.run("TBB_glm_matrix_transforms",
            [&]
            {
              std::vector<glm::mat4> matrices(matrix_count, glm::mat4(1.0f));
              std::vector<glm::vec3> positions(matrix_count);
              std::vector<glm::vec3> rotations(matrix_count);
              std::vector<glm::vec3> scales(matrix_count);

              // Initialize data
              for (size_t i = 0; i < matrix_count; ++i)
              {
                positions[i] = glm::vec3(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
                rotations[i] =
                 glm::vec3(static_cast<float>(i * 0.1f), static_cast<float>(i * 0.2f), static_cast<float>(i * 0.3f));
                scales[i] = glm::vec3(1.0f + static_cast<float>(i) * 0.01f);
              }

              tbb::parallel_for(tbb::blocked_range<size_t>(0, matrix_count),
                                [&](const tbb::blocked_range<size_t>& range)
                                {
                                  for (size_t i = range.begin(); i != range.end(); ++i)
                                  {
                                    glm::mat4 transform = glm::mat4(1.0f);
                                    transform           = glm::translate(transform, positions[i]);
                                    transform   = glm::rotate(transform, rotations[i].x, glm::vec3(1.0f, 0.0f, 0.0f));
                                    transform   = glm::rotate(transform, rotations[i].y, glm::vec3(0.0f, 1.0f, 0.0f));
                                    transform   = glm::rotate(transform, rotations[i].z, glm::vec3(0.0f, 0.0f, 1.0f));
                                    transform   = glm::scale(transform, scales[i]);
                                    matrices[i] = transform;
                                  }
                                });

              ankerl::nanobench::doNotOptimizeAway(matrices[0][0][0]);
            });

  // ouly parallel matrix transformations
  bench.run("ouly_glm_matrix_transforms",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::vector<glm::mat4> matrices(matrix_count, glm::mat4(1.0f));
              std::vector<glm::vec3> positions(matrix_count);
              std::vector<glm::vec3> rotations(matrix_count);
              std::vector<glm::vec3> scales(matrix_count);

              // Initialize data
              for (size_t i = 0; i < matrix_count; ++i)
              {
                positions[i] = glm::vec3(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
                rotations[i] =
                 glm::vec3(static_cast<float>(i * 0.1f), static_cast<float>(i * 0.2f), static_cast<float>(i * 0.3f));
                scales[i] = glm::vec3(1.0f + static_cast<float>(i) * 0.01f);
              }

              // Use a simpler indexed approach for ouly parallel_for
              std::vector<size_t> indices(matrix_count);
              std::iota(indices.begin(), indices.end(), 0);

              ouly::parallel_for(
               [&matrices, &positions, &rotations, &scales](size_t& index, ouly::worker_context const& ctx)
               {
                 glm::mat4 transform = glm::mat4(1.0f);
                 transform           = glm::translate(transform, positions[index]);
                 transform           = glm::rotate(transform, rotations[index].x, glm::vec3(1.0f, 0.0f, 0.0f));
                 transform           = glm::rotate(transform, rotations[index].y, glm::vec3(0.0f, 1.0f, 0.0f));
                 transform           = glm::rotate(transform, rotations[index].z, glm::vec3(0.0f, 0.0f, 1.0f));
                 transform           = glm::scale(transform, scales[index]);
                 matrices[index]     = transform;
               },
               std::span(indices), ouly::workgroup_id(0));

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(matrices[0][0][0]);
            });
}

// Benchmark: Physics simulation with GLM
void bench_glm_physics_simulation()
{
  std::cout << "Benchmarking GLM physics simulation...\n";

  ankerl::nanobench::Bench bench;
  bench.title("GLM Physics Simulation").unit("particle").warmup(3).epochIterations(10);

  constexpr size_t particle_count = 5000;
  constexpr float  dt             = 0.016f; // 60 FPS timestep

  // TBB physics simulation
  bench.run("TBB_glm_physics",
            [&]
            {
              std::vector<MathWorkItem> particles(particle_count);

              // Initialize particles
              for (size_t i = 0; i < particle_count; ++i)
              {
                particles[i] =
                 MathWorkItem(static_cast<float>(i % 100) - 50.0f, static_cast<float>((i / 100) % 100) - 50.0f,
                              static_cast<float>(i / 10000) - 50.0f);
                particles[i].velocity = glm::vec3(static_cast<float>((i * 13) % 200 - 100) * 0.1f,
                                                  static_cast<float>((i * 17) % 200 - 100) * 0.1f,
                                                  static_cast<float>((i * 19) % 200 - 100) * 0.1f);
              }

              // Simulate multiple timesteps
              for (int step = 0; step < 10; ++step)
              {
                tbb::parallel_for(tbb::blocked_range<size_t>(0, particle_count),
                                  [&particles, dt](const tbb::blocked_range<size_t>& range)
                                  {
                                    for (size_t i = range.begin(); i != range.end(); ++i)
                                    {
                                      compute_math_intensive_task(particles[i], dt);

                                      // Add some inter-particle interaction
                                      if (i > 0)
                                      {
                                        glm::vec3 diff = particles[i].position - particles[i - 1].position;
                                        float     dist = glm::length(diff);
                                        if (dist > 0.0f && dist < 10.0f)
                                        {
                                          glm::vec3 force = glm::normalize(diff) * (10.0f - dist) * 0.1f;
                                          particles[i].acceleration += force;
                                        }
                                      }
                                    }
                                  });
              }

              ankerl::nanobench::doNotOptimizeAway(particles[0].position.x);
            });

  // ouly physics simulation
  bench.run("ouly_glm_physics",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::vector<MathWorkItem> particles(particle_count);

              // Initialize particles
              for (size_t i = 0; i < particle_count; ++i)
              {
                particles[i] =
                 MathWorkItem(static_cast<float>(i % 100) - 50.0f, static_cast<float>((i / 100) % 100) - 50.0f,
                              static_cast<float>(i / 10000) - 50.0f);
                particles[i].velocity = glm::vec3(static_cast<float>((i * 13) % 200 - 100) * 0.1f,
                                                  static_cast<float>((i * 17) % 200 - 100) * 0.1f,
                                                  static_cast<float>((i * 19) % 200 - 100) * 0.1f);
              }

              // Simulate multiple timesteps
              for (int step = 0; step < 10; ++step)
              {
                ouly::parallel_for(
                 [dt, &particles](MathWorkItem& particle, ouly::worker_context const& ctx)
                 {
                   compute_math_intensive_task(particle, dt);

                   // Add some inter-particle interaction
                   auto index = &particle - particles.data();
                   if (index > 0 && index < static_cast<long>(particles.size()))
                   {
                     glm::vec3 diff = particle.position - particles[index - 1].position;
                     float     dist = glm::length(diff);
                     if (dist > 0.0f && dist < 10.0f)
                     {
                       glm::vec3 force = glm::normalize(diff) * (10.0f - dist) * 0.1f;
                       particle.acceleration += force;
                     }
                   }
                 },
                 std::span(particles), ouly::workgroup_id(0));
              }

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(particles[0].position.x);
            });
}

int main(int argc, char* argv[])
{
  std::cout << "Starting ouly vs oneTBB performance comparison...\n";
  std::cout << "TBB Version: " << TBB_VERSION_MAJOR << "." << TBB_VERSION_MINOR << "\n";

  // Initialize TBB global control for consistent thread count
  tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, 4);

  try
  {
    bench_task_submission();
    bench_parallel_for();
    bench_work_stealing();
    bench_multi_workgroup();
    bench_memory_allocation();

    // New GLM-based benchmarks
    bench_glm_vector_math();
    bench_glm_matrix_math();
    bench_glm_physics_simulation();

    std::cout << "\nTBB vs ouly comparison benchmarks completed successfully!\n";

    // Enhanced JSON output for CI tracking and analysis
    std::string output_file = "tbb_comparison_results.json";
    if (argc > 1)
    {
      output_file = argv[1];
    }

    // Create a comprehensive benchmark that includes representative results for JSON output
    std::cout << "\nGenerating detailed JSON output...\n";
    ankerl::nanobench::Bench json_bench;
    json_bench.title("TBB vs Ouly Performance Comparison").unit("operation").warmup(3).epochIterations(50);

    // Run simplified versions of each benchmark for JSON output
    {
      // Task submission comparison
      tbb::task_arena  arena(4);
      std::atomic<int> counter{0};
      json_bench.run("TBB_task_submission",
                     [&]
                     {
                       arena.enqueue(
                        [&counter]()
                        {
                          counter.fetch_add(1);
                        });
                     });
    }

    {
      ouly::scheduler scheduler;
      scheduler.create_group(ouly::workgroup_id(0), 0, 4);
      scheduler.begin_execution();
      std::atomic<int> counter{0};
      auto             ctx = ouly::worker_context::get(ouly::workgroup_id(0));

      json_bench.run("ouly_task_submission",
                     [&]
                     {
                       ouly::async(ctx, ouly::workgroup_id(0),
                                   [&counter](ouly::worker_context const&)
                                   {
                                     counter.fetch_add(1);
                                   });
                     });
      scheduler.end_execution();
    }

    // Parallel for comparison
    {
      std::vector<int> data(1000);
      std::iota(data.begin(), data.end(), 0);

      json_bench.run("TBB_parallel_for_small",
                     [&]
                     {
                       tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
                                         [&data](const tbb::blocked_range<size_t>& range)
                                         {
                                           for (size_t i = range.begin(); i != range.end(); ++i)
                                           {
                                             data[i] *= 2;
                                           }
                                         });
                     });
    }

    {
      ouly::scheduler scheduler;
      scheduler.create_group(ouly::workgroup_id(0), 0, 4);
      scheduler.begin_execution();

      std::vector<int> data(1000);
      std::iota(data.begin(), data.end(), 0);

      json_bench.run("ouly_parallel_for_small",
                     [&]
                     {
                       ouly::parallel_for(
                        [](int& value, ouly::worker_context const&)
                        {
                          value *= 2;
                        },
                        std::span(data), ouly::workgroup_id(0));
                     });

      scheduler.end_execution();
    }

    // GLM math benchmarks for JSON output
    {
      std::vector<MathWorkItem> data(1000);
      for (size_t i = 0; i < 1000; ++i)
      {
        data[i] = MathWorkItem(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
      }

      json_bench.run("TBB_glm_vector_math_small",
                     [&]
                     {
                       tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
                                         [&data](const tbb::blocked_range<size_t>& range)
                                         {
                                           for (size_t i = range.begin(); i != range.end(); ++i)
                                           {
                                             compute_math_intensive_task(data[i]);
                                           }
                                         });
                     });
    }

    {
      ouly::scheduler scheduler;
      scheduler.create_group(ouly::workgroup_id(0), 0, 4);
      scheduler.begin_execution();

      std::vector<MathWorkItem> data(1000);
      for (size_t i = 0; i < 1000; ++i)
      {
        data[i] = MathWorkItem(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3));
      }

      json_bench.run("ouly_glm_vector_math_small",
                     [&]
                     {
                       ouly::parallel_for(
                        [](MathWorkItem& item, ouly::worker_context const&)
                        {
                          compute_math_intensive_task(item);
                        },
                        std::span(data), ouly::workgroup_id(0));
                     });

      scheduler.end_execution();
    }

    // Output comprehensive JSON results
    std::ofstream json_file(output_file);
    if (json_file.is_open())
    {
      ankerl::nanobench::render(ankerl::nanobench::templates::json(), json_bench, json_file);
      json_file.close();
      std::cout << "Detailed JSON results saved to " << output_file << "\n";
    }
    else
    {
      std::cerr << "Failed to open " << output_file << " for writing\n";
      return 1;
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "TBB comparison benchmark failed with exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
