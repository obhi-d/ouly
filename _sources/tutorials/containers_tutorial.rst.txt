Containers Tutorial
==================

OULY provides high-performance containers with STL-compatible interfaces and support for custom allocators. This tutorial covers the available containers and their optimal usage patterns.

Container Overview
------------------

OULY includes several specialized containers:

* **small_vector** - Stack-optimized vector for small collections
* **dynamic_array** - High-performance resizable array
* **sparse_vector** - Memory-efficient sparse storage  
* **intrusive_list** - Zero-allocation linked list
* **soavector** - Structure of Arrays container

Each container is designed for specific use cases and performance characteristics.

Small Vector
------------

The small_vector stores small collections on the stack, avoiding heap allocations:

.. code-block:: cpp

   #include <ouly/containers/small_vector.hpp>
   #include <iostream>

   int main() {
       // Small vector with stack storage for up to 8 elements
       ouly::small_vector<int, 8> numbers;
       
       // These additions use stack storage (no heap allocation)
       for (int i = 0; i < 8; ++i) {
           numbers.push_back(i);
       }
       
       std::cout << "Capacity: " << numbers.capacity() << "\n";  // 8
       std::cout << "Is on stack: " << !numbers.is_heap_allocated() << "\n";  // true
       
       // This triggers heap allocation
       numbers.push_back(8);
       std::cout << "After growth - on heap: " << numbers.is_heap_allocated() << "\n";  // true
       
       // STL-compatible interface
       for (const auto& num : numbers) {
           std::cout << num << " ";
       }
       std::cout << "\n";
       
       return 0;
   }

**Use Cases:**
* Function parameters/return values with typically small sizes
* Collections with known small upper bounds
* Avoiding heap allocations for performance-critical paths

Dynamic Array
-------------

The dynamic_array provides optimized growth strategies and custom allocator support:

.. code-block:: cpp

   #include <ouly/containers/dynamic_array.hpp>
   #include <ouly/allocators/linear_arena_allocator.hpp>

   int main() {
       // Using custom allocator
       ouly::linear_arena_allocator<> arena(1024 * 1024);
       
       ouly::dynamic_array<float> positions;
       positions.set_allocator(&arena);
       
       // Efficient bulk operations
       positions.resize(1000);
       positions.reserve(2000);  // Pre-allocate capacity
       
       // Fill with data
       for (size_t i = 0; i < positions.size(); ++i) {
           positions[i] = static_cast<float>(i) * 0.1f;
       }
       
       std::cout << "Array size: " << positions.size() << "\n";
       std::cout << "Array capacity: " << positions.capacity() << "\n";
       
       // Efficient range operations
       auto subrange = positions.subrange(100, 200);
       for (auto value : subrange) {
           std::cout << value << " ";
       }
       
       return 0;
   }

Sparse Vector
-------------

The sparse_vector efficiently stores data for sparse indices:

.. code-block:: cpp

   #include <ouly/containers/sparse_vector.hpp>
   #include <iostream>

   struct Component {
       float x, y, z;
       Component(float x, float y, float z) : x(x), y(y), z(z) {}
   };

   int main() {
       ouly::sparse_vector<Component> components;
       
       // Insert at sparse indices
       components.emplace(10, 1.0f, 2.0f, 3.0f);
       components.emplace(100, 4.0f, 5.0f, 6.0f);
       components.emplace(1000, 7.0f, 8.0f, 9.0f);
       
       std::cout << "Size: " << components.size() << "\n";  // 3 elements
       std::cout << "Dense storage, sparse indices\n";
       
       // Check if indices exist
       if (components.contains(100)) {
           auto& comp = components[100];
           std::cout << "Component at 100: (" << comp.x << ", " << comp.y << ", " << comp.z << ")\n";
       }
       
       // Iterate over all elements (dense iteration)
       for (auto& [index, component] : components) {
           std::cout << "Index " << index << ": (" 
                     << component.x << ", " << component.y << ", " << component.z << ")\n";
       }
       
       return 0;
   }

**Use Cases:**
* Entity-component systems with sparse entity IDs
* Sparse matrices and mathematical computations
* Cache-friendly storage for sparse data sets

Intrusive List
--------------

The intrusive_list provides zero-allocation linked list functionality:

