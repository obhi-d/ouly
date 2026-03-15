# OULY Performance Tracking

**Generated:** 2026-03-15 20:01:15 UTC

## 📊 Latest Performance Results

**Build Number:** 126
**Commit Hash:** 20aa652b

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.90 | 256410256 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 205.78 | 4859559 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.51 | 221729490 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 197.67 | 5058937 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 629486.30 | 1589 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 604678.10 | 1654 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 147459.10 | 6782 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 122410.43 | 8169 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 204536.20 | 4889 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 166467.50 | 6007 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50073174.90 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50143264.90 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 8333971.20 | 120 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 8278486.10 | 121 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 561371.40 | 1781 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 556434.20 | 1797 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 145917.83 | 6853 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 138704.80 | 7210 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 147670.29 | 6772 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 147034.83 | 6801 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50064022.20 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50125534.80 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 9721513.10 | 103 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 9404747.60 | 106 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100088462.40 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41167725.60 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100100949.30 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41128547.00 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100200510.00 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1831593.50 | 546 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100281744.50 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1167030.20 | 857 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50061842.00 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12699.60 | 78743 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50116898.70 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12668.60 | 78935 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 121942.70 | 8201 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 20330.18 | 49188 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100080366.50 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48277848.90 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100099780.90 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48146706.20 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100223031.60 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1792572.10 | 558 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100259840.50 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1152652.20 | 868 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50069481.00 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15289.50 | 65404 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50123058.40 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 15109.44 | 66184 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 260401.90 | 3840 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 20364.90 | 49104 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 246878.80 | 4051 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 172464.50 | 5798 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 176890.70 | 5653 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 334751.10 | 2987 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 350211.90 | 2855 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 327444.50 | 3054 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 170807.40 | 5855 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 169671.20 | 5894 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 121236.71 | 8248 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 396304.60 | 2523 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 428319.10 | 2335 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 390575.90 | 2560 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51562079.20 | 19 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51032341.40 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51098697.00 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 118487.30 | 8440 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 119735.12 | 8352 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 147839.40 | 6764 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 144691.50 | 6911 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 160708.50 | 6222 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88509.90 | 11298 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 88956.70 | 11241 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230582.60 | 4337 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 135479.50 | 7381 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 70523.30 | 14180 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 100832.90 | 9917 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 797389.60 | 1254 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 711683.80 | 1405 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 398291.00 | 2511 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60035492.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60106389.10 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60130898.70 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 65197.20 | 15338 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64063.10 | 15610 | 0.00 |

## 📈 Performance Trends

The following charts show performance trends over build numbers, 
with build number on the X-axis and performance metrics on the Y-axis.
Performance is grouped by measurement type.

### Coalescingarenaallocdealloc Performance

**Coalescing Arena Allocator Performance**

Measures the performance of the coalescing arena allocator, which manages memory by coalescing adjacent free blocks to reduce fragmentation. This benchmark tests allocation and deallocation operations using the arena-based memory management system. Lower times indicate better allocator performance.

#### Execution Time
![Coalescingarenaallocdealloc Performance Trend](performance_trend_coalescingarenaallocdealloc.svg)

#### Throughput
![Coalescingarenaallocdealloc Throughput Trend](throughput_trend_coalescingarenaallocdealloc.svg)


### Coroutine Creation Performance

**Coroutine Creation Overhead**

Measures the fundamental cost of creating coroutine objects compared to regular function calls. This includes coroutine frame allocation, initial suspension setup, and coroutine handle creation. Tests both V1 and V2 scheduler implementations to identify any scheduler-specific overhead in coroutine management. Lower creation times enable efficient use of coroutines for fine-grained asynchronous operations.

#### Execution Time
![Coroutine Creation Performance Trend](performance_trend_coroutinecreation.svg)

#### Throughput
![Coroutine Creation Throughput Trend](throughput_trend_coroutinecreation.svg)


### Coroutine Fan Out In Performance

**Coroutine Fan-Out/Fan-In Patterns**

