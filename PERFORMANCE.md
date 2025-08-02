# OULY Performance Tracking

**Generated:** 2025-08-02 12:16:26 UTC

## 📊 Latest Performance Results

**Build Number:** 70
**Commit Hash:** 040ca374

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 146087.20 | 6845 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 131888.50 | 7582 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 159081.80 | 6286 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88058.90 | 11356 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 98588.70 | 10143 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230600.40 | 4337 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64608.70 | 15478 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 72391.40 | 13814 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79495.58 | 12579 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 238462.30 | 4194 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 240404.00 | 4160 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 291922.00 | 3426 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60019899.30 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 65666838.60 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59980382.40 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 76978.10 | 12991 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68830.80 | 14528 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 127303.90 | 7855 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 116979.70 | 8548 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 242904.00 | 4117 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 224816.30 | 4448 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 225909.30 | 4427 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223546.00 | 4473 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 116183.20 | 8607 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 121806.62 | 8210 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 115591.10 | 8651 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 327317.10 | 3055 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 368966.50 | 2710 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 312965.30 | 3195 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51042963.00 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 62103173.40 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51064820.40 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 128251.70 | 7797 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 131571.25 | 7600 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 207.08 | 4829052 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.30 | 158730159 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 205.89 | 4856962 | 0.00 |

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

### Scheduler Comparison Performance Details

#### Execution Time
![scheduler_comparison Performance Trend](performance_trend_scheduler_comparison.svg)

#### Throughput
![scheduler_comparison Throughput Trend](throughput_trend_scheduler_comparison.svg)

## 📋 Data Summary

- **Total benchmark runs:** 40
- **Build range:** 70 - 70
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** gcc-4.2, gcc-14, clang-18, gcc-14.2
- **Test categories:** scheduler_comparison, allocator_performance

---
*This report is automatically generated from benchmark results stored in the perfo branch.*