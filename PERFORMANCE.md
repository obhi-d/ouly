# OULY Performance Tracking

**Generated:** 2025-12-23 10:59:16 UTC

## 📊 Latest Performance Results

**Build Number:** 123
**Commit Hash:** 58f51bb9

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | CoroutineCreation_V1 | 519353.00 | 1925 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 516983.60 | 1934 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 270393.50 | 3698 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 128098.00 | 7807 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 170559.30 | 5863 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 166047.80 | 6022 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50068277.40 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50127680.00 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 9106296.80 | 110 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8903706.80 | 112 | 0.00 |
| gcc-14.2 | CoroutineCreation_V1 | 583556.30 | 1714 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 572286.20 | 1747 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 141904.10 | 7047 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 118486.20 | 8440 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 225999.80 | 4425 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 175360.80 | 5703 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50066369.00 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50120169.20 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7382537.60 | 135 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7359979.40 | 136 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 195.46 | 5116136 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.90 | 256410256 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.64 | 4886610 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100080641.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41167146.60 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100095953.00 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41006747.20 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100257476.40 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1848823.60 | 541 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100277935.60 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1169200.90 | 855 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50067184.00 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12691.70 | 78792 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50121645.80 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12668.70 | 78935 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 118831.60 | 8415 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18862.30 | 53016 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100078708.30 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48141881.60 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100092977.50 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48103814.60 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100235676.20 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1809838.60 | 553 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100269850.70 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1169691.00 | 855 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50068477.90 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15275.50 | 65464 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50133168.30 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14979.00 | 66760 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 257633.90 | 3881 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18766.89 | 53285 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 127111.88 | 7867 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 141901.80 | 7047 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 159753.20 | 6260 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 265030.40 | 3773 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 289849.80 | 3450 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 233639.60 | 4280 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 134892.70 | 7413 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 108389.00 | 9226 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 115632.88 | 8648 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 248403.20 | 4026 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 243668.40 | 4104 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 242390.00 | 4126 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51014577.40 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51268937.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51135930.90 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 119564.20 | 8364 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 119851.80 | 8344 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 142771.30 | 7004 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 141263.50 | 7079 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 227615.60 | 4393 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87849.90 | 11383 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 88328.80 | 11321 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 228452.20 | 4377 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64877.10 | 15414 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 69069.50 | 14478 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79387.10 | 12597 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 238162.30 | 4199 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 238713.40 | 4189 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 297521.10 | 3361 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60473832.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60126402.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59982118.90 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64879.00 | 15413 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64696.70 | 15457 | 0.00 |

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

- **Total benchmark runs:** 3032
- **Build range:** 71 - 123
- **Date range:** 2025-12-23 to 2025-12-23
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*