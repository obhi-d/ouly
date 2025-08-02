# OULY Performance Tracking

**Generated:** 2025-08-02 04:49:47 UTC

## 📊 Latest Performance Results

**Build Number:** 63
**Commit Hash:** 0c44a80d

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.41 | 156006240 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 203.98 | 4902441 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 229.63 | 4354832 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 148352.70 | 6741 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 127716.30 | 7830 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 206899.40 | 4833 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87811.10 | 11388 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 97320.90 | 10275 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 228929.40 | 4368 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64704.30 | 15455 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 71840.50 | 13920 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79623.90 | 12559 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236994.30 | 4220 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 239941.80 | 4168 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 267579.20 | 3737 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60046641.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 65680702.50 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59963661.30 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 76860.80 | 13011 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68604.50 | 14576 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 150011.30 | 6666 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 120755.30 | 8281 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 162084.90 | 6170 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219037.70 | 4565 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 228454.30 | 4377 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223164.40 | 4481 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108029.50 | 9257 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 121331.40 | 8242 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113320.50 | 8825 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 287848.70 | 3474 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 254956.00 | 3922 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 242088.90 | 4131 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51244659.60 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 60566066.60 | 17 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51089330.40 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 127900.70 | 7819 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130095.90 | 7687 | 0.00 |

## 📈 Performance Trends

The following charts show performance trends over build numbers, 
with build number on the X-axis and performance metrics on the Y-axis.

### Overall Performance Trends

#### Execution Time Trends
![OULY Performance Trends - Elapsed Time](ouly_performance_trends_elapsed.svg)

#### Throughput Trends
![OULY Performance Trends - Throughput](ouly_performance_trends_throughput.svg)

### Allocator Performance Performance Details

#### Execution Time
![allocator_performance Performance Trend](performance_trend_allocator_performance.svg)

#### Throughput
![allocator_performance Throughput Trend](throughput_trend_allocator_performance.svg)

### General Performance Details

#### Execution Time
![general Performance Trend](performance_trend_general.svg)

#### Throughput
![general Throughput Trend](throughput_trend_general.svg)

### Scheduler Comparison Performance Details

#### Execution Time
![scheduler_comparison Performance Trend](performance_trend_scheduler_comparison.svg)

#### Throughput
![scheduler_comparison Throughput Trend](throughput_trend_scheduler_comparison.svg)

## 📋 Data Summary

- **Total benchmark runs:** 77
- **Build range:** 0 - 63
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** unknown, gcc-14.2, gcc-4.2, clang-18, gcc-14
- **Test categories:** general, scheduler_comparison, allocator_performance

---
*This report is automatically generated from benchmark results stored in the perfo branch.*