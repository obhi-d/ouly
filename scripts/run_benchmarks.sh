#!/bin/bash
# Local Performance Benchmark Runner for Ouly
# This script builds and runs performance benchmarks locally

set -e

# Configuration
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
OUTPUT_DIR="${OUTPUT_DIR:-benchmark_results}"
COMPILER="${CXX:-g++}"

echo "🚀 Ouly Performance Benchmark Runner"
echo "======================================"
echo "Build Type: $BUILD_TYPE"
echo "Build Directory: $BUILD_DIR"
echo "Output Directory: $OUTPUT_DIR"
echo "Compiler: $COMPILER"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Configure and build
echo "📦 Configuring CMake..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_CXX_COMPILER="$COMPILER" \
    -DOULY_BUILD_TESTS=ON

echo "🔨 Building..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Run benchmarks
echo "⚡ Running performance benchmarks..."
cd "$BUILD_DIR/unit_tests"

# Generate timestamp for results
TIMESTAMP=$(date '+%Y-%m-%d_%H-%M-%S')
COMPILER_NAME=$(basename "$COMPILER")
OUTPUT_FILE="../../$OUTPUT_DIR/benchmark_results_${COMPILER_NAME}_${BUILD_TYPE}_${TIMESTAMP}.json"

echo "Running benchmarks and saving results to: $OUTPUT_FILE"
./ouly-performance-bench "$OUTPUT_FILE"

echo ""
echo "✅ Benchmarks completed successfully!"
echo "📊 Results saved to: $OUTPUT_FILE"

# If Python is available, try to generate a quick analysis
if command -v python3 &> /dev/null; then
    echo ""
    echo "📈 Generating quick analysis..."
    
    # Create a simple analysis script inline
    cat > "../../$OUTPUT_DIR/quick_analysis.py" << 'EOF'
#!/usr/bin/env python3
import json
import sys
from pathlib import Path

def analyze_results(filename):
    try:
        with open(filename, 'r') as f:
            data = json.load(f)
        
        print(f"📊 Quick Analysis of {filename}")
        print("=" * 50)
        print(f"Timestamp: {data.get('timestamp', 'unknown')}")
        print(f"Status: {data.get('status', 'unknown')}")
        print(f"Note: {data.get('note', 'N/A')}")
        print("")
        print("✨ Benchmark completed successfully!")
        print("   Detailed performance data available in nanobench output above.")
        
    except Exception as e:
        print(f"Could not analyze results: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        analyze_results(sys.argv[1])
    else:
        print("Usage: python quick_analysis.py <results_file>")
EOF

    python3 "$OUTPUT_DIR/quick_analysis.py" "$OUTPUT_FILE"
    rm "$OUTPUT_DIR/quick_analysis.py"
fi

echo ""
echo "🎯 Next steps:"
echo "  - Review detailed benchmark output above"
echo "  - Compare with previous results in $OUTPUT_DIR"
echo "  - Commit performance-critical changes and monitor CI benchmarks"
