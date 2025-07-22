# Workflow Status Guide

## 🔄 CI Workflows

### Main CI Workflow [![CI](https://github.com/obhi-d/ouly/actions/workflows/ci.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/ci.yml)

This is the primary build and test workflow that validates code quality and functionality:

- **Platforms**: Linux, Windows, macOS
- **Compilers**: GCC, Clang, MSVC
- **Tests**: Unit tests, integration tests, code coverage
- **Status**: Should always be green for the main branch

### Performance Benchmarks [![Performance](https://github.com/obhi-d/ouly/actions/workflows/performance.yml/badge.svg)](https://github.com/obhi-d/ouly/actions/workflows/performance.yml)

This workflow runs performance benchmarks and tracks performance over time:

- **Primary benchmarks**: Run on Linux with GCC 11 and Clang 14
- **Build verification**: Cross-platform build testing
- **Status interpretation**:
  - ✅ **Green**: All performance benchmarks completed successfully
  - ⚠️ **Yellow/Orange**: Some build verification failures (usually macOS due to compiler requirements)
  - ❌ **Red**: Actual benchmark execution failures

## 📝 Important Notes

### macOS Build Behavior

The performance workflow includes macOS in build verification, but **macOS failures are expected and normal**:

- **Why**: macOS runners may have Clang versions < 17, which is required for C++20 features used in Ouly
- **Impact**: These failures do **not** indicate issues with the codebase
- **Badge status**: The workflow is designed to continue and show success even with macOS build failures

### Performance Tracking

- **Historical data**: Stored in the `performance-tracking` branch
- **Benchmark results**: Available as workflow artifacts
- **Trend analysis**: Use `scripts/analyze_performance.py` for historical analysis

### When to Be Concerned

🚨 **Red Performance Badge**: Only worry if:
- The actual benchmarking steps fail (not build verification)
- Linux GCC/Clang builds fail
- Performance regression detection triggers

✅ **Yellow/Orange Performance Badge**: Normal when:
- macOS build verification fails due to compiler version
- Windows build has minor issues
- Some cross-platform verification steps fail

## 🔧 Running Locally

For local performance testing without CI dependencies:

```bash
# Run benchmarks locally
./scripts/run_benchmarks.sh

# Custom configuration
BUILD_TYPE=Release CXX=clang++ ./scripts/run_benchmarks.sh
```

## 📊 Performance Analysis

```bash
# Clone performance history
git clone https://github.com/obhi-d/ouly.git
cd ouly
git checkout performance-tracking

# Analyze trends
python3 scripts/analyze_performance.py results/ -o report.md
```
