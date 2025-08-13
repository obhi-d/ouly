# OULY Performance Tracking

**Generated:** 2025-08-13 17:02:31 UTC

## 📊 Latest Performance Results

**Build Number:** 84
**Commit Hash:** e9db0763

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 149293.10 | 6698 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 111695.25 | 8953 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 230644.30 | 4336 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219983.30 | 4546 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 231508.90 | 4319 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 226044.70 | 4424 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108949.43 | 9179 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 122411.50 | 8169 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113731.20 | 8793 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 335320.40 | 2982 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 453503.00 | 2205 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 315715.40 | 3167 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 50993142.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65326158.70 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51150625.60 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 170531.20 | 5864 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130590.90 | 7658 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 203.98 | 4902441 | 0.00 |

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

- **Total benchmark runs:** 540
- **Build range:** 71 - 84
- **Date range:** 2025-08-13 to 2025-08-13
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*