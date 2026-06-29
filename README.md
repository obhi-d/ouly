# OULY - Optimized Utility Library

<div align="center">

[![CI](https://github.com/obhi-d/ouly/actions/workflows/ci.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/ci.yml)
[![Coverity Scan](https://scan.coverity.com/projects/30824/badge.svg)](https://scan.coverity.com/projects/obhi-d-ouly)
[![codecov](https://codecov.io/gh/obhi-d/ouly/graph/badge.svg?token=POS8O18G9B)](https://codecov.io/gh/obhi-d/ouly)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**A modern C++20 high-performance utility library for performance-critical applications**

Task schedulers, memory allocators, cache-friendly containers, ECS, reflection-driven serialization, and supporting utilities

</div>

OULY is a comprehensive C++20 library designed for high-performance applications where every nanosecond counts. It provides zero-cost abstractions, cache-friendly data structures, and lock-free algorithms optimized for modern multi-core systems.

## Library at a Glance

| Module                       | Headers                                 | What it provides                                                                                                                                      |
| ---------------------------- | --------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| Task Scheduling              | `ouly/scheduler/`                       | Work-stealing task schedulers (three implementations), `parallel_for`, adaptive `auto_parallel_for`, coroutine tasks, static and dynamic flow graphs  |
| Memory Allocators            | `ouly/allocators/`                      | Linear/arena/pool allocators, thread-safe allocators, coalescing GPU-style suballocators with defragmentation, virtual memory and memory-mapped files |
| Containers                   | `ouly/containers/`                      | Lock-free MPMC queue, SoA vector, small vector, sparse vector/table, intrusive list, blackboard                                                       |
| Entity Component System      | `ouly/ecs/`                             | Registries with revision tracking, configurable component storage, collections, sparse-to-dense maps                                                  |
| Serialization and Reflection | `ouly/serializers/`, `ouly/reflection/` | Compile-time reflection driving binary, YAML, and user-defined structured formats                                                                     |
| Utilities and DSL            | `ouly/utility/`, `ouly/dsl/`            | Delegates, program argument parsing, views, intrusive pointers, hashing, a lightweight YAML parser and a macro expression evaluator                   |

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

### Task Scheduling

Work-stealing schedulers with workgroup organization, benchmarked against Intel TBB.
OULY ships three scheduler implementations that share the same submission API:

- `ouly::v1::scheduler`: the original implementation with per-workgroup worker queues
- `ouly::v2::scheduler`: work-stealing design with Chase-Lev deques; it lives in an inline
  namespace, so it is also available as `ouly::scheduler`
- `ouly::v3::scheduler`: game-engine oriented design with fixed worker-to-workgroup membership
  and condition-variable parking, so idle workers release their CPUs without spinning

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

The v3 scheduler is a drop-in replacement for game loops: workgroup membership is fixed at
`begin_execution()` time, queue accounting is exact, and idle workers block on a
condition variable instead of spinning while long tasks run elsewhere:

```cpp
ouly::v3::scheduler scheduler;
scheduler.create_group(ouly::default_workgroup_id, 0, 4);
scheduler.begin_execution();

auto const& ctx = ouly::v3::task_context::this_context::get();
scheduler.submit(ctx, ouly::default_workgroup_id,
    [](ouly::v3::task_context const&) { /* work */ });

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

For irregular workloads, `ouly::auto_parallel_for` (in `ouly/scheduler/auto_parallel_for.hpp`)
uses TBB-style adaptive partitioning that reacts to load imbalance and work-stealing patterns
instead of splitting the range into fixed chunks (unit_tests/test_auto_parallel_for.cpp).

#### Coroutine Support

Submit co_task<T> directly (unit_tests/scheduler_comparison_tests.cpp):

```cpp
#include <ouly/scheduler/co_task.hpp>

ouly::co_task<void> my_task() {
    co_return;
}

ouly::v2::scheduler scheduler;
scheduler.create_group(ouly::default_workgroup_id, 0, 2);
scheduler.begin_execution();
auto const& ctx = ouly::v2::task_context::this_context::get();

auto task = my_task();
scheduler.submit(ctx, ouly::default_workgroup_id, task);
task.wait();

scheduler.end_execution();
```

#### Flow Graphs for Task Dependencies

`ouly::flow_graph` executes a static dependency graph: connect nodes, add tasks, start once
(unit_tests/flow_graph_tests.cpp):

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

graph.add(n1, [](auto const&) {/*work*/});
graph.add(n2, [](auto const&) {/*work*/});

graph.start(ctx);
graph.cooperative_wait(ctx);

scheduler.end_execution();
```

`ouly::dynamic_flow_graph` (in `ouly/scheduler/dynamic_flow_graph.hpp`) extends this to the
persistent game-loop pattern: nodes and edges can be added or removed while the graph is
running, cycles are first class (a frame can loop back onto itself), and nodes fire each time
they accumulate `in_degree` triggers. The graph is seeded with `signal()` and drained with
`request_stop()` (unit_tests/dynamic_flow_graph_tests.cpp).

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

#### Thread-Safe Allocators
Frame-oriented allocators for multi-threaded producers (unit_tests/thread_safe_allocators.cpp):

```cpp
#include <ouly/allocators/ts_shared_linear_allocator.hpp>
#include <ouly/allocators/ts_thread_local_allocator.hpp>

// Lock-free shared arenas: many threads bump-allocate from the same pages
ouly::ts_shared_linear_allocator shared;
void* p = shared.allocate(128);
shared.reset();   // frame boundary: call from a single thread

// Per-thread arenas: zero synchronization on the allocation fast path
ouly::ts_thread_local_allocator scratch;
void* q = scratch.allocate(64);
scratch.reset();  // generation-based invalidation, single-threaded
```

#### Coalescing Allocators
Offset-based allocators meant for GPU/memory range suballocation (unit_tests/coalescing_allocator.cpp):

```cpp
#include <ouly/allocators/coalescing_allocator.hpp>

ouly::coalescing_allocator allocator;
auto off1 = allocator.allocate(256); // returns an offset, not a pointer
auto off2 = allocator.allocate(256);
allocator.deallocate(off1, 256);     // adjacent free blocks are merged
```

The arena variant, `ouly::coalescing_arena_allocator`, manages multiple arenas and reports arena
add/remove events to a user-provided memory manager satisfying the `CoalescingMemoryManager`
concept. See unit_tests/coalescing_allocator.cpp for a complete example with a manager.

#### Defragmenting Allocators
`ouly::coalescing_arena_defrag_allocator` and `ouly::coalescing_defrag_allocator`
(unit_tests/coalescing_defrag_allocator.cpp) extend the coalescing allocators with compaction
passes for long-lived heaps such as GPU memory:

- Allocations keep stable `allocation_id` handles across defragmentation
- A defragmentation pass emits `move_memory` callbacks to the user's memory manager in a
  memmove-safe order, followed by `rebind_alloc` notifications with each allocation's new placement
- Dedicated (pinned) allocations bypass compaction entirely

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

`ouly::object_pool` (unit_tests/object_pool.cpp) provides an intrusive free-list pool for
fixed-size object recycling.

#### Memory-mapped Files and Virtual Memory
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

#### Concurrent Queue
A lock-free multi-producer, multi-consumer FIFO queue built from a chain of fixed-size buckets
with monotonic cursors and per-slot commit flags (unit_tests/concurrent_queue.cpp):

```cpp
#include <ouly/containers/concurrent_queue.hpp>

ouly::concurrent_queue<int> queue;
queue.emplace(42);   // multi-producer safe
queue.enqueue(7);

int value = 0;
if (queue.try_dequeue(value)) { /* multi-consumer safe */ }
```

A fast variant (`ouly::cfg::single_threaded_consumer_for_each`) trades dequeue support for
single-threaded `for_each` traversal, useful for collect-then-process patterns.

#### Structure of Arrays (SoA) Vector
Cache-friendly vector over aggregate types (unit_tests/soavector.cpp):

```cpp
#include <ouly/containers/soavector.hpp>

struct Position { float x, y, z; };
ouly::soavector<Position> positions;
positions.emplace_back(Position{1,2,3});
positions.emplace_back(Position{4,5,6});
```

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

#### Sparse Vector and Sparse Table
Sparse index storage with page-wise allocation (unit_tests/sparse_vector.cpp):

```cpp
#include <ouly/containers/sparse_vector.hpp>

ouly::sparse_vector<int> v;
v.emplace_at(1, 100);
v.emplace_at(10, 200);

bool has10 = v.contains(10);
int  val1  = v[1];
v.erase(10);
```

`ouly::sparse_table` (unit_tests/sparse_table.cpp) builds on the same idea but hands out stable
links on insertion, so elements can be erased and looked up through handles while iteration stays
pool-contiguous.

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

#### Blackboard
Name-indexed (or type-indexed via config) blob storage for heterogeneous data (unit_tests/blackboard.cpp):

```cpp
#include <ouly/containers/blackboard.hpp>

ouly::blackboard board;
board.emplace<std::uint32_t>("param1", 50);
auto value = board.get<std::uint32_t>("param1");
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

Entities are typed handles with optional revision bits that detect stale references after an
entity slot is recycled. Use `ouly::ecs::registry`/`entity` when revision tracking is not needed.

#### Component Storage Strategies

```cpp
// Storage configs (from unit_tests/ecs_tests.cpp patterns)
using Direct = ouly::cfg::use_direct_mapping;
ouly::ecs::components<int, ouly::ecs::rxentity<>, Direct> fast_ints;
fast_ints.set_max(registry.max_size());
fast_ints.emplace_at(e, 7);
```

Storage is configurable per component type: dense storage with a sparse index (default), direct
mapping for components present on most entities, and an optional self index for erase-by-value.

#### Efficient Component Access and Iteration

```cpp
// Access and update
if (ints.contains(e)) { ints[e] += 5; }
```

#### Collections for Grouped Processing

```cpp
// Collections (unit_tests/ecs_collection_tests.cpp)
ouly::ecs::collection<ouly::ecs::rxentity<>> subset;
subset.emplace(e);
```

Collections track entity membership in lazily allocated bitset pages, so dense subsets cost
memory only where entities actually live. `ouly::ecs::map` provides sparse-to-dense index
mapping with swap-and-pop removal (unit_tests/ecs_map_tests.cpp).

#### Advanced ECS Patterns

```cpp
struct Position { float x, y, z; };
struct Velocity { float dx, dy, dz; };

ouly::ecs::components<Position, ouly::ecs::rxentity<>> positions;
ouly::ecs::components<Velocity, ouly::ecs::rxentity<>> velocities;
ouly::ecs::collection<ouly::ecs::rxentity<>>           physics_entities;

// Batch entity creation
auto create_projectile = [&](float x, float y, float dx, float dy) {
    auto projectile = registry.emplace();
    positions.emplace_at(projectile, x, y, 0.0f);
    velocities.emplace_at(projectile, dx, dy, 0.0f);
    physics_entities.emplace(projectile);
    return projectile;
};

// Iterate all stored components together with their entities
positions.for_each([](auto entity, Position& pos) {
    // process every stored position
});

// Iterate only the subset of entities tracked by a collection
physics_entities.for_each(positions, [](auto entity, Position& pos) {
    // process positions of entities in the collection
});
```

### Serialization and Reflection

Serialization in OULY is driven by compile-time reflection (`ouly/reflection/`): aggregate types
with up to 64 members are reflected automatically, and any type can opt in explicitly with a
static `reflect()` function built from `ouly::bind`. The same reflection data drives every
format: binary, YAML, and any user-defined structured stream that models the
`StructuredInputStream`/`StructuredOutputStream` concepts in `ouly/serializers/serializers.hpp`.

#### Binary Serialization

Binary in-memory and stream adapters (unit_tests/binary_stream.cpp, binary_serializer.cpp):

```cpp
#include <ouly/serializers/binary_stream.hpp>
#include <ouly/serializers/serializers.hpp>
#include <ouly/reflection/reflection.hpp>

struct Test
{
  int a = 0;
  std::string b;
  Test() = default;
  Test(int v, std::string s) : a(v), b(std::move(s)) {}
  // Explicit reflection for non-aggregate types
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

// Implicit reflection for aggregate types with up to 64 members
struct Cfg { int a; std::string b; };

Cfg cfg{100, "value"};
std::string y = ouly::yml::to_string(cfg);

Cfg parsed{};
ouly::yml::from_string(parsed, y);
```

#### Custom Serialization

Types that cannot use reflection can provide stream operators (`operator<<` / `operator>>`)
accepting the serializer. Binary read and write accept an explicit endianness template argument
(little-endian by default):

```cpp
std::vector<Test> transforms;
// ... populate transforms ...

ouly::binary_output_stream out;
ouly::write<std::endian::big>(out, transforms);   // explicit endianness

ouly::binary_input_stream in(out.get_string());
ouly::read<std::endian::big>(in, transforms);
```

### Utilities and DSL

Supporting components for common programming tasks:

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

Bind free, lambda, and member functions with small object optimization (unit_tests/basic_tests.cpp):

```cpp
#include <ouly/utility/delegate.hpp>

using del_t = ouly::delegate<int(int,int)>;
auto d1 = del_t::bind([](int a,int b){return a+b;});
struct Calc{ int mul(int a,int b){return a*b;} } c;
auto d2 = del_t::bind<&Calc::mul>(c);
int s = d1(2,3); // 5
int p = d2(2,3); // 6
```

#### Macro Expression Evaluator

`ouly::microexpr` (unit_tests/microexpr_tests.cpp) evaluates boolean macro expressions, useful
for preprocessor-style configuration filters:

```cpp
#include <ouly/dsl/microexpr.hpp>

ouly::microexpr expr([](std::string_view name) -> std::optional<int> {
    if (name == "FEATURE_A") return 1;
    return std::nullopt; // undefined macro
});

bool enabled = expr.evaluate("$FEATURE_A && !$FEATURE_B");
```

#### More Utilities

- **Views**: `zip_view` for iterating multiple spans in lockstep, `projected_view` for iterating
  a single member across an array of structs, and `subrange` with binary splitting for
  divide-and-conquer algorithms
- **Smart pointers**: `intrusive_ptr` for reference-counted objects without control blocks,
  `tagged_ptr`/`compressed_ptr` for pointer tagging, `tagged_int` for type-safe integer handles
- **Hashing**: `wyhash32`/`wyhash64` and `komihash64` fast non-cryptographic hashes
- **Strings**: compile-time `string_literal`, string utilities, fast `from_chars`/`to_chars`
  wrappers with optional fast_float support
- **Optionals**: `optional_ref`, `optional_val`, and `nullable_optional` for storage-efficient
  optional semantics

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

OULY ships debugging aids in the `debug_helpers/` directory:

- `containers.natvis`: Visual Studio visualizers for OULY containers
- `pretty_printer_lldb.py`: LLDB pretty printers (Linux/macOS)
- `pretty_printer.py`: GDB pretty printers

Internal assertions use the `OULY_ASSERT` macro, which defaults to `assert` from `<cassert>`.
It can be overridden by defining `OULY_ASSERT` before including OULY headers, or by providing
a `ouly_user_defines.hpp` header that is picked up automatically by `ouly/utility/user_config.hpp`.

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