Evaluates the performance of scatter-gather patterns implemented using coroutines compared to traditional parallel_for constructs and TBB equivalents. Tests scenarios where work is distributed across multiple coroutines and results are collected back. Critical for understanding coroutine performance in data-parallel and map-reduce style algorithms.

#### Execution Time
![Coroutine Fan Out In Performance Trend](performance_trend_coroutinefanoutin.svg)

#### Throughput
![Coroutine Fan Out In Throughput Trend](throughput_trend_coroutinefanoutin.svg)


### Coroutine Memory Performance

**Coroutine Memory Overhead**

Evaluates the memory allocation overhead of creating large numbers of coroutines simultaneously. Tests coroutine frame allocation patterns, memory fragmentation effects, and the efficiency of the coroutine memory management system. Important for applications that create many concurrent coroutines and need to understand memory scalability characteristics.

#### Execution Time
![Coroutine Memory Performance Trend](performance_trend_coroutinememory.svg)

#### Throughput
![Coroutine Memory Throughput Trend](throughput_trend_coroutinememory.svg)


### Coroutine Parallel Compute Performance

**Coroutine vs Regular Task Parallel Computation**

Compares coroutine-based parallel computation against traditional task-based approaches for CPU-intensive workloads. Measures whether coroutines introduce significant overhead when used for computational tasks versus their traditional use for I/O-bound operations. Tests both scheduler versions to identify performance differences in computational contexts.

#### Execution Time
![Coroutine Parallel Compute Performance Trend](performance_trend_coroutineparallelcompute.svg)

#### Throughput
![Coroutine Parallel Compute Throughput Trend](throughput_trend_coroutineparallelcompute.svg)


### Coroutine Submission Performance

**Coroutine vs Lambda Submission**

Compares the submission overhead of coroutines versus equivalent lambda functions to the task scheduler. Measures the cost of packaging coroutines as schedulable tasks including any additional metadata, type erasure, and scheduler integration overhead. Helps quantify the runtime cost difference between coroutine-based and traditional callback-based asynchronous patterns.

#### Execution Time
![Coroutine Submission Performance Trend](performance_trend_coroutinesubmission.svg)

#### Throughput
![Coroutine Submission Throughput Trend](throughput_trend_coroutinesubmission.svg)


### Coroutine Suspension Performance

**Coroutine Suspension/Resumption Overhead**

Measures the performance cost of coroutine suspension points and subsequent resumption. This includes saving coroutine state, yielding control back to the scheduler, and later restoring execution context. Critical for understanding the overhead of using 'co_await' constructs and designing efficient coroutine-based control flow.

#### Execution Time
![Coroutine Suspension Performance Trend](performance_trend_coroutinesuspension.svg)

#### Throughput
![Coroutine Suspension Throughput Trend](throughput_trend_coroutinesuspension.svg)


### Coroutine Task Chaining Performance

**Coroutine Task Chaining Performance**

Benchmarks sequential task dependencies implemented using coroutines versus traditional nested task submission patterns. Measures the efficiency of coroutine-based pipelines where each stage waits for the previous stage to complete. Compares against both manual task chaining and TBB equivalent implementations to understand the performance trade-offs.

#### Execution Time
![Coroutine Task Chaining Performance Trend](performance_trend_coroutinetaskchaining.svg)

#### Throughput
![Coroutine Task Chaining Throughput Trend](throughput_trend_coroutinetaskchaining.svg)


### Matrixops Performance

**Matrix Operations Performance**

Evaluates parallel matrix computations including transformations, multiplications, and other linear algebra operations. Tests the scheduler's performance on computationally intensive tasks that benefit from parallel execution. Includes both CPU-bound calculations and memory access patterns typical of numerical computing workloads.

#### Execution Time
![Matrixops Performance Trend](performance_trend_matrixops.svg)

#### Throughput
![Matrixops Throughput Trend](throughput_trend_matrixops.svg)


### Mixedworkload Performance

**Mixed Workload Performance**

