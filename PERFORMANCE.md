# OULY Performance Tracking

**Generated:** 2025-08-17 02:13:40 UTC

## 📊 Latest Performance Results

**Build Number:** 93
**Commit Hash:** 78ac2403

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 140141.00 | 7136 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 109646.30 | 9120 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 247397.00 | 4042 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 222980.40 | 4485 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 232500.10 | 4301 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 258000.70 | 3876 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 218341.70 | 4580 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 164518.50 | 6078 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 114489.30 | 8734 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 499409.30 | 2002 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 417758.30 | 2394 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 353198.00 | 2831 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51211614.30 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 65394423.00 | 15 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51068610.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 130836.70 | 7643 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 133593.90 | 7485 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 204.58 | 4888063 | 0.00 |

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

- **Total benchmark runs:** 840
- **Build range:** 71 - 93
- **Date range:** 2025-08-17 to 2025-08-17
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*