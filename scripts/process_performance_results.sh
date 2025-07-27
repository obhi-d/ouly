#!/bin/bash
set -euo pipefail

# Performance tracking workflow script for GitHub Actions
# This script processes benchmark results and commits them to the performance-tracking branch

RESULTS_DIR="results"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M")
COMPILER_VERSION=$(gcc --version | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
COMMIT_HASH="${GITHUB_SHA:-$(git rev-parse HEAD)}"
COMMIT_SHORT="${COMMIT_HASH:0:8}"
BRANCH="${GITHUB_REF_NAME:-$(git branch --show-current)}"

echo "🔄 Processing performance results..."
echo "Timestamp: $TIMESTAMP"
echo "Compiler: gcc-$COMPILER_VERSION"
echo "Commit: $COMMIT_SHORT"
echo "Branch: $BRANCH"

# Create directory structure for results
RESULT_DIR="$RESULTS_DIR/$TIMESTAMP/gcc-$COMPILER_VERSION/$BRANCH/$COMMIT_SHORT"
mkdir -p "$RESULT_DIR"

# Copy benchmark results from build directory
if [ -d "build/unit_tests" ]; then
    echo "📁 Copying benchmark results..."
    cp build/unit_tests/*.json "$RESULT_DIR/" 2>/dev/null || echo "⚠️  No JSON files found"
    cp build/unit_tests/*.txt "$RESULT_DIR/" 2>/dev/null || echo "⚠️  No TXT files found"
    echo "✅ Results copied to $RESULT_DIR"
else
    echo "❌ Build directory not found!"
    exit 1
fi

# Generate performance report
echo "📊 Generating performance report..."
python3 scripts/generate_performance_report.py --results-dir "$RESULTS_DIR" --output PERFORMANCE_REPORT.md

# Update index.md with latest results
echo "📝 Updating results index..."
cat > "$RESULTS_DIR/index.md" << EOF
# Ouly Performance Tracking

This branch contains historical performance benchmark results.

## Latest Results

- **Timestamp**: $TIMESTAMP
- **Commit**: $COMMIT_SHORT
- **Branch**: $BRANCH
- **Compiler**: gcc-$COMPILER_VERSION

## Results Structure

Each directory contains benchmark results for a specific build:
- GCC $COMPILER_VERSION Release build
- Results organized by timestamp, compiler, branch, and commit

## Tracked Components

- **Scheduler**: Task scheduler performance (V1 vs V2 vs TBB comparison)
  - Task submission performance
  - Parallel for operations  
  - Matrix operations
  - Mixed workloads
  - Task throughput
  - Nested parallel workloads
- **Allocators**: Memory allocator performance
  - Thread-safe shared linear allocator
  - Thread-safe thread local allocator
  - Coalescing arena allocator

## Usage

Results are stored in JSON format and can be processed with analysis tools.
Each benchmark includes median, min, max, mean times and relative error.

### Performance Report

The comprehensive performance report is available in [PERFORMANCE_REPORT.md](../PERFORMANCE_REPORT.md).

### Directory Structure

\`\`\`
results/
  ├── index.md (this file)
  ├── YYYY-MM-DD_HH-MM/
  │   └── gcc-X.Y/
  │       └── branch-name/
  │           └── commit-hash/
  │               ├── *.json (benchmark data)
  │               └── *.txt (summary results)
  └── ...
\`\`\`

## Latest Benchmark Files

EOF

# List the latest benchmark files
find "$RESULT_DIR" -name "*.json" -o -name "*.txt" | sort | while read file; do
    filename=$(basename "$file")
    echo "- [\`$filename\`]($TIMESTAMP/gcc-$COMPILER_VERSION/$BRANCH/$COMMIT_SHORT/$filename)" >> "$RESULTS_DIR/index.md"
done

echo "" >> "$RESULTS_DIR/index.md"
echo "*Last updated: $(date)*" >> "$RESULTS_DIR/index.md"

echo "✅ Performance tracking update completed!"
echo "📁 Results directory: $RESULT_DIR"
echo "📊 Performance report: PERFORMANCE_REPORT.md"
echo "📝 Results index: $RESULTS_DIR/index.md"
