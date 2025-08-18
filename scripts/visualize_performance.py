#!/usr/bin/env python3
"""
Performance visualization script for OULY benchmarks.
Generates SVG plots and PERFORMANCE.md report from benchmark results.
"""

import argparse
import json
import os
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Configure matplotlib for high-quality SVG output (not bitmap)
import matplotlib
matplotlib.use('SVG')  # Force SVG backend
plt.style.use('default')  # Use default style to avoid seaborn dependency issues
plt.rcParams['figure.figsize'] = (12, 8)
plt.rcParams['savefig.format'] = 'svg'
plt.rcParams['savefig.dpi'] = 300
plt.rcParams['svg.fonttype'] = 'none'  # Ensure text remains as text in SVG, not paths
plt.rcParams['font.size'] = 10
plt.rcParams['axes.labelsize'] = 12
plt.rcParams['axes.titlesize'] = 14
plt.rcParams['legend.fontsize'] = 10
plt.rcParams['xtick.labelsize'] = 10
plt.rcParams['ytick.labelsize'] = 10

def parse_filename(filename: str) -> Optional[Dict[str, str]]:
    """Parse benchmark filename to extract metadata.
    
    Expected format: <compiler-id>-<commit-hash>-<build-number>-<test_id>.json
    For example: gcc-14-abc12345-100-allocator.json
    
    Also handles coroutine benchmark format: coroutine_<test_type>_<compiler>_<timestamp>.json
    For example: coroutine_overhead_comparison_gcc-15.2_20250818_034152.json
    """
    # New pattern for coroutine benchmarks: coroutine_<test_type>_<compiler>_<timestamp>.json
    coroutine_pattern = r'coroutine_([^_]+_[^_]+)_([^_]+)_(\d{8})_(\d{6})\.json'
    match = re.match(coroutine_pattern, filename)
    
    if match:
        test_type = match.group(1)
        compiler = match.group(2)
        date = match.group(3)
        time = match.group(4)
        timestamp = f"{date}{time}"  # YYYYMMDDHHMMSS
        return {
            'compiler': compiler,
            'commit_hash': 'unknown',
            'build_number': timestamp,  # Use timestamp as sortable build number
            'test_id': f'coroutine_{test_type}'
        }
    
    # Updated pattern to handle compound compiler names like "gcc-14" or "clang-18"
    pattern = r'([^-]+-[^-]+)-([^-]+)-(\d+)-([^.]+)\.json'
    match = re.match(pattern, filename)
    
    if match:
        return {
            'compiler': match.group(1),
            'commit_hash': match.group(2),
            'build_number': match.group(3),
            'test_id': match.group(4)
        }
    
    # Fallback to the original pattern for simpler compiler names
    pattern_fallback = r'([^-]+)-([^-]+)-(\d+)-([^.]+)\.json'
    match = re.match(pattern_fallback, filename)
    
    if match:
        return {
            'compiler': match.group(1),
            'commit_hash': match.group(2),
            'build_number': match.group(3),
            'test_id': match.group(4)
        }
    
    return None

