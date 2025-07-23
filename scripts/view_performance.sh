#!/bin/bash
# Performance Results Viewer for Ouly
# This script helps you easily access and view performance benchmark results

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🚀 Ouly Performance Results Viewer${NC}"
echo "====================================="

# Function to check if we're in the right repository
check_repo() {
    if [[ ! -f "CMakeLists.txt" ]] || [[ ! -d "include/ouly" ]]; then
        echo -e "${RED}Error: This script must be run from the Ouly repository root${NC}"
        exit 1
    fi
}

# Function to show available results
show_results_overview() {
    echo -e "\n${YELLOW}📊 Available Performance Data:${NC}"
    
    # Check GitHub Actions artifacts
    echo -e "\n${BLUE}1. GitHub Actions Artifacts (Last 30 days):${NC}"
    echo "   → Visit: https://github.com/obhi-d/ouly/actions/workflows/performance.yml"
    echo "   → Download benchmark-results-* artifacts"
    echo "   → Look for performance-visualizations artifacts"
    
    # Check performance-tracking branch
    echo -e "\n${BLUE}2. Historical Data (performance-tracking branch):${NC}"
    if git ls-remote --heads origin performance-tracking | grep -q performance-tracking; then
        echo -e "   ${GREEN}✓ performance-tracking branch exists${NC}"
        
        # Show latest results if we can access them
        if git show-branch performance-tracking >/dev/null 2>&1; then
            echo "   → Switch to branch: git checkout performance-tracking"
            echo "   → Results in: results/ directory"
        else
            echo "   → Fetch branch: git fetch origin performance-tracking"
            echo "   → Switch to: git checkout performance-tracking"
        fi
    else
        echo -e "   ${YELLOW}⚠ performance-tracking branch not found${NC}"
        echo "   → Run benchmarks on main branch to create it"
    fi
    
    # Check local results
    echo -e "\n${BLUE}3. Local Results:${NC}"
    if [[ -d "benchmark_results" ]]; then
        local count=$(find benchmark_results -name "*.json" | wc -l)
        if [[ $count -gt 0 ]]; then
            echo -e "   ${GREEN}✓ Found $count local result files${NC}"
            echo "   → Location: benchmark_results/"
        else
            echo -e "   ${YELLOW}⚠ benchmark_results/ exists but no JSON files found${NC}"
        fi
    else
        echo -e "   ${YELLOW}⚠ No local benchmark_results/ directory${NC}"
        echo "   → Run: ./scripts/run_benchmarks.sh"
    fi
}

# Function to show how to run benchmarks
show_how_to_run() {
    echo -e "\n${YELLOW}🏃 How to Run Benchmarks:${NC}"
    echo ""
    echo -e "${BLUE}Local Benchmarks:${NC}"
    echo "   ./scripts/run_benchmarks.sh"
    echo ""
    echo -e "${BLUE}Custom Configuration:${NC}"
    echo "   BUILD_TYPE=Release CXX=clang++ ./scripts/run_benchmarks.sh"
    echo ""
    echo -e "${BLUE}CI Benchmarks:${NC}"
    echo "   → Push to main branch"
    echo "   → Create pull request"
    echo "   → Manual trigger via GitHub Actions"
}

# Function to show how to view visualizations
show_how_to_visualize() {
    echo -e "\n${YELLOW}📈 How to View Performance Visualizations:${NC}"
    echo ""
    echo -e "${BLUE}Option 1: GitHub Actions Artifacts${NC}"
    echo "   1. Go to: https://github.com/obhi-d/ouly/actions/workflows/performance.yml"
    echo "   2. Click on latest successful run"
    echo "   3. Download 'performance-visualizations' artifact"
    echo "   4. Extract and open 'performance_report.html'"
    echo ""
    echo -e "${BLUE}Option 2: Generate Locally${NC}"
    echo "   1. Switch to performance-tracking branch: git checkout performance-tracking"
    echo "   2. Install dependencies: pip install matplotlib seaborn pandas"
    echo "   3. Generate visualizations: python3 scripts/visualize_performance.py results/"
    echo "   4. Open: performance_visualizations/performance_report.html"
    echo ""
    echo -e "${BLUE}Option 3: View in performance-tracking branch${NC}"
    echo "   1. Switch to branch: git checkout performance-tracking"
    echo "   2. Look for: visualizations/ directory"
    echo "   3. Open: visualizations/performance_report.html"
}

# Function to show latest results summary
show_latest_summary() {
    echo -e "\n${YELLOW}📋 Latest Results Summary:${NC}"
    
    # Try to get info from performance-tracking branch
    if git ls-remote --heads origin performance-tracking | grep -q performance-tracking; then
        echo -e "   ${GREEN}✓ Performance tracking is active${NC}"
        echo "   → Latest CI run data available in artifacts"
        echo "   → Historical trends in performance-tracking branch"
    else
        echo -e "   ${YELLOW}⚠ No performance tracking data yet${NC}"
        echo "   → Run benchmarks to start collecting data"
    fi
    
    # Check for local results
    if [[ -d "benchmark_results" ]] && [[ $(find benchmark_results -name "*.json" | wc -l) -gt 0 ]]; then
        echo -e "   ${GREEN}✓ Local benchmark data available${NC}"
        local latest=$(find benchmark_results -name "*.json" -exec ls -t {} + | head -1)
        if [[ -n "$latest" ]]; then
            echo "   → Latest local result: $(basename "$latest")"
        fi
    fi
}

# Main execution
main() {
    check_repo
    
    case "${1:-overview}" in
        "overview"|"")
            show_results_overview
            show_latest_summary
            ;;
        "run"|"benchmark")
            show_how_to_run
            ;;
        "visualize"|"graph"|"charts")
            show_how_to_visualize
            ;;
        "help"|"-h"|"--help")
            echo -e "${BLUE}Usage: $0 [command]${NC}"
            echo ""
            echo "Commands:"
            echo "  overview     Show available performance data (default)"
            echo "  run          Show how to run benchmarks"
            echo "  visualize    Show how to create visualizations"
            echo "  help         Show this help message"
            ;;
        *)
            echo -e "${RED}Unknown command: $1${NC}"
            echo "Use '$0 help' for available commands"
            exit 1
            ;;
    esac
    
    echo ""
    echo -e "${GREEN}📚 For more information, see: docs/PERFORMANCE_BENCHMARKS.md${NC}"
}

main "$@"
