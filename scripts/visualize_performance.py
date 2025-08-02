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
    """
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
                except ValueError:
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
    # Handle scheduler comparison benchmarks
    if '_V1' in benchmark_name or '_V2' in benchmark_name or '_TBB' in benchmark_name:
        return benchmark_name.rsplit('_', 1)[0]  # Remove _V1, _V2, _TBB suffix
    
    # Handle allocator benchmarks - return as-is since they don't have variants
    return benchmark_name

def extract_measurement_type(benchmark_name: str) -> str:
    """Extract the base measurement type from benchmark name."""
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
                measurement_type = plot_file.replace('performance_trend_', '').replace('.svg', '')
                measurement_types.add(measurement_type)
            elif 'throughput_trend_' in plot_file:
                measurement_type = plot_file.replace('throughput_trend_', '').replace('.svg', '')
                measurement_types.add(measurement_type)
        
        for measurement_type in sorted(measurement_types):
            # Create a nice display name from the measurement type
            display_name = measurement_type.replace('_', ' ').title()
            
            # For safe filenames, we need to reverse the process
            perf_plot = f'performance_trend_{measurement_type}.svg'
            throughput_plot = f'throughput_trend_{measurement_type}.svg'
            
            if perf_plot in plot_files or throughput_plot in plot_files:
                md_content.append(f"### {display_name} Performance")
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
