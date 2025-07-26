#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include "ouly/allocators/coalescing_arena_allocator.hpp"
#include "ouly/allocators/ts_shared_linear_allocator.hpp"
#include "ouly/allocators/ts_thread_local_allocator.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

// Include oneTBB headers for comparison benchmarks
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_arena.h>
#include <tbb/tbb.h>

// Helper struct for coalescing arena allocator benchmarks
struct simple_memory_manager
{
  std::vector<std::pair<ouly::arena_id, std::vector<uint8_t>>> arenas;

  void add(ouly::arena_id id, ouly::allocation_size_type size)
  {
    arenas.emplace_back(id, std::vector<uint8_t>(size));
  }

  void remove(ouly::arena_id id)
  {
    arenas.erase(std::remove_if(arenas.begin(), arenas.end(),
                                [id](const auto& arena)
                                {
                                  return arena.first == id;
                                }),
                 arenas.end());
  }

  auto get_memory(ouly::arena_id id) -> uint8_t*
  {
    auto it = std::find_if(arenas.begin(), arenas.end(),
                           [id](const auto& arena)
                           {
                             return arena.first == id;
                           });
    return it != arenas.end() ? it->second.data() : nullptr;
  }
};

// Memory allocator benchmarks
void bench_ts_shared_linear_allocator()
{
  std::cout << "Benchmarking ts_shared_linear_allocator...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Thread-Safe Shared Linear Allocator").unit("allocation").warmup(10).epochIterations(100);

  // Single-threaded allocation benchmark
  bench.run("ts_shared_linear single-thread alloc/dealloc",
            [&]
            {
              ouly::ts_shared_linear_allocator allocator;
              std::vector<void*>               ptrs;
              ptrs.reserve(1000);

              // Allocate
              for (int i = 0; i < 1000; ++i)
              {
                void* ptr = allocator.allocate(64 + (i % 128));
                ptrs.push_back(ptr);
                ankerl::nanobench::doNotOptimizeAway(ptr);
              }

              // Deallocate in reverse order
              for (auto it = ptrs.rbegin(); it != ptrs.rend(); ++it)
              {
                allocator.deallocate(*it, 64);
              }
            });

  // Multi-threaded allocation benchmark
  bench.run("ts_shared_linear multi-thread alloc",
            [&]
            {
              ouly::ts_shared_linear_allocator allocator;
              constexpr int                    num_threads       = 4;
              constexpr int                    allocs_per_thread = 250;

              std::vector<std::thread> threads;
              std::atomic<int>         ready_count{0};
              std::atomic<bool>        start_flag{false};

              for (int t = 0; t < num_threads; ++t)
              {
                threads.emplace_back(
                 [&, t]()
                 {
                   ready_count.fetch_add(1);
                   while (!start_flag.load())
                   {
                     std::this_thread::yield();
                   }

                   std::vector<void*> ptrs;
                   for (int i = 0; i < allocs_per_thread; ++i)
                   {
                     void* ptr = allocator.allocate(32 + (i % 64));
                     ptrs.push_back(ptr);
                     ankerl::nanobench::doNotOptimizeAway(ptr);
                   }

                   // Try to deallocate some (may fail due to concurrent access)
                   for (auto it = ptrs.rbegin(); it != ptrs.rend(); ++it)
                   {
                     allocator.deallocate(*it, 32);
                   }
                 });
              }

              // Wait for all threads to be ready
              while (ready_count.load() < num_threads)
              {
                std::this_thread::yield();
              }

              start_flag.store(true);

              for (auto& t : threads)
              {
                t.join();
              }
            });

  // Reset benchmark
  bench.run("ts_shared_linear reset",
            [&]
            {
              ouly::ts_shared_linear_allocator allocator;

              // Allocate some memory
              for (int i = 0; i < 100; ++i)
              {
                void* ptr = allocator.allocate(128);
                ankerl::nanobench::doNotOptimizeAway(ptr);
              }

              // Benchmark reset
              allocator.reset();
            });
}