Tests scheduler performance under heterogeneous workloads combining different types of tasks: I/O-bound, CPU-bound, memory-intensive, and short-duration tasks. Measures the scheduler's ability to handle diverse work characteristics simultaneously while maintaining good load balancing and thread utilization.

#### Execution Time
![Mixedworkload Performance Trend](performance_trend_mixedworkload.svg)

#### Throughput
![Mixedworkload Throughput Trend](throughput_trend_mixedworkload.svg)


### Nestedparallel Performance

**Nested Parallel Execution**

Benchmarks performance when parallel constructs are nested within other parallel regions. Tests the scheduler's handling of hierarchical parallelism, including dynamic thread allocation, work stealing across nested contexts, and avoiding oversubscription. Critical for applications using recursive parallel algorithms.

#### Execution Time
![Nestedparallel Performance Trend](performance_trend_nestedparallel.svg)

#### Throughput
![Nestedparallel Throughput Trend](throughput_trend_nestedparallel.svg)


### Parallelforvectorops Performance

**Parallel For Vector Operations**

Benchmarks parallel execution of vector operations using the auto_parallel_for construct. Tests the scheduler's ability to efficiently distribute vector computations across worker threads, including work stealing and load balancing. Measures both the overhead of parallelization and the effectiveness of parallel execution for computational workloads.

#### Execution Time
![Parallelforvectorops Performance Trend](performance_trend_parallelforvectorops.svg)

#### Throughput
![Parallelforvectorops Throughput Trend](throughput_trend_parallelforvectorops.svg)


### Tasksubmission Performance

**Task Submission Performance**

Measures the overhead of submitting tasks to the scheduler work queues. This includes the time to package work items, enqueue them into the scheduler's internal data structures, and wake up worker threads. Lower submission times enable better work distribution and reduced latency for fine-grained parallel workloads.

#### Execution Time
![Tasksubmission Performance Trend](performance_trend_tasksubmission.svg)

#### Throughput
![Tasksubmission Throughput Trend](throughput_trend_tasksubmission.svg)


### Taskthroughput Performance

**Task Throughput Measurement**

Measures the maximum task processing rate (tasks per second) that the scheduler can sustain. Uses minimal-work tasks to focus on scheduler overhead rather than computation time. Higher throughput indicates better efficiency in task management, queue operations, and thread coordination mechanisms.

#### Execution Time
![Taskthroughput Performance Trend](performance_trend_taskthroughput.svg)

#### Throughput
![Taskthroughput Throughput Trend](throughput_trend_taskthroughput.svg)


### Tssharedlinearsinglethread Performance

**Thread-Safe Shared Linear Allocator (Single Thread)**

Tests the performance of a thread-safe shared linear allocator when used in single-threaded scenarios. Linear allocators provide O(1) allocation by simply incrementing a pointer, but this variant includes thread-safety mechanisms. Measures the overhead of synchronization primitives when not actually needed (single-threaded usage).

#### Execution Time
![Tssharedlinearsinglethread Performance Trend](performance_trend_tssharedlinearsinglethread.svg)

#### Throughput
![Tssharedlinearsinglethread Throughput Trend](throughput_trend_tssharedlinearsinglethread.svg)


### Tsthreadlocalsinglethread Performance

**Thread-Safe Thread-Local Allocator (Single Thread)**

Evaluates a thread-local allocator's performance in single-threaded contexts. Thread-local allocators avoid synchronization overhead by maintaining separate memory pools per thread. This benchmark shows the baseline performance when thread-local storage is accessed from the owning thread.

#### Execution Time
![Tsthreadlocalsinglethread Performance Trend](performance_trend_tsthreadlocalsinglethread.svg)

#### Throughput
![Tsthreadlocalsinglethread Throughput Trend](throughput_trend_tsthreadlocalsinglethread.svg)


## 📋 Data Summary

- **Total benchmark runs:** 3208
- **Build range:** 71 - 126
- **Date range:** 2026-03-15 to 2026-03-15
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*