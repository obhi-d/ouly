# OULY Performance Tracking

**Generated:** 2025-08-02 10:57:01 UTC

## 📊 Latest Performance Results

**Build Number:** 67
**Commit Hash:** 080ad935

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.41 | 156006240 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 217.91 | 4589051 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.11 | 163666121 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 205.38 | 4869023 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 151019.50 | 6622 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 134824.30 | 7417 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 161570.40 | 6189 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219068.00 | 4565 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 228453.50 | 4377 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 236892.30 | 4221 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 197893.90 | 5053 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 120536.50 | 8296 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113454.30 | 8814 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 321033.70 | 3115 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 371974.10 | 2688 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 240906.80 | 4151 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 52139196.50 | 19 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65868193.40 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51048985.70 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 134385.50 | 7441 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 132547.00 | 7544 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 128808.20 | 7763 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 122801.80 | 8143 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 238813.10 | 4187 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88076.80 | 11354 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99249.80 | 10076 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 231209.80 | 4325 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64885.40 | 15412 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 73370.40 | 13629 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79516.80 | 12576 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236231.10 | 4233 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 240176.50 | 4164 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 347566.60 | 2877 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60282221.90 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71207200.20 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60059465.30 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 61505.20 | 16259 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68379.00 | 14624 | 0.00 |

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

- **Total benchmark runs:** 160
- **Build range:** 63 - 67
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*