void bench_ts_thread_local_allocator()
{
  std::cout << "Benchmarking ts_thread_local_allocator...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Thread-Safe Thread Local Allocator").unit("allocation").warmup(10).epochIterations(100);

  // Single-threaded allocation benchmark
  bench.run("ts_thread_local single-thread alloc/dealloc",
            [&]
            {
              ouly::ts_thread_local_allocator allocator;
              std::vector<void*>              ptrs;
              ptrs.reserve(1000);

              // Allocate
              for (int i = 0; i < 1000; ++i)
              {
                void* ptr = allocator.allocate(64 + (i % 128));
                ptrs.push_back(ptr);
                ankerl::nanobench::doNotOptimizeAway(ptr);
              }

              // Deallocate in reverse order
              for (auto it = ptrs.rbegin(); it != ptrs.rend(); ++it)
              {
                allocator.deallocate(*it, 64);
              }
            });

  // Multi-threaded allocation benchmark - should be very fast due to thread-local arenas
  bench.run("ts_thread_local multi-thread alloc",
            [&]
            {
              ouly::ts_thread_local_allocator allocator;
              constexpr int                   num_threads       = 4;
              constexpr int                   allocs_per_thread = 250;

              std::vector<std::thread> threads;
              std::atomic<int>         ready_count{0};
              std::atomic<bool>        start_flag{false};

              for (int t = 0; t < num_threads; ++t)
              {
                threads.emplace_back(
                 [&, t]()
                 {
                   ready_count.fetch_add(1);
                   while (!start_flag.load())
                   {
                     std::this_thread::yield();
                   }

                   std::vector<void*> ptrs;
                   for (int i = 0; i < allocs_per_thread; ++i)
                   {
                     void* ptr = allocator.allocate(32 + (i % 64));
                     ptrs.push_back(ptr);
                     ankerl::nanobench::doNotOptimizeAway(ptr);
                   }

                   // Deallocate in reverse order (should mostly succeed due to thread-local arenas)
                   for (auto it = ptrs.rbegin(); it != ptrs.rend(); ++it)
                   {
                     allocator.deallocate(*it, 32);
                   }
                 });
              }

              // Wait for all threads to be ready
              while (ready_count.load() < num_threads)
              {
                std::this_thread::yield();
              }

              start_flag.store(true);

              for (auto& t : threads)
              {
                t.join();
              }
            });

  // Reset benchmark
  bench.run("ts_thread_local reset",
            [&]
            {
              ouly::ts_thread_local_allocator allocator;

              // Allocate some memory from multiple threads
              std::vector<std::thread> threads;
              for (int t = 0; t < 4; ++t)
              {
                threads.emplace_back(
                 [&]()
                 {
                   for (int i = 0; i < 25; ++i)
                   {
                     void* ptr = allocator.allocate(128);
                     ankerl::nanobench::doNotOptimizeAway(ptr);
                   }
                 });
              }

              for (auto& t : threads)
              {
                t.join();
              }

              // Benchmark reset
              allocator.reset();
            });
}

