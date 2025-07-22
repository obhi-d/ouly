#!/usr/bin/env python3
"""
Performance Analysis Script for Ouly Benchmarks

This script analyzes benchmark results stored in the performance-tracking branch
and generates reports showing performance trends over time.
"""

import json
import os
import sys
import argparse
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Any
import statistics

def load_benchmark_results(results_dir: Path) -> Dict[str, List[Dict[str, Any]]]:
    """Load all benchmark results from the results directory."""
    results = {}
    
    for result_file in results_dir.rglob("*.json"):
        try:
            with open(result_file, 'r') as f:
                data = json.load(f)
                
            # Extract timestamp from directory name or file
            timestamp_str = result_file.parent.name.split('_')[0]
            compiler = "unknown"
            
            # Determine compiler from filename
            if "gcc" in result_file.name:
                compiler = "gcc"
            elif "clang" in result_file.name:
                compiler = "clang"
                
            key = f"{compiler}_{result_file.stem}"
            if key not in results:
                results[key] = []
                
            data['timestamp'] = timestamp_str
            data['file'] = str(result_file)
            results[key].append(data)
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"Warning: Could not parse {result_file}: {e}")
            
    return results

def analyze_performance_trends(results: Dict[str, List[Dict[str, Any]]]) -> Dict[str, Any]:
    """Analyze performance trends across benchmark runs."""
    analysis = {}
    
    for key, result_list in results.items():
        if len(result_list) < 2:
            continue
            
        # Sort by timestamp
        sorted_results = sorted(result_list, key=lambda x: x.get('timestamp', ''))
        
        analysis[key] = {
            'total_runs': len(sorted_results),
            'latest_run': sorted_results[-1].get('timestamp', 'unknown'),
            'oldest_run': sorted_results[0].get('timestamp', 'unknown'),
            'status': sorted_results[-1].get('status', 'unknown')
        }
        
    return analysis

def generate_performance_report(analysis: Dict[str, Any], output_file: Path):
    """Generate a performance report in markdown format."""
    with open(output_file, 'w') as f:
        f.write("# Ouly Performance Analysis Report\n\n")
        f.write(f"**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        f.write("## Benchmark Overview\n\n")
        f.write("| Configuration | Total Runs | Latest Run | Status |\n")
        f.write("|---------------|------------|------------|--------|\n")
        
        for config, data in analysis.items():
            f.write(f"| {config} | {data['total_runs']} | {data['latest_run']} | {data['status']} |\n")
        
        f.write("\n## Analysis Notes\n\n")
        f.write("- Performance benchmarks track allocator and scheduler performance\n")
        f.write("- Results are collected daily and on main branch updates\n")
        f.write("- Detailed timing data is available in individual JSON files\n")
        f.write("\n## Tracked Components\n\n")
        f.write("- **ts_shared_linear_allocator**: Thread-safe shared allocator\n")
        f.write("- **ts_thread_local_allocator**: Thread-local allocator\n") 
        f.write("- **coalescing_arena_allocator**: Coalescing arena allocator\n")
        f.write("- **scheduler**: Task scheduler (submission, parallel_for, work stealing)\n")

def main():
    parser = argparse.ArgumentParser(description="Analyze Ouly performance benchmark results")
    parser.add_argument("results_dir", help="Directory containing benchmark results")
    parser.add_argument("-o", "--output", help="Output report file", default="performance_report.md")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    
    args = parser.parse_args()
    
    results_dir = Path(args.results_dir)
    if not results_dir.exists():
        print(f"Error: Results directory {results_dir} does not exist")
        sys.exit(1)
    
    if args.verbose:
        print(f"Loading benchmark results from {results_dir}")
    
    # Load and analyze results
    results = load_benchmark_results(results_dir)
    analysis = analyze_performance_trends(results)
    
    if args.verbose:
        print(f"Loaded {len(results)} benchmark configurations")
        for key, data in analysis.items():
            print(f"  {key}: {data['total_runs']} runs")
    
    # Generate report
    output_file = Path(args.output)
    generate_performance_report(analysis, output_file)
    
    print(f"Performance report generated: {output_file}")

if __name__ == "__main__":
    main()