.. code-block:: cpp

   #include <ouly/containers/intrusive_list.hpp>
   #include <iostream>

   struct Node : public ouly::intrusive_list_node<Node> {
       int value;
       Node(int v) : value(v) {}
   };

   int main() {
       ouly::intrusive_list<Node> list;
       
       // Create nodes (can be allocated anywhere)
       Node node1(10);
       Node node2(20);
       Node node3(30);
       
       // Insert into list (no heap allocation)
       list.push_back(node1);
       list.push_back(node2);
       list.push_front(node3);
       
       std::cout << "List contents: ";
       for (const auto& node : list) {
           std::cout << node.value << " ";
       }
       std::cout << "\n";
       
       // Remove specific node
       list.remove(node2);
       
       std::cout << "After removal: ";
       for (const auto& node : list) {
           std::cout << node.value << " ";
       }
       std::cout << "\n";
       
       return 0;
   }

**Use Cases:**
* Memory pools where allocation is managed separately
* Real-time systems requiring predictable performance
* Embedded systems with strict memory constraints

Structure of Arrays (SoA)
--------------------------

The soavector stores aggregate types in Structure of Arrays format for better cache utilization and SIMD optimization:

.. code-block:: cpp

   #include <ouly/containers/soavector.hpp>
   #include <iostream>

   // Define aggregate structure
   struct Particle {
       float x, y, z;    // position
       float vx, vy, vz; // velocity
       int life;         // lifetime
       float mass;       // mass
   };

   int main() {
       // soavector automatically expands Particle into separate arrays
       ouly::soavector<Particle> particles;
       
       // Add particles - constructor arguments match struct members
       for (int i = 0; i < 100; ++i) {
           particles.emplace_back(
               static_cast<float>(i),      // x
               static_cast<float>(i * 2),  // y  
               0.0f,                       // z
               1.0f, 0.0f, 0.0f,          // velocity
               100,                        // life
               1.0f                        // mass
           );
       }
       
       // Access individual arrays for vectorization
       auto& x_positions = particles.get<0>();  // Array of x coordinates
       auto& y_positions = particles.get<1>();  // Array of y coordinates
       auto& z_positions = particles.get<2>();  // Array of z coordinates
       auto& x_velocities = particles.get<3>(); // Array of x velocities
       auto& lifetimes = particles.get<6>();    // Array of lifetimes
       
       // SIMD-friendly operations on individual arrays
       for (size_t i = 0; i < particles.size(); ++i) {
           x_positions[i] += x_velocities[i];
           y_positions[i] += particles.get<4>()[i]; // y velocity
           z_positions[i] += particles.get<5>()[i]; // z velocity
           lifetimes[i] -= 1;
       }
       
       // Individual element access (reconstructs the aggregate)
       auto particle = particles[0];
       std::cout << "Particle 0: x=" << particle.x 
                 << " y=" << particle.y 
                 << " life=" << particle.life << "\n";
       
       // Iterate over all particles (less efficient than array access)
       for (const auto& p : particles) {
           if (p.life <= 0) {
               // Handle dead particle
           }
       }
       
       return 0;
   }

**Use Cases:**
* Game engines (particles, entities, physics)
* Scientific computing with large datasets
* Any application requiring SIMD optimizations

Container Algorithms
---------------------

OULY containers work with STL algorithms and provide additional utilities:

.. code-block:: cpp

   #include <ouly/containers/dynamic_array.hpp>
   #include <algorithm>
   #include <numeric>

   int main() {
       ouly::dynamic_array<int> numbers = {5, 2, 8, 1, 9, 3};
       
       // STL algorithms work seamlessly
       std::sort(numbers.begin(), numbers.end());
       
       auto sum = std::accumulate(numbers.begin(), numbers.end(), 0);
       std::cout << "Sum: " << sum << "\n";
       
       // Find element
       auto it = std::find(numbers.begin(), numbers.end(), 8);
       if (it != numbers.end()) {
           std::cout << "Found 8 at position: " << std::distance(numbers.begin(), it) << "\n";
       }
       
       // Custom algorithms
       numbers.for_each([](int& x) { x *= 2; });
       
       return 0;
   }

Custom Allocators
-----------------

All OULY containers support custom allocators:

.. code-block:: cpp

   #include <ouly/containers/dynamic_array.hpp>
   #include <ouly/allocators/pool_allocator.hpp>

   int main() {
       // Pool allocator for fixed-size allocations
       ouly::pool_allocator<> pool(1024, 100);  // 100 blocks of 1KB each
       
       // Container using custom allocator
       ouly::dynamic_array<char> buffer;
       buffer.set_allocator(&pool);
       
       // All allocations will use the pool
       buffer.resize(1024);
       buffer.resize(2048);  // Will allocate second block
       
       std::cout << "Pool utilization: " << pool.used_blocks() << "/" << pool.total_blocks() << "\n";
       
       return 0;
   }

