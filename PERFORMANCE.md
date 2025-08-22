# OULY Performance Tracking

**Generated:** 2025-08-22 21:57:35 UTC

## 📊 Latest Performance Results

**Build Number:** 104
**Commit Hash:** f227a550

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 581570.00 | 1719 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 568748.00 | 1758 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 156393.50 | 6394 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 124529.50 | 8030 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 205194.40 | 4873 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 170011.10 | 5882 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50065013.30 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50109090.00 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7245522.80 | 138 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7296046.00 | 137 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 518250.10 | 1930 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 522741.60 | 1913 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 143089.29 | 6989 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 124100.50 | 8058 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 169195.00 | 5910 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 160091.17 | 6246 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50065862.00 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50113320.30 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8631272.00 | 116 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8553211.20 | 117 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100074781.00 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41066713.70 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100090040.50 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41006916.40 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100225227.80 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1829626.00 | 547 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100278758.50 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1166679.80 | 857 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50059848.30 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12688.80 | 78810 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50111248.90 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12664.90 | 78958 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 119785.50 | 8348 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18489.80 | 54084 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100074372.70 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48102142.40 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100090679.00 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48146761.20 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100230788.30 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1792629.60 | 558 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100282824.80 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1165615.00 | 858 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50064602.90 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15277.70 | 65455 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50113852.20 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14977.10 | 66769 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 256669.90 | 3896 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18577.90 | 53827 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 194.97 | 5128994 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 218.21 | 4582741 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 135910.90 | 7358 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 135733.50 | 7367 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 259454.60 | 3854 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219152.70 | 4563 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 234001.60 | 4273 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223802.40 | 4468 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 121556.90 | 8227 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 135369.80 | 7387 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 139676.00 | 7159 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 251408.40 | 3978 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 238626.30 | 4191 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241878.40 | 4134 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51138353.10 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 50993852.10 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51040394.50 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 116008.50 | 8620 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117337.00 | 8522 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 149456.30 | 6691 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 148347.20 | 6741 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 156930.40 | 6372 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87720.00 | 11400 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 87366.40 | 11446 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 227670.40 | 4392 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64651.70 | 15467 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 69184.20 | 14454 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79340.40 | 12604 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236264.60 | 4233 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 236359.80 | 4231 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 351045.60 | 2849 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60035368.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60023414.40 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59953454.70 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 124640.50 | 8023 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64526.85 | 15497 | 0.00 |

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

- **Total benchmark runs:** 1624
- **Build range:** 71 - 104
- **Date range:** 2025-08-22 to 2025-08-22
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*