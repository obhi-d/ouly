# Ouly Performance Benchmarking

This directory contains comprehensive performance benchmarks for the Ouly library's scheduler and thread-safe allocators, along with CI/CD integration for performance tracking.

## 🎯 Benchmarked Components

### Thread-Safe Allocators
- **`ts_shared_linear_allocator`**: Thread-safe linear allocator with shared arena management
- **`ts_thread_local_allocator`**: Thread-safe allocator with per-thread arena optimization  
- **`coalescing_arena_allocator`**: Arena allocator with memory coalescing capabilities

### Scheduler
- **Task submission**: Performance of submitting tasks to the scheduler
- **Parallel for**: Performance of parallel for loop execution
- **Work stealing**: Performance of work stealing between worker threads
- **Multi-workgroup**: Performance with multiple workgroups

## 🚀 Quick Start

### Local Benchmarking

Run benchmarks locally using the provided script:

```bash
# Run with default settings (Release build, system compiler)
./scripts/run_benchmarks.sh

# Customize build settings
BUILD_TYPE=Release CXX=clang++ ./scripts/run_benchmarks.sh

# Results will be saved to benchmark_results/ directory
```

### Manual Build and Run

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DOULY_BUILD_TESTS=ON
cmake --build build --config Release -j$(nproc)

# Run benchmarks
cd build/unit_tests
./ouly-performance-bench [output_file.json]
```

## 📊 CI Performance Tracking

The project includes automated performance tracking through GitHub Actions:

### Workflow Features
- **Automated benchmarks**: Runs on main branch pushes and daily schedule
- **Multi-compiler testing**: Tests with GCC 11 and Clang 14
- **Performance history**: Stores results in `performance-tracking` branch
- **Cross-platform verification**: Builds on Linux, macOS, and Windows
- **PR integration**: Comments performance info on pull requests

### Workflow Files
- `.github/workflows/performance.yml`: Main performance tracking workflow

### Performance Data Storage
Results are automatically stored in the `performance-tracking` branch:
```
results/
├── 2024-01-15_14-30-22_abc12345/
│   ├── benchmark_results_gcc-11_Release.json
│   └── benchmark_results_clang-14_Release.json
└── index.md
```

## 📈 Analysis Tools

### Performance Analysis Script
Analyze historical performance data:

```bash
# Analyze results from performance-tracking branch
git checkout performance-tracking
python3 scripts/analyze_performance.py results/ -o performance_report.md

# View verbose analysis
python3 scripts/analyze_performance.py results/ -v
```

### Benchmark Output Format
Results are saved in JSON format:
```json
{
  "timestamp": 1705328622,
  "status": "completed", 
  "note": "Detailed results available in nanobench output"
}
```

Detailed performance metrics are displayed in the console output using nanobench.

## 🔧 Benchmark Implementation

### Architecture
- **`bench_performance.cpp`**: Main benchmark implementation
- **nanobench**: High-precision benchmarking library
- **Multi-threaded tests**: Realistic concurrent usage scenarios
- **Memory patterns**: Various allocation patterns and sizes

### Key Benchmark Scenarios

#### Allocator Benchmarks
- Single-threaded allocation/deallocation
- Multi-threaded concurrent allocation
- Memory reset performance
- Fragmentation and coalescing (coalescing allocator)

#### Scheduler Benchmarks  
- Task submission throughput
- Parallel for loop performance
- Work stealing efficiency
- Multi-workgroup coordination

### Performance Metrics
For each benchmark, the following metrics are captured:
- **Median time**: Most representative performance
- **Min/Max time**: Performance range
- **Mean time**: Average performance
- **Relative error**: Measurement precision
- **Iterations**: Number of benchmark runs

## 🎛️ Configuration

### Build Configuration
The benchmarks require:
- C++20 compiler support
- Release build for accurate performance measurement
- CMake 3.15+
- nanobench (automatically fetched)

### Benchmark Tuning
Key parameters in `bench_performance.cpp`:
- `warmup(10)`: Warmup iterations before measurement
- `epochIterations(100)`: Measurement iterations per benchmark
- Thread counts and allocation sizes for realistic scenarios

## 📋 Usage Guidelines

### When to Run Benchmarks
- Before merging performance-critical changes
- When modifying allocator or scheduler code
- Regularly to establish performance baselines
- After major compiler or dependency updates

### Interpreting Results
- **Lower times are better** for all benchmarks
- **Median time** is the most reliable metric
- **Relative error < 5%** indicates good measurement precision
- Compare results across different compilers and build types

### Performance Regression Detection
- Monitor trends in the `performance-tracking` branch
- Compare PR benchmarks with main branch baselines
- Look for significant increases in median times
- Consider both absolute performance and relative changes

## 🔮 Future Enhancements

Planned improvements to the benchmarking system:
- Automatic performance regression detection in CI
- Performance comparison visualizations
- Memory usage tracking alongside timing
- Integration with performance monitoring services
- Custom benchmark scenarios for specific use cases

## 🤝 Contributing

When adding new benchmarks:
1. Follow the existing benchmark structure in `bench_performance.cpp`
2. Use appropriate warmup and iteration counts
3. Include both single-threaded and multi-threaded scenarios
4. Update this documentation with new benchmark descriptions
5. Ensure benchmarks are deterministic and reproducible

---

**Note**: Performance benchmarks should always be run in Release mode for accurate results. Debug builds will show significantly different performance characteristics.
