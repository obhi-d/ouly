# OULY Performance Tracking

**Generated:** 2025-08-14 22:04:53 UTC

## 📊 Latest Performance Results

**Build Number:** 90
**Commit Hash:** ce6245fa

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.10 | 163934426 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 219.91 | 4547315 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 222.52 | 4493978 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 227510.10 | 4395 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 139553.70 | 7166 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 253256.10 | 3949 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88424.20 | 11309 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99206.40 | 10080 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230027.80 | 4347 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 84436.80 | 11843 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 119834.90 | 8345 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 99471.80 | 10053 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 736436.80 | 1358 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 441585.90 | 2265 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 284127.80 | 3520 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60183172.10 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71213605.90 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60006844.50 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 63584.80 | 15727 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 69544.90 | 14379 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 155971.80 | 6411 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 127030.70 | 7872 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 226299.40 | 4419 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219673.00 | 4552 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 226769.30 | 4410 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223548.30 | 4473 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108355.80 | 9229 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 131098.20 | 7628 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 141988.70 | 7043 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 313484.60 | 3190 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 243872.40 | 4101 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241899.70 | 4134 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51027725.40 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65342561.40 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51057920.00 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 131152.50 | 7625 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130701.50 | 7651 | 0.00 |

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

- **Total benchmark runs:** 740
- **Build range:** 71 - 90
- **Date range:** 2025-08-14 to 2025-08-14
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*