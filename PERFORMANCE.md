# OULY Performance Tracking

**Generated:** 2025-08-03 07:25:06 UTC

## 📊 Latest Performance Results

**Build Number:** 78
**Commit Hash:** ec3cf5cc

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.02 | 166112957 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 207.99 | 4807923 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 193.26 | 5174376 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 230133.80 | 4345 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 111623.00 | 8959 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 245238.10 | 4078 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219852.60 | 4549 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 225844.70 | 4428 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223638.60 | 4472 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108183.50 | 9244 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 123064.40 | 8126 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 138229.80 | 7234 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 530976.00 | 1883 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 279312.90 | 3580 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241009.20 | 4149 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51076867.30 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 60850023.20 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51102284.60 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 142072.90 | 7039 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 129421.30 | 7727 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 216207.00 | 4625 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 122863.40 | 8139 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 182655.60 | 5475 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 119557.30 | 8364 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 128874.60 | 7759 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 283397.80 | 3529 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 110464.12 | 9053 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 119487.10 | 8369 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 99201.40 | 10081 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 312615.10 | 3199 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 260616.50 | 3837 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 266685.80 | 3750 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60093470.50 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71164691.00 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59965497.80 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 124839.90 | 8010 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 69335.80 | 14423 | 0.00 |

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

- **Total benchmark runs:** 320
- **Build range:** 71 - 78
- **Date range:** 2025-08-03 to 2025-08-03
- **Compilers tested:** gcc-14, clang-18, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*