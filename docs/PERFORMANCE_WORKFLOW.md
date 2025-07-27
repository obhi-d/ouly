# Performance Workflow Integration Guide

## Overview

The OULY project now has a comprehensive performance tracking system that integrates between the **main branch** (source code + workflow) and the **performance-tracking branch** (results + reports).

## How It Works

### 1. Workflow Location
- **Workflow file**: `.github/workflows/performance.yml` is in the **main branch**
- **Execution**: GitHub Actions runs the workflow when code is pushed to main/develop
- **Results storage**: Performance data is committed to the **performance-tracking branch**

### 2. Workflow Triggers

The performance workflow runs on:
- **Push to main/develop branches** - Automatic performance tracking
- **Pull requests to main** - Performance impact analysis  
- **Schedule (daily at 2 AM UTC)** - Regular performance monitoring
- **Manual dispatch** - On-demand benchmarking with options:
  - `all` - Run all benchmarks (default)
  - `scheduler` - Run only scheduler benchmarks
  - `allocator` - Run only allocator benchmarks
  - `comparison` - Run scheduler vs TBB comparison

### 3. Benchmark Execution

The workflow runs multiple benchmark types:
- **Scheduler Comparison**: `ouly-scheduler-comparison-bench`
  - Task submission performance
  - Parallel for operations
  - Matrix operations
  - Mixed workloads  
  - Task throughput
  - Nested parallel workloads
  - V1 vs V2 vs TBB comparison
  
- **Allocator Performance**: `ouly-performance-bench`
  - Thread-safe shared linear allocator
  - Thread-safe thread local allocator
  - Coalescing arena allocator

### 4. Multi-Compiler Support

Benchmarks run on both:
- **GCC 14** - Primary development compiler
- **Clang 19** - Alternative compiler for comparison

### 5. Results Processing

1. **Collection**: Benchmark JSON/TXT files are generated
2. **Organization**: Results are organized by timestamp/compiler/branch/commit
3. **Storage**: Files are committed to performance-tracking branch
4. **Reporting**: Comprehensive markdown reports are generated

## Directory Structure

### Main Branch
```
main/
├── .github/workflows/performance.yml  # Workflow definition
├── scripts/
│   ├── generate_performance_report.py # Report generator
│   ├── process_performance_results.sh # Result processor
│   └── ...                           # Other analysis tools
├── src/                              # Source code
├── unit_tests/                       # Benchmark executables
└── ...
```

### Performance-Tracking Branch
```
performance-tracking/
├── results/
│   ├── index.md                      # Latest results summary
│   └── YYYY-MM-DD_HH-MM/             # Results by timestamp
│       └── compiler/                 # Results by compiler
│           └── branch/               # Results by branch
│               └── commit/           # Results by commit
│                   ├── *.json        # Raw benchmark data
│                   └── *.txt         # Human-readable results
├── scripts/                          # Copy of processing scripts
├── PERFORMANCE_REPORT.md             # Comprehensive report
└── README.md                         # Documentation
```

## Workflow Jobs

### 1. `benchmark`
- Runs on Ubuntu with GCC 14 and Clang 19
- Builds project in Release mode
- Executes benchmark suite
- Uploads results as artifacts

### 2. `performance-tracking`
- Downloads benchmark artifacts
- Switches to performance-tracking branch
- Processes and organizes results
- Generates comprehensive reports
- Commits everything to performance-tracking branch

### 3. `build-verification` 
- Verifies builds work across platforms
- Continues even if some platforms fail
- Provides build status summary

### 4. `workflow-summary`
- Reports overall workflow status
- Summarizes any issues or failures

## Key Features

### Automatic Performance Regression Detection
- Results are automatically committed after each push
- Historical data enables trend analysis
- Performance impacts are tracked per commit

### Manual Benchmark Control
- Workflow dispatch allows selective benchmark runs
- Useful for testing specific performance improvements
- Saves CI time when only testing certain components

### Multi-Platform Verification
- Primary benchmarks run on controlled Linux environment
- Additional verification on macOS/Windows (with fallbacks)

### Comprehensive Reporting
- JSON data for programmatic analysis
- Markdown reports for human review
- Historical trend tracking

## Usage Examples

### Trigger Manual Benchmark
1. Go to GitHub Actions in your repository
2. Select "Performance Benchmarks" workflow
3. Click "Run workflow"
4. Choose benchmark type (all/scheduler/allocator/comparison)
5. Click "Run workflow"

### View Latest Results
- **Quick Overview**: Check [performance-tracking/results/index.md](https://github.com/your-repo/tree/performance-tracking/results/index.md)
- **Detailed Report**: View [performance-tracking/PERFORMANCE_REPORT.md](https://github.com/your-repo/blob/performance-tracking/PERFORMANCE_REPORT.md)
- **Raw Data**: Browse the results directory structure

### Performance Impact Analysis
- Pull requests automatically trigger benchmarks
- Results are uploaded as artifacts
- Compare with main branch results to assess impact

## Integration Benefits

1. **Separation of Concerns**:
   - Source code stays in main branch
   - Performance data isolated in tracking branch
   - Clean repository organization

2. **Continuous Monitoring**:
   - Automatic execution on code changes
   - Historical performance tracking
   - Regression detection

3. **Flexible Execution**:
   - Multiple trigger types
   - Selective benchmark running
   - Multi-compiler support

4. **Comprehensive Analysis**:
   - Detailed JSON data for analysis
   - Human-readable reports
   - Trend visualization capabilities

## Troubleshooting

### Workflow Fails to Find Benchmark Executables
- Check CMake configuration in main branch
- Ensure `OULY_BUILD_TESTS=ON` is set
- Verify benchmark targets are built correctly

### Performance-Tracking Branch Access Issues
- Ensure GitHub token has sufficient permissions
- Check branch protection rules
- Verify workflow has write access

### Missing Performance Scripts
- Scripts are copied from main branch to performance-tracking
- If missing, they're automatically copied during workflow execution
- Can be manually synchronized if needed

### Results Not Appearing
- Check workflow execution logs
- Verify benchmark completion
- Ensure performance-tracking branch exists

This integration provides a robust, automated performance tracking system that scales with your development workflow while maintaining clean separation between source code and performance data.
