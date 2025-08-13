# OULY - Optimized Utility Library

<div align="center">

[![CI](https://github.com/obhi-d/ouly/actions/workflows/ci.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/ci.yml)
[![Performance](https://github.com/obhi-d/ouly/actions/workflows/performance.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/performance.yml)
[![Coverity Scan](https://scan.coverity.com/projects/30824/badge.svg)](https://scan.coverity.com/projects/obhi-d-ouly)
[![codecov](https://codecov.io/gh/obhi-d/ouly/graph/badge.svg?token=POS8O18G9B)](https://codecov.io/gh/obhi-d/ouly)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**A modern C++20 high-performance utility library for performance-critical applications**

Featuring memory allocators, containers, ECS, task schedulers, flow graphs, and serialization

</div>

OULY is a comprehensive C++20 library designed for high-performance applications where every nanosecond counts. It provides zero-cost abstractions, cache-friendly data structures, and lock-free algorithms optimized for modern multi-core systems.

## Core Principles

- **Zero-Cost Abstractions**: Template-heavy design that compiles away overhead
- **Cache-Friendly**: Structure of Arrays (SoA) patterns and memory-optimized layouts
- **Lock-Free**: Atomic operations and work-stealing algorithms for scalability
- **NUMA-Aware**: Thread affinity and memory locality optimizations
- **Performance-First**: Every component benchmarked against industry standards

## Quick Start

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

# Build with release optimizations (choose your platform preset)
cmake --preset=linux-release
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
#include <cstdint>
#include <iostream>

#include <ouly/allocators/linear_allocator.hpp>
#include <ouly/containers/array_types.hpp>      // dynamic_array
#include <ouly/scheduler/scheduler.hpp>         // v2 scheduler, task_context, async

int main() {
    // Linear allocator (unit_tests/linear_allocator.cpp)
    ouly::linear_allocator<> alloc(1024);
    auto* p = ouly::allocate<std::uint8_t>(alloc, 64);
    alloc.deallocate(p, 64);

    // Dynamic array (unit_tests/dynamic_array.cpp)
    ouly::dynamic_array<int> arr;
    arr = ouly::dynamic_array<int>(10, 0);

    // v2 scheduler (unit_tests/scheduler_tests.cpp)
    ouly::v2::scheduler scheduler;
    scheduler.create_group(ouly::default_workgroup_id, 0, 4);
    scheduler.begin_execution();

    auto const& ctx = ouly::v2::task_context::this_context::get();
    scheduler.submit(ctx, ouly::default_workgroup_id,
        [](ouly::v2::task_context const& wctx) {
            std::cout << "Worker " << wctx.get_worker().get_index() << " says hi\n";
        });

    scheduler.end_execution();
}
```

## Core Components

### Memory Allocators

OULY provides specialized allocators optimized for different allocation patterns:

#### Linear Allocators
LIFO-friendly, extremely fast allocation (see unit_tests/linear_allocator.cpp):

```cpp
#include <ouly/allocators/linear_allocator.hpp>

ouly::linear_allocator<> allocator(1000);
auto* start  = ouly::allocate<std::uint8_t>(allocator, 40);
auto* off100 = ouly::allocate<std::uint8_t>(allocator, 100);

// LIFO deallocation reclaims if it's the most recent block
allocator.deallocate(off100, 100);
off100 = ouly::allocate<std::uint8_t>(allocator, 100);
```

Linear arena allocator with multiple arenas and rewind (see unit_tests/linear_allocator.cpp):

```cpp
#include <ouly/allocators/linear_arena_allocator.hpp>

using arena_alloc = ouly::linear_arena_allocator<>;
arena_alloc allocator(1024);

auto* a = ouly::allocate<std::uint8_t>(allocator, 40);
auto* b = ouly::allocate<std::uint8_t>(allocator, 100);

allocator.rewind();      // reset offsets in all arenas
allocator.smart_rewind(); // may release unused arenas
```

#### Arena Allocators
Coalescing arena allocator (unit_tests/coalescing_allocator.cpp):

```cpp
#include <ouly/allocators/coalescing_arena_allocator.hpp>

ouly::coalescing_arena_allocator arena;
auto* p1 = ouly::allocate<std::uint8_t>(arena, 128);
auto* p2 = ouly::allocate<std::uint8_t>(arena, 256);
arena.deallocate(p2, 256);
arena.deallocate(p1, 128);
```

#### Pool Allocators
Fixed-size blocks with optional STL interop (unit_tests/pool_allocator.cpp):

```cpp
#include <ouly/allocators/pool_allocator.hpp>
#include <ouly/allocators/std_allocator_wrapper.hpp>

ouly::pool_allocator<> pool(8, 1000);
using std_alloc_u64 = ouly::allocator_ref<std::uint64_t, ouly::pool_allocator<>>;
std::vector<std::uint64_t, std_alloc_u64> v(std_alloc_u64(pool));
v.resize(256);
```

#### Allocator Wrappers
Use custom allocators with STL containers (unit_tests/pool_allocator.cpp):

```cpp
#include <ouly/allocators/std_allocator_wrapper.hpp>

ouly::pool_allocator<> pool(8, 1000);
using VecAlloc = ouly::allocator_ref<int, ouly::pool_allocator<>>;
std::vector<int, VecAlloc> vec(VecAlloc(pool));
```

#### Memory-mapped Allocators and Virtual Memory
Map files or allocate virtual memory (unit_tests/memory_mapped_allocators.cpp):

```cpp
#include <ouly/allocators/mmap_file.hpp>
#include <ouly/allocators/virtual_allocator.hpp>

// Read-write mapping
ouly::mmap_sink sink;
sink.map("data.bin");
std::ranges::fill(sink, static_cast<unsigned char>(0xAB));
sink.sync();

// Read-only mapping
ouly::mmap_source source;
source.map("data.bin");
auto first = source[0];

// Virtual allocator
ouly::virtual_allocator<> valloc;
void* ptr = valloc.allocate(4096);
valloc.deallocate(ptr, 4096);
```

### High-Performance Containers

Cache-friendly containers with STL-like interfaces:

#### Small Vector
Stack-optimized for small sizes (unit_tests/small_vector.cpp):

```cpp
#include <ouly/containers/small_vector.hpp>

ouly::small_vector<std::string, 5> v;
v.emplace_back("a");
v.emplace_back("b");
v.insert(v.begin() + 1, "x");
v.erase(v.begin());
```

#### Sparse Vector
Sparse index storage with views (unit_tests/sparse_vector.cpp):

```cpp
#include <ouly/containers/sparse_vector.hpp>

ouly::sparse_vector<int> v;
v.emplace_at(1, 100);
v.emplace_at(10, 200);

bool has10 = v.contains(10);
int  val1  = v[1];
v.erase(10);
```

#### Structure of Arrays (SoA) Vector
Cache-friendly vector over aggregate types (unit_tests/soavector.cpp):

```cpp
#include <ouly/containers/soavector.hpp>

struct Position { float x, y, z; };
ouly::soavector<Position> positions;
positions.emplace_back(Position{1,2,3});
positions.emplace_back(Position{4,5,6});
```

#### Intrusive List
Zero-allocation linked lists (unit_tests/intrusive_list.cpp):

```cpp
#include <ouly/containers/intrusive_list.hpp>

struct object { std::string value; ouly::list_hook hook; };
using list_t = ouly::intrusive_list<&object::hook, true, true>; // has tail, unique

list_t il;
object a{"a"}, b{"b"}, c{"c"};
il.push_back(a); il.push_back(b); il.push_back(c);
for (auto& it : il) { /* use it.value */ }
```

### Multi-Threading and Task Scheduling

Advanced work-stealing scheduler with workgroup organization:

#### Basic Task Scheduling (v2)

```cpp
#include <ouly/scheduler/scheduler.hpp>

ouly::v2::scheduler scheduler;
scheduler.create_group(ouly::default_workgroup_id, 0, 4);
scheduler.begin_execution();

auto const& ctx = ouly::v2::task_context::this_context::get();
scheduler.submit(ctx, ouly::default_workgroup_id,
    [](ouly::v2::task_context const& wctx) {
        // your work here
    });

scheduler.end_execution();
```

#### Parallel Algorithms

Use the provided context, not a group id (unit_tests/scheduler_tests.cpp):

```cpp
#include <ouly/scheduler/parallel_for.hpp>

std::vector<uint32_t> data(10000);
std::iota(data.begin(), data.end(), 0);

auto const& ctx = ouly::v2::task_context::this_context::get();
ouly::parallel_for(
    [](uint32_t& elem, ouly::v2::task_context const&) { elem *= 2; },
    data, ctx);

// Range-based overload
ouly::parallel_for(
    [](auto begin, auto end, ouly::v2::task_context const&) {
        for (auto it = begin; it != end; ++it) { *it += 1; }
    },
    data, ctx);
```

#### Coroutine Support

Submit co_task<T> directly (unit_tests/scheduler_comparison_tests.cpp):

```cpp
#include <ouly/scheduler/task.hpp>

ouly::co_task<void> my_task() {
    co_return;
}

ouly::v2::scheduler scheduler;
scheduler.create_group(ouly::default_workgroup_id, 0, 2);
scheduler.begin_execution();
auto const& ctx = ouly::v2::task_context::this_context::get();

auto task = my_task();
scheduler.submit(ctx, ouly::default_workgroup_id, task);
task.sync_wait_result();

scheduler.end_execution();
```

#### Flow Graph for Complex Dependencies

Use flow_graph with v2 scheduler (unit_tests/flow_graph_tests.cpp):

```cpp
#include <ouly/scheduler/flow_graph.hpp>

using Scheduler = ouly::v2::scheduler;
ouly::flow_graph<Scheduler> graph;

Scheduler scheduler;
scheduler.create_group(ouly::default_workgroup_id, 0, 2);
scheduler.begin_execution();

auto const& ctx = Scheduler::context_type::this_context::get();
auto n1 = graph.create_node();
auto n2 = graph.create_node();
graph.connect(n1, n2);

graph.add(n1, Scheduler::delegate_type::bind([](auto const&) {/*work*/}));
graph.add(n2, Scheduler::delegate_type::bind([](auto const&) {/*work*/}));

graph.start(ctx);
graph.cooperative_wait(ctx);

scheduler.end_execution();
```

### Entity Component System (ECS)

High-performance ECS framework optimized for cache efficiency and batch processing:

#### Basic Entity Management

```cpp
#include <ouly/ecs/registry.hpp>
#include <ouly/ecs/components.hpp>

ouly::ecs::rxregistry<> registry;                  // revision-tracked registry
ouly::ecs::components<int, ouly::ecs::rxentity<>> ints; // component table

ints.set_max(registry.max_size());
auto e = registry.emplace();
ints.emplace_at(e, 42);
bool has = ints.contains(e);
```

#### Component Storage Strategies

```cpp
// Storage configs (from unit_tests/ecs_tests.cpp patterns)
using Direct = ouly::cfg::use_direct_mapping;
ouly::ecs::components<int, ouly::ecs::rxentity<>, Direct> fast_ints;
fast_ints.set_max(registry.max_size());
fast_ints.emplace_at(e, 7);
```

#### Efficient Component Access and Iteration

```cpp
// Access and update
if (ints.contains(e)) { ints[e] += 5; }
```

#### Collections for Grouped Processing

```cpp
// Collections (unit_tests/ecs_tests.cpp)
ouly::ecs::collection<ouly::ecs::rxentity<>> subset;
subset.emplace(e);
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

### Serialization

Flexible and high-performance serialization framework supporting multiple formats:

#### Binary Serialization

Binary in-memory and stream adapters (unit_tests/binary_stream.cpp, binary_serializer.cpp):

```cpp
#include <ouly/serializers/binary_stream.hpp>
#include <ouly/serializers/serializers.hpp>
#include <ouly/reflection/reflection.hpp>

struct Test 
{ 
  int a; std::string b; 
  static auto reflect() noexcept {
    return ouly::bind(ouly::bind<"a", &Test::a>(), ouly::bind<"b", &Test::b>());
  }
};

// In-memory
ouly::binary_output_stream out;
Test t1{10, "hello"};
ouly::write(out, t1);

ouly::binary_input_stream in(out.get_string());
Test t2{};
ouly::read(in, t2);

// File stream
std::ofstream ofs("data.bin", std::ios::binary);
ouly::binary_ostream bos(ofs);
bos.stream_out(t1);
```

#### YAML Serialization

Human-readable YAML for reflected types and STL (unit_tests/yaml_output_serializer.cpp, yaml_input_serializer.cpp):

```cpp
#include <ouly/serializers/lite_yml.hpp>
#include <ouly/reflection/reflection.hpp>

struct Cfg { int a; std::string b; };

Cfg cfg{100, "value"};
std::string y = ouly::yml::to_string(cfg);

Cfg parsed{};
ouly::yml::from_string(parsed, y);
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

### Utilities and Support Libraries

Comprehensive utility components for common programming tasks:

#### Command-Line Argument Parsing

Sink-style parsing (unit_tests/program_args.cpp):

```cpp
#include <ouly/utility/program_args.hpp>

const char* argv[] = {"--flag", "--name=ouly", "-n=[1,2,3]"};
ouly::program_args args;
args.parse_args(3, argv);

bool flag = false; std::string_view name; std::vector<int> ns;
args.sink(flag, "flag");
args.sink(name, "name");
args.sink(ns,   "numbers", "n");
```

#### Type-Safe Function Delegates

Bind free, lambda, and member functions (unit_tests/basic_tests.cpp):

```cpp
#include <ouly/utility/delegate.hpp>

using del_t = ouly::delegate<int(int,int)>;
auto d1 = del_t::bind([](int a,int b){return a+b;});
struct Calc{ int mul(int a,int b){return a*b;} } c;
auto d2 = del_t::bind<&Calc::mul>(c);
int s = d1(2,3); // 5
int p = d2(2,3); // 6
```


#### Memory Mapping

Use mmap_source/mmap_sink (unit_tests/memory_mapped_allocators.cpp):

```cpp
#include <ouly/allocators/mmap_file.hpp>

ouly::mmap_sink sink; sink.map("file.bin");
sink[0] = std::byte{0xFF};

ouly::mmap_source src; src.map("file.bin");
auto first = src[0];
```

## Testing and Development

### Building the Library

OULY uses CMake with preset configurations for different build types:

```bash
# Debug build with full testing
cmake --preset=linux-default
cmake --build build/linux-default

# Release build optimized for performance
cmake --preset=linux-release
cmake --build build/linux-release

# Platform-specific presets
cmake --preset=macos-default
cmake --preset=windows-default
```

### Running Tests

```bash
# Build with testing enabled
cmake -B build -DOULY_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run all unit tests
cd build/unit_tests
ctest --verbose

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

## Documentation and Resources

### API Documentation

- **Online Documentation**: [OULY API Reference](https://ouly-optimized-utility-library.readthedocs.io/en/latest/)
- **Header Documentation**: All headers include comprehensive Doxygen documentation
- **Examples Repository**: [OULY Examples](https://github.com/obhi-d/ouly-examples)

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

## Related Projects

OULY works well with these complementary libraries:

- **Graphics**: [GLM](https://github.com/g-truc/glm) for mathematics
- **Networking**: [Asio](https://github.com/chriskohlhoff/asio) for async I/O
- **Testing**: [Catch2](https://github.com/catchorg/Catch2) for unit tests
- **Benchmarking**: [nanobench](https://github.com/martinus/nanobench) for microbenchmarks

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---