def load_benchmark_data(results_dir: Path) -> pd.DataFrame:
    """Load all benchmark JSON files and create a unified DataFrame."""
    
    data_rows = []
    
    for json_file in results_dir.rglob('*.json'):
        try:
            file_info = parse_filename(json_file.name)
            if not file_info:
                # Try alternative formats (benchmark_results.json, etc.)
                print(f"Trying alternative format for: {json_file.name}")
                # For files like benchmark_results.json, extract build info from directory or metadata
                file_info = {'compiler': 'unknown', 'commit_hash': 'unknown', 'build_number': '0', 'test_id': 'general'}
                
            with open(json_file, 'r') as f:
                benchmark_data = json.load(f)
            
            # Extract file modification time as proxy for run date
            file_time = datetime.fromtimestamp(json_file.stat().st_mtime)
            
            # Process each benchmark result
            for result in benchmark_data.get('results', []):
                # Convert build_number to int, default to 0 if not numeric
                try:
                    build_number = int(file_info['build_number'])
                except (ValueError, TypeError):
                    build_number = 0
                    
                # Calculate additional metrics
                median_elapsed = result.get('median(elapsed)', 0)
                throughput = 1.0 / median_elapsed if median_elapsed > 0 else 0
                
                row = {
                    'compiler': file_info['compiler'],
                    'commit_hash': file_info['commit_hash'],
                    'build_number': build_number,
                    'test_id': file_info['test_id'],
                    'run_date': file_time,
                    'benchmark_name': result.get('name', 'unknown'),
                    'median_elapsed': median_elapsed,
                    'median_instructions': result.get('median(instructions)', 0),
                    'relative_error': result.get('median(relative error %)', 0),
                    'operations_per_sec': throughput,
                    'elapsed_ns': median_elapsed * 1e9,  # Convert to nanoseconds for better readability
                    'instructions_per_cycle': result.get('median(instructions)', 0) / result.get('median(cpucycles)', 1) if result.get('median(cpucycles)', 0) > 0 else 0
                }
                data_rows.append(row)
                
        except Exception as e:
            print(f"Error processing {json_file}: {e}")
            
    if not data_rows:
        print("Warning: No valid benchmark data found")
        return pd.DataFrame()
        
    df = pd.DataFrame(data_rows)
    df = df.sort_values(['build_number', 'run_date'])
    return df

def extract_measurement_type(benchmark_name: str) -> str:
    """Extract the base measurement type from benchmark name."""
    # Handle coroutine benchmarks - group by logical categories
    if 'CoroutineCreation' in benchmark_name:
        return 'coroutine_creation'
    elif 'CoroutineSubmission' in benchmark_name or 'LambdaSubmission' in benchmark_name:
        return 'coroutine_submission'
    elif 'SuspensionOverhead' in benchmark_name:
        return 'coroutine_suspension'
    elif 'CoroutineMemory' in benchmark_name:
        return 'coroutine_memory'
    elif 'ParallelCompute' in benchmark_name:
        return 'coroutine_parallel_compute'
    elif 'TaskChaining' in benchmark_name:
        return 'coroutine_task_chaining'
    elif 'FanOutIn' in benchmark_name:
        return 'coroutine_fan_out_in'
    
    # Handle scheduler comparison benchmarks
    if '_V1' in benchmark_name or '_V2' in benchmark_name or '_TBB' in benchmark_name:
        return benchmark_name.rsplit('_', 1)[0]  # Remove _V1, _V2, _TBB suffix
    
    # Handle allocator benchmarks - return as-is since they don't have variants
    return benchmark_name

