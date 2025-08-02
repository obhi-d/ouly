# OULY Performance Tracking

**Generated:** 2025-08-02 11:37:17 UTC

## 📊 Latest Performance Results

**Build Number:** 68
**Commit Hash:** 22af8415

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 193.56 | 5166357 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.02 | 166112957 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 207.99 | 4807923 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 137475.20 | 7274 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 120668.80 | 8287 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 239462.10 | 4176 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 223736.80 | 4470 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 223420.20 | 4476 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223637.60 | 4472 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 120901.30 | 8271 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 121715.50 | 8216 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 112837.20 | 8862 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 253233.80 | 3949 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 256423.80 | 3900 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241788.50 | 4136 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51152698.20 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65324769.50 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51084806.40 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 133912.83 | 7468 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 129398.00 | 7728 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 171823.83 | 5820 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 118116.40 | 8466 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 151311.30 | 6609 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87703.60 | 11402 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99545.70 | 10046 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230426.00 | 4340 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64886.10 | 15412 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 72829.80 | 13731 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 80107.50 | 12483 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 239211.30 | 4180 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 239402.60 | 4177 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 371155.90 | 2694 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60052483.10 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 66235673.70 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60044705.60 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 67341.60 | 14850 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 73060.30 | 13687 | 0.00 |

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
- **Build range:** 68 - 68
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** clang-18, gcc-14.2, gcc-14, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*