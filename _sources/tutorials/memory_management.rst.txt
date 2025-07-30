Memory Management Tutorial
==========================

OULY provides a comprehensive set of memory allocators designed for different use cases and performance requirements. This tutorial will guide you through the available allocators and when to use each one.

Overview of Allocators
-----------------------

OULY includes several allocator types:

* **Linear Allocators** - Fast sequential allocation, LIFO deallocation only
* **Arena Allocators** - Block-based allocation with configurable strategies  
* **Pool Allocators** - Fixed-size block allocation for specific object types
* **Thread-safe Variants** - Lock-free allocators for multi-threaded use

Linear Allocator
----------------

The linear allocator is the fastest allocator for sequential allocation patterns:

.. code-block:: cpp

   #include <ouly/allocators/linear_allocator.hpp>
   #include <iostream>

   int main() {
       // Create a 1KB linear allocator
       ouly::linear_allocator<> allocator(1024);
       
       // Allocate some memory
       void* ptr1 = allocator.allocate(256);
       void* ptr2 = allocator.allocate(128);
       
       std::cout << "Available space: " << allocator.available() << " bytes\n";
       std::cout << "Used space: " << allocator.used() << " bytes\n";
       
       // Linear allocators support LIFO deallocation only
       allocator.deallocate(ptr2, 128);  // Must deallocate in reverse order
       allocator.deallocate(ptr1, 256);
       
       return 0;
   }

**Use Cases:**
* Temporary allocations with predictable lifetimes
* Frame-based allocations in games
* Parser/compiler temporary storage

Linear Arena Allocator
-----------------------

The arena allocator provides automatic memory management with block allocation:

.. code-block:: cpp

   #include <ouly/allocators/linear_arena_allocator.hpp>
   #include <vector>

   int main() {
       // Create arena with 4KB initial size
       ouly::linear_arena_allocator<> arena(4096);
       
       // Use with STL containers
       using ArenaVector = std::vector<int, ouly::std_allocator_wrapper<int, decltype(arena)>>;
       ArenaVector numbers(ouly::std_allocator_wrapper<int, decltype(arena)>(arena));
       
       // Add many elements - arena will automatically grow
       for (int i = 0; i < 1000; ++i) {
           numbers.push_back(i);
       }
       
       std::cout << "Vector size: " << numbers.size() << "\n";
       std::cout << "Arena blocks: " << arena.block_count() << "\n";
       
       // Memory is automatically freed when arena is destroyed
       return 0;
   }

Pool Allocator
--------------

Pool allocators are optimal for fixed-size allocations:

.. code-block:: cpp

   #include <ouly/allocators/pool_allocator.hpp>
   #include <iostream>

   struct GameObject {
       float x, y, z;
       int health;
       // 16 bytes total
   };

   int main() {
       // Create pool for 100 GameObjects (16 bytes each)
       ouly::pool_allocator<> pool(sizeof(GameObject), 100);
       
       std::vector<GameObject*> objects;
       
       // Allocate game objects
       for (int i = 0; i < 50; ++i) {
           auto* obj = static_cast<GameObject*>(pool.allocate(sizeof(GameObject)));
           new(obj) GameObject{float(i), float(i * 2), 0.0f, 100};
           objects.push_back(obj);
       }
       
       std::cout << "Allocated " << objects.size() << " objects\n";
       std::cout << "Pool utilization: " << pool.used_blocks() << "/" << pool.total_blocks() << "\n";
       
       // Clean up
       for (auto* obj : objects) {
           obj->~GameObject();
           pool.deallocate(obj, sizeof(GameObject));
       }
       
       return 0;
   }

**Use Cases:**
* Object pools for games (entities, particles, etc.)
* Network packet buffers
* Database record caching

Configuration System
--------------------

OULY allocators use a template-based configuration system:

.. code-block:: cpp

   #include <ouly/allocators/config.hpp>
   #include <ouly/allocators/linear_allocator.hpp>

   int main() {
       // Configure allocator with custom alignment and memory tracking
       using Config = ouly::config<
           ouly::cfg::min_alignment<32>,    // 32-byte minimum alignment
           ouly::cfg::track_memory,         // Enable memory tracking
           ouly::cfg::compute_stats         // Compute allocation statistics
       >;
       
       ouly::linear_allocator<Config> allocator(1024);
       
       // All allocations will respect 32-byte minimum alignment
       void* aligned_ptr = allocator.allocate(100);
       
       return 0;
   }

Available allocator configuration options:

* ``ouly::cfg::min_alignment<N>`` - Set minimum memory alignment requirements
* ``ouly::cfg::track_memory`` - Enable memory usage tracking
* ``ouly::cfg::compute_stats`` - Collect basic allocation statistics  
* ``ouly::cfg::compute_atomic_stats`` - Collect thread-safe allocation statistics
* ``ouly::cfg::underlying_allocator<T>`` - Specify custom underlying allocator
* ``ouly::cfg::allocator_type<T>`` - Set allocator type for arena-based allocators
* ``ouly::cfg::atom_size<N>`` - Set allocation atom size (power of 2)
* ``ouly::cfg::atom_size_npt<N>`` - Set allocation atom size (not power of 2)
* ``ouly::cfg::atom_count<N>`` - Set number of atoms per allocation unit
* ``ouly::cfg::granularity<N>`` - Set allocation granularity
* ``ouly::cfg::max_bucket<N>`` - Set maximum bucket size for bucket allocators
* ``ouly::cfg::search_window<N>`` - Set search window size for allocation strategies
* ``ouly::cfg::strategy<T>`` - Specify allocation strategy for complex allocators
* ``ouly::cfg::fallback_start<T>`` - Set fallback strategy when primary fails
* ``ouly::cfg::fixed_max_per_slot<N>`` - Maximum allocations per slot
* ``ouly::cfg::extension<T>`` - Add extension functionality
* ``ouly::cfg::manager<T>`` - Set memory manager implementation
* ``ouly::cfg::debug_tracer<T>`` - Enable debug tracing with custom tracer
* ``ouly::cfg::base_stats<T>`` - Set base statistics collection type

