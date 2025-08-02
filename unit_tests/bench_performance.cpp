// SPDX-License-Identifier: MIT
#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include "ouly/allocators/coalescing_arena_allocator.hpp"
#include "ouly/allocators/ts_shared_linear_allocator.hpp"
#include "ouly/allocators/ts_thread_local_allocator.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
#include <cstdlib>
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

int main(int argc, char* argv[])
{
  std::cout << "Starting ouly performance benchmarks...\n";

  try
  {
    bench_ts_shared_linear_allocator();
    bench_ts_thread_local_allocator();
    bench_coalescing_arena_allocator();

    std::cout << "\nBenchmarks completed successfully!\n";

    // Enhanced JSON output for CI tracking and analysis
    std::string output_file = "benchmark_results.json";
    if (argc > 1)
    {
      output_file = argv[1];
    }

    // Check for CI environment variables to create consistent filenames
    const char* commit_hash_env  = std::getenv("GITHUB_SHA");
    const char* build_number_env = std::getenv("GITHUB_RUN_NUMBER");
    const char* compiler_id_env  = std::getenv("COMPILER_ID");

    constexpr int commit_hash_length = 8;
    std::string   commit_hash =
     (commit_hash_env != nullptr) ? std::string(commit_hash_env).substr(0, commit_hash_length) : "local";
    std::string build_number = (build_number_env != nullptr) ? std::string(build_number_env) : "0";
    std::string compiler_id  = (compiler_id_env != nullptr) ? std::string(compiler_id_env) : "unknown";

    // If we're in CI and no custom output file was specified, use the standard naming convention
    if ((commit_hash_env != nullptr || build_number_env != nullptr) && argc <= 1)
    {
      output_file = compiler_id + "-" + commit_hash + "-" + build_number + "-allocator_performance.json";
      std::cout << "Using CI naming convention: " << output_file << '\n';
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
