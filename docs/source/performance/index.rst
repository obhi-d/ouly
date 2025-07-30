Performance Guide
================

Optimization techniques and performance analysis for OULY components.

.. toctree::
   :maxdepth: 2

   optimization_guide
   memory_profiling
   parallel_performance
   best_practices

Overview
--------

This section covers performance optimization with OULY, including:

* **Optimization Guide** - Techniques for maximizing performance  
* **Memory Profiling** - Tools and techniques for memory analysis
* **Parallel Performance** - Scaling and threading considerations
* **Best Practices** - Proven patterns for high-performance applications

Performance Philosophy
----------------------

OULY is designed with performance as a primary goal:

**Zero-Cost Abstractions**
   Template-heavy design that compiles to optimal machine code.

**Cache-Friendly Design**
   Structure of Arrays (SoA) patterns and memory layout optimization.

**Lock-Free Algorithms**
   Atomic operations and work-stealing for minimal contention.

**Memory Layout Optimization**
   Structure of Arrays (SoA) patterns and cache-friendly access.

Optimization Strategies
-----------------------

**Memory Access Patterns**

Optimize for cache efficiency:

.. code-block:: cpp

   // Poor: Array of Structures (AoS)
   struct Particle { float x, y, z, mass; };
   std::vector<Particle> particles;

   // Better: Structure of Arrays (SoA)
   struct Particle { float x, y, z, mass; };
   ouly::soavector<Particle> particles;

   // Access individual components
   auto& x_coords = particles.get<0>();  // x coordinates
   auto& y_coords = particles.get<1>();  // y coordinates
   // Process arrays with vectorized operations

**Memory Allocation Strategies**

Choose allocators based on usage patterns:

.. code-block:: cpp

   // Frame-based allocations (games)
   ouly::linear_allocator<> frame_allocator(1024 * 1024);

   // Fixed-size objects (object pools)
   ouly::pool_allocator<> entity_pool(sizeof(Entity), 10000);

   // Growing collections (dynamic content)
   ouly::linear_arena_allocator<> dynamic_allocator(1024 * 1024);

**Parallel Processing Optimization**

Optimize task granularity and work distribution:

.. code-block:: cpp

   // Optimal grain size for parallel algorithms
   constexpr size_t OPTIMAL_GRAIN_SIZE = 1000;  // Tune for your hardware

   void parallel_process(ouly::task_context& ctx, std::vector<float>& data) {
       if (data.size() <= OPTIMAL_GRAIN_SIZE) {
           // Process directly
           process_range(data.begin(), data.end());
           return;
       }
       
       // Split work
       auto mid = data.begin() + data.size() / 2;
       
       // Process halves in parallel
       auto left_future = ouly::async(ctx, ctx.current_workgroup(), 
           [&](auto& ctx) { parallel_process(ctx, {data.begin(), mid}); });
       
       parallel_process(ctx, {mid, data.end()});
       left_future.wait();
   }

**Compilation Optimization**

Enable compiler optimizations for maximum performance:

