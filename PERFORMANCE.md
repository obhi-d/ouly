# OULY Performance Tracking

**Generated:** 2025-08-28 15:03:36 UTC

## 📊 Latest Performance Results

**Build Number:** 108
**Commit Hash:** b8e56daa

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 584875.10 | 1710 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 576050.50 | 1736 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 154328.50 | 6480 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 125127.60 | 7992 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 202108.10 | 4948 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 159910.00 | 6254 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50071537.70 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50147175.30 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7936281.70 | 126 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7971829.50 | 125 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 517875.50 | 1931 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 513651.60 | 1947 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 146967.40 | 6804 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 124240.80 | 8049 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 167032.00 | 5987 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 159536.00 | 6268 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50059996.80 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50116186.90 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8623443.20 | 116 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8488815.00 | 118 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.42 | 155763240 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.91 | 255754476 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.09 | 4899799 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.11 | 163666121 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 204.18 | 4897639 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100075672.10 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48116460.00 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100091394.10 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48147926.20 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100227705.80 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1787158.00 | 560 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100283300.50 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1171052.80 | 854 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50064597.40 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15276.60 | 65460 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50113660.30 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14974.10 | 66782 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 255878.10 | 3908 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18647.00 | 53628 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100088506.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41084416.00 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100110278.50 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41051973.20 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100241715.90 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1833991.80 | 545 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100269596.90 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1160772.20 | 861 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50064396.50 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12690.80 | 78797 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50146884.10 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12669.80 | 78928 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 121249.40 | 8247 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18131.00 | 55154 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 131987.40 | 7576 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 131494.50 | 7605 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 123804.00 | 8077 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 176846.50 | 5655 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 120229.12 | 8317 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 276414.10 | 3618 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 95893.70 | 10428 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 103808.60 | 9633 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 99848.10 | 10015 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 327860.50 | 3050 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 236306.00 | 4232 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 263143.30 | 3800 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60083659.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 59940770.90 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59965687.60 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 65058.00 | 15371 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64784.50 | 15436 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 176917.90 | 5652 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 141788.00 | 7053 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 244372.40 | 4092 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 571146.30 | 1751 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 338521.00 | 2954 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 258867.60 | 3863 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 135477.10 | 7381 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 131352.50 | 7613 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 180003.60 | 5555 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 431790.90 | 2316 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 506780.60 | 1973 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 319404.20 | 3131 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51273347.30 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51060384.50 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51124647.00 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 116320.20 | 8597 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 118079.50 | 8469 | 0.00 |

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

- **Total benchmark runs:** 1888
- **Build range:** 71 - 108
- **Date range:** 2025-08-28 to 2025-08-28
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*