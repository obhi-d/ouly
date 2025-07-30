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

# Configure matplotlib for SVG output
plt.style.use('default')  # Use default style to avoid seaborn dependency issues
plt.rcParams['figure.figsize'] = (12, 8)
plt.rcParams['savefig.format'] = 'svg'
plt.rcParams['savefig.dpi'] = 300

def parse_filename(filename: str) -> Optional[Dict[str, str]]:
    """Parse benchmark filename to extract metadata.
    
    Expected format: <compiler-id>-<commit-hash>-<build-number>-<test_id>.json
    """
    pattern = r'([^-]+)-([^-]+)-([^-]+)-([^.]+)\.json'
    match = re.match(pattern, filename)
    
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
                print(f"Skipping file with unexpected format: {json_file.name}")
                continue
                
            with open(json_file, 'r') as f:
                benchmark_data = json.load(f)
            
            # Extract file modification time as proxy for run date
            file_time = datetime.fromtimestamp(json_file.stat().st_mtime)
            
            # Process each benchmark result
            for result in benchmark_data.get('results', []):
                row = {
                    'compiler': file_info['compiler'],
                    'commit_hash': file_info['commit_hash'],
                    'build_number': int(file_info['build_number']),
                    'test_id': file_info['test_id'],
                    'run_date': file_time,
                    'benchmark_name': result.get('name', 'unknown'),
                    'median_elapsed': result.get('median(elapsed)', 0),
                    'median_instructions': result.get('median(instructions)', 0),
                    'relative_error': result.get('median(relative error %)', 0),
                    'operations_per_sec': 1.0 / result.get('median(elapsed)', 1e-9) if result.get('median(elapsed)', 0) > 0 else 0
                }
                data_rows.append(row)
                
        except Exception as e:
            print(f"Error processing {json_file}: {e}")
            
    if not data_rows:
        print("Warning: No valid benchmark data found")
        return pd.DataFrame()
        
    df = pd.DataFrame(data_rows)
    df = df.sort_values('run_date')
    return df

def create_performance_plots(df: pd.DataFrame, output_dir: Path) -> List[str]:
    """Create performance visualization plots and return list of generated files."""
    
    if df.empty:
        print("No data to plot")
        return []
    
    output_dir.mkdir(parents=True, exist_ok=True)
    generated_files = []
    
    # Get unique test IDs and compilers
    test_ids = df['test_id'].unique()
    compilers = df['compiler'].unique()
    
    for test_id in test_ids:
        test_data = df[df['test_id'] == test_id]
        
        if test_data.empty:
            continue
            
        # Plot 1: Median Elapsed Time Over Build Numbers
        plt.figure(figsize=(12, 8))
        
        for compiler in compilers:
            comp_data = test_data[test_data['compiler'] == compiler]
            if not comp_data.empty:
                # Group by benchmark name and plot each one
                for bench_name in comp_data['benchmark_name'].unique():
                    bench_data = comp_data[comp_data['benchmark_name'] == bench_name]
                    if len(bench_data) > 1:  # Only plot if we have multiple points
                        plt.plot(bench_data['build_number'], bench_data['median_elapsed'] * 1e9, 
                               marker='o', label=f'{compiler} - {bench_name}', linewidth=2, markersize=6)
        
        plt.xlabel('Build Number')
        plt.ylabel('Median Elapsed Time (ns)')
        plt.title(f'Performance Trend - {test_id.replace("_", " ").title()}')
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        
        filename = f'performance_trend_{test_id}.svg'
        filepath = output_dir / filename
        plt.savefig(filepath, format='svg', bbox_inches='tight')
        plt.close()
        generated_files.append(filename)
        
        # Plot 2: Operations per Second Over Build Numbers
        plt.figure(figsize=(12, 8))
        
        for compiler in compilers:
            comp_data = test_data[test_data['compiler'] == compiler]
            if not comp_data.empty:
                for bench_name in comp_data['benchmark_name'].unique():
                    bench_data = comp_data[comp_data['benchmark_name'] == bench_name]
                    if len(bench_data) > 1:
                        plt.plot(bench_data['build_number'], bench_data['operations_per_sec'], 
                               marker='s', label=f'{compiler} - {bench_name}', linewidth=2, markersize=6)
        
        plt.xlabel('Build Number')
        plt.ylabel('Operations per Second')
        plt.title(f'Throughput Trend - {test_id.replace("_", " ").title()}')
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, alpha=0.3)
        plt.yscale('log')  # Log scale for ops/sec
        plt.tight_layout()
        
        filename = f'throughput_trend_{test_id}.svg'
        filepath = output_dir / filename
        plt.savefig(filepath, format='svg', bbox_inches='tight')
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
        
        for plot_file in plot_files:
            if 'performance_trend' in plot_file:
                test_id = plot_file.replace('performance_trend_', '').replace('.svg', '')
                md_content.append(f"### {test_id.replace('_', ' ').title()} - Execution Time")
                md_content.append("")
                md_content.append(f"![{test_id} Performance Trend]({plot_file})")
                md_content.append("")
            elif 'throughput_trend' in plot_file:
                test_id = plot_file.replace('throughput_trend_', '').replace('.svg', '')
                md_content.append(f"### {test_id.replace('_', ' ').title()} - Throughput")
                md_content.append("")
                md_content.append(f"![{test_id} Throughput Trend]({plot_file})")
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
    md_content.append("*This report is automatically generated from benchmark results stored in the performance-tracking branch.*")
    
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
