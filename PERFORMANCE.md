# OULY Performance Tracking

**Generated:** 2025-08-03 22:35:32 UTC

## 📊 Latest Performance Results

**Build Number:** 79
**Commit Hash:** bad25b40

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 206.13 | 4851344 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 205.19 | 4873532 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 145497.60 | 6873 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 114989.20 | 8696 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 180234.00 | 5548 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 444133.10 | 2252 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 332296.80 | 3009 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 264591.50 | 3779 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 171375.30 | 5835 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 214581.50 | 4660 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 141556.10 | 7064 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 454646.90 | 2200 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 343910.60 | 2908 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241054.20 | 4148 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51023578.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65320754.00 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51073457.20 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 143363.60 | 6975 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 132013.12 | 7575 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 257919.80 | 3877 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 123786.70 | 8078 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 167506.50 | 5970 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87856.40 | 11382 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99776.70 | 10022 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 229803.00 | 4352 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 135135.90 | 7400 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 75402.10 | 13262 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79914.60 | 12513 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 241095.30 | 4148 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 354043.70 | 2825 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 352328.50 | 2838 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60213369.50 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71064610.10 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60000745.00 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 60276.80 | 16590 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68755.33 | 14544 | 0.00 |

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

- **Total benchmark runs:** 360
- **Build range:** 71 - 79
- **Date range:** 2025-08-03 to 2025-08-03
- **Compilers tested:** gcc-14, clang-18, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*