# OULY Performance Tracking

**Generated:** 2025-08-31 21:46:08 UTC

## 📊 Latest Performance Results

**Build Number:** 116
**Commit Hash:** 13a6b08e

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 205.08 | 4876146 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.90 | 256410256 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 205.28 | 4871395 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 581837.50 | 1719 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 565427.60 | 1769 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 157828.50 | 6336 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 127337.00 | 7853 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 203729.90 | 4908 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 175161.83 | 5709 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50064598.20 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50112333.70 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7242880.90 | 138 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7236223.40 | 138 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 520117.50 | 1923 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 513550.30 | 1947 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 151993.70 | 6579 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 136642.20 | 7318 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 160150.90 | 6244 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 151516.80 | 6600 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50064276.20 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50110867.50 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8800868.20 | 114 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8665527.40 | 115 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 170755.80 | 5856 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 156172.30 | 6403 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 163405.90 | 6120 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219390.50 | 4558 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 234427.80 | 4266 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 229568.70 | 4356 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 116064.70 | 8616 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 116416.40 | 8590 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 119539.00 | 8365 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 356047.10 | 2809 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 387587.50 | 2580 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 367608.80 | 2720 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51246744.80 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51085520.00 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51071775.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 116731.00 | 8567 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117838.38 | 8486 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 152096.90 | 6575 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 151780.86 | 6588 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 163532.20 | 6115 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88465.60 | 11304 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 87684.10 | 11405 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 228726.50 | 4372 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 133019.40 | 7518 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 70753.07 | 14134 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 99285.70 | 10072 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 709444.70 | 1410 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 434305.80 | 2303 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 353713.80 | 2827 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60201524.20 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60019897.20 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60046085.80 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 123133.10 | 8121 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 66135.00 | 15121 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100072796.70 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41032432.50 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100090942.70 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41009335.30 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100221349.10 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1828813.70 | 547 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100302574.00 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1172611.10 | 853 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50059555.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12690.90 | 78797 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50114275.60 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12674.90 | 78896 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 121169.20 | 8253 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18266.40 | 54745 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100073469.10 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48104875.50 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100087784.30 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48099752.00 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100223076.70 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1793321.90 | 558 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100257460.70 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1166425.70 | 857 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50061248.80 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15274.40 | 65469 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50108663.90 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14982.90 | 66743 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 257312.60 | 3886 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18723.80 | 53408 | 0.00 |

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

- **Total benchmark runs:** 2416
- **Build range:** 71 - 116
- **Date range:** 2025-08-31 to 2025-08-31
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*