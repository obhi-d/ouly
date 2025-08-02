# OULY Performance Tracking

**Generated:** 2025-08-02 14:56:40 UTC

## 📊 Latest Performance Results

**Build Number:** 72
**Commit Hash:** 381a3e22

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 147799.40 | 6766 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 117736.30 | 8494 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 239469.20 | 4176 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87885.80 | 11378 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 100373.00 | 9963 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 231345.00 | 4323 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 65036.10 | 15376 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 84924.20 | 11775 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 80236.50 | 12463 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 240482.10 | 4158 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 267478.30 | 3739 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 353630.90 | 2828 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60039180.30 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71438419.20 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60021657.30 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 61492.60 | 16262 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 69757.90 | 14335 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 247369.20 | 4043 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 117758.50 | 8492 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 220114.20 | 4543 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219526.00 | 4555 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 225371.00 | 4437 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 240703.70 | 4154 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 173700.20 | 5757 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 165690.20 | 6035 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 231285.10 | 4324 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 447417.70 | 2235 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 446436.60 | 2240 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 327940.10 | 3049 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 50978459.00 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65299819.40 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51084679.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 152813.40 | 6544 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130927.75 | 7638 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 206.51 | 4842427 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 206.49 | 4842850 | 0.00 |

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

- **Total benchmark runs:** 80
- **Build range:** 71 - 72
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** gcc-14, gcc-14.2, gcc-4.2, clang-18
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*