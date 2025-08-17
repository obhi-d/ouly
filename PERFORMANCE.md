# OULY Performance Tracking

**Generated:** 2025-08-17 04:15:58 UTC

## 📊 Latest Performance Results

**Build Number:** 94
**Commit Hash:** 6979ae2c

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 218.51 | 4576450 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.11 | 163666121 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 195.77 | 5108035 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 150573.60 | 6641 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 137860.00 | 7254 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 242582.20 | 4122 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88080.20 | 11353 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 98716.00 | 10130 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230620.90 | 4336 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64838.80 | 15423 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 74368.70 | 13447 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79167.60 | 12631 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236703.30 | 4225 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 239616.70 | 4173 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 263722.60 | 3792 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60087994.40 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71154776.50 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59976838.20 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 60428.60 | 16548 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68943.50 | 14505 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 138571.80 | 7216 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 126673.60 | 7894 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 165573.20 | 6040 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219143.30 | 4563 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 227539.00 | 4395 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223966.40 | 4465 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108716.00 | 9198 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 128874.70 | 7759 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113270.44 | 8828 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 247940.10 | 4033 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 250580.00 | 3991 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 255503.30 | 3914 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51130568.00 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 60772971.20 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51051512.40 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 130297.40 | 7675 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 133392.10 | 7497 | 0.00 |

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

- **Total benchmark runs:** 880
- **Build range:** 71 - 94
- **Date range:** 2025-08-17 to 2025-08-17
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*