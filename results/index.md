# Ouly Performance Tracking

This branch contains historical performance benchmark results.

## Latest Results

- **Timestamp**: 2025-07-23_08-45-43
- **Commit**: 9c47d515
- **Branch**: main

## Results Structure

Each directory contains benchmark results for a specific build:
- GCC 14 Release build
- Clang 19 Release build

## Tracked Components

- **ts_shared_linear_allocator**: Thread-safe shared linear allocator performance
- **ts_thread_local_allocator**: Thread-local allocator performance  
- **coalescing_arena_allocator**: Coalescing arena allocator performance
- **scheduler**: Task scheduler performance (task submission, parallel_for, work stealing)

## Usage

Results are stored in JSON format and can be processed with analysis tools.
Each benchmark includes median, min, max, mean times and relative error.
