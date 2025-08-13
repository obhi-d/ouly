# Contributing to OULY

<div align="center">

**Thank you for your interest in contributing to OULY!**

*We welcome contributions that help make OULY faster, more reliable, and easier to use.*

</div>

This guide will help you understand our development workflow, coding standards, and how to submit high-quality contributions to the OULY library.

## 📋 Table of Contents

- [Getting Started](#-getting-started)
- [Development Environment](#-development-environment)
- [Coding Standards](#-coding-standards)
- [Testing Guidelines](#-testing-guidelines)
- [Performance Considerations](#-performance-considerations)
- [Submission Process](#-submission-process)
- [Documentation Standards](#-documentation-standards)
- [Community Guidelines](#-community-guidelines)

## 🚀 Getting Started

### Prerequisites

- **Compiler**: GCC 14+, Clang 19+, or MSVC v17+
- **Build System**: CMake 3.20+
- **Git**: For version control
- **Platform**: Linux, macOS, or Windows

### Fork and Clone

```bash
# Fork the repository on GitHub, then clone your fork
git clone https://github.com/your-username/ouly.git
cd ouly

# Add upstream remote
git remote add upstream https://github.com/obhi-d/ouly.git

# Create a feature branch
git checkout -b feature/your-feature-name
```

### Initial Build

```bash
# Configure for development (includes tests)
cmake --preset=linux-default  # or macos-default/windows-default

# Build and run tests
cmake --build build/linux-default
cd build/linux-default && ctest
```

## 🛠 Development Environment

### Build Configurations

OULY provides several CMake presets for different development scenarios:

```bash
# Development builds (with tests and debug info)
cmake --preset=linux-default
cmake --preset=macos-default
cmake --preset=windows-default

# Release builds (optimized)
cmake --preset=linux-release
cmake --preset=macos-release
cmake --preset=windows-release
```

### CMake Options

```cmake
# Key CMake options for development
option(OULY_BUILD_TESTS "Build unit tests" ON)        # Enable for development
option(ASAN_ENABLED "Enable AddressSanitizer" OFF)    # Enable for debugging
option(OULY_REC_STATS "Enable statistics" OFF)        # Enable for profiling
option(OULY_TEST_COVERAGE "Build test coverage" OFF)  # Enable for coverage
```

### Development Build Example

```bash
# Development build with all debugging features
cmake -B build-dev \
    -DCMAKE_BUILD_TYPE=Debug \
    -DOULY_BUILD_TESTS=ON \
    -DASAN_ENABLED=ON \
    -DOULY_REC_STATS=ON \
    -DOULY_TEST_COVERAGE=ON

cmake --build build-dev
cd build-dev && ctest --verbose
```

## 📝 Coding Standards

### File Organization

```
include/ouly/
├── allocators/           # Memory allocation components
├── containers/           # Data structures
├── ecs/                 # Entity Component System
├── scheduler/           # Task scheduling
├── utility/             # General utilities
└── core/                # Core functionality

src/ouly/                # Implementation files (.cpp)
unit_tests/              # Test files
docs/                    # Documentation
```

### Header File Structure

Every header file must follow this structure:

```cpp
// SPDX-License-Identifier: MIT

#pragma once

#include "ouly/core/required_headers.hpp"
#include <standard_headers>

namespace ouly
{

/**
 * @brief Brief description of the class/component
 *
 * Detailed description explaining:
 * - Purpose and use cases
 * - Key features and limitations
 * - Thread safety guarantees
 * - Performance characteristics
 *
 * @tparam Config Configuration template parameter (if applicable)
 *
 * ## Usage Example:
 *
 * ```cpp
 * // Comprehensive example showing typical usage
 * component<config<alignment<16>>> my_component(constructor_args);
 * 
 * // Demonstrate key operations
 * my_component.important_operation();
 * ```
 *
 * @note Important usage notes
 * @warning Critical warnings about limitations or requirements
 */
template <typename Config = ouly::config<>>
class component_name
{
public:
    // Public types, constants, and interface
private:
    // Private implementation details
};

} // namespace ouly
```

### Naming Conventions

#### Files and Directories
- **Headers**: `snake_case.hpp`
- **Sources**: `snake_case.cpp`
- **Tests**: `component_name_tests.cpp`
- **Benchmarks**: `bench_component_name.cpp`

#### Code Elements
- **Classes/Structs**: `snake_case`
- **Template Parameters**: `PascalCase`
- **Functions/Methods**: `snake_case`
- **Variables**: `snake_case`
- **Constants**: `UPPER_SNAKE_CASE`
- **Macros**: `OULY_UPPER_SNAKE_CASE`

#### Thread Safety Prefixes
- **Thread-safe variants**: `ts_` prefix (e.g., `ts_linear_allocator`)
- **Lock-free structures**: Document clearly in class documentation

### Code Style Guidelines

#### Templates and Configuration

```cpp
// Use configuration pattern for customizable components
template <typename Config = ouly::config<>>
class my_allocator : ouly::detail::statistics<my_allocator_tag, Config>
{
public:
    using tag = my_allocator_tag;
    using statistics = ouly::detail::statistics<my_allocator_tag, Config>;
    using size_type = typename Config::size_type;
    
    // Constructor with perfect forwarding for config
    template <typename... Args>
    explicit my_allocator(size_type capacity, Args&&... config_args)
        : statistics(std::forward<Args>(config_args)...)
        , capacity_(capacity)
    {
        // Implementation
    }
};
```

#### Error Handling

```cpp
// Use assertions for debug-time checks
void critical_function(void* ptr, size_t size)
{
    OULY_ASSERT(ptr != nullptr, "Pointer cannot be null");
    OULY_ASSERT(size > 0, "Size must be positive");
    OULY_ASSERT(is_aligned(ptr, alignment_), "Pointer must be aligned");
    
    // Function implementation
}

// Return error codes for runtime errors
enum class allocation_result
{
    success,
    insufficient_memory,
    invalid_alignment,
    invalid_size
};

auto allocate(size_t size, alignment align) -> allocation_result
{
    if (size == 0) return allocation_result::invalid_size;
    if (!is_power_of_two(align.value)) return allocation_result::invalid_alignment;
    
    // Implementation
    return allocation_result::success;
}
```

#### Performance-Critical Code

```cpp
// Use OULY_LIKELY/OULY_UNLIKELY for branch prediction
if (OULY_LIKELY(fast_path_condition))
{
    // Common case - optimize for this path
    return fast_implementation();
}
else
{
    // Rare case - less optimized is acceptable
    return slow_fallback();
}

// Prefer constexpr for compile-time computation
template <size_t Alignment>
constexpr bool is_valid_alignment() noexcept
{
    return Alignment > 0 && (Alignment & (Alignment - 1)) == 0;
}

// Use noexcept where appropriate
auto get_size() const noexcept -> size_type { return size_; }
```

### Memory Management

```cpp
// Use RAII consistently
class resource_manager
{
public:
    explicit resource_manager(size_t capacity)
        : data_(allocate_memory(capacity))
        , capacity_(capacity)
    {
    }
    
    ~resource_manager()
    {
        if (data_) {
            deallocate_memory(data_);
        }
    }
    
    // Move-only semantics for expensive resources
    resource_manager(resource_manager const&) = delete;
    resource_manager& operator=(resource_manager const&) = delete;
    
    resource_manager(resource_manager&& other) noexcept
        : data_(std::exchange(other.data_, nullptr))
        , capacity_(std::exchange(other.capacity_, 0))
    {
    }
    
    resource_manager& operator=(resource_manager&& other) noexcept
    {
        if (this != &other) {
            if (data_) deallocate_memory(data_);
            data_ = std::exchange(other.data_, nullptr);
            capacity_ = std::exchange(other.capacity_, 0);
        }
        return *this;
    }
};
```

## 🧪 Testing Guidelines

### Test Organization

Each component should have comprehensive tests covering:

1. **Basic Operations**: Constructor, destructor, basic methods
2. **Edge Cases**: Empty containers, null pointers, zero sizes
3. **Error Conditions**: Invalid parameters, out-of-memory scenarios
4. **Thread Safety**: Concurrent access patterns (if applicable)
5. **Performance**: Benchmarks against industry standards

### Test File Structure

```cpp
#include "ouly/component/component_name.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

using namespace ouly;

TEST_CASE("component_name basic operations", "[component_name]")
{
    SECTION("construction and destruction")
    {
        // Test basic construction
        component_name<> comp(constructor_args);
        REQUIRE(comp.is_valid());
        
        // Test various configurations
        using CustomConfig = config<alignment<32>, debug_mode>;
        component_name<CustomConfig> custom_comp(args);
        REQUIRE(custom_comp.get_alignment() == 32);
    }
    
    SECTION("basic operations")
    {
        component_name<> comp(args);
        
        // Test primary functionality
        auto result = comp.primary_operation();
        REQUIRE(result == expected_value);
        
        // Test state changes
        comp.modify_state();
        REQUIRE(comp.get_state() == new_expected_state);
    }
}

TEST_CASE("component_name edge cases", "[component_name]")
{
    SECTION("empty/null inputs")
    {
        component_name<> comp;
        
        // Test behavior with empty inputs
        auto result = comp.operation_with_empty_input({});
        REQUIRE(result.empty());
    }
    
    SECTION("boundary conditions")
    {
        // Test minimum and maximum valid values
        // Test off-by-one scenarios
    }
}

TEST_CASE("component_name error handling", "[component_name]")
{
    SECTION("invalid parameters")
    {
        // Test error handling for invalid inputs
        component_name<> comp;
        
        // Verify proper error reporting
        auto result = comp.operation_with_invalid_input(invalid_value);
        REQUIRE(result == error_code::invalid_parameter);
    }
}

// Template tests for generic components
TEMPLATE_TEST_CASE("component_name with different types", "[component_name][template]",
                   int, float, double, std::string)
{
    using ComponentType = component_name<TestType>;
    ComponentType comp;
    
    // Test type-specific behavior
    TestType value = /* appropriate test value */;
    comp.template_operation(value);
    
    REQUIRE(comp.template get<TestType>() == value);
}
```

### Performance Testing

```cpp
// Benchmark tests should be in separate files (bench_component_name.cpp)
#include "ouly/component/component_name.hpp"
#include <nanobench.h>

void benchmark_component_performance()
{
    ankerl::nanobench::Bench bench;
    bench.title("Component Performance Comparison");
    bench.relative(true);
    
    // Setup test data
    constexpr size_t test_size = 10000;
    std::vector<test_data> data(test_size);
    
    // Benchmark OULY component
    ouly::component_name<> ouly_comp(test_size);
    bench.run("OULY component", [&]() {
        for (auto const& item : data) {
            ouly_comp.process(item);
        }
    });
    
    // Benchmark standard alternative
    standard_component std_comp(test_size);
    bench.run("Standard component", [&]() {
        for (auto const& item : data) {
            std_comp.process(item);
        }
    });
}
```

### Test Categories

Use consistent test tags for filtering:

- `[component_name]` - Basic component tests
- `[allocator]` - Memory allocator tests  
- `[container]` - Container tests
- `[ecs]` - Entity Component System tests
- `[scheduler]` - Task scheduler tests
- `[serialization]` - Serialization tests
- `[utility]` - Utility tests
- `[benchmark]` - Performance benchmarks
- `[stress]` - Stress tests
- `[thread_safety]` - Multi-threading tests

### Running Tests

```bash
# Run all tests
cd build && ctest

# Run specific categories
ctest -R "allocator"      # Run allocator tests
ctest -R "scheduler"      # Run scheduler tests
ctest -R "benchmark"      # Run benchmarks

# Run tests with specific patterns
ctest -R "flow_graph"     # Run flow graph related tests
ctest -R "stress"         # Run stress tests

# Run with verbose output
ctest --verbose

# Run tests in parallel
ctest -j$(nproc)
```

## ⚡ Performance Considerations

### Benchmarking Requirements

All performance-critical components must include benchmarks comparing against:

1. **STL equivalents** (where applicable)
2. **Industry standards** (e.g., Intel TBB for scheduling)
3. **Previous OULY versions** (for regression testing)

### Benchmark Structure

```cpp
// Use consistent benchmark reporting
class BenchmarkReporter
{
public:
    static void save_results(ankerl::nanobench::Bench& bench, 
                           std::string const& test_id,
                           std::string const& commit_hash = "",
                           std::string const& build_number = "")
    {
        // Save JSON results for CI tracking
        std::string json_filename = generate_filename(test_id, "json");
        std::ofstream json_file(json_filename);
        bench.render(ankerl::nanobench::templates::json(), json_file);
        
        // Save human-readable results
        std::string txt_filename = generate_filename(test_id, "txt");
        std::ofstream txt_file(txt_filename);
        bench.render(ankerl::nanobench::templates::mustache(
            "{{#result}}{{name}}: {{median(elapsed)}} seconds\n{{/result}}"
        ), txt_file);
        
        std::cout << "📊 Benchmark results: " << json_filename << std::endl;
    }
    
    static void print_system_info()
    {
        std::cout << "🖥️  System Information:" << std::endl;
        std::cout << "   Hardware Concurrency: " << std::thread::hardware_concurrency() << " threads" << std::endl;
        std::cout << "   Compiler: " << get_compiler_info() << std::endl;
        std::cout << std::endl;
    }
};
```

### Performance Guidelines

1. **Minimize Allocations**: Prefer stack allocation and object reuse
2. **Cache Efficiency**: Design data structures for spatial locality
3. **Branch Prediction**: Use `OULY_LIKELY`/`OULY_UNLIKELY` macros
4. **Template Instantiation**: Minimize template bloat
5. **Lock-Free When Possible**: Use atomics instead of mutexes where safe

## 📤 Submission Process

### Before Submitting

1. **Ensure all tests pass**:
   ```bash
   cd build && ctest
   ```

2. **Run performance benchmarks**:
   ```bash
   ./unit_tests/bench_performance
   ./unit_tests/bench_scheduler_comparison
   ```

3. **Check for memory issues** (if applicable):
   ```bash
   cmake -B build-asan -DASAN_ENABLED=ON
   cmake --build build-asan && cd build-asan && ctest
   ```

4. **Update documentation** if adding new features

### Pull Request Guidelines

#### Pull Request Title Format

```
[Component] Brief description of changes

Examples:
[allocators] Add support for custom alignment in pool allocator  
[scheduler] Optimize task stealing for NUMA systems
[ecs] Fix component iteration bug with sparse storage
[docs] Update flow_graph documentation with advanced examples
```

#### Pull Request Description Template

```markdown
## Summary
Brief description of what this PR accomplishes.

## Changes Made
- List of specific changes
- Include any API changes
- Note any breaking changes

## Testing
- [ ] Unit tests added/updated
- [ ] Performance benchmarks run
- [ ] Memory safety verified (if applicable)
- [ ] Documentation updated

## Performance Impact
- Benchmark results if performance-related
- Memory usage impact if applicable

## Breaking Changes
- List any breaking changes
- Migration guide for users

## Related Issues
Fixes #issue_number
Related to #issue_number
```

### Commit Message Format

```
[component] Brief description (50 chars max)

Detailed explanation of the change, including:
- Why the change was necessary
- What was changed
- Any side effects or considerations

Fixes #123
```

### Review Process

1. **Automated Checks**: CI will run tests and benchmarks
2. **Code Review**: Maintainers will review code quality and design
3. **Performance Review**: Performance-critical changes need benchmark verification
4. **Documentation Review**: Ensure adequate documentation for new features

## 📚 Documentation Standards

### API Documentation

All public APIs must include comprehensive Doxygen documentation:

```cpp
/**
 * @brief Brief one-line description
 *
 * Detailed description explaining:
 * - What the function/class does
 * - When to use it
 * - Thread safety guarantees
 * - Performance characteristics
 *
 * @param param_name Description of parameter, including valid ranges
 * @param config_args Perfect forwarded configuration arguments
 * 
 * @tparam Config Configuration type controlling behavior
 * @tparam Args Variadic template for configuration arguments
 *
 * @return Description of return value and possible error conditions
 *
 * @throw std::exception_type When this exception might be thrown
 *
 * @note Important usage notes
 * @warning Critical warnings about usage
 * @see related_function() for related functionality
 *
 * ## Example Usage:
 *
 * ```cpp
 * // Show typical usage pattern
 * my_class<config<alignment<16>>> obj(constructor_args);
 * auto result = obj.my_function(param_value);
 * if (result == expected_value) {
 *     // Handle success
 * }
 * ```
 *
 * ## Thread Safety:
 * - Safe for concurrent read access
 * - Not safe for concurrent write access
 * - Use ts_variant for thread-safe operations
 *
 * ## Performance Notes:
 * - O(1) time complexity for basic operations
 * - Optimized for cache locality
 * - Consider memory alignment for best performance
 */
template <typename Config = ouly::config<>, typename... Args>
[[nodiscard]] auto my_function(param_type param_name, Args&&... config_args) 
    -> result_type;
```

### README and Guide Updates

When adding new features, update relevant documentation:

1. **README.md**: Add to appropriate sections with examples
2. **Component docs**: Update detailed documentation
3. **Examples**: Add to examples directory if complex
4. **Migration guides**: For breaking changes

## 🤝 Community Guidelines

### Code of Conduct

- **Be respectful**: Treat all contributors with respect
- **Be constructive**: Provide helpful feedback and suggestions
- **Be collaborative**: Work together to improve OULY
- **Be patient**: Remember that everyone is learning

### Getting Help

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For questions and general discussion
- **Code Reviews**: For feedback on implementation approaches

### Recognition

Contributors will be recognized in:
- Release notes for significant contributions
- README acknowledgments
- Git commit history

## 🏆 Advanced Contributing

### Performance Optimization Guidelines

1. **Profile Before Optimizing**: Use tools like `perf`, `valgrind`, or platform profilers
2. **Benchmark Systematically**: Compare against baseline implementations
3. **Consider Cache Effects**: Design for cache-friendly data access patterns
4. **Optimize Hot Paths**: Focus on frequently executed code
5. **Document Trade-offs**: Explain performance decisions in comments

### Adding New Components

When adding major new components:

1. **Design Document**: Create RFC for significant features
2. **API Design**: Follow OULY patterns and conventions  
3. **Comprehensive Testing**: Include unit tests, benchmarks, and stress tests
4. **Documentation**: Full API docs plus usage guides
5. **Integration**: Ensure compatibility with existing components

---

<div align="center">

**Thank you for contributing to OULY!**

*Your contributions help make high-performance C++ development more accessible and efficient.*

[🐛 Report Issues](https://github.com/obhi-d/ouly/issues) | [💬 Join Discussions](https://github.com/obhi-d/ouly/discussions) | [📖 View Docs](https://ouly-optimized-utility-library.readthedocs.io/)

</div>
