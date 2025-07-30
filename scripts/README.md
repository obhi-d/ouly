# OULY Performance Tracking System

This document describes the automated performance tracking system for the OULY project.

## Overview

The performance tracking system automatically runs benchmarks on every push to `main` and `develop` branches, stores results in a dedicated `perfo` branch, and generates visualizations and reports.

## System Components

### 1. Modified Benchmark Code (`bench_scheduler_comparison.cpp`)

- **New naming format**: Results are saved as `<compiler-id>-<commit-hash>-<build-number>-<test_id>.json`
- **Test ID output**: Each benchmark outputs `TEST_ID: <test_id>` to stdout for CI tracking
- **Environment integration**: Reads `GITHUB_SHA` and `GITHUB_RUN_NUMBER` from environment

### 2. CI Integration (`.github/workflows/ci.yml`)

- **Benchmark execution**: Runs benchmarks after tests on pushes to main/develop
- **Artifact upload**: Stores benchmark results as workflow artifacts
- **Performance tracking job**: Processes results and commits to perfo branch

### 3. Dedicated Performance Workflow (`.github/workflows/performance.yml`)

- **Scheduled runs**: Daily comprehensive benchmarks at 2 AM UTC
- **Extended testing**: Longer, more thorough benchmark suites
- **Manual trigger**: Can be run on-demand with different benchmark types

### 4. Python Scripts (`scripts/`)

#### `visualize_performance.py`
- Generates SVG performance plots over time
- Creates comprehensive PERFORMANCE.md report
- Supports multiple compilers and test categories

#### `cleanup_old_results.py`
- Maintains perfo branch size
- Keeps only the last 20 builds by default
- Preserves files with unexpected naming formats

#### `test_performance_tracking.sh`
- Local testing script for the performance system
- Builds project, runs benchmarks, and tests visualization

## Performance-Tracking Branch Structure

```
perfo/
├── results/
│   ├── <compiler>-<commit>-<build>-<test_id>.json
│   ├── <compiler>-<commit>-<build>-<test_id>.txt
│   └── ...
├── scripts/
│   ├── visualize_performance.py
│   ├── cleanup_old_results.py
│   └── test_performance_tracking.sh
├── PERFORMANCE.md          # Auto-generated report with plots
├── *.svg                   # Performance trend plots
└── README.md              # This documentation
```

## Benchmark File Naming Convention

Format: `<compiler-id>-<commit-hash>-<build-number>-<test_id>.<extension>`

Examples:
- `gcc-14-a1b2c3d4-1234-scheduler_comparison.json`
- `clang-18-e5f6g7h8-1235-allocator_bench.txt`

Components:
- **compiler-id**: Compiler identifier (e.g., `gcc-14`, `clang-18`)
- **commit-hash**: First 8 characters of git commit SHA
- **build-number**: GitHub Actions run number
- **test_id**: Short identifier for the benchmark type
- **extension**: `.json` for machine-readable, `.txt` for human-readable

## Test IDs

Current test IDs used in benchmarks:
- `scheduler_comparison`: Comprehensive scheduler vs TBB benchmarks
- `allocator_bench`: Memory allocator performance tests (if available)

## Performance Visualization

### Generated Plots
- **Performance Trend**: Execution time over build numbers (SVG format)
- **Throughput Trend**: Operations per second over build numbers (SVG format)

### PERFORMANCE.md Report
- Latest benchmark results table
- Embedded SVG plots
- Historical data summary
- Compiler comparison

## Usage

### Local Testing
```bash
# Run local performance test
./scripts/test_performance_tracking.sh

# Generate visualization from existing results
python3 scripts/visualize_performance.py results/ -o output/ -v

# Clean up old results (dry run)
python3 scripts/cleanup_old_results.py results/ --dry-run
```

### CI Integration
The system runs automatically on:
- **Push to main/develop**: Quick benchmarks + performance tracking
- **Scheduled (daily)**: Extended benchmarks + comprehensive analysis
- **Manual trigger**: On-demand with selectable benchmark types

### Accessing Results
- **Performance tracking branch**: `git checkout perfo`
- **Latest report**: View `PERFORMANCE.md` in the perfo branch
- **Raw data**: Browse `results/` directory for JSON/TXT files
- **Workflow artifacts**: Download from GitHub Actions workflow runs

## Configuration

### Benchmark Retention
- **Default**: Keep last 20 builds
- **Modify**: Edit `cleanup_old_results.py` call in workflows
- **Manual cleanup**: Run cleanup script with `--keep N` parameter

### Visualization Options
- **Output format**: SVG (configurable in `visualize_performance.py`)
- **Plot types**: Time series, throughput trends
- **Styling**: Matplotlib default style (no external dependencies)

### Adding New Benchmarks
1. Add new test executable to CMake build
2. Update CI workflow to run the new benchmark
3. Ensure benchmark outputs appropriate `TEST_ID` and uses new naming format
4. Update this documentation with new test IDs

## Troubleshooting

### No Benchmark Results
- Check if benchmark executable was built successfully
- Verify environment variables (`GITHUB_SHA`, `GITHUB_RUN_NUMBER`) are set
- Look for benchmark output in CI logs

### Visualization Failures
- Ensure Python dependencies are installed: `matplotlib pandas numpy`
- Check that JSON files are valid and contain expected structure
- Verify `results/` directory contains properly named files

### Performance-Tracking Branch Issues
- Branch is created automatically if it doesn't exist
- If corrupted, delete and let CI recreate it
- Ensure proper Git configuration in CI (user.name, user.email)

## Future Enhancements

- **Regression detection**: Automatic alerts for performance degradation
- **Multi-platform tracking**: Separate results by OS/architecture
- **Interactive visualizations**: Web-based dashboard for results
- **Benchmark comparisons**: Side-by-side analysis of different implementations
