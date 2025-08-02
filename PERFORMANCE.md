# OULY Performance Tracking

**Generated:** 2025-08-02 09:57:48 UTC

## 📊 Latest Performance Results

**Build Number:** 66
**Commit Hash:** 4f95f8ee

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.41 | 156006240 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.20 | 238095238 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 230.83 | 4332193 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 222.92 | 4485914 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 148223.70 | 6747 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 128391.70 | 7789 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 231524.50 | 4319 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88181.00 | 11340 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99581.10 | 10042 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 231162.80 | 4326 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 74306.20 | 13458 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 72646.10 | 13765 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 80583.17 | 12410 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 450117.00 | 2222 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 437250.00 | 2287 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 351276.10 | 2847 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60185584.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 65576563.90 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59995068.80 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 83026.40 | 12044 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 67771.00 | 14756 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 143797.20 | 6954 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 121785.00 | 8211 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 244859.30 | 4084 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 220032.90 | 4545 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 230086.70 | 4346 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 224033.40 | 4464 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108735.60 | 9197 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 122910.10 | 8136 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113345.20 | 8823 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 576782.70 | 1734 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 249903.80 | 4002 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 243251.40 | 4111 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51087419.20 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 60911701.80 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51123053.70 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 141876.70 | 7048 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 144557.70 | 6918 | 0.00 |

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
- **Build range:** 66 - 66
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** clang-18, gcc-14, gcc-4.2, gcc-14.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*