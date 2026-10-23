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

OULY is a C++20 library for programs where allocation patterns, cache behaviour and thread
wake-ups matter: game engines, simulations, tools and services. It collects the building blocks
such programs keep rewriting (a work-stealing scheduler, frame and offset allocators, small and
structure-of-arrays containers, an ECS, and reflection-based serialization) behind small,
STL-like interfaces that you can adopt one header at a time.

**Documentation:** [obhi-d.github.io/ouly](https://obhi-d.github.io/ouly/) has the tutorials and the API reference.

## Library at a Glance

| Module                       | Headers                                 | What it provides                                                                                                                                      |
| ---------------------------- | --------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| Task Scheduling              | `ouly/scheduler/`                       | Three work-stealing schedulers, value tasks and continuations, structured task scopes, `parallel_for`, coroutine tasks, and static and dynamic flow graphs |
| Memory Allocators            | `ouly/allocators/`                      | Linear/arena/stack/pool allocators, thread-safe allocators, coalescing GPU-style suballocators with defragmentation, virtual memory and memory-mapped files |
| Containers                   | `ouly/containers/`                      | Small vector, SoA vector, sparse vector/table, block vector, intrusive list, lock-free MPMC queue, blackboard, index maps and bitsets                   |
| Entity Component System      | `ouly/ecs/`                             | Registries with optional revision tracking, configurable component storage, collections, sparse-to-dense maps                                        |
| Serialization and Reflection | `ouly/serializers/`, `ouly/reflection/` | Compile-time reflection driving binary, YAML, and user-defined structured formats                                                                     |
| Utilities and DSL            | `ouly/utility/`, `ouly/dsl/`            | Delegates, program argument parsing, views, intrusive pointers, hashing, a lightweight YAML parser and a macro expression evaluator                   |

## Quick Start

### Requirements

- A C++20 compiler with concepts and coroutines. CI builds and tests GCC 16, Clang 18,
  Apple Clang (macos-latest) and MSVC (windows-latest).
- CMake 3.25 or newer. The presets use Ninja.
- Linux, macOS or Windows.

OULY is a compiled library (the scheduler, the coalescing allocators, the thread-safe allocators
and the DSL parsers have `.cpp` files), so link the `ouly::ouly` target rather than only adding the
include path.

### Build

```bash
git clone https://github.com/obhi-d/ouly.git
cd ouly

cmake --preset=linux-release          # or macos-release / windows-release
cmake --build build/linux-release
ctest --test-dir build/linux-release  # the release presets also build the unit tests
```

### Use it from CMake

```cmake
# As a subdirectory (or through FetchContent)
add_subdirectory(external/ouly)
target_link_libraries(your_target PRIVATE ouly::ouly)

# Or after `cmake --install`
find_package(ouly REQUIRED)
target_link_libraries(your_target PRIVATE ouly::ouly)
```

### First program

```cpp
#include <ouly/scheduler/parallel_for.hpp>
#include <ouly/scheduler/scheduler.hpp>

#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

int main()
{
  // Workgroup 0 spans worker threads [0, 4). Worker 0 is the thread that calls begin_execution().
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::default_workgroup_id, 0, 4);
  scheduler.begin_execution();

  auto const& ctx = ouly::task_context::this_context::get();

  std::vector<std::uint32_t> values(100'000);
  std::iota(values.begin(), values.end(), 0U);

  // Split the range across the workgroup; returns once every element is processed.
  ouly::parallel_for([](std::uint32_t& value, ouly::task_context const&) { value *= 2; }, values, ctx);

  // A value task; get(ctx) runs queued work on this thread while it waits.
  auto sum = ouly::submit_task(ctx, [&values] { return std::accumulate(values.begin(), values.end(), std::uint64_t{0}); });
  std::cout << "sum = " << sum.get(ctx) << '\n';

  scheduler.end_execution();
}
```

## A Short Tour

Each example below is a complete fragment that compiles against the current headers. The linked
pages go into the details.

### Task scheduling

Work is grouped into workgroups that own a range of worker threads. Tasks can be fire-and-forget
callables, reference-counted value tasks with continuations, structured fork/join scopes, or
`co_task` coroutines.

```cpp
#include <ouly/scheduler/scheduler.hpp>

ouly::scheduler scheduler;
scheduler.create_group(ouly::default_workgroup_id, 0, 4);
scheduler.begin_execution();
auto const& ctx = ouly::task_context::this_context::get();

// Fire and forget
scheduler.submit(ctx, [](ouly::task_context const&) { /* runs on a worker of workgroup 0 */ });

// Value tasks and continuations
auto loaded  = ouly::submit_task(ctx, [] { return 21; });
auto doubled = loaded.then(ctx, [](int value) { return value * 2; });
int  result  = doubled.get(ctx); // 42

// Fork/join
ouly::task_scope scope;
for (int i = 0; i < 8; ++i)
{
  scope.run(ctx, [i] { process(i); });
}
scope.join(ctx);

scheduler.end_execution();
```

There are three scheduler generations with the same submission API. `ouly::scheduler` is
`ouly::v2::scheduler` unless you define `OULY_SCHEDULER_VERSION` to `v1` or `v3`; `v3` parks idle
workers on a condition variable instead of spinning, which suits game loops that share the machine
with render, audio and IO threads. `parallel_for`, `auto_parallel_for` (adaptive splitting for
uneven work), `flow_graph` and `dynamic_flow_graph` build on the same task context.
See the [scheduler tutorial](https://obhi-d.github.io/ouly/tutorials/scheduler_tutorial.html).

### Memory allocators

```cpp
#include <ouly/allocators/coalescing_allocator.hpp>
#include <ouly/allocators/linear_arena_allocator.hpp>

// Bump allocation from 64 KiB arenas; rewind() recycles all of it at once (e.g. once per frame)
ouly::linear_arena_allocator<> frame(64 * 1024);
auto* positions = ouly::allocate<float>(frame, 1024 * sizeof(float));
frame.rewind();

// Offset suballocation for memory you do not address directly, such as one large GPU buffer
ouly::coalescing_allocator buffer(256U * 1024U * 1024U);
auto                       id     = buffer.allocate(4096, 256); // size, power-of-two alignment
auto                       offset = buffer.get_offset(id);
buffer.deallocate(id); // O(1); neighbouring free ranges merge
```

Also available: `linear_stack_allocator` with rewind points, `pool_allocator` and `object_pool`,
the thread-safe `ts_shared_linear_allocator` and `ts_thread_local_allocator`,
`virtual_allocator`, memory-mapped files, STL allocator adaptors, and
`coalescing_arena_allocator`, which spreads suballocations over many arenas (GPU heaps, for
example), keeps one emptied arena per memory class for reuse, and defragments by evacuating whole
arenas under a copy budget. See the
[memory management tutorial](https://obhi-d.github.io/ouly/tutorials/memory_management.html).

### Containers

```cpp
#include <ouly/containers/small_vector.hpp>
#include <ouly/containers/soavector.hpp>

ouly::small_vector<int, 8> ids; // up to 8 ints live inside the object
ids.push_back(1);
ids.push_back(2);
bool inline_storage = ids.is_inlined(); // true until it outgrows 8 elements or you reserve() more

// Structure of arrays: every member of vertex is stored in its own contiguous array
struct vertex { float x, y, z; };
ouly::soavector<vertex> vertices;
vertices.emplace_back(1.0F, 2.0F, 3.0F);
vertices.emplace_back(4.0F, 5.0F, 6.0F);
float* xs = vertices.data<0>(); // all x components, contiguous
```

Like `std::vector`, a `small_vector` that has moved to the heap keeps its buffer through `clear()`,
`pop_back()` and `erase()`; `shrink_to_fit()` moves the elements back inline when they fit. The
module also has sparse vectors and tables with stable handles, block vectors, an intrusive list, a
lock-free MPMC queue and a type-erased blackboard. See the
[containers tutorial](https://obhi-d.github.io/ouly/tutorials/containers_tutorial.html).

### Entity component system

```cpp
#include <ouly/ecs/collection.hpp>
#include <ouly/ecs/components.hpp>
#include <ouly/ecs/registry.hpp>

struct velocity { float dx = 0, dy = 0; };

ouly::ecs::registry<>                      registry;   // hands out and recycles entity ids
ouly::ecs::components<velocity>            velocities; // velocity per entity
ouly::ecs::collection<ouly::ecs::entity<>> moving;     // a subset of entities

auto player = registry.emplace();
velocities.emplace_at(player, 1.0F, 0.0F);
moving.emplace(player);

velocities[player].dy = 2.0F;

// Visit the components of the entities in the collection
moving.for_each(velocities, [](ouly::ecs::entity<>, velocity& v) { v.dx *= 0.5F; });

velocities.erase(player);
moving.erase(player);
registry.erase(player); // the id is recycled by a later emplace()
```

Use `rxregistry`/`rxentity` to carry revision bits that detect stale handles after an id is
recycled. Component storage can be dense with a sparse index (the default), directly mapped, or
backed by a sparse vector. See the [ECS tutorial](https://obhi-d.github.io/ouly/tutorials/ecs_tutorial.html).

### Serialization

```cpp
#include <ouly/serializers/binary_stream.hpp>
#include <ouly/serializers/lite_yml.hpp>
#include <ouly/serializers/serializers.hpp>

struct settings
{
  std::string      name;
  int              width  = 0;
  int              height = 0;
  std::vector<int> lods;
};

settings in{.name = "main", .width = 1280, .height = 720, .lods = {0, 1, 2}};

// YAML: aggregates are reflected automatically
std::string yaml = ouly::yml::to_string(in);
settings    from_yaml;
ouly::yml::from_string(from_yaml, yaml);

// Binary, little endian by default
ouly::binary_output_stream out;
ouly::write(out, in);

ouly::binary_input_stream bin(out.get_string());
settings                  from_binary;
ouly::read(bin, from_binary);
```

Types that are not aggregates opt in with a static `reflect()` built from `ouly::bind`, or with an
`ouly::convert<T>` specialization. The same reflection data drives any structured format that
models `StructuredInputStream`/`StructuredOutputStream`, such as a JSON library. See the
[serialization tutorial](https://obhi-d.github.io/ouly/tutorials/serialization_tutorial.html).

### Utilities and DSL

```cpp
#include <ouly/dsl/microexpr.hpp>
#include <ouly/utility/program_args.hpp>

ouly::program_args<> args;
args.parse_args(argc, argv);

int              threads = 4;
std::string_view config;
args.sink(threads, "threads", "t", "worker thread count");
args.sink(config, "config", "c", "configuration file");

ouly::microexpr defines(
 [](std::string_view name) -> std::optional<int>
 {
   if (name == "USE_VULKAN")
   {
     return 1;
   }
   return std::nullopt; // not defined
 });
bool vulkan_only = defines.evaluate("$(USE_VULKAN) && !$(USE_D3D12)"); // true
```

The utility module also has delegates, `zip`/`projected_view`/`subrange` views, `intrusive_ptr`,
`tagged_ptr` and `tagged_int`, wyhash/komihash, `string_literal`, `from_chars`/`to_chars`
wrappers, optional types, `uint128_t`, and an event-driven YAML parser (`ouly/dsl/lite_yml.hpp`).
See the [utilities tutorial](https://obhi-d.github.io/ouly/tutorials/utilities_tutorial.html).

## Building, Testing and Benchmarks

| Preset                                 | What it configures                                                       |
| -------------------------------------- | ------------------------------------------------------------------------ |
| `linux-default`, `macos-default`       | Debug build with unit tests                                              |
| `linux-release`, `macos-release`       | Optimized build (`RelWithDebInfo` on Linux, `Release` on macOS) with unit tests |
| `windows-test`, `windows-release`      | Debug / `RelWithDebInfo` builds with unit tests                          |
| `linux-clang-tidy`                     | Debug Clang build that runs clang-tidy while compiling                   |

```bash
cmake --preset=linux-default
cmake --build build/linux-default
ctest --test-dir build/linux-default --output-on-failure
```

Without presets, pass `-DOULY_BUILD_TESTS=ON`. On GCC and Clang the unit tests are always built
with AddressSanitizer. Benchmarks against Intel oneTBB live in `unit_tests/bench_*.cpp` and are
built with `-DOULY_BUILD_BENCHMARKS=ON` (CMake fetches nanobench and oneTBB). Other options:
`OULY_ENABLE_CLANG_TIDY`, `OULY_REC_STATS` (allocator statistics) and
`OULY_SCHEDULER_PMR_INTERNAL_ALLOCATOR`; see
[getting started](https://obhi-d.github.io/ouly/getting_started.html) for the full list.

The documentation is built with Doxygen, Sphinx and Breathe:

```bash
pip install -r docs/requirements.txt
doxygen Doxyfile
sphinx-build -b html docs/source docs/build/html
```

## Debug Support

OULY ships debugging aids in the `debug_helpers/` directory:

- `containers.natvis`: Visual Studio visualizers for OULY containers (added to the target automatically with MSVC)
- `pretty_printer_lldb.py`: LLDB pretty printers
- `pretty_printer.py`: GDB pretty printers

Internal assertions use the `OULY_ASSERT` macro, which defaults to `assert` from `<cassert>`.
Define it before including OULY headers, or put it in an `ouly_user_defines.hpp` header on the
include path; `ouly/utility/user_config.hpp` picks that header up automatically.

## Contributing

Contributions are welcome. [CONTRIBUTING.md](CONTRIBUTING.md) covers the build, the tests, code
style, clang-tidy and what CI checks. Bugs and feature requests go to
[GitHub Issues](https://github.com/obhi-d/ouly/issues).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
