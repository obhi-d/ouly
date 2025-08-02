# OULY Performance Tracking

**Generated:** 2025-08-02 12:05:42 UTC

## 📊 Latest Performance Results

**Build Number:** 69
**Commit Hash:** ee777f37

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 135091.50 | 7402 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 118040.50 | 8472 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 248137.50 | 4030 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 224635.40 | 4452 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 224725.50 | 4450 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 222993.40 | 4484 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 120325.80 | 8311 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 123480.70 | 8098 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113104.20 | 8841 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 297576.20 | 3360 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 249360.80 | 4010 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241570.20 | 4140 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51066729.30 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 70023913.20 | 14 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51065290.30 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 126990.20 | 7875 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 144271.60 | 6931 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 140621.00 | 7111 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 124794.30 | 8013 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 214944.00 | 4652 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87922.50 | 11374 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99485.10 | 10052 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 231352.60 | 4322 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64923.40 | 15403 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 74066.50 | 13501 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 80036.62 | 12494 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 238502.20 | 4193 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 241690.20 | 4138 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 298148.50 | 3354 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60202469.00 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 65932123.50 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59962067.90 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 79552.80 | 12570 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 69112.30 | 14469 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.21 | 161030596 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 194.70 | 5136131 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 208.09 | 4805613 | 0.00 |

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
- **Build range:** 69 - 69
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** gcc-14.2, clang-18, gcc-14, gcc-4.2
- **Test categories:** scheduler_comparison, allocator_performance

---
*This report is automatically generated from benchmark results stored in the perfo branch.*