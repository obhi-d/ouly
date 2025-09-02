# OULY Performance Tracking

**Generated:** 2025-09-02 22:12:51 UTC

## 📊 Latest Performance Results

**Build Number:** 117
**Commit Hash:** 4320b3f6

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100083052.90 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48146615.50 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100102540.80 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48121314.50 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100240597.80 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1787868.20 | 559 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100284846.20 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1161012.70 | 861 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50071305.70 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15273.50 | 65473 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50149669.30 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14975.90 | 66774 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 256292.50 | 3902 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18425.40 | 54273 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100078297.60 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41059086.50 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100092257.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41168602.60 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100227874.70 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1830013.50 | 546 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100304211.50 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1166950.10 | 857 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50065912.30 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12686.70 | 78823 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50112094.40 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12667.70 | 78941 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 122167.30 | 8185 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 19089.70 | 52384 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 149809.70 | 6675 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 148787.80 | 6721 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 160880.40 | 6216 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 121188.30 | 8252 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 120106.20 | 8326 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 278074.20 | 3596 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 95115.60 | 10514 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 93386.30 | 10708 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 98771.40 | 10124 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 790720.40 | 1265 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 432345.20 | 2313 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 358068.50 | 2793 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60072893.30 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60113231.90 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60070462.20 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 122454.43 | 8166 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64987.40 | 15388 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 147490.70 | 6780 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 147974.60 | 6758 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 155793.20 | 6419 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 220460.10 | 4536 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 236856.80 | 4222 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 282641.30 | 3538 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 128720.60 | 7769 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 108592.00 | 9209 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113343.90 | 8823 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 250555.30 | 3991 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 245374.60 | 4075 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241913.20 | 4134 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51077876.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51142760.70 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51023276.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 122247.50 | 8180 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117920.40 | 8480 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 195.86 | 5105688 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.90 | 256410256 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 193.36 | 5171700 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | CoroutineCreation_V1 | 524825.90 | 1905 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 518383.80 | 1929 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 157135.40 | 6364 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 143409.33 | 6973 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 167577.90 | 5967 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 164094.30 | 6094 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50071167.00 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50150041.40 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 9396998.80 | 106 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 9500112.20 | 105 | 0.00 |
| gcc-14.2 | CoroutineCreation_V1 | 587209.60 | 1703 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 567399.50 | 1762 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 155012.80 | 6451 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 121317.80 | 8243 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 216258.20 | 4624 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 164039.60 | 6096 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50064192.80 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50112563.60 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7276630.90 | 137 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7301450.40 | 137 | 0.00 |

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

- **Total benchmark runs:** 2504
- **Build range:** 71 - 117
- **Date range:** 2025-09-02 to 2025-09-02
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*