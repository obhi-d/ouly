# OULY Performance Tracking

**Generated:** 2025-08-02 15:50:52 UTC

## 📊 Latest Performance Results

**Build Number:** 73
**Commit Hash:** 63af0e7b

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.41 | 156006240 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 193.06 | 5179737 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 205.68 | 4861921 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 225285.60 | 4439 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 123902.50 | 8071 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 154131.20 | 6488 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88221.50 | 11335 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99368.40 | 10064 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230684.80 | 4335 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64904.80 | 15407 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 72038.20 | 13882 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79889.80 | 12517 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 237043.70 | 4219 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 241838.70 | 4135 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 348297.40 | 2871 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60084130.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 65887881.20 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60036196.80 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 78628.90 | 12718 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68874.50 | 14519 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 176793.10 | 5656 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 114545.90 | 8730 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 239577.30 | 4174 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 220975.60 | 4525 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 227201.20 | 4401 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 260943.30 | 3832 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 174094.10 | 5744 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 148348.00 | 6741 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 115640.90 | 8647 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 425035.60 | 2353 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 438631.10 | 2280 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 467616.20 | 2139 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51114707.60 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 60738722.40 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51129581.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 147625.83 | 6774 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130839.40 | 7643 | 0.00 |

## 📈 Performance Trends

The following charts show performance trends over build numbers, 
with build number on the X-axis and performance metrics on the Y-axis.

### Overall Performance Trends

#### Execution Time Trends
![OULY Performance Trends - Elapsed Time](ouly_performance_trends_elapsed.svg)

#### Throughput Trends
![OULY Performance Trends - Throughput](ouly_performance_trends_throughput.svg)

### Allocator Performance Performance Details

#### Execution Time
![allocator_performance Performance Trend](performance_trend_allocator_performance.svg)

#### Throughput
![allocator_performance Throughput Trend](throughput_trend_allocator_performance.svg)

### Scheduler Comparison Performance Details

#### Execution Time
![scheduler_comparison Performance Trend](performance_trend_scheduler_comparison.svg)

#### Throughput
![scheduler_comparison Throughput Trend](throughput_trend_scheduler_comparison.svg)

## 📋 Data Summary

- **Total benchmark runs:** 120
- **Build range:** 71 - 73
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** gcc-14, clang-18, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*