void bench_coalescing_arena_allocator()
{
  std::cout << "Benchmarking coalescing_arena_allocator...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Coalescing Arena Allocator").unit("allocation").warmup(10).epochIterations(50);

  // Memory manager for coalescing allocator
  struct simple_memory_manager
  {
    std::vector<std::pair<ouly::arena_id, std::vector<uint8_t>>> arenas;

    void add(ouly::arena_id id, ouly::allocation_size_type size)
    {
      arenas.emplace_back(id, std::vector<uint8_t>(size));
    }

    void remove(ouly::arena_id id)
    {
      arenas.erase(std::remove_if(arenas.begin(), arenas.end(),
                                  [id](const auto& arena)
                                  {
                                    return arena.first == id;
                                  }),
                   arenas.end());
    }

    void* get_arena_ptr(ouly::arena_id id)
    {
      auto it = std::find_if(arenas.begin(), arenas.end(),
                             [id](const auto& arena)
                             {
                               return arena.first == id;
                             });
      return it != arenas.end() ? it->second.data() : nullptr;
    }
  };

  // Allocation/deallocation benchmark
  bench.run("coalescing_arena alloc/dealloc",
            [&]
            {
              simple_memory_manager            manager;
              ouly::coalescing_arena_allocator allocator;
              allocator.set_arena_size(10000);

              std::vector<ouly::ca_allocation> allocations;
              allocations.reserve(100);

              // Mixed allocation sizes
              std::vector<size_t> sizes = {16, 32, 64, 128, 256, 512, 1024};

              // Allocate
              for (int i = 0; i < 100; ++i)
              {
                size_t size  = sizes[i % sizes.size()];
                auto   alloc = allocator.allocate(size, manager);
                allocations.push_back(alloc);
                ankerl::nanobench::doNotOptimizeAway(alloc.get_offset());
              }

              // Deallocate in random order to test coalescing
              std::random_device rd;
              std::mt19937       g(rd());
              std::shuffle(allocations.begin(), allocations.end(), g);

              for (const auto& alloc : allocations)
              {
                allocator.deallocate(alloc.get_allocation_id(), manager);
              }
            });

  // Fragmentation test
  bench.run("coalescing_arena fragmentation",
            [&]
            {
              simple_memory_manager            manager;
              ouly::coalescing_arena_allocator allocator;
              allocator.set_arena_size(10000);

              std::vector<ouly::ca_allocation> allocations;

              // Create fragmentation by allocating and deallocating every other allocation
              for (int i = 0; i < 50; ++i)
              {
                auto alloc = allocator.allocate(64, manager);
                allocations.push_back(alloc);
              }

              // Deallocate every other allocation
              for (size_t i = 1; i < allocations.size(); i += 2)
              {
                allocator.deallocate(allocations[i].get_allocation_id(), manager);
              }

              // Now try to allocate larger blocks (should trigger coalescing)
              for (int i = 0; i < 20; ++i)
              {
                auto alloc = allocator.allocate(128, manager);
                ankerl::nanobench::doNotOptimizeAway(alloc.get_offset());
              }
            });
}

void bench_scheduler()
{
  std::cout << "Benchmarking scheduler...\n";

  ankerl::nanobench::Bench bench;
  bench.title("Scheduler Performance").unit("task").warmup(5).epochIterations(10);

  // Task submission benchmark
  bench.run("scheduler task_submission",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::atomic<int> counter{0};
              constexpr int    num_tasks = 1000;

              auto ctx = ouly::task_context::get(ouly::workgroup_id(0));

              for (int i = 0; i < num_tasks; ++i)
              {
                ouly::async(ctx, ouly::workgroup_id(0),
                            [&counter](ouly::task_context const&)
                            {
                              counter.fetch_add(1, std::memory_order_relaxed);
                            });
              }

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(counter.load());
            });

  // Parallel for benchmark
  bench.run("scheduler parallel_for",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::vector<int> data(10000);
              std::iota(data.begin(), data.end(), 0);

              ouly::parallel_for(
               [](int& value, ouly::task_context const&)
               {
                 value *= 2;
                 ankerl::nanobench::doNotOptimizeAway(value);
               },
               std::span(data), ouly::workgroup_id(0));

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(data[0]);
            });

  // Work stealing benchmark
  bench.run("scheduler work_stealing",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::atomic<long long> result{0};
              constexpr int          num_tasks = 500;

              auto ctx = ouly::task_context::get(ouly::workgroup_id(0));

              // Submit compute-intensive tasks that vary in duration
              for (int i = 0; i < num_tasks; ++i)
              {
                ouly::async(ctx, ouly::workgroup_id(0),
                            [&result, i](ouly::task_context const&)
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

  // Multi-workgroup benchmark
  bench.run("scheduler multi_workgroup",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 2);
              scheduler.create_group(ouly::workgroup_id(1), 2, 2);
              scheduler.begin_execution();

              std::atomic<int> counter{0};
              constexpr int    tasks_per_group = 250;

              auto ctx = ouly::task_context::get(ouly::workgroup_id(0));

              // Submit tasks to both workgroups
              for (int i = 0; i < tasks_per_group; ++i)
              {
                ouly::async(ctx, ouly::workgroup_id(0),
                            [&counter](ouly::task_context const&)
                            {
                              counter.fetch_add(1, std::memory_order_relaxed);
                            });

                ouly::async(ctx, ouly::workgroup_id(1),
                            [&counter](ouly::task_context const&)
                            {
                              counter.fetch_add(1, std::memory_order_relaxed);
                            });
              }

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(counter.load());
            });
}