def create_performance_plots(df: pd.DataFrame, output_dir: Path) -> List[str]:
    """Create performance visualization plots split by measurement type."""
    generated_files = []
    
    if df.empty:
        return generated_files
    
    # Create output directory if it doesn't exist
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Color palette for different lines
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', 
              '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf']
    
    # Extract measurement types from benchmark names
    df = df.copy()
    df['measurement_type'] = df['benchmark_name'].apply(extract_measurement_type)
    
    # Get unique values for grouping
    measurement_types = df['measurement_type'].unique()
    compilers = df['compiler'].unique()
    
    # Create plots per measurement type
    for measurement_type in measurement_types:
        type_data = df[df['measurement_type'] == measurement_type]
        
        if type_data.empty or len(type_data.groupby(['compiler', 'benchmark_name'])) == 0:
            continue
            
        # Plot 1: Elapsed Time for this measurement type
        plt.figure(figsize=(12, 8))
        color_idx = 0
        
        for compiler in compilers:
            comp_data = type_data[type_data['compiler'] == compiler]
            if comp_data.empty:
                continue
                
            for bench_name in comp_data['benchmark_name'].unique():
                bench_data = comp_data[comp_data['benchmark_name'] == bench_name]
                if len(bench_data) > 1:  # Only plot if we have multiple points
                    # Aggregate by build number to handle multiple runs per build
                    agg_data = bench_data.groupby('build_number').agg({
                        'elapsed_ns': 'median',
                        'run_date': 'first'
                    }).reset_index()
                    
                    # Create clean label (remove redundant measurement type prefix)
                    label_suffix = bench_name.replace(measurement_type, '').strip('_')
                    if label_suffix:
                        label = f'{compiler} - {label_suffix}'
                    else:
                        label = f'{compiler}'
                    
                    plt.plot(agg_data['build_number'], agg_data['elapsed_ns'], 
                           marker='o', label=label, linewidth=2, markersize=6,
                           color=colors[color_idx % len(colors)])
                    color_idx += 1
        
        plt.xlabel('Build Number', fontsize=12)
        plt.ylabel('Median Elapsed Time (nanoseconds)', fontsize=12)
        plt.title(f'{measurement_type.replace("_", " ").title()} - Performance Trend', fontsize=14, fontweight='bold')
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, alpha=0.3)
        plt.yscale('log')
        plt.tight_layout()
        
        # Create safe filename
        safe_name = measurement_type.replace('_', '').replace(' ', '').lower()
        filename = f'performance_trend_{safe_name}.svg'
        filepath = output_dir / filename
        plt.savefig(filepath, format='svg', bbox_inches='tight', dpi=300)
        plt.close()
        generated_files.append(filename)
        
        # Plot 2: Throughput for this measurement type
        plt.figure(figsize=(12, 8))
        color_idx = 0
        
        for compiler in compilers:
            comp_data = type_data[type_data['compiler'] == compiler]
            if comp_data.empty:
                continue
                
            for bench_name in comp_data['benchmark_name'].unique():
                bench_data = comp_data[comp_data['benchmark_name'] == bench_name]
                if len(bench_data) > 1:
                    # Aggregate by build number
                    agg_data = bench_data.groupby('build_number').agg({
                        'operations_per_sec': 'median',
                        'run_date': 'first'
                    }).reset_index()
                    
                    # Create clean label (remove redundant measurement type prefix)
                    label_suffix = bench_name.replace(measurement_type, '').strip('_')
                    if label_suffix:
                        label = f'{compiler} - {label_suffix}'
                    else:
                        label = f'{compiler}'
                    
                    plt.plot(agg_data['build_number'], agg_data['operations_per_sec'], 
                           marker='s', label=label, linewidth=2, markersize=6,
                           color=colors[color_idx % len(colors)])
                    color_idx += 1
        
        plt.xlabel('Build Number', fontsize=12)
        plt.ylabel('Operations per Second', fontsize=12)
        plt.title(f'{measurement_type.replace("_", " ").title()} - Throughput Trend', fontsize=14, fontweight='bold')
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, alpha=0.3)
        plt.yscale('log')
        plt.tight_layout()
        
        filename = f'throughput_trend_{safe_name}.svg'
        filepath = output_dir / filename
        plt.savefig(filepath, format='svg', bbox_inches='tight', dpi=300)
        plt.close()
        generated_files.append(filename)
    
    return generated_files

