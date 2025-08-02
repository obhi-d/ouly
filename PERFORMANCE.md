# OULY Performance Tracking

**Generated:** 2025-08-02 18:28:09 UTC

## 📊 Latest Performance Results

**Build Number:** 76
**Commit Hash:** 5ab3cce2

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 218.20 | 4582951 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.11 | 163666121 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 196.27 | 5095022 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 139193.80 | 7184 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 112839.60 | 8862 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 251238.00 | 3980 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 378350.30 | 2643 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 227647.00 | 4393 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223426.10 | 4476 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 127138.40 | 7865 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 122139.00 | 8187 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 112892.00 | 8858 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 253194.60 | 3950 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 248460.80 | 4025 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 242521.70 | 4123 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51617242.80 | 19 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 60649445.90 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51036842.30 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 150297.50 | 6653 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130652.90 | 7654 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 147034.10 | 6801 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 120716.10 | 8284 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 157949.40 | 6331 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 91106.90 | 10976 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 119486.70 | 8369 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 278496.30 | 3591 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 133265.50 | 7504 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 93500.30 | 10695 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 98836.30 | 10118 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 427474.00 | 2339 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 583640.30 | 1713 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 351765.90 | 2843 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60284934.60 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71147913.30 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59997623.40 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 61951.60 | 16142 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 67337.60 | 14851 | 0.00 |

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

- **Total benchmark runs:** 240
- **Build range:** 71 - 76
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** gcc-14, clang-18, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*