# OULY Performance Tracking

**Generated:** 2025-08-13 22:07:41 UTC

## 📊 Latest Performance Results

**Build Number:** 86
**Commit Hash:** 8c7aae3a

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.20 | 238095238 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.59 | 4887824 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.11 | 163666121 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 194.73 | 5135206 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 141814.70 | 7051 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 123167.90 | 8119 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 246951.00 | 4049 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219155.10 | 4563 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 228060.70 | 4385 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223275.70 | 4479 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108712.90 | 9199 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 168338.30 | 5940 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 140208.70 | 7132 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 249154.00 | 4014 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 249912.50 | 4001 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241143.10 | 4147 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51088044.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65378750.60 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51115324.50 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 117492.50 | 8511 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 132773.70 | 7532 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 146253.60 | 6837 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 124171.10 | 8053 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 164492.80 | 6079 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88306.70 | 11324 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 98896.60 | 10112 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 231261.10 | 4324 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64574.20 | 15486 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 72695.40 | 13756 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79433.10 | 12589 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 239356.30 | 4178 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 238654.00 | 4190 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 349088.30 | 2865 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60095964.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71226577.20 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59983616.30 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 74618.00 | 13402 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 69974.21 | 14291 | 0.00 |

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

- **Total benchmark runs:** 620
- **Build range:** 71 - 86
- **Date range:** 2025-08-13 to 2025-08-13
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*