def get_benchmark_description(measurement_type: str) -> str:
    """Get detailed description for benchmark measurement types."""
    descriptions = {
        'coalescingarenaallocdealloc': (
            "**Coalescing Arena Allocator Performance**\n\n"
            "Measures the performance of the coalescing arena allocator, which manages memory by "
            "coalescing adjacent free blocks to reduce fragmentation. This benchmark tests allocation "
            "and deallocation operations using the arena-based memory management system. Lower times "
            "indicate better allocator performance."
        ),
        'tssharedlinearsinglethread': (
            "**Thread-Safe Shared Linear Allocator (Single Thread)**\n\n"
            "Tests the performance of a thread-safe shared linear allocator when used in single-threaded "
            "scenarios. Linear allocators provide O(1) allocation by simply incrementing a pointer, but "
            "this variant includes thread-safety mechanisms. Measures the overhead of synchronization "
            "primitives when not actually needed (single-threaded usage)."
        ),
        'tsthreadlocalsinglethread': (
            "**Thread-Safe Thread-Local Allocator (Single Thread)**\n\n"
            "Evaluates a thread-local allocator's performance in single-threaded contexts. Thread-local "
            "allocators avoid synchronization overhead by maintaining separate memory pools per thread. "
            "This benchmark shows the baseline performance when thread-local storage is accessed from "
            "the owning thread."
        ),
        'tasksubmission': (
            "**Task Submission Performance**\n\n"
            "Measures the overhead of submitting tasks to the scheduler work queues. This includes the "
            "time to package work items, enqueue them into the scheduler's internal data structures, "
            "and wake up worker threads. Lower submission times enable better work distribution and "
            "reduced latency for fine-grained parallel workloads."
        ),
        'parallelforvectorops': (
            "**Parallel For Vector Operations**\n\n"
            "Benchmarks parallel execution of vector operations using the auto_parallel_for construct. "
            "Tests the scheduler's ability to efficiently distribute vector computations across worker "
            "threads, including work stealing and load balancing. Measures both the overhead of "
            "parallelization and the effectiveness of parallel execution for computational workloads."
        ),
        'matrixops': (
            "**Matrix Operations Performance**\n\n"
            "Evaluates parallel matrix computations including transformations, multiplications, and "
            "other linear algebra operations. Tests the scheduler's performance on computationally "
            "intensive tasks that benefit from parallel execution. Includes both CPU-bound calculations "
            "and memory access patterns typical of numerical computing workloads."
        ),
        'mixedworkload': (
            "**Mixed Workload Performance**\n\n"
            "Tests scheduler performance under heterogeneous workloads combining different types of "
            "tasks: I/O-bound, CPU-bound, memory-intensive, and short-duration tasks. Measures the "
            "scheduler's ability to handle diverse work characteristics simultaneously while maintaining "
            "good load balancing and thread utilization."
        ),
        'taskthroughput': (
            "**Task Throughput Measurement**\n\n"
            "Measures the maximum task processing rate (tasks per second) that the scheduler can sustain. "
            "Uses minimal-work tasks to focus on scheduler overhead rather than computation time. "
            "Higher throughput indicates better efficiency in task management, queue operations, and "
            "thread coordination mechanisms."
        ),
        'nestedparallel': (
            "**Nested Parallel Execution**\n\n"
            "Benchmarks performance when parallel constructs are nested within other parallel regions. "
            "Tests the scheduler's handling of hierarchical parallelism, including dynamic thread "
            "allocation, work stealing across nested contexts, and avoiding oversubscription. Critical "
            "for applications using recursive parallel algorithms."
        ),
        'coroutine_creation': (
            "**Coroutine Creation Overhead**\n\n"
            "Measures the fundamental cost of creating coroutine objects compared to regular function calls. "
            "This includes coroutine frame allocation, initial suspension setup, and coroutine handle creation. "
            "Tests both V1 and V2 scheduler implementations to identify any scheduler-specific overhead in "
            "coroutine management. Lower creation times enable efficient use of coroutines for fine-grained "
            "asynchronous operations."
        ),
        'coroutine_submission': (
            "**Coroutine vs Lambda Submission**\n\n"
            "Compares the submission overhead of coroutines versus equivalent lambda functions to the task "
            "scheduler. Measures the cost of packaging coroutines as schedulable tasks including any "
            "additional metadata, type erasure, and scheduler integration overhead. Helps quantify the "
            "runtime cost difference between coroutine-based and traditional callback-based asynchronous patterns."
        ),
        'coroutine_suspension': (
            "**Coroutine Suspension/Resumption Overhead**\n\n"
            "Measures the performance cost of coroutine suspension points and subsequent resumption. "
            "This includes saving coroutine state, yielding control back to the scheduler, and later "
            "restoring execution context. Critical for understanding the overhead of using 'co_await' "
            "constructs and designing efficient coroutine-based control flow."
        ),
        'coroutine_memory': (
            "**Coroutine Memory Overhead**\n\n"
            "Evaluates the memory allocation overhead of creating large numbers of coroutines simultaneously. "
            "Tests coroutine frame allocation patterns, memory fragmentation effects, and the efficiency of "
            "the coroutine memory management system. Important for applications that create many concurrent "
            "coroutines and need to understand memory scalability characteristics."
        ),
        'coroutine_parallel_compute': (
            "**Coroutine vs Regular Task Parallel Computation**\n\n"
            "Compares coroutine-based parallel computation against traditional task-based approaches for "
            "CPU-intensive workloads. Measures whether coroutines introduce significant overhead when used "
            "for computational tasks versus their traditional use for I/O-bound operations. Tests both "
            "scheduler versions to identify performance differences in computational contexts."
        ),
        'coroutine_task_chaining': (
            "**Coroutine Task Chaining Performance**\n\n"
            "Benchmarks sequential task dependencies implemented using coroutines versus traditional nested "
            "task submission patterns. Measures the efficiency of coroutine-based pipelines where each "
            "stage waits for the previous stage to complete. Compares against both manual task chaining "
            "and TBB equivalent implementations to understand the performance trade-offs."
        ),
        'coroutine_fan_out_in': (
            "**Coroutine Fan-Out/Fan-In Patterns**\n\n"
            "Evaluates the performance of scatter-gather patterns implemented using coroutines compared to "
            "traditional parallel_for constructs and TBB equivalents. Tests scenarios where work is "
            "distributed across multiple coroutines and results are collected back. Critical for understanding "
            "coroutine performance in data-parallel and map-reduce style algorithms."
        )
    }
    
    return descriptions.get(measurement_type, f"Performance metrics for {measurement_type.replace('_', ' ').title()}")

