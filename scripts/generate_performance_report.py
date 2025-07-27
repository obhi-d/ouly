#!/usr/bin/env python3
"""
Performance Report Generator for OULY project.
This script analyzes benchmark JSON files and generates a comprehensive markdown report.
"""

import json
import os
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Optional
import argparse
import re


class PerformanceReportGenerator:
    def __init__(self, results_dir: Path, output_file: Path):
        self.results_dir = results_dir
        self.output_file = output_file
        self.system_info = {}
        self.benchmarks = {}
        
    def parse_json_files(self):
        """Parse all JSON files in the results directory."""
        json_files = list(self.results_dir.rglob("*.json"))
        
        for json_file in json_files:
            try:
                with open(json_file, 'r') as f:
                    data = json.load(f)
                    
                # Extract benchmark type from filename
                filename = json_file.name
                if "scheduler_comparison" in filename:
                    self._parse_scheduler_comparison(data, json_file)
                elif "benchmark_results" in filename:
                    self._parse_allocator_benchmarks(data, json_file)
                    
            except Exception as e:
                print(f"Error parsing {json_file}: {e}")
                
    def _parse_scheduler_comparison(self, data: Dict, file_path: Path):
        """Parse scheduler comparison benchmark data."""
        if "results" not in data:
            return
            
        # Extract metadata from filename
        filename = file_path.name
        compiler_match = re.search(r'gcc-(\d+\.\d+)', filename)
        timestamp_match = re.search(r'(\d{8}_\d{6})', filename)
        
        compiler = compiler_match.group(1) if compiler_match else "unknown"
        timestamp = timestamp_match.group(1) if timestamp_match else "unknown"
        
        # Extract system info
        if "context" in data:
            context = data["context"]
            self.system_info = {
                "timestamp": timestamp,
                "compiler": f"gcc-{compiler}",
                "cpu_name": context.get("cpu", {}).get("name", "Unknown"),
                "threads": context.get("cpu", {}).get("physicalCores", 0),
                "os": context.get("cpu", {}).get("operatingSystem", "Unknown"),
            }
        
        # Parse benchmark results
        for result in data["results"]:
            benchmark_name = result["name"]
            category = self._categorize_scheduler_benchmark(benchmark_name)
            
            if category not in self.benchmarks:
                self.benchmarks[category] = []
                
            self.benchmarks[category].append({
                "name": benchmark_name,
                "median_ns": result.get("median(elapsed)", 0) * 1e9,  # Convert to nanoseconds
                "operations_per_sec": 1.0 / result.get("median(elapsed)", 1e-9),
                "instructions": result.get("median(instructions)", 0),
                "cycles": result.get("median(cpucycles)", 0),
                "ipc": result.get("median(instructions)", 0) / max(result.get("median(cpucycles)", 1), 1),
                "error_percent": result.get("errorPercentage", 0),
                "iterations": result.get("iterations", 0),
                "file_path": str(file_path.relative_to(self.results_dir))
            })
            
    def _parse_allocator_benchmarks(self, data: Dict, file_path: Path):
        """Parse allocator benchmark data."""
        if "results" not in data:
            return
            
        # Parse allocator results
        for result in data["results"]:
            benchmark_name = result["name"]
            category = self._categorize_allocator_benchmark(benchmark_name)
            
            if category not in self.benchmarks:
                self.benchmarks[category] = []
                
            self.benchmarks[category].append({
                "name": benchmark_name,
                "median_ns": result.get("median(elapsed)", 0) * 1e9,
                "operations_per_sec": 1.0 / result.get("median(elapsed)", 1e-9),
                "instructions": result.get("median(instructions)", 0),
                "cycles": result.get("median(cpucycles)", 0),
                "ipc": result.get("median(instructions)", 0) / max(result.get("median(cpucycles)", 1), 1),
                "error_percent": result.get("errorPercentage", 0),
                "iterations": result.get("iterations", 0),
                "file_path": str(file_path.relative_to(self.results_dir))
            })
            
    def _categorize_scheduler_benchmark(self, name: str) -> str:
        """Categorize scheduler benchmarks."""
        if "TaskSubmission" in name:
            return "Task Submission"
        elif "ParallelFor" in name:
            return "Parallel For"
        elif "Matrix" in name:
            return "Matrix Operations"
        elif "Mixed" in name:
            return "Mixed Workload"
        elif "Throughput" in name:
            return "Task Throughput"
        elif "Nested" in name:
            return "Nested Parallel"
        elif "Reduce" in name:
            return "Parallel Reduce"
        else:
            return "Other Scheduler"
            
    def _categorize_allocator_benchmark(self, name: str) -> str:
        """Categorize allocator benchmarks."""
        if "ts_shared_linear" in name:
            return "Thread-Safe Shared Linear Allocator"
        elif "ts_thread_local" in name:
            return "Thread-Safe Thread Local Allocator"
        elif "coalescing_arena" in name:
            return "Coalescing Arena Allocator"
        else:
            return "Other Allocator"
            
    def generate_report(self):
        """Generate the markdown performance report."""
        with open(self.output_file, 'w') as f:
            self._write_header(f)
            self._write_system_info(f)
            self._write_benchmark_summary(f)
            self._write_detailed_results(f)
            self._write_footer(f)
            
    def _write_header(self, f):
        """Write the report header."""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"""# OULY Performance Report

**Generated:** {timestamp}

This report contains comprehensive performance benchmarks for the OULY library, including scheduler and allocator performance comparisons.

""")

    def _write_system_info(self, f):
        """Write system information."""
        f.write("## System Information\n\n")
        f.write("| Property | Value |\n")
        f.write("|----------|-------|\n")
        
        if self.system_info:
            for key, value in self.system_info.items():
                f.write(f"| {key.replace('_', ' ').title()} | {value} |\n")
        else:
            f.write("| Status | No system information available |\n")
        f.write("\n")
        
    def _write_benchmark_summary(self, f):
        """Write benchmark summary."""
        f.write("## Benchmark Summary\n\n")
        
        total_benchmarks = sum(len(benchmarks) for benchmarks in self.benchmarks.values())
        f.write(f"**Total Benchmarks:** {total_benchmarks}\n\n")
        
        for category, benchmarks in self.benchmarks.items():
            f.write(f"- **{category}:** {len(benchmarks)} benchmarks\n")
        f.write("\n")
        
    def _write_detailed_results(self, f):
        """Write detailed benchmark results."""
        f.write("## Detailed Results\n\n")
        
        for category, benchmarks in self.benchmarks.items():
            f.write(f"### {category}\n\n")
            
            # Create table header
            f.write("| Benchmark | Time (ns) | Ops/sec | Instructions | IPC | Error % |\n")
            f.write("|-----------|-----------|---------|--------------|-----|----------|\n")
            
            # Sort benchmarks by performance (median time)
            sorted_benchmarks = sorted(benchmarks, key=lambda x: x["median_ns"])
            
            for benchmark in sorted_benchmarks:
                name = benchmark["name"]
                time_ns = f"{benchmark['median_ns']:,.2f}"
                ops_per_sec = f"{benchmark['operations_per_sec']:,.0f}"
                instructions = f"{benchmark['instructions']:,.0f}"
                ipc = f"{benchmark['ipc']:.3f}"
                error = f"{benchmark['error_percent']:.1f}%"
                
                f.write(f"| {name} | {time_ns} | {ops_per_sec} | {instructions} | {ipc} | {error} |\n")
            
            f.write("\n")
            
            # Add performance insights
            if sorted_benchmarks:
                fastest = sorted_benchmarks[0]
                slowest = sorted_benchmarks[-1]
                
                f.write("#### Performance Insights\n\n")
                f.write(f"- **Fastest:** {fastest['name']} ({fastest['median_ns']:,.2f} ns)\n")
                f.write(f"- **Slowest:** {slowest['name']} ({slowest['median_ns']:,.2f} ns)\n")
                
                if len(sorted_benchmarks) > 1:
                    speedup = slowest['median_ns'] / fastest['median_ns']
                    f.write(f"- **Performance Ratio:** {speedup:.1f}x difference\n")
                
                f.write("\n")
        
    def _write_footer(self, f):
        """Write report footer."""
        f.write("---\n\n")
        f.write("## Methodology\n\n")
        f.write("- Benchmarks were run using nanobench with multiple iterations\n")
        f.write("- All times are median values across multiple runs\n")
        f.write("- IPC = Instructions Per Cycle\n")
        f.write("- Error percentages indicate measurement stability\n\n")
        
        f.write("## Files Analyzed\n\n")
        analyzed_files = set()
        for benchmarks in self.benchmarks.values():
            for benchmark in benchmarks:
                analyzed_files.add(benchmark["file_path"])
        
        for file_path in sorted(analyzed_files):
            f.write(f"- `{file_path}`\n")
        
        f.write(f"\n*Report generated on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}*\n")


def main():
    parser = argparse.ArgumentParser(description="Generate OULY performance report")
    parser.add_argument("--results-dir", type=Path, default=Path("results"),
                       help="Directory containing benchmark results")
    parser.add_argument("--output", type=Path, default=Path("PERFORMANCE_REPORT.md"),
                       help="Output markdown file")
    
    args = parser.parse_args()
    
    if not args.results_dir.exists():
        print(f"Results directory {args.results_dir} does not exist")
        sys.exit(1)
    
    generator = PerformanceReportGenerator(args.results_dir, args.output)
    generator.parse_json_files()
    generator.generate_report()
    
    print(f"Performance report generated: {args.output}")


if __name__ == "__main__":
    main()
