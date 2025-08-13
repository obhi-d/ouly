# OULY - Optimized Utility Library

<div align="center">

[![CI](https://github.com/obhi-d/ouly/actions/workflows/ci.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/ci.yml)
[![Performance](https://github.com/obhi-d/ouly/actions/workflows/performance.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/performance.yml)
[![Coverity Scan](https://scan.coverity.com/projects/30824/badge.svg)](https://scan.coverity.com/projects/obhi-d-ouly)
[![codecov](https://codecov.io/gh/obhi-d/ouly/graph/badge.svg?token=POS8O18G9B)](https://codecov.io/gh/obhi-d/ouly)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**A modern C++20 high-performance utility library for performance-critical applications**

*Featuring memory allocators, containers, ECS, task schedulers, flow graphs, and serialization*

</div>

OULY is a comprehensive C++20 library designed for high-performance applications where every nanosecond counts. It provides zero-cost abstractions, cache-friendly data structures, and lock-free algorithms optimized for modern multi-core systems.

## 🎯 Core Principles

- **Zero-Cost Abstractions**: Template-heavy design that compiles away overhead
- **Cache-Friendly**: Structure of Arrays (SoA) patterns and memory-optimized layouts
- **Lock-Free**: Atomic operations and work-stealing algorithms for scalability
- **NUMA-Aware**: Thread affinity and memory locality optimizations
- **Performance-First**: Every component benchmarked against industry standards

## � Quick Start

### Requirements

- **Compiler**: GCC 14+, Clang 19+, or MSVC v17+
- **Standard**: C++20 with concepts, coroutines, and ranges support
- **Build System**: CMake 3.20+
- **Platform**: Linux, macOS, Windows

### Installation

```bash
# Clone the repository
git clone https://github.com/obhi-d/ouly.git
cd ouly

# Build with release optimizations
cmake --preset=linux-release  # or macos-release, windows-release
cmake --build build/linux-release

# Install system-wide (optional)
cmake --install build/linux-release
```

### Integration into Your Project

```cmake
# Method 1: Find installed package
find_package(ouly REQUIRED)
target_link_libraries(your_target PRIVATE ouly::ouly)

# Method 2: Add as subdirectory
add_subdirectory(external/ouly)
target_link_libraries(your_target PRIVATE ouly::ouly)
```

### Minimal Example

```cpp
#include <ouly/allocators/linear_allocator.hpp>
#include <ouly/containers/dynamic_array.hpp>
#include <ouly/scheduler/scheduler_v2.hpp>

int main() {
    // High-performance linear allocator
    ouly::linear_allocator<> allocator(1024 * 1024);
    
    // Container with custom allocator
    ouly::dynamic_array<int> data;
    data.reserve(1000);
    
    // Multi-threaded task scheduler
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::default_workgroup_id, 0, 4);
    scheduler.begin_execution();
    
    // Submit parallel work
    scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id,
        [&](ouly::task_context const& ctx) {
            // Your high-performance code here
            std::cout << "Running on worker " << ctx.get_worker().get_index() << std::endl;
        });
    
    scheduler.end_execution();
    return 0;
}
```

## 🚀 Core Components

### 🧠 Memory Allocators

OULY provides specialized allocators optimized for different allocation patterns:

#### Linear Allocators
Perfect for temporary allocations with LIFO deallocation:

```cpp
// Basic linear allocator with 1MB capacity
ouly::linear_allocator<> allocator(1024 * 1024);

// Allocate with automatic alignment
void* data = allocator.allocate(256, ouly::alignment<16>{});

// Custom configuration with debug tracking
using DebugConfig = ouly::config<ouly::alignment<32>, ouly::debug_mode>;
ouly::linear_allocator<DebugConfig> debug_alloc(512 * 1024);

// Thread-safe variant for concurrent access
ouly::ts_linear_allocator<> mt_allocator(2 * 1024 * 1024);
```

#### Arena Allocators
Block-based allocation with configurable growth strategies:

```cpp
// Arena with 4KB blocks, doubling growth strategy
ouly::arena_allocator<> arena;

// Pre-reserve blocks for predictable performance
arena.reserve_blocks(10);

// Allocate various sizes efficiently
auto ptr1 = arena.allocate(64);
auto ptr2 = arena.allocate(1024);
auto ptr3 = arena.allocate(128, ouly::alignment<64>{});

// Reset arena for reuse
arena.reset();
```

#### Pool Allocators  
Fixed-size block allocation for consistent performance:

```cpp
// Pool for 64-byte objects with 100 initial blocks
ouly::pool_allocator<> pool(64, 100);

// Zero-initialized allocation
void* obj = pool.allocate(64, ouly::alignment<16>{});

// Thread-safe pool allocator
ouly::ts_pool_allocator<> mt_pool(32, 200);

// Custom configuration with statistics
using StatsConfig = ouly::config<ouly::enable_stats, ouly::alignment<8>>;
ouly::pool_allocator<StatsConfig> stats_pool(128, 50);
```

#### Allocator Wrappers
STL-compatible allocator adapters:

```cpp
// Wrap any OULY allocator for STL containers
ouly::linear_allocator<> backing_alloc(1024 * 1024);
using Allocator = ouly::allocator_wrapper<int, decltype(backing_alloc)>;

std::vector<int, Allocator> vec(Allocator(&backing_alloc));
std::unordered_map<std::string, int, std::hash<std::string>, std::equal_to<>, 
                   ouly::allocator_wrapper<std::pair<const std::string, int>, 
                   decltype(backing_alloc)>> map(Allocator(&backing_alloc));
```

### 📦 High-Performance Containers

Cache-friendly containers with STL-like interfaces:

#### Dynamic Array
Optimized vector replacement with configurable growth:

```cpp
// Basic dynamic array
ouly::dynamic_array<float> array;
array.resize(1000);
array.push_back(3.14f);

// With custom allocator
ouly::linear_allocator<> backing(1024 * 1024);
ouly::dynamic_array<int, ouly::allocator_wrapper<int, decltype(backing)>> 
    custom_array(ouly::allocator_wrapper<int, decltype(backing)>(&backing));

// Reserve capacity for performance
custom_array.reserve(10000);
```

#### Small Vector
Stack-allocated storage for small collections:

```cpp
// Uses stack storage for up to 16 elements
ouly::small_vector<int, 16> small_vec = {1, 2, 3, 4, 5};

// No heap allocation until exceeding capacity
for (int i = 6; i <= 16; ++i) {
    small_vec.push_back(i);  // Still on stack
}

small_vec.push_back(17);  // Now allocates on heap

// Perfect for function-local collections
auto process_batch = [](auto const& items) {
    ouly::small_vector<float, 8> results;
    for (auto const& item : items) {
        results.push_back(process(item));
    }
    return results;
};
```

#### Sparse Vector
Efficient storage for sparse data:

```cpp
// Sparse vector with 32-bit indices
ouly::sparse_vector<std::string> sparse_data;

// Insert at arbitrary indices
sparse_data.insert(0, "first");
sparse_data.insert(1000, "sparse");
sparse_data.insert(50000, "very_sparse");

// Check existence and access
if (sparse_data.contains(1000)) {
    std::cout << sparse_data[1000] << std::endl;
}

// Iterate only over existing elements
sparse_data.for_each([](uint32_t index, std::string const& value) {
    std::cout << "Index " << index << ": " << value << std::endl;
});
```

#### Structure of Arrays (SoA) Vector
Cache-friendly layout for multiple data streams:

```cpp
// Define SoA types
using Position = ouly::soa_vector<float, float, float>;  // x, y, z
using Velocity = ouly::soa_vector<float, float, float>;  // vx, vy, vz

Position positions;
Velocity velocities;

// Add elements
positions.emplace_back(1.0f, 2.0f, 3.0f);
velocities.emplace_back(0.1f, 0.2f, 0.0f);

// Access individual components efficiently
auto& x_values = positions.get<0>();  // All x coordinates
auto& y_values = positions.get<1>();  // All y coordinates

// Vectorized operations possible
for (size_t i = 0; i < positions.size(); ++i) {
    x_values[i] += velocities.get<0>()[i];
    y_values[i] += velocities.get<1>()[i];
}
```

#### Intrusive Containers
Zero-allocation linked structures:

```cpp
// Intrusive list node
struct Task : ouly::intrusive_list_node<Task> {
    std::string name;
    int priority;
    
    Task(std::string n, int p) : name(std::move(n)), priority(p) {}
};

// Intrusive list container
ouly::intrusive_list<Task> task_queue;

// Add tasks (no allocation - they manage themselves)
Task task1("render", 10);
Task task2("audio", 5);

task_queue.push_back(task1);
task_queue.push_front(task2);

// Iterate with range-based for
for (auto& task : task_queue) {
    std::cout << task.name << " (priority: " << task.priority << ")" << std::endl;
}
```

### ⚡ Multi-Threading and Task Scheduling

Advanced work-stealing scheduler with workgroup organization:

#### Basic Task Scheduling

```cpp
// Initialize scheduler with workgroups
ouly::scheduler scheduler;

// Create workgroups for different task types
scheduler.create_group(ouly::workgroup_id(0), 0, 4);  // Default: 4 threads
scheduler.create_group(ouly::workgroup_id(1), 4, 2);  // IO: 2 threads  
scheduler.create_group(ouly::workgroup_id(2), 6, 2);  // Render: 2 threads

scheduler.begin_execution();

// Submit simple task
scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id,
    [](ouly::task_context const& ctx) {
        std::cout << "Task on worker " << ctx.get_worker().get_index() << std::endl;
    });

// Submit task to specific workgroup
scheduler.submit(ouly::main_worker_id, ouly::workgroup_id(1),
    [](ouly::task_context const& ctx) {
        // IO-intensive work runs on IO workgroup
        load_file("data.bin");
    });

scheduler.end_execution();
```

#### Parallel Algorithms

```cpp
// Parallel for with range-based processing
std::vector<float> data(100000, 1.0f);

ouly::parallel_for(
    [](auto begin, auto end, ouly::task_context const& ctx) {
        for (auto it = begin; it != end; ++it) {
            *it = std::sin(*it);  // Process range
        }
    },
    data,
    ouly::default_workgroup_id
);

// Parallel for with element-based processing
ouly::parallel_for(
    [](float& element, ouly::task_context const& ctx) {
        element = std::sqrt(element);  // Process individual element
    },
    data,
    ouly::default_workgroup_id
);

// Custom partitioning strategy
struct FinegrainedTraits : ouly::default_partitioner_traits {
    static constexpr uint32_t batches_per_worker = 16;
    static constexpr uint32_t parallel_execution_threshold = 4;
};

ouly::parallel_for(
    [](float& x, ouly::task_context const& ctx) { x *= 2.0f; },
    data,
    ouly::default_workgroup_id,
    FinegrainedTraits{}
);
```

#### Coroutine Support

```cpp
// Coroutine task for async computation
ouly::co_task<int> fibonacci(int n) {
    if (n <= 1) co_return n;
    
    auto task1 = fibonacci(n - 1);
    auto task2 = fibonacci(n - 2);
    
    // Submit sub-tasks
    scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, task1);
    scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, task2);
    
    // Await results
    int result1 = task1.sync_wait_result();
    int result2 = task2.sync_wait_result();
    
    co_return result1 + result2;
}

// Use coroutine
auto fib_task = fibonacci(20);
scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, fib_task);
int result = fib_task.sync_wait_result();
```

#### Flow Graph for Complex Dependencies

```cpp
// Create flow graph for complex task dependencies
ouly::flow_graph<ouly::scheduler> flow(scheduler);

// Add dependent tasks
auto task_a = flow.add("load_data", [](ouly::task_context const& ctx) {
    return load_database_data();
});

auto task_b = flow.add("process_data", [](ouly::task_context const& ctx) {
    return transform_data();
});

auto task_c = flow.add("save_results", [](ouly::task_context const& ctx) {
    save_to_file();
});

// Define dependencies: B depends on A, C depends on B
task_b.depends_on(task_a);
task_c.depends_on(task_b);

// Execute flow graph
flow.prepare();
flow.start(ouly::default_workgroup_id);
flow.wait_for_completion();
```

### 🎮 Entity Component System (ECS)

High-performance ECS framework optimized for cache efficiency and batch processing:

#### Basic Entity Management

```cpp
// Create entity registries
ouly::ecs::registry<> registry;          // Basic registry
ouly::ecs::rxregistry<> safe_registry;   // With revision tracking

// Create entities
auto entity1 = registry.emplace();
auto entity2 = registry.emplace();
auto entity3 = safe_registry.emplace();

// Entity lifecycle
registry.destroy(entity1);
bool exists = registry.contains(entity2);

// Safe entity validation (prevents use-after-free)
bool is_valid = safe_registry.is_valid(entity3);
```

#### Component Storage Strategies

```cpp
// Define component types
struct Position { float x, y, z; };
struct Velocity { float dx, dy, dz; };
struct Health { int current, maximum; };

// Dense storage (default) - cache-friendly iteration
ouly::ecs::components<Position> positions;

// Sparse storage - better for sparse components
using SparseConfig = ouly::cfg::use_sparse<>;
ouly::ecs::components<Health, ouly::ecs::entity<>, SparseConfig> health_components;

// Direct mapping - fastest access for packed components
using DirectConfig = ouly::cfg::use_direct_mapping<>;
ouly::ecs::components<Velocity, ouly::ecs::entity<>, DirectConfig> velocities;

// Add components to entities
positions.emplace_at(entity1, 10.0f, 20.0f, 0.0f);
velocities.emplace_at(entity1, 1.0f, 0.5f, 0.0f);
health_components.emplace_at(entity1, 100, 100);
```

#### Efficient Component Access and Iteration

```cpp
// Direct component access
if (positions.contains(entity1)) {
    Position& pos = positions[entity1];
    pos.x += 1.0f;
}

// Batch processing with for_each
positions.for_each([&](ouly::ecs::entity<> e, Position& pos) {
    if (velocities.contains(e)) {
        Velocity& vel = velocities[e];
        pos.x += vel.dx;
        pos.y += vel.dy;
        pos.z += vel.dz;
    }
});

// Multi-component iteration (entities with both components)
auto movement_system = [](auto entity, Position& pos, Velocity const& vel) {
    pos.x += vel.dx;
    pos.y += vel.dy;
    pos.z += vel.dz;
};

// Process only entities that have both components
positions.for_each_with(velocities, movement_system);
```

#### Collections for Grouped Processing

```cpp
// Create collections for entity grouping
ouly::ecs::collection<ouly::ecs::entity<>> physics_entities;
ouly::ecs::collection<ouly::ecs::entity<>> render_entities;
ouly::ecs::collection<ouly::ecs::entity<>> ai_entities;

// Add entities to collections
physics_entities.emplace(entity1);
physics_entities.emplace(entity2);
render_entities.emplace(entity1);

// Process entities in specific collections
physics_entities.for_each(positions, velocities, 
    [](auto entity, Position& pos, Velocity const& vel) {
        // Physics update only for entities in physics collection
        pos.x += vel.dx * 0.016f;  // 60 FPS timestep
        pos.y += vel.dy * 0.016f;
        pos.z += vel.dz * 0.016f;
    });

// Collection-based iteration with single component
render_entities.for_each(positions, [](auto entity, Position const& pos) {
    render_object_at_position(entity, pos);
});
```

#### Advanced ECS Patterns

```cpp
// Entity archetypes with template specialization
template<typename... Components>
struct entity_archetype {
    ouly::ecs::entity<> entity;
    std::tuple<Components...> components;
};

using MovableEntity = entity_archetype<Position, Velocity>;
using RenderableEntity = entity_archetype<Position, Sprite>;

// Batch entity creation
auto create_projectile = [&](float x, float y, float dx, float dy) {
    auto projectile = registry.emplace();
    positions.emplace_at(projectile, x, y, 0.0f);
    velocities.emplace_at(projectile, dx, dy, 0.0f);
    physics_entities.emplace(projectile);
    return projectile;
};

// System scheduling with dependencies
class PhysicsSystem {
public:
    void update(float dt) {
        // Update positions based on velocities
        positions.for_each_with(velocities, 
            [dt](auto e, Position& pos, Velocity const& vel) {
                pos.x += vel.dx * dt;
                pos.y += vel.dy * dt;
                pos.z += vel.dz * dt;
            });
    }
};

class RenderSystem {
public:
    void render() {
        // Render all entities with positions
        render_entities.for_each(positions, 
            [](auto e, Position const& pos) {
                draw_sprite_at(e, pos.x, pos.y);
            });
    }
};
```

### 🔄 Serialization

Flexible and high-performance serialization framework supporting multiple formats:

#### Binary Serialization

Zero-copy binary serialization with endianness control:

```cpp
// Define serializable structure
struct GameState {
    std::string player_name;
    int32_t level;
    std::vector<float> position;
    std::unordered_map<std::string, double> stats;
};

// Serialize to binary format
ouly::binary_stream output;
GameState state{"Player1", 42, {10.0f, 20.0f, 5.0f}, {{"health", 85.5}, {"mana", 120.0}}};

// Write with specific endianness
ouly::write<std::endian::little>(output, state);

// Read back from binary
ouly::binary_stream input(output.data(), output.size());
GameState loaded_state;
ouly::read<std::endian::little>(input, loaded_state);

// File I/O with binary streams
std::ofstream file("gamestate.bin", std::ios::binary);
ouly::binary_ostream file_stream(file);
ouly::write<std::endian::little>(file_stream, state);
```

#### YAML Serialization

Human-readable YAML with simple API:

```cpp
// Configuration structure
struct AppConfig {
    std::string app_name;
    int version;
    std::vector<std::string> plugins;
    struct NetworkConfig {
        std::string host;
        uint16_t port;
        bool use_ssl;
    } network;
    struct LogConfig {
        bool debug_mode;
        int log_level;
        std::string log_file;
    } logging;
};

// Create configuration
AppConfig config;
config.app_name = "MyGameEngine";
config.version = 3;
config.plugins = {"renderer", "audio", "input", "networking"};
config.network = {"localhost", 8080, true};
config.logging = {true, 2, "game.log"};

// Serialize to YAML string
std::string yaml_content = ouly::yml::to_string(config);
/* Output:
app_name: MyGameEngine
version: 3
plugins:
  - renderer
  - audio
  - input
  - networking
network:
  host: localhost
  port: 8080
  use_ssl: true
logging:
  debug_mode: true
  log_level: 2
  log_file: game.log
*/

// Deserialize from YAML
AppConfig parsed_config;
ouly::yml::from_string(parsed_config, yaml_content);

// File-based YAML operations
ouly::yml::to_file(config, "config.yaml");
ouly::yml::from_file(parsed_config, "config.yaml");
```

#### Custom Serialization

```cpp
// Custom serializable types
struct Transform {
    ouly::vec3 position;
    ouly::quat rotation;
    ouly::vec3 scale;
    
    // Custom binary serialization
    template<std::endian E, typename Stream>
    void serialize(Stream& stream) const {
        ouly::write<E>(stream, position);
        ouly::write<E>(stream, rotation);
        ouly::write<E>(stream, scale);
    }
    
    template<std::endian E, typename Stream>
    void deserialize(Stream& stream) {
        ouly::read<E>(stream, position);
        ouly::read<E>(stream, rotation);
        ouly::read<E>(stream, scale);
    }
};

// Batch serialization for performance
std::vector<Transform> transforms;
// ... populate transforms ...

ouly::binary_stream batch_output;
for (auto const& transform : transforms) {
    ouly::write<std::endian::native>(batch_output, transform);
}
```

### 🧩 Utilities and Support Libraries

Comprehensive utility components for common programming tasks:

#### Command-Line Argument Parsing

```cpp
// Declare program arguments
ouly::program_args args;

// Define arguments with type safety
auto input_file = args.decl<std::string>("input", "i")
    .doc("Input file path")
    .required();

auto output_file = args.decl<std::string>("output", "o")
    .doc("Output file path")
    .default_value("output.txt");

auto thread_count = args.decl<int>("threads", "t")
    .doc("Number of worker threads")
    .default_value(std::thread::hardware_concurrency());

auto verbose = args.decl<bool>("verbose", "v")
    .doc("Enable verbose output");

auto quality = args.decl<float>("quality", "q")
    .doc("Quality factor (0.0 to 1.0)")
    .default_value(0.8f);

// Parse command line
if (!args.parse_args(argc, argv)) {
    std::cerr << "Usage: " << argv[0] << " " << args.usage() << std::endl;
    return 1;
}

// Use parsed values
std::cout << "Processing " << input_file.value() << std::endl;
std::cout << "Using " << thread_count.value() << " threads" << std::endl;

if (verbose.value()) {
    std::cout << "Output: " << output_file.value() << std::endl;
    std::cout << "Quality: " << quality.value() << std::endl;
}
```

#### Type-Safe Function Delegates

```cpp
// Create delegate for function signature
ouly::delegate<int(int, int)> math_operation;

// Bind lambda function
math_operation = ouly::delegate<int(int, int)>::bind([](int a, int b) {
    return a + b;
});

// Call delegate
int result = math_operation(5, 3);  // result = 8

// Bind member function
struct Calculator {
    int multiply(int a, int b) { return a * b; }
    int add(int a, int b) { return a + b; }
};

Calculator calc;
auto multiply_delegate = ouly::delegate<int(int, int)>::bind<&Calculator::multiply>(calc);
auto add_delegate = ouly::delegate<int(int, int)>::bind<&Calculator::add>(calc);

// Use with callbacks
void process_values(std::vector<int> const& values, 
                   ouly::delegate<int(int, int)> operation) {
    int accumulator = values[0];
    for (size_t i = 1; i < values.size(); ++i) {
        accumulator = operation(accumulator, values[i]);
    }
    return accumulator;
}

std::vector<int> numbers = {1, 2, 3, 4, 5};
int sum = process_values(numbers, add_delegate);      // 15
int product = process_values(numbers, multiply_delegate); // 120
``


#### Memory Mapping

```cpp
// Memory-mapped file access
ouly::memory_mapped_file mapped_file("large_dataset.bin");

if (mapped_file.is_valid()) {
    // Direct access to file contents without loading into memory
    auto* data = static_cast<float const*>(mapped_file.data());
    size_t float_count = mapped_file.size() / sizeof(float);
    
    // Process data directly from memory mapping
    float sum = 0.0f;
    for (size_t i = 0; i < float_count; ++i) {
        sum += data[i];
    }
    
    std::cout << "Sum of " << float_count << " floats: " << sum << std::endl;
}

// Memory-mapped allocator for large datasets
ouly::memory_mapped_allocator mm_allocator("working_space.tmp", 1024 * 1024 * 1024); // 1GB
auto* workspace = mm_allocator.allocate(1024 * 1024);  // 1MB allocation
```

## � Testing and Development

### Building the Library

OULY uses CMake with preset configurations for different build types:

```bash
# Debug build with full testing
cmake --preset=linux-default     # Debug + tests + examples
cmake --build build/linux-default

# Release build optimized for performance  
cmake --preset=linux-release     # Release optimizations
cmake --build build/linux-release

# Platform-specific presets
cmake --preset=macos-default     # macOS debug build
cmake --preset=windows-default   # Windows debug build
```

### Running Tests

```bash
# Build with testing enabled
cmake -B build -DOULY_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run all unit tests
cd build/unit_tests
ctest --verbose

# Run specific test categories
ctest -R "allocator"     # Run allocator tests
ctest -R "scheduler"     # Run scheduler tests  
ctest -R "ecs"           # Run ECS tests

# Run performance benchmarks
cd build
./unit_tests/bench_main

# Memory safety testing with AddressSanitizer
cmake -B build-asan -DASAN_ENABLED=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan
cd build-asan/unit_tests && ctest
```

### Performance Benchmarking

OULY includes comprehensive benchmarks comparing against industry standards:

```bash
# Run scheduler comparison against Intel TBB
./scripts/run_benchmarks.sh

# Analyze performance results
./scripts/analyze_performance.py

# Generate performance reports
./unit_tests/bench_performance      # Memory allocator benchmarks
./unit_tests/bench_scheduler_comparison  # Task scheduler comparison
```

## 📚 Documentation and Resources

### API Documentation

- **Online Documentation**: [OULY API Reference](https://ouly-optimized-utility-library.readthedocs.io/en/latest/)
- **Header Documentation**: All headers include comprehensive Doxygen documentation
- **Examples Repository**: [OULY Examples](https://github.com/obhi-d/ouly-examples)

### Performance Comparisons

OULY has been benchmarked against industry-standard libraries:

| Component         | Comparison    | Performance Gain                           |
| ----------------- | ------------- | ------------------------------------------ |
| Task Scheduler    | Intel TBB     | 15-30% faster task submission              |
| Memory Allocators | System malloc | 2-10x faster for specific patterns         |
| ECS Components    | EnTT          | Comparable performance, better cache usage |
| Containers        | STL           | 10-25% better cache performance            |

### Debug Support

OULY includes comprehensive debugging aids:

```cpp
// Debug helpers (Windows)
#include "debug_helpers/containers.natvis"  // Visual Studio visualizers

// LLDB/GDB pretty printers (Linux/macOS)  
#include "debug_helpers/pretty_printer_lldb.py"
#include "debug_helpers/pretty_printer.py"

// Built-in assertions and validation
#define OULY_DEBUG_MODE  // Enable debug checks
#include <ouly/core/debug.hpp>

void some_function() {
    OULY_ASSERT(condition, "Custom error message");
    OULY_DEBUG_ONLY(expensive_validation());
}
```

### Contributing

We welcome contributions! See our [Contributing Guide](CONTRIBUTING.md) for details on:

- Code style and conventions
- Submitting pull requests  
- Running performance benchmarks
- Adding new features
- Writing tests

### Community and Support

- **GitHub Issues**: [Report bugs and request features](https://github.com/obhi-d/ouly/issues)
- **Discussions**: [Community forum and Q&A](https://github.com/obhi-d/ouly/discussions)
- **Performance Reports**: Automated benchmarking in CI/CD pipeline

## 🔗 Related Projects

OULY works well with these complementary libraries:

- **Graphics**: [GLM](https://github.com/g-truc/glm) for mathematics
- **Networking**: [Asio](https://github.com/chriskohlhoff/asio) for async I/O
- **Testing**: [Catch2](https://github.com/catchorg/Catch2) for unit tests
- **Benchmarking**: [nanobench](https://github.com/martinus/nanobench) for microbenchmarks

## � License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Intel TBB** team for inspiring the task scheduler design
- **EnTT** project for ECS architecture insights  
- **C++ Standards Committee** for C++20 features enabling zero-cost abstractions
- **Contributors** who have helped improve performance and reliability

---

<div align="center">

**Built with ❤️ for high-performance C++ applications**

[⭐ Star us on GitHub](https://github.com/obhi-d/ouly) | [📖 Read the Docs](https://ouly-optimized-utility-library.readthedocs.io/) | [🐛 Report Issues](https://github.com/obhi-d/ouly/issues)

</div>