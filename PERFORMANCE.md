# OULY Performance Tracking

**Generated:** 2025-08-12 05:20:50 UTC

## 📊 Latest Performance Results

**Build Number:** 82
**Commit Hash:** 10d42ca0

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.20 | 238095238 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.99 | 4878287 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 7.62 | 131233596 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 195.67 | 5110645 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 156174.70 | 6403 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 119241.70 | 8386 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 221383.40 | 4517 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 218913.00 | 4568 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 229285.30 | 4361 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223784.00 | 4469 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108314.30 | 9232 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 124944.40 | 8004 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 112934.00 | 8855 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 448474.70 | 2230 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 321315.20 | 3112 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241952.90 | 4133 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51171991.60 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65268121.90 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51066233.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 157516.00 | 6349 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130375.50 | 7670 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 146759.70 | 6814 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 127632.90 | 7835 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 234701.30 | 4261 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87743.42 | 11397 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 98654.70 | 10136 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230804.00 | 4333 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64951.60 | 15396 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 72618.90 | 13771 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79215.20 | 12624 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 238794.00 | 4188 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 239206.80 | 4180 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 347539.50 | 2877 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60339218.60 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71151292.20 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59943010.50 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 78397.64 | 12755 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 67595.20 | 14794 | 0.00 |

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

- **Total benchmark runs:** 480
- **Build range:** 71 - 82
- **Date range:** 2025-08-12 to 2025-08-12
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*