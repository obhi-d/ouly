# OULY Performance Tracking

**Generated:** 2025-12-23 10:59:36 UTC

## 📊 Latest Performance Results

**Build Number:** 124
**Commit Hash:** 8713f508

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 136303.70 | 7337 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 119909.00 | 8340 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 150096.40 | 6662 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 100116.10 | 9988 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 102610.70 | 9746 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 257529.20 | 3883 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 80996.25 | 12346 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 80671.70 | 12396 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 98316.70 | 10171 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 369025.40 | 2710 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 548962.60 | 1822 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 364796.60 | 2741 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60024764.00 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60075617.20 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59964832.80 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 63240.69 | 15813 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 65392.20 | 15292 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 145842.40 | 6857 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 139190.90 | 7184 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 251912.20 | 3970 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 220078.50 | 4544 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 258132.80 | 3874 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 255560.80 | 3913 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 217017.70 | 4608 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 166254.60 | 6015 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 116636.60 | 8574 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 578933.90 | 1727 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 432361.30 | 2313 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 276026.30 | 3623 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51090668.50 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51058236.70 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51088187.10 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 150198.50 | 6658 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117932.43 | 8479 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 8.42 | 118764846 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.90 | 256410256 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 214.70 | 4657662 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.11 | 163666121 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 192.46 | 5195885 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 580950.80 | 1721 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 561784.90 | 1780 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 152307.40 | 6566 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 123510.40 | 8096 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 201978.60 | 4951 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 166155.40 | 6018 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50063683.60 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50110315.80 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7231262.20 | 138 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7270079.90 | 138 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 520787.00 | 1920 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 522434.00 | 1914 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 160931.70 | 6214 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 147077.80 | 6799 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 161423.17 | 6195 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 153607.00 | 6510 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50059865.30 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50116697.70 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8726624.60 | 115 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8711887.10 | 115 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100078214.90 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41033537.60 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100101603.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41029781.70 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100221258.60 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1818558.00 | 550 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100268555.10 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1164726.30 | 859 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50067275.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12689.80 | 78803 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50113187.70 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12668.70 | 78935 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 122028.70 | 8195 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18135.00 | 55142 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100077148.00 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48118583.10 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100093579.80 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48142698.40 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100229597.10 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1814711.00 | 551 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100268661.50 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1159467.20 | 862 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50065388.10 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15275.50 | 65464 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50121227.50 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14977.00 | 66769 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 257821.70 | 3879 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18595.80 | 53776 | 0.00 |

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

- **Total benchmark runs:** 3120
- **Build range:** 71 - 124
- **Date range:** 2025-12-23 to 2025-12-23
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*