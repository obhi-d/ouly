# OULY Performance Tracking

**Generated:** 2025-11-22 20:17:49 UTC

## 📊 Latest Performance Results

**Build Number:** 122
**Commit Hash:** 9a4795dc

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100072895.20 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48070431.60 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100090844.80 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48182803.20 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100223865.10 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1782623.50 | 561 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100264990.80 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1160285.70 | 862 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50064539.20 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15274.50 | 65469 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50115162.20 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14977.00 | 66769 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 256850.20 | 3893 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18235.00 | 54840 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100083174.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 43897679.40 | 23 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100117683.00 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 43990557.90 | 23 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100379280.70 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 2247560.60 | 445 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100324353.60 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1210235.30 | 826 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50069354.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 14139.64 | 70723 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50132615.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 16068.50 | 62234 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 126662.00 | 7895 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 15801.40 | 63286 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 155188.60 | 6444 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 155029.20 | 6450 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 168314.00 | 5941 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87922.00 | 11374 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 88200.60 | 11338 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 230361.80 | 4341 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 65049.40 | 15373 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 70041.70 | 14277 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79507.30 | 12577 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 297101.30 | 3366 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 430158.50 | 2325 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 307183.00 | 3255 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60097989.30 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60078287.40 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60006889.20 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 60789.40 | 16450 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64123.60 | 15595 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 140311.20 | 7127 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 138223.50 | 7235 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 187539.60 | 5332 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 217436.90 | 4599 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 221294.60 | 4519 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 235134.40 | 4253 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 103733.20 | 9640 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 101646.10 | 9838 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 110638.60 | 9038 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 442814.50 | 2258 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 816456.50 | 1225 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 452923.30 | 2208 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 54049060.20 | 19 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 54108027.80 | 18 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 53846518.10 | 19 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 136133.70 | 7346 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 136745.90 | 7313 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | CoroutineCreation_V1 | 520571.20 | 1921 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 523311.30 | 1911 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 132668.50 | 7538 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 137510.50 | 7272 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 152870.20 | 6541 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 151398.40 | 6605 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50064840.10 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50113680.60 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8784520.90 | 114 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8557265.90 | 117 | 0.00 |
| gcc-14.2 | CoroutineCreation_V1 | 441395.00 | 2266 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 425671.70 | 2349 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 282633.50 | 3538 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 216957.20 | 4609 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 393875.60 | 2539 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 275181.80 | 3634 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50066751.70 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50135480.40 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7534465.10 | 133 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7393288.80 | 135 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 22.05 | 45351474 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 3.64 | 274725275 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 133.23 | 7505817 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.81 | 262467192 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 203.38 | 4916904 | 0.00 |

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

- **Total benchmark runs:** 2944
- **Build range:** 71 - 122
- **Date range:** 2025-11-22 to 2025-11-22
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*