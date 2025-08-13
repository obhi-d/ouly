# OULY Performance Tracking

**Generated:** 2025-08-13 22:54:35 UTC

## 📊 Latest Performance Results

**Build Number:** 88
**Commit Hash:** 8928ec1a

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.21 | 161030596 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 220.12 | 4542977 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 189.46 | 5278159 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 270515.30 | 3697 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 116317.75 | 8597 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 236714.10 | 4225 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219011.90 | 4566 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 228421.60 | 4378 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223387.00 | 4477 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108374.90 | 9227 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 168157.80 | 5947 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 138545.00 | 7218 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 291613.80 | 3429 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 249125.20 | 4014 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 243557.80 | 4106 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51084723.80 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 60627938.50 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51059060.40 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 128370.30 | 7790 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130831.90 | 7643 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 140440.10 | 7120 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 116647.67 | 8573 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 172728.40 | 5789 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88020.20 | 11361 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 100507.50 | 9950 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230309.90 | 4342 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64815.92 | 15428 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 73234.60 | 13655 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79610.92 | 12561 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 238982.10 | 4184 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 241420.60 | 4142 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 347612.80 | 2877 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 59943221.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71224552.10 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59710580.10 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 59888.80 | 16698 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 63076.70 | 15854 | 0.00 |

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

- **Total benchmark runs:** 660
- **Build range:** 71 - 88
- **Date range:** 2025-08-13 to 2025-08-13
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*