**Advanced Configuration Examples:**

.. code-block:: cpp

   // High-performance arena with custom strategy
   using PerformanceConfig = ouly::config<
       ouly::cfg::strategy<ouly::strat::best_fit_v2>,
       ouly::cfg::min_alignment<64>,
       ouly::cfg::granularity<4096>
   >;
   
   ouly::arena_allocator<PerformanceConfig> arena(16 * 1024 * 1024);

   // Debug-enabled allocator with tracing
   using DebugConfig = ouly::config<
       ouly::cfg::track_memory,
       ouly::cfg::compute_atomic_stats,
       ouly::cfg::debug_tracer<MyCustomTracer>
   >;
   
   ouly::coalescing_arena_allocator<DebugConfig> debug_arena(8 * 1024 * 1024);

   // Pool allocator with custom atom configuration
   using PoolConfig = ouly::config<
       ouly::cfg::atom_size<256>,      // 256-byte atoms
       ouly::cfg::atom_count<64>,      // 64 atoms per allocation
       ouly::cfg::max_bucket<16>       // Maximum 16 buckets
   >;
   
   ouly::pool_allocator<PoolConfig> pool;

Thread-Safe Allocators
-----------------------

For multi-threaded applications, use thread-safe allocator variants:

.. code-block:: cpp

   #include <ouly/allocators/ts_shared_linear_allocator.hpp>
   #include <thread>
   #include <vector>

   int main() {
       // Thread-safe shared linear allocator
       ouly::ts_shared_linear_allocator<> shared_allocator(1024 * 1024);
       
       std::vector<std::thread> threads;
       
       // Launch multiple threads that allocate memory
       for (int i = 0; i < 4; ++i) {
           threads.emplace_back([&shared_allocator, i]() {
               for (int j = 0; j < 100; ++j) {
                   void* ptr = shared_allocator.allocate(256);
                   // Use memory...
                   std::this_thread::sleep_for(std::chrono::microseconds(10));
               }
           });
       }
       
       // Wait for all threads
       for (auto& t : threads) {
           t.join();
       }
       
       return 0;
   }

Address Space Allocators
------------------------

OULY provides specialized allocators that work with address spaces rather than direct memory allocation. These are particularly useful for GPU memory management and virtual memory systems:

.. code-block:: cpp

   #include <ouly/allocators/arena_allocator.hpp>
   #include <ouly/allocators/coalescing_arena_allocator.hpp>

   int main() {
       // Arena allocator for managing address ranges
       // Can be used with GPU memory pools or virtual address spaces
       ouly::arena_allocator<> arena;
       
       // Coalescing arena allocator reduces fragmentation
       // by merging adjacent free blocks
       ouly::coalescing_arena_allocator<> coalescing_arena;
       
       // These allocators manage address ranges, not actual memory
       // Useful for:
       // - GPU memory allocation coordination
       // - Virtual memory management
       // - Address space partitioning
       
       // Example: GPU memory allocation wrapper
       struct GPUMemoryAllocator {
           ouly::arena_allocator<> address_allocator;
           void* gpu_memory_pool;
           
           void* allocate_gpu_memory(size_t size) {
               // Get address offset from arena allocator
               auto offset = address_allocator.allocate(size);
               if (offset == nullptr) return nullptr;
               
               // Convert offset to actual GPU memory address
               return static_cast<char*>(gpu_memory_pool) + 
                      reinterpret_cast<uintptr_t>(offset);
           }
       };
       
       return 0;
   }

**Use Cases for Address Space Allocators:**

* **GPU Memory Management** - Coordinate address ranges in GPU memory pools
* **Virtual Memory Systems** - Manage virtual address space allocation
* **Memory Mapped Files** - Allocate ranges within large mapped regions
* **Custom Memory Backends** - Abstract address management from physical allocation

Best Practices
--------------

1. **Choose the Right Allocator**
   
   * Linear: Temporary/frame-based allocations
   * Arena: Growing collections with unknown final size
   * Pool: Fixed-size objects with frequent allocation/deallocation

2. **Alignment Considerations**
   
   * Use appropriate alignment for your data types
   * SIMD operations often require 16 or 32-byte alignment
   * Cache line alignment (64 bytes) for performance-critical data

3. **Memory Layout**
   
   * Group related allocations together for cache efficiency
   * Consider Structure of Arrays (SoA) layouts for better vectorization

4. **Debugging**
   
   * Enable debug mode during development
   * Use statistics to monitor allocation patterns
   * Profile memory usage in release builds

5. **Thread Safety**
   
   * Use thread-safe allocators sparingly (performance overhead)
   * Consider thread-local allocators for better performance
   * Synchronize access to shared allocators when necessary

Next Steps
----------

* Learn about :doc:`containers_tutorial` that work with custom allocators
* Explore :doc:`ecs_tutorial` for data-oriented memory layouts
* Check :doc:`../performance/index` for memory optimization techniques
