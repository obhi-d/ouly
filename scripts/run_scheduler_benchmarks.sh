#!/bin/bash
# Quick runner script for the OULY scheduler comparison benchmarks
# This script demonstrates the JSON output generation for performance tracking

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
BENCHMARK_EXECUTABLE="$BUILD_DIR/unit_tests/ouly-scheduler-comparison-bench"

# Set up environment
export LD_LIBRARY_PATH="$BUILD_DIR/gnu_15.1_cxx20_64_release:$LD_LIBRARY_PATH"

# Check if benchmark executable exists
if [ ! -f "$BENCHMARK_EXECUTABLE" ]; then
    echo "❌ Benchmark executable not found: $BENCHMARK_EXECUTABLE"
    echo "📋 Please build the project first with:"
    echo "   cmake --build build --target ouly-scheduler-comparison-bench"
    exit 1
fi

echo "🚀 Running OULY Scheduler Comparison Benchmarks"
echo "================================================"
echo "📁 Working directory: $(pwd)"
echo "🔧 Executable: $BENCHMARK_EXECUTABLE"
echo ""

# Run the benchmark
"$BENCHMARK_EXECUTABLE" "$@"

echo ""
echo "📊 Benchmark Results:"
echo "===================="

# Show generated JSON files (for performance tracking integration)
if ls scheduler_comparison_*.json 1> /dev/null 2>&1; then
    echo "✅ JSON files generated for performance tracking:"
    for json_file in scheduler_comparison_*.json; do
        echo "   📄 $json_file"
        
        # Show a preview of the JSON structure
        if command -v jq &> /dev/null; then
            echo "      📈 Preview (first benchmark result):"
            jq -r '(.context // {}) | "         Platform: \(.platform // "unknown")", "         CPU: \(.cpu // "unknown")", "         Date: \(.date // "unknown")"' "$json_file" 2>/dev/null || true
            jq -r '.results[0] | "         First Result: \(.name) - \(.median * 1e9 | round) ns/op"' "$json_file" 2>/dev/null || true
        fi
        echo ""
    done
else
    echo "⚠️  No JSON files found. Check if benchmark completed successfully."
fi

# Show generated text files
if ls scheduler_comparison_*.txt 1> /dev/null 2>&1; then
    echo "📋 Human-readable results:"
    for txt_file in scheduler_comparison_*.txt; do
        echo "   📄 $txt_file"
    done
    echo ""
fi

echo "🎯 Integration Notes:"
echo "===================="
echo "• JSON files are compatible with the performance-tracking branch workflow"
echo "• Files follow nanobench JSON format with full statistical data"
echo "• Benchmark names include scheduler version (V1, V2, TBB) for comparison"
echo "• Results include both timing and instruction count metrics"
echo ""
echo "📚 For CI integration, these files can be processed by:"
echo "• scripts/analyze_performance.py (trend analysis)"
echo "• scripts/visualize_performance.py (chart generation)"
echo "• GitHub Actions performance workflow (automatic tracking)"