.. code-block:: cmake

   # CMake optimization flags
   set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
   
   # Enable link-time optimization
   set_property(TARGET your_target PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

Profiling and Analysis
----------------------

**Memory Profiling with Valgrind**

.. code-block:: bash

   # Memory usage analysis
   valgrind --tool=massif ./your_program
   ms_print massif.out.* > memory_profile.txt

   # Cache miss analysis  
   valgrind --tool=cachegrind ./your_program
   cg_annotate cachegrind.out.* > cache_analysis.txt

**CPU Profiling with perf**

.. code-block:: bash

   # Profile CPU usage
   perf record -g ./your_program
   perf report > cpu_profile.txt

   # Analyze memory access patterns
   perf mem record ./your_program
   perf mem report > memory_access.txt

**OULY Built-in Profiling**

Enable statistics collection for performance analysis:

.. code-block:: cpp

   // Enable allocator statistics
   using DebugConfig = ouly::config<
       ouly::cfg::compute_stats,     // Basic statistics
       ouly::cfg::track_memory       // Memory tracking
   >;
   ouly::linear_allocator<DebugConfig> allocator(1024 * 1024);

   // ... use allocator ...

   // Access statistics if available (implementation-dependent)
   // Check allocator documentation for statistics access methods

Platform-Specific Optimizations
--------------------------------

**x86_64 Optimizations**

.. code-block:: cpp

   // Enable AVX/AVX2 for vectorized operations
   #ifdef __AVX2__
   // Use OULY's SIMD-optimized containers
   struct Position { float x, y, z; };
   ouly::soavector<Position> positions;
   #endif

   // Optimize for specific CPU architectures
   #ifdef __INTEL_COMPILER
   #pragma intel optimization_level 3
   #endif

**ARM Optimizations**

.. code-block:: cpp

   // NEON SIMD optimizations
   #ifdef __ARM_NEON
   // ARM-specific optimizations
   #endif

   // Apple Silicon optimizations
   #ifdef __aarch64__
   // 64-bit ARM optimizations
   #endif

Scaling Considerations
----------------------

**Thread Scaling**

.. code-block:: cpp

   // Optimal thread count
   unsigned int optimal_threads = std::min(
       std::thread::hardware_concurrency(),
       static_cast<unsigned int>(workload_size / MIN_WORK_PER_THREAD)
   );

   ouly::scheduler scheduler(optimal_threads);
   
   // Create workgroups BEFORE begin_execution
   auto workgroup = scheduler.create_workgroup();
   
   // Begin execution
   scheduler.begin_execution();
   
   // ... use scheduler ...
   
   scheduler.end_execution();
   scheduler.shutdown();

**Memory Scaling**

.. code-block:: cpp

   // Scale allocator sizes based on expected load
   size_t memory_budget = get_available_memory() * 0.8;  // 80% of available
   
   ouly::linear_arena_allocator<> allocator(memory_budget);

**Manual NUMA Optimization**

.. code-block:: cpp

   // OULY scheduler does not have built-in NUMA support
   // For NUMA optimization, manually configure:
   
   #include <numa.h>  // Linux NUMA API
   
   void setup_numa_optimization() {
       // 1. Set thread affinity manually
       // 2. Allocate memory on appropriate NUMA nodes  
       // 3. Partition work based on NUMA topology
       
       if (numa_available() >= 0) {
           int num_nodes = numa_num_configured_nodes();
           // Configure based on NUMA topology
       }
   }

Performance Testing
-------------------

**Performance Testing with External Tools**

For comprehensive performance testing, integrate with external benchmarking libraries:

.. code-block:: cpp

   #include <benchmark/benchmark.h>
   #include <ouly/ouly.hpp>

   static void BM_LinearAllocator(benchmark::State& state) {
       ouly::linear_allocator<> allocator(1024 * 1024);
       
       for (auto _ : state) {
           void* ptr = allocator.allocate(64);
           benchmark::DoNotOptimize(ptr);
           allocator.deallocate(ptr, 64);
       }
   }
   BENCHMARK(BM_LinearAllocator);

**Performance Monitoring in CI**

.. code-block:: cmake

   # Add performance tests to CMake (using external benchmark library)
   find_package(benchmark REQUIRED)
   add_executable(performance_tests performance_tests.cpp)
   target_link_libraries(performance_tests ouly::ouly benchmark::benchmark)
   
   # Run performance tests in CI
   add_test(NAME performance_regression_test COMMAND performance_tests)

Common Performance Pitfalls
----------------------------

**Memory Allocation Anti-patterns**

.. code-block:: cpp

   // AVOID: Frequent small allocations
   for (int i = 0; i < 1000000; ++i) {
       auto* ptr = new int(i);  // Very expensive
       delete ptr;
   }

   // BETTER: Use pool allocator
   ouly::pool_allocator<> pool(sizeof(int), 1000000);
   for (int i = 0; i < 1000000; ++i) {
       auto* ptr = static_cast<int*>(pool.allocate(sizeof(int)));
       new(ptr) int(i);
       // Batch cleanup later
   }

**Container Growth Anti-patterns**

.. code-block:: cpp

   // AVOID: Repeated growth
   ouly::dynamic_array<int> numbers;
   for (int i = 0; i < 1000000; ++i) {
       numbers.push_back(i);  // Multiple reallocations
   }

   // BETTER: Reserve capacity
   ouly::dynamic_array<int> numbers;
   numbers.reserve(1000000);  // Single allocation
   for (int i = 0; i < 1000000; ++i) {
       numbers.push_back(i);
   }

**Threading Anti-patterns**

.. code-block:: cpp

   // AVOID: Too many small tasks
   for (int i = 0; i < 1000000; ++i) {
       scheduler.submit(workgroup, [i]() { process_single_item(i); });
   }

   // BETTER: Batch processing
   constexpr size_t BATCH_SIZE = 1000;
   for (size_t i = 0; i < 1000000; i += BATCH_SIZE) {
       scheduler.submit(workgroup, [i]() {
           for (size_t j = i; j < std::min(i + BATCH_SIZE, 1000000UL); ++j) {
               process_single_item(j);
           }
       });
   }

Continuous Performance Monitoring
----------------------------------

Set up automated performance monitoring using external tools:

.. code-block:: yaml

   # GitHub Actions performance monitoring (using external benchmark tools)
   name: Performance Monitoring
   on: [push, pull_request]
   
   jobs:
     benchmark:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v2
         - name: Install benchmark library
           run: |
             git clone https://github.com/google/benchmark.git
             cd benchmark && cmake -B build && cmake --build build --target install
         - name: Build performance tests
           run: |
             cmake -B build -DOULY_BUILD_TESTS=ON
             cmake --build build
         - name: Run custom benchmarks
           run: ./build/performance_tests --benchmark_format=json > results.json

This comprehensive performance guide helps you get the most out of OULY in your applications. Regular profiling and optimization should be part of your development workflow for performance-critical applications.