def generate_performance_md(df: pd.DataFrame, plot_files: List[str], output_path: Path):
    """Generate PERFORMANCE.md with latest results and embedded plots."""
    
    md_content = []
    md_content.append("# OULY Performance Tracking")
    md_content.append("")
    md_content.append(f"**Generated:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S UTC')}")
    md_content.append("")
    
    if df.empty:
        md_content.append("⚠️ No performance data available.")
        md_content.append("")
    else:
        # Latest results summary
        latest_build = df['build_number'].max()
        latest_data = df[df['build_number'] == latest_build]
        
        md_content.append("## 📊 Latest Performance Results")
        md_content.append("")
        md_content.append(f"**Build Number:** {latest_build}")
        md_content.append(f"**Commit Hash:** {latest_data['commit_hash'].iloc[0] if not latest_data.empty else 'N/A'}")
        md_content.append("")
        
        # Group by test_id for latest results
        for test_id in latest_data['test_id'].unique():
            test_results = latest_data[latest_data['test_id'] == test_id]
            
            md_content.append(f"### {test_id.replace('_', ' ').title()}")
            md_content.append("")
            md_content.append("| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |")
            md_content.append("|----------|-----------|------------------|---------|---------|")
            
            for _, row in test_results.iterrows():
                md_content.append(f"| {row['compiler']} | {row['benchmark_name']} | "
                                f"{row['median_elapsed']*1e9:.2f} | "
                                f"{row['operations_per_sec']:.0f} | "
                                f"{row['relative_error']:.2f} |")
            md_content.append("")
        
        # Performance trends
        md_content.append("## 📈 Performance Trends")
        md_content.append("")
        md_content.append("The following charts show performance trends over build numbers, ")
        md_content.append("with build number on the X-axis and performance metrics on the Y-axis.")
        md_content.append("Performance is grouped by measurement type.")
        md_content.append("")
        
        # Get measurement types from plot files
        measurement_types = set()
        for plot_file in plot_files:
            if 'performance_trend_' in plot_file:
                safe_name = plot_file.replace('performance_trend_', '').replace('.svg', '')
                measurement_types.add(safe_name)
            elif 'throughput_trend_' in plot_file:
                safe_name = plot_file.replace('throughput_trend_', '').replace('.svg', '')
                measurement_types.add(safe_name)
        
        # Create mapping from safe names back to original measurement types for description lookup
        safe_to_original = {
            'coroutinecreation': 'coroutine_creation',
            'coroutinesubmission': 'coroutine_submission', 
            'coroutinesuspension': 'coroutine_suspension',
            'coroutinememory': 'coroutine_memory',
            'coroutineparallelcompute': 'coroutine_parallel_compute',
            'coroutinetaskchaining': 'coroutine_task_chaining',
            'coroutinefanoutin': 'coroutine_fan_out_in'
        }
        
        for safe_name in sorted(measurement_types):
            # Get original measurement type for description lookup
            original_measurement_type = safe_to_original.get(safe_name, safe_name)
            
            # Create a nice display name from the original measurement type
            display_name = original_measurement_type.replace('_', ' ').title()
            
            # Use safe name for filenames
            perf_plot = f'performance_trend_{safe_name}.svg'
            throughput_plot = f'throughput_trend_{safe_name}.svg'
            
            if perf_plot in plot_files or throughput_plot in plot_files:
                md_content.append(f"### {display_name} Performance")
                md_content.append("")
                
                # Add detailed description using original measurement type
                description = get_benchmark_description(original_measurement_type)
                md_content.append(description)
                md_content.append("")
                
                if perf_plot in plot_files:
                    md_content.append("#### Execution Time")
                    md_content.append(f"![{display_name} Performance Trend]({perf_plot})")
                    md_content.append("")
                
                if throughput_plot in plot_files:
                    md_content.append("#### Throughput")
                    md_content.append(f"![{display_name} Throughput Trend]({throughput_plot})")
                    md_content.append("")
                md_content.append("")
        
        # Data summary
        md_content.append("## 📋 Data Summary")
        md_content.append("")
        md_content.append(f"- **Total benchmark runs:** {len(df)}")
        md_content.append(f"- **Build range:** {df['build_number'].min()} - {df['build_number'].max()}")
        md_content.append(f"- **Date range:** {df['run_date'].min().strftime('%Y-%m-%d')} to {df['run_date'].max().strftime('%Y-%m-%d')}")
        md_content.append(f"- **Compilers tested:** {', '.join(df['compiler'].unique())}")
        md_content.append(f"- **Test categories:** {', '.join(df['test_id'].unique())}")
        md_content.append("")
    
    md_content.append("---")
    md_content.append("*This report is automatically generated from benchmark results stored in the perfo branch.*")
    
    # Write the file
    with open(output_path, 'w') as f:
        f.write('\n'.join(md_content))

