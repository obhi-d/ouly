# OULY Performance Tracking

**Generated:** 2025-08-05 06:46:58 UTC

## 📊 Latest Performance Results

**Build Number:** 81
**Commit Hash:** aea35232

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 223.82 | 4467876 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.41 | 156006240 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 218.91 | 4568087 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 140562.60 | 7114 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 121280.70 | 8245 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 252180.20 | 3965 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219222.60 | 4562 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 228872.70 | 4369 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223610.90 | 4472 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 109909.40 | 9098 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 124137.00 | 8056 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 112875.90 | 8859 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 287169.60 | 3482 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 389210.20 | 2569 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241916.00 | 4134 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51281185.70 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65312894.50 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51114909.10 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 126630.90 | 7897 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 131599.10 | 7599 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 152332.40 | 6565 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 128221.50 | 7799 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 153707.80 | 6506 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88091.33 | 11352 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 100007.00 | 9999 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230806.30 | 4333 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64617.20 | 15476 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 73613.90 | 13584 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79417.70 | 12592 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 237363.50 | 4213 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 237966.60 | 4202 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 346169.80 | 2889 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60020347.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71196208.50 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60025255.70 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 71190.40 | 14047 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68369.62 | 14626 | 0.00 |

## 📈 Performance Trends

The following charts show performance trends over build numbers, 
with build number on the X-axis and performance metrics on the Y-axis.
Performance is grouped by measurement type.

### Coalescingarenaallocdealloc Performance

#### Execution Time
![Coalescingarenaallocdealloc Performance Trend](performance_trend_coalescingarenaallocdealloc.svg)

#### Throughput
![Coalescingarenaallocdealloc Throughput Trend](throughput_trend_coalescingarenaallocdealloc.svg)


### Matrixops Performance

#### Execution Time
![Matrixops Performance Trend](performance_trend_matrixops.svg)

#### Throughput
![Matrixops Throughput Trend](throughput_trend_matrixops.svg)


### Mixedworkload Performance

#### Execution Time
![Mixedworkload Performance Trend](performance_trend_mixedworkload.svg)

#### Throughput
![Mixedworkload Throughput Trend](throughput_trend_mixedworkload.svg)


### Nestedparallel Performance

#### Execution Time
![Nestedparallel Performance Trend](performance_trend_nestedparallel.svg)

#### Throughput
![Nestedparallel Throughput Trend](throughput_trend_nestedparallel.svg)


### Parallelforvectorops Performance

#### Execution Time
![Parallelforvectorops Performance Trend](performance_trend_parallelforvectorops.svg)

#### Throughput
![Parallelforvectorops Throughput Trend](throughput_trend_parallelforvectorops.svg)


### Tasksubmission Performance

#### Execution Time
![Tasksubmission Performance Trend](performance_trend_tasksubmission.svg)

#### Throughput
![Tasksubmission Throughput Trend](throughput_trend_tasksubmission.svg)


### Taskthroughput Performance

#### Execution Time
![Taskthroughput Performance Trend](performance_trend_taskthroughput.svg)

#### Throughput
![Taskthroughput Throughput Trend](throughput_trend_taskthroughput.svg)


### Tssharedlinearsinglethread Performance

#### Execution Time
![Tssharedlinearsinglethread Performance Trend](performance_trend_tssharedlinearsinglethread.svg)

#### Throughput
![Tssharedlinearsinglethread Throughput Trend](throughput_trend_tssharedlinearsinglethread.svg)


### Tsthreadlocalsinglethread Performance

#### Execution Time
![Tsthreadlocalsinglethread Performance Trend](performance_trend_tsthreadlocalsinglethread.svg)

#### Throughput
![Tsthreadlocalsinglethread Throughput Trend](throughput_trend_tsthreadlocalsinglethread.svg)


## 📋 Data Summary

- **Total benchmark runs:** 440
- **Build range:** 71 - 81
- **Date range:** 2025-08-05 to 2025-08-05
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*