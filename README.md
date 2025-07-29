# Optimized Utility Library (OULY)

<div align="center">

[![CI](https://github.com/obhi-d/ouly/actions/workflows/ci.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/ci.yml)
[![Performance](https://github.com/obhi-d/ouly/actions/workflows/performance.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/performance.yml)
[![Coverity Scan](https://scan.coverity.com/projects/30824/badge.svg)](https://scan.coverity.com/projects/obhi-d-ouly)
[![codecov](https://codecov.io/gh/obhi-d/ouly/graph/badge.svg?token=POS8O18G9B)](https://codecov.io/gh/obhi-d/ouly)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

</div>

Ouly is a modern C++20 library providing high-performance containers, memory allocators, and utilities designed for performance-critical applications.

## 🚀 Key Features

### 🧠 Memory Management

Efficient memory allocators for different use cases:

```cpp
// Create a linear arena allocator with 1MB size
ouly::linear_arena_allocator<> allocator(1024 * 1024);

// Allocate memory
void* data = allocator.allocate(1024);

// Memory is automatically released when allocator is destroyed
```

```cpp
// Pool allocator - efficient for fixed-size allocations
ouly::pool_allocator<> pool(64, 100); // 100 blocks of 64 bytes

// Zero-initialized allocation with alignment
void* aligned_data = pool.allocate(64, ouly::alignment<16>{});
```

### 📦 Containers

Modern containers with STL-like interfaces but improved performance:

```cpp
// Small vector with stack-based storage for small sizes
ouly::small_vector<int, 16> vec = {1, 2, 3, 4};
vec.push_back(5);  // No heap allocation until more than 16 elements

// Dynamic array with custom allocator
ouly::dynamic_array<float> array;
array.resize(100);

```

### 🎮 Entity Component System (ECS)

High-performance ECS framework for game development and simulation applications:

```cpp
// Create an entity registry
ouly::ecs::registry<> registry;

// Create entities
auto entity1 = registry.emplace();
auto entity2 = registry.emplace();

// Define component types
struct Position {
    float x, y, z;
};

struct Velocity {
    float x, y, z;
};

// Create component storage
ouly::ecs::components<Position> positions;
ouly::ecs::components<Velocity> velocities;

// Add components to entities
positions.emplace_at(entity1, 0.0f, 0.0f, 0.0f);
velocities.emplace_at(entity1, 1.0f, 2.0f, 0.0f);

// Access components
auto& pos = positions[entity1];
pos.x += velocities[entity1].x;

// Iterate over components
positions.for_each([&](ouly::ecs::entity<> e, Position& pos) {
    if (velocities.contains(e)) {
        auto& vel = velocities[e];
        pos.x += vel.x;
        pos.y += vel.y;
        pos.z += vel.z;
    }
});

// Using revised entities for safety
ouly::ecs::rxregistry<> safe_registry;  // With revision tracking
auto safe_entity = safe_registry.emplace();
// If entity is recycled, old references become invalid
bool isValid = safe_registry.is_valid(safe_entity);
```

```cpp
// Custom component storage configuration
// Using sparse vector for component storage
using SparseConfig = ouly::cfg::use_sparse<>;
ouly::ecs::components<Position, ouly::ecs::entity<>, SparseConfig> sparse_positions;

// Direct mapping for frequently accessed components
using DirectConfig = ouly::cfg::use_direct_mapping<>;
ouly::ecs::components<Transform, ouly::ecs::entity<>, DirectConfig> transforms;

// Collection for efficient entity iteration
ouly::ecs::collection<ouly::ecs::entity<>> physics_entities;
physics_entities.emplace(entity1);

// Process entities with specific components
physics_entities.for_each(positions, [&](auto entity, auto& pos) {
    // Only processes entities in the physics_entities collection
    // Update physics for entity
});
```

### 🔄 Serialization

Flexible and efficient serialization framework for multiple formats:

#### Binary Serialization

Performance-oriented binary serialization with built-in endianness support:

```cpp
// Binary serialization with endianness control
struct ComplexData {
  std::string name;
  int32_t id;
  std::vector<float> values;
  std::unordered_map<std::string, double> attributes;
};

// Write to binary stream with little endian format
ouly::binary_stream output;
ComplexData data{"sensor", 42, {1.0f, 2.0f, 3.0f}, {{"temp", 23.5}, {"humidity", 65.2}}};
ouly::write<std::endian::little>(output, data);

// Read from binary stream
ouly::binary_stream input(output.data(), output.size());
ComplexData read_data;
ouly::read<std::endian::little>(input, read_data);

// Serializing to file
std::ofstream file("data.bin", std::ios::binary);
ouly::binary_ostream file_stream(file);
ouly::write<std::endian::little>(file_stream, data);
```

#### YAML Serialization

Lightweight YAML parsing and generation with an intuitive API:

```cpp
// Simple YAML serialization
struct Config {
  std::string app_name;
  int version;
  std::vector<std::string> plugins;
  struct {
    bool debug_mode;
    int log_level;
  } settings;
};

// Create a config object
Config config;
config.app_name = "MyApp";
config.version = 2;
config.plugins = {"auth", "storage", "network"};
config.settings.debug_mode = true;
config.settings.log_level = 3;

// Serialize to YAML string
std::string yaml_data = ouly::yml::to_string(config);
// Result:
// app_name: MyApp
// version: 2
// plugins:
//   - auth
//   - storage
//   - network
// settings:
//   debug_mode: true
//   log_level: 3

// Deserialize from YAML string
Config parsed_config;
ouly::yml::from_string(parsed_config, yaml_data);
```

### 🧩 Utilities

Powerful utility components:

```cpp
// Command-line argument parsing
ouly::program_args args;
args.parse_args(argc, argv);

auto number = args.decl<int>("number", "n").doc("A number argument");
auto verbose = args.decl<bool>("verbose", "v").doc("Enable verbose output");

if (verbose) {
  std::cout << "Number: " << *number.value() << std::endl;
}
```

```cpp
// Type-safe delegate (function wrapper with small object optimization)
ouly::delegate<int(int, int)> del = ouly::delegate<int(int, int)>::bind([](int a, int b) {
  return a + b;
});

int result = del(5, 3); // result = 8
```

### 🧩 Parallel Task Scheduler

High-performance multi-threaded task scheduler with workgroup organization and coroutine support:

```cpp
// Initialize the scheduler
ouly::scheduler scheduler;

// Create workgroups for different task categories
// Parameters: (workgroup_id, thread_start_index, thread_count, priority)
scheduler.create_group(ouly::workgroup_id(0), 0, 4);  // Default workgroup with 4 threads
scheduler.create_group(ouly::workgroup_id(1), 4, 2);  // IO group with 2 threads
scheduler.create_group(ouly::workgroup_id(2), 6, 2);  // Render group with 2 threads

// Start scheduler execution
scheduler.begin_execution();

// Submit a lambda task to the default workgroup
scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, 
    [](ouly::task_context const& ctx) {
        // Task code executed by a worker in the default workgroup
        std::cout << "Task running on worker " << ctx.get_worker().get_index() << std::endl;
    });

// Submit a task to a specific worker
ouly::worker_id target_worker(1);
scheduler.submit(ouly::main_worker_id, target_worker, ouly::default_workgroup_id,
    [](ouly::task_context const& ctx) {
        // Task code executed specifically by worker 1
    });

// Wait for tasks to complete and shutdown
scheduler.end_execution();
```

```cpp
// Coroutine task example
ouly::co_task<int> calculate_value()
{
    // Initial state is suspended
    int result = 0;
    
    // Simulate work
    for (int i = 0; i < 1000; i++)
        result += i;
        
    co_return result;  // Return result when complete
}

// Using a coroutine task
auto task = calculate_value();
scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, task);

// Wait for result synchronously
int value = task.sync_wait_result();
```

```cpp
// Parallel execution with parallel_for
std::vector<float> data(10000, 1.0f);

// Process data in parallel using range-based execution
ouly::parallel_for(
    [](auto begin, auto end, ouly::task_context const& ctx) {
        // Process items in range [begin, end)
        for (auto it = begin; it != end; ++it)
            *it = *it * 2.0f;
    },
    data,
    ouly::default_workgroup_id
);

// Process data in parallel using element-based execution
ouly::parallel_for(
    [](float& element, ouly::task_context const& ctx) {
        // Process individual element
        element = element * 2.0f;
    },
    data,
    ouly::default_workgroup_id
);

// Custom execution control with task traits
struct custom_traits : ouly::default_partitioner_traits {
    static constexpr uint32_t batches_per_worker = 8;           // More granular batching
    static constexpr uint32_t parallel_execution_threshold = 8; // Parallelize smaller workloads
};

ouly::parallel_for(
    [](float& element, ouly::task_context const& ctx) {
        element = std::sqrt(element);
    },
    data,
    ouly::default_workgroup_id,
    custom_traits{}
);
```

## 🔧 Installation

### CMake (recommended)

```bash
# Clone the repository
git clone https://github.com/obhi-d/ouly.git
cd ouly

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install
cmake --install build
```

### Integration into your CMake project

```cmake
# In your CMakeLists.txt
find_package(ouly REQUIRED)
target_link_libraries(your_target PRIVATE ouly::ouly)
```

## 📋 Requirements

- C++20 compatible compiler
- CMake 3.15 or higher
- gcc-14 or clang-19 or msvc v17
## 🧪 Testing

```bash
cmake -B build -DOULY_BUILD_TESTS=ON
cmake --build build
cd build/unit_tests
ctest
```

## 📚 Documentation

For detailed documentation, please visit:
[OULY Documentation](https://ouly-optimized-utility-library.readthedocs.io/en/latest/)

## 📖 Quick Example

```cpp
#include <ouly/allocators/linear_allocator.hpp>
#include <ouly/containers/dynamic_array.hpp>
#include <iostream>

int main() {
    // Create a 10KB arena allocator
    ouly::linear_allocator<> arena(10 * 1024);
    
    // Create a dynamic array using our allocator
    using MyAllocator = ouly::allocator_wrapper<int, decltype(arena)>;
    ouly::vector<int, MyAllocator> values(MyAllocator(&arena));
    
    // Add values
    for (int i = 0; i < 100; i++) {
        values.push_back(i);
    }
    
    // Sum values using algorithm
    int sum = std::accumulate(values.begin(), values.end(), 0);
    std::cout << "Sum: " << sum << std::endl;
    
    return 0;
}
```

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.