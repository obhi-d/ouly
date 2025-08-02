# OULY Performance Tracking

**Generated:** 2025-08-02 20:06:40 UTC

## 📊 Latest Performance Results

**Build Number:** 74
**Commit Hash:** 08c4c46c

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 205.98 | 4854918 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 218.71 | 4572265 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 134958.30 | 7410 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 118218.10 | 8459 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 248665.00 | 4021 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219085.60 | 4564 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 228851.90 | 4370 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223877.60 | 4467 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 120504.30 | 8298 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 125437.12 | 7972 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113463.20 | 8813 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 311512.50 | 3210 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 273088.70 | 3662 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241170.00 | 4146 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51129068.40 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65345930.40 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51086300.90 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 137700.40 | 7262 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130057.10 | 7689 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 149834.10 | 6674 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 119093.60 | 8397 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 161610.10 | 6188 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87786.10 | 11391 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99368.70 | 10064 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 231451.60 | 4321 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 130762.40 | 7647 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 73174.57 | 13666 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79425.40 | 12590 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 237409.80 | 4212 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 246694.10 | 4054 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 345975.70 | 2890 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60151515.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71140919.70 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60042108.60 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 63517.80 | 15744 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 69315.60 | 14427 | 0.00 |

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

- **Total benchmark runs:** 160
- **Build range:** 71 - 74
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*