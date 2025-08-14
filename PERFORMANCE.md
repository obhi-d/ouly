# OULY Performance Tracking

**Generated:** 2025-08-14 23:11:22 UTC

## 📊 Latest Performance Results

**Build Number:** 91
**Commit Hash:** e5bd687e

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 158769.70 | 6298 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 124094.75 | 8058 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 179092.70 | 5584 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 362570.00 | 2758 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 271868.00 | 3678 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 281096.20 | 3558 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 171846.10 | 5819 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 172786.17 | 5788 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 140542.67 | 7115 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 624560.90 | 1601 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 452989.40 | 2208 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 322162.90 | 3104 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51107705.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65259405.40 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51122267.40 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 116313.40 | 8597 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 129549.63 | 7719 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 182782.50 | 5471 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 127282.00 | 7857 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 244835.00 | 4084 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87930.70 | 11373 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99305.82 | 10070 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 229241.90 | 4362 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64953.00 | 15396 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 73584.00 | 13590 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79809.70 | 12530 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236582.70 | 4227 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 247107.30 | 4047 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 321207.30 | 3113 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60135563.00 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 65605630.10 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60011061.90 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64436.23 | 15519 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68524.70 | 14593 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.11 | 243309002 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.88 | 4880906 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 207.18 | 4826721 | 0.00 |

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

- **Total benchmark runs:** 780
- **Build range:** 71 - 91
- **Date range:** 2025-08-14 to 2025-08-14
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*