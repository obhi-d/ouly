# OULY Project Guide for AI Coding Agents

## Project Overview
OULY is a modern C++20 high-performance utility library providing memory allocators, containers, ECS (Entity Component System), task scheduler, and serialization components for performance-critical applications.

## Architecture & Key Components

### Memory Management (`include/ouly/allocators/`)
- **Linear allocators**: Sequential allocation with LIFO deallocation only
- **Arena allocators**: Block-based allocation with configurable strategies  
- **Pool allocators**: Fixed-size block allocation for specific object types
- **Thread-safe variants**: Prefixed with `ts_` (e.g., `ts_shared_linear_allocator`)
- **Configuration pattern**: Template-based config classes control behavior (alignment, statistics, underlying allocator)

### Task Scheduler (`include/ouly/scheduler/`)
- **Two implementations**: `scheduler_v1.hpp` (legacy) and `scheduler_v2.hpp` (active)
- **Workgroup architecture**: Tasks organized into workgroups with Chase-Lev work-stealing queues
- **Key types**: `workgroup_id`, `worker_id`, `task_context` for thread coordination
- **Submission patterns**: `scheduler.submit(current, group, callable)` and `async()` helper
- **Coroutine support**: Submit `co_task<T>` objects directly to scheduler

### Entity Component System (`include/ouly/ecs/`)
- **Entity management**: `registry<>` creates/destroys entities with revision tracking
- **Component storage**: `components<T>` provides sparse/dense storage strategies
- **Configuration-driven**: `cfg::use_sparse<>`, `cfg::use_direct_mapping<>` control storage
- **Collections**: Group entities for efficient batch processing
- **Thread safety**: `rxregistry<>` provides revision-based safety

### Containers (`include/ouly/containers/`)
- **STL-like interface** with custom allocator support
- **Performance focus**: `small_vector<T, N>` uses stack storage, `dynamic_array<T>` optimized for growth
- **Specialized containers**: `sparse_vector<>`, `intrusive_list<>`, `soavector<>` (Structure of Arrays)

## Development Workflows

### Build System
```bash
# Use CMake presets for different configurations
cmake --preset=macos-default  # Debug build with tests
cmake --preset=macos-release  # Release build
cmake --build build/macos-default
```

### Testing
```bash
# Build and run tests
cmake -B build -DOULY_BUILD_TESTS=ON
cmake --build build
cd build/unit_tests && ctest
```

### Performance Benchmarks
```bash
# Run local benchmarks
./scripts/run_benchmarks.sh
# Analyze results
./scripts/analyze_performance.py
```

## Coding Patterns & Conventions

### Template Configuration Pattern
Most components use template-based configuration:
```cpp
// Allocator with custom config
using MyConfig = ouly::config<ouly::alignment<16>, ouly::debug_mode>;
ouly::linear_allocator<MyConfig> alloc(1024);

// ECS storage configuration  
ouly::ecs::components<Position, ouly::ecs::entity<>, ouly::cfg::use_sparse<>> positions;
```

### Error Handling
- **Assertions**: `OULY_ASSERT()` for debug-time checks
- **No exceptions**: Library avoids exceptions for performance
- **Return codes**: Functions return success/failure through return values or output parameters

### Memory Alignment
- **Explicit alignment**: `ouly::alignment<N>{}` parameter for allocations
- **Platform awareness**: `OULY_PACK_TAGGED_POINTER` macro for pointer optimization

### Task Submission Patterns
```cpp
// From within a task (preferred)
ouly::async(current_context, target_workgroup, [](auto& ctx) { /* work */ });

// Direct scheduler submission
scheduler.submit(current_context, workgroup, callable, args...);
```

### Naming Conventions
- **Types**: `snake_case` for classes, `CamelCase` for templates
- **Files**: `snake_case.hpp` for headers
- **Namespaces**: `ouly::ecs`, `ouly::detail` (internal), `ouly::inline v2` (versioning)
- **Thread safety**: `ts_` prefix for thread-safe variants

## Integration Points

### Cross-Component Usage
- **Allocators + Containers**: Containers accept custom allocators via template parameters
- **ECS + Scheduler**: Entity processing often uses `parallel_for()` with workgroups
- **Serialization + ECS**: Components can be serialized using binary/YAML serializers

### External Dependencies
- **Testing**: Catch2 for unit tests, nanobench for performance tests
- **Build**: CMake 3.20+, Ninja generator preferred
- **Documentation**: Sphinx + Breathe for API docs from Doxygen

## Performance Considerations
- **Zero-cost abstractions**: Template-heavy design compiled away
- **Cache-friendly**: Structure of Arrays (SoA) patterns in containers
- **Lock-free**: Scheduler uses atomic operations and work-stealing queues
- **NUMA awareness**: Scheduler supports worker thread affinity

## Testing Strategy
- **Unit tests**: `unit_tests/` with component-specific test files
- **Benchmarks**: Performance comparison with industry standards (TBB)
- **Memory safety**: AddressSanitizer integration via `ASAN_ENABLED` option