You can also use OULY containers with STL allocator adaptors:

.. code-block:: cpp

   #include <ouly/allocators/std_allocator_wrapper.hpp>
   #include <vector>

   int main() {
       ouly::linear_arena_allocator<> arena(1024);
       
       // Use OULY allocator with STL containers
       using AllocType = ouly::std_allocator_wrapper<int, decltype(arena)>;
       std::vector<int, AllocType> vec(AllocType(arena));
       
       vec.push_back(1);
       vec.push_back(2);
       vec.push_back(3);
       
       return 0;
   }

Container Configuration
-----------------------

OULY containers can be customized using configuration templates in the ``ouly::cfg`` namespace:

**Blackboard Configuration**

The blackboard container supports custom hash map implementations:

.. code-block:: cpp

   #include <ouly/containers/blackboard.hpp>
   #include <unordered_map>

   // Using custom hash map type
   using CustomConfig = ouly::config<
       ouly::cfg::map<std::unordered_map>
   >;
   
   ouly::blackboard<CustomConfig> board;

**Available Container Configuration Options:**

* ``ouly::cfg::map<T>`` - Specify custom hash map implementation for blackboard
* ``ouly::cfg::name_map<T>`` - Custom key-to-offset mapping for blackboard  
* ``ouly::cfg::name_val_map<T>`` - Complete custom blackboard hash map
* ``ouly::cfg::pool_size<N>`` - Set pool size for internal allocations
* ``ouly::cfg::index_pool_size<N>`` - Pool size for index management
* ``ouly::cfg::use_sparse`` - Enable sparse storage strategy
* ``ouly::cfg::use_direct_mapping`` - Enable direct mapping strategy
* ``ouly::cfg::custom_vector<T>`` - Use custom vector implementation

**Common Configuration Examples:**

.. code-block:: cpp

   // Sparse container with custom pool size
   using SparseConfig = ouly::config<
       ouly::cfg::use_sparse,
       ouly::cfg::pool_size<8192>
   >;

   // Direct mapping with large index pool  
   using FastConfig = ouly::config<
       ouly::cfg::use_direct_mapping,
       ouly::cfg::index_pool_size<16384>
   >;

   // Custom vector backend
   using VectorConfig = ouly::config<
       ouly::cfg::custom_vector<std::vector<int>>
   >;

Performance Considerations
--------------------------

**Container Selection**

1. **small_vector** - Best for collections typically < N elements
2. **dynamic_array** - General-purpose high-performance array
3. **sparse_vector** - When indices are sparse but iteration should be dense
4. **intrusive_list** - When you need stable references and zero allocation
5. **soavector** - When you need SIMD optimization and cache efficiency

**Performance Comparison: AoS vs SoA**

.. code-block:: cpp

   // Array of Structures (AoS) - poor cache utilization for partial access
   struct Particle { float x, y, z; int life; };
   std::vector<Particle> aos_particles;

   // Structure of Arrays (SoA) - better cache utilization and SIMD-friendly
   struct Particle { float x, y, z; int life; };
   ouly::soavector<Particle> soa_particles;  // Automatically splits into separate arrays

   // When you only need to update positions:
   // AoS loads entire Particle structs (16 bytes each)
   // SoA loads only position arrays (4 bytes each)

**Growth Strategies**

.. code-block:: cpp

   ouly::dynamic_array<int> numbers;
   
   // Pre-allocate if final size is known
   numbers.reserve(1000);
   
   // Use bulk operations when possible
   numbers.resize(1000);  // Better than 1000 push_back calls

Best Practices
--------------

1. **Choose the Right Container**
   
   * Analyze access patterns (random vs sequential)
   * Consider element lifetime and stability requirements
   * Profile different containers for your use case

2. **Memory Management**
   
   * Use custom allocators for better control
   * Pre-allocate capacity when size is predictable
   * Consider memory alignment for SIMD operations

3. **Performance Optimization**
   
   * Use SoA layout for vectorizable operations
   * Prefer intrusive containers in memory-constrained environments
   * Batch operations when possible

4. **API Design**
   
   * Containers are designed to work with range-based for loops
   * STL algorithms work seamlessly
   * Custom iterators provide additional safety and features

Next Steps
----------

* Explore :doc:`ecs_tutorial` for advanced container usage patterns
* Learn about :doc:`memory_management` for optimal allocator selection
* Check :doc:`../performance/index` for container optimization tips