// TBB comparison benchmarks
void bench_tbb_parallel_for()
{
  std::cout << "Benchmarking TBB parallel_for...\n";

  ankerl::nanobench::Bench bench;
  bench.title("TBB Parallel For").unit("task").warmup(5).epochIterations(10);

  bench.run("tbb parallel_for",
            [&]
            {
              constexpr int    num_elements = 10000;
              std::vector<int> data(num_elements);
              std::iota(data.begin(), data.end(), 0);

              // TBB parallel_for
              tbb::parallel_for(0, num_elements,
                                [&data](int i)
                                {
                                  data[i] *= 2;
                                  ankerl::nanobench::doNotOptimizeAway(data[i]);
                                });

              ankerl::nanobench::doNotOptimizeAway(data[0]);
            });
}

void bench_tbb_task_arena()
{
  std::cout << "Benchmarking TBB task_arena...\n";

  ankerl::nanobench::Bench bench;
  bench.title("TBB Task Arena").unit("task").warmup(5).epochIterations(10);

  bench.run("tbb task_arena",
            [&]
            {
              constexpr int num_tasks = 1000;

              // Use a task_arena with 4 threads
              tbb::task_arena arena{4};

              arena.execute(
               [&]
               {
                 tbb::parallel_for(0, num_tasks,
                                   [](int i)
                                   {
                                     // Simple task that does some work
                                     long long sum = 0;
                                     for (int j = 0; j < 100; ++j)
                                     {
                                       sum += i * j;
                                     }
                                     ankerl::nanobench::doNotOptimizeAway(sum);
                                   });
               });
            });
}

void bench_tbb_global_control()
{
  std::cout << "Benchmarking TBB global_control...\n";

  ankerl::nanobench::Bench bench;
  bench.title("TBB Global Control").unit("task").warmup(5).epochIterations(10);

  bench.run("tbb global_control",
            [&]
            {
              constexpr int num_tasks = 1000;

              // Limit TBB to 4 threads
              tbb::global_control c(tbb::global_control::max_allowed_parallelism, 4);

              tbb::parallel_for(0, num_tasks,
                                [](int i)
                                {
                                  // Simple task that does some work
                                  long long sum = 0;
                                  for (int j = 0; j < 100; ++j)
                                  {
                                    sum += i * j;
                                  }
                                  ankerl::nanobench::doNotOptimizeAway(sum);
                                });
            });
}

