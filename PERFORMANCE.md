# OULY Performance Tracking

**Generated:** 2025-08-05 04:43:16 UTC

## 📊 Latest Performance Results

**Build Number:** 80
**Commit Hash:** 99f9ecbc

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 194.56 | 5139803 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 222.21 | 4500248 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 149923.70 | 6670 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 133410.90 | 7496 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 160745.00 | 6221 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219249.50 | 4561 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 231563.60 | 4318 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 224211.80 | 4460 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 110032.10 | 9088 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 123121.50 | 8122 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113482.50 | 8812 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 251109.20 | 3982 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 363825.20 | 2749 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 245779.20 | 4069 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 52380149.60 | 19 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 60840628.90 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51026807.60 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 150269.40 | 6655 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 129501.40 | 7722 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 151631.10 | 6595 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 120830.70 | 8276 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 249328.80 | 4011 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87817.40 | 11387 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99287.60 | 10072 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230949.60 | 4330 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 65694.20 | 15222 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 72272.40 | 13837 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79873.60 | 12520 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 239099.80 | 4182 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 242068.30 | 4131 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 348408.10 | 2870 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60090611.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 70976214.80 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59982999.30 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 60592.70 | 16504 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 69219.80 | 14447 | 0.00 |

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

- **Total benchmark runs:** 400
- **Build range:** 71 - 80
- **Date range:** 2025-08-05 to 2025-08-05
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*