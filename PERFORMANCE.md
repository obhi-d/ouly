# OULY Performance Tracking

**Generated:** 2025-08-02 12:41:18 UTC

## 📊 Latest Performance Results

**Build Number:** 71
**Commit Hash:** c050fff9

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 5.61 | 178253119 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 3.91 | 255754476 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 205.08 | 4876146 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.32 | 158227848 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.42 | 4891793 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 126209.10 | 7923 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 116116.30 | 8612 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 163095.10 | 6131 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 223901.00 | 4466 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 224047.20 | 4463 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 224244.60 | 4459 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108207.40 | 9242 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 118721.10 | 8423 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 111379.40 | 8978 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 251493.50 | 3976 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 250408.50 | 3993 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 239941.00 | 4168 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 50860632.10 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 63637348.20 | 16 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 50346300.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 235533.70 | 4246 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 132064.10 | 7572 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 142321.10 | 7026 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 120508.20 | 8298 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 154965.90 | 6453 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88391.00 | 11313 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 99035.90 | 10097 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230296.30 | 4342 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64954.90 | 15395 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 73034.00 | 13692 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 80154.50 | 12476 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 239075.70 | 4183 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 243740.50 | 4103 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 347341.00 | 2879 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60173519.90 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 71352189.20 | 14 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59988618.50 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 78727.70 | 12702 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68783.10 | 14538 | 0.00 |

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

- **Total benchmark runs:** 40
- **Build range:** 71 - 71
- **Date range:** 2025-08-02 to 2025-08-02
- **Compilers tested:** gcc-14, clang-18, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*