// TBB vs ouly comparison benchmarks
void bench_tbb_vs_ouly_comparison()
{
  std::cout << "Benchmarking TBB vs ouly comparison...\n";

  ankerl::nanobench::Bench bench;
  bench.title("TBB vs Ouly Scheduler Comparison").unit("operation").warmup(5).epochIterations(50);

  constexpr int    num_tasks = 1000;
  constexpr size_t data_size = 10000;

  // Task submission comparison
  bench.run("TBB task_arena submit",
            [&]
            {
              tbb::task_arena  arena(4);
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

  bench.run("ouly scheduler async",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::atomic<int> counter{0};
              auto             ctx = ouly::task_context::get(ouly::workgroup_id(0));

              for (int i = 0; i < num_tasks; ++i)
              {
                ouly::async(ctx, ouly::workgroup_id(0),
                            [&counter](ouly::task_context const&)
                            {
                              counter.fetch_add(1, std::memory_order_relaxed);
                            });
              }

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(counter.load());
            });

  // Parallel for comparison
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
                                    data[i] = data[i] * 2 + 1;
                                  }
                                });

              ankerl::nanobench::doNotOptimizeAway(data[0]);
            });

  bench.run("ouly parallel_for",
            [&]
            {
              ouly::scheduler scheduler;
              scheduler.create_group(ouly::workgroup_id(0), 0, 4);
              scheduler.begin_execution();

              std::vector<int> data(data_size);
              std::iota(data.begin(), data.end(), 0);

              ouly::parallel_for(
               [](int& value, ouly::task_context const&)
               {
                 value = value * 2 + 1;
               },
               std::span(data), ouly::workgroup_id(0));

              scheduler.end_execution();
              ankerl::nanobench::doNotOptimizeAway(data[0]);
            });
}

int main(int argc, char* argv[])
{
  std::cout << "Starting ouly performance benchmarks...\n";

  try
  {
    bench_ts_shared_linear_allocator();
    bench_ts_thread_local_allocator();
    bench_coalescing_arena_allocator();
    bench_scheduler();
    bench_tbb_parallel_for();
    bench_tbb_task_arena();
    bench_tbb_global_control();
    bench_tbb_vs_ouly_comparison();

    std::cout << "\nBenchmarks completed successfully!\n";

    // Enhanced JSON output for CI tracking and analysis
    std::string output_file = "benchmark_results.json";
    if (argc > 1)
    {
      output_file = argv[1];
    }

    // Create a comprehensive benchmark that includes representative results for JSON output
    std::cout << "\nGenerating detailed JSON output...\n";
    ankerl::nanobench::Bench json_bench;
    json_bench.title("Ouly Performance Benchmarks").unit("operation").warmup(3).epochIterations(100);

    // Run simplified versions of each benchmark for JSON output
    {
      ouly::ts_shared_linear_allocator allocator;
      json_bench.run("ts_shared_linear_single_thread",
                     [&]
                     {
                       void* ptr = allocator.allocate(64);
                       ankerl::nanobench::doNotOptimizeAway(ptr);
                       allocator.deallocate(ptr, 64);
                     });
    }

    {
      ouly::ts_thread_local_allocator allocator;
      json_bench.run("ts_thread_local_single_thread",
                     [&]
                     {
                       void* ptr = allocator.allocate(64);
                       ankerl::nanobench::doNotOptimizeAway(ptr);
                       allocator.deallocate(ptr, 64);
                     });
      allocator.reset();
    }

    {
      simple_memory_manager            manager;
      ouly::coalescing_arena_allocator allocator;
      allocator.set_arena_size(10000);
      json_bench.run("coalescing_arena_alloc_dealloc",
                     [&]
                     {
                       auto allocation = allocator.allocate(64, manager);
                       ankerl::nanobench::doNotOptimizeAway(allocation.get_offset());
                       allocator.deallocate(allocation.get_allocation_id(), manager);
                     });
    }

    // Basic scheduler benchmark for JSON
    {
      json_bench.run("scheduler_task_submission",
                     [&]
                     {
                       ouly::scheduler scheduler;
                       scheduler.create_group(ouly::workgroup_id(0), 0, 1);
                       scheduler.begin_execution();

                       std::atomic<int> counter{0};
                       auto             ctx = ouly::task_context::get(ouly::workgroup_id(0));

                       ouly::async(ctx, ouly::workgroup_id(0),
                                   [&counter](ouly::task_context const&)
                                   {
                                     counter.fetch_add(1, std::memory_order_relaxed);
                                   });

                       scheduler.end_execution();
                       ankerl::nanobench::doNotOptimizeAway(counter.load());
                     });
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
    std::cerr << "Benchmark failed with exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
