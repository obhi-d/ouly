# Ouly Performance Tracking

This branch contains historical performance benchmark results with detailed JSON data.

## Latest Results

- **Timestamp**: 2025-07-23_20-58-55
- **Commit**: 78577a99
- **Branch**: main

## Benchmark Data Structure

Each directory contains:
- **JSON files**: Detailed nanobench results with timing data, iterations, and statistics
- **TXT files**: Human-readable console output
- **SUMMARY.md**: Extracted key metrics and overview

## Tracked Components

- **ts_shared_linear_allocator**: Thread-safe shared linear allocator performance
- **ts_thread_local_allocator**: Thread-local allocator performance (with our recent optimizations!)
- **coalescing_arena_allocator**: Coalescing arena allocator performance
- **scheduler**: Task scheduler performance (task submission, parallel_for, work stealing)

## JSON Data Format

The JSON files follow nanobench's format with:
- Individual measurement data points
- Statistical summaries (median, percentile errors)
- Timing in seconds (multiply by 1e9 for nanoseconds)
- Performance counters (when available)

## Usage

Results can be processed with the provided visualization scripts or any JSON-compatible tool.
Each result includes metadata like compiler, build type, and platform information.
