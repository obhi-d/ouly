# OULY Performance Tracking

**Generated:** 2025-08-14 01:18:33 UTC

## 📊 Latest Performance Results

**Build Number:** 89
**Commit Hash:** 6517a10b

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 220.91 | 4526730 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.02 | 166112957 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 193.86 | 5158362 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 146174.30 | 6841 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 118778.30 | 8419 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 185077.90 | 5403 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 442479.40 | 2260 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 271339.50 | 3685 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 252445.40 | 3961 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 165625.60 | 6038 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 176199.30 | 5675 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113464.40 | 8813 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 602732.90 | 1659 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 452579.20 | 2210 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 268187.60 | 3729 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 50971152.10 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65256887.10 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51058384.90 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 130998.10 | 7634 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 131842.70 | 7585 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 182230.50 | 5488 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 87942.30 | 11371 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 154473.50 | 6474 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87671.80 | 11406 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 101137.90 | 9887 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 229800.20 | 4352 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64645.80 | 15469 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 73152.60 | 13670 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79235.00 | 12621 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236232.30 | 4233 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 239872.20 | 4169 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 349158.20 | 2864 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60022975.90 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71051712.10 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59999673.30 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 125053.60 | 7997 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 69124.20 | 14467 | 0.00 |

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

- **Total benchmark runs:** 700
- **Build range:** 71 - 89
- **Date range:** 2025-08-14 to 2025-08-14
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*