def main():
    parser = argparse.ArgumentParser(description='Generate performance visualizations and reports')
    parser.add_argument('results_dir', type=Path, help='Directory containing benchmark results')
    parser.add_argument('-o', '--output', type=Path, default=Path('.'), help='Output directory for plots and reports')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    
    args = parser.parse_args()
    
    if not args.results_dir.exists():
        print(f"Error: Results directory {args.results_dir} does not exist")
        sys.exit(1)
    
    if args.verbose:
        print(f"Loading benchmark data from {args.results_dir}")
    
    # Load benchmark data
    df = load_benchmark_data(args.results_dir)
    
    if df.empty:
        print("Warning: No benchmark data found")
        # Still generate empty report
        plot_files = []
    else:
        if args.verbose:
            print(f"Loaded {len(df)} benchmark results")
            print(f"Build range: {df['build_number'].min()} - {df['build_number'].max()}")
            print(f"Test IDs: {', '.join(df['test_id'].unique())}")
            print(f"Compilers: {', '.join(df['compiler'].unique())}")
        
        # Create plots
        if args.verbose:
            print("Generating performance plots...")
        plot_files = create_performance_plots(df, args.output)
        
        if args.verbose:
            print(f"Generated {len(plot_files)} plot files")
    
    # Generate PERFORMANCE.md
    if args.verbose:
        print("Generating PERFORMANCE.md...")
    
    generate_performance_md(df, plot_files, args.output / 'PERFORMANCE.md')
    
    if args.verbose:
        print("Performance report generation complete!")

if __name__ == '__main__':
    main()
