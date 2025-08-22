# OULY Performance Tracking

**Generated:** 2025-08-22 17:17:24 UTC

## 📊 Latest Performance Results

**Build Number:** 102
**Commit Hash:** b32023a6

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.20 | 238095238 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.99 | 4878287 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.02 | 166112957 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 221.21 | 4520591 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 577180.50 | 1733 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 560457.20 | 1784 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 154582.60 | 6469 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 125710.60 | 7955 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 211592.00 | 4726 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 171910.90 | 5817 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50064466.70 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50111329.60 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7302061.30 | 137 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7276452.40 | 137 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 524882.10 | 1905 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 531902.20 | 1880 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 144108.80 | 6939 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 138303.00 | 7231 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 146536.14 | 6824 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 142528.90 | 7016 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50060190.70 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50112009.30 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8801141.90 | 114 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8783973.90 | 114 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100073733.30 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41076719.60 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100090527.20 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41013633.40 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100226395.60 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1835190.30 | 545 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100280639.90 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1165567.40 | 858 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50064550.80 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12689.70 | 78804 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50111835.60 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12669.70 | 78928 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 122769.10 | 8145 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18264.10 | 54752 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100070582.70 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48084182.10 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100089156.40 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48089235.60 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100225937.00 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1786100.50 | 560 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100267739.40 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1149524.30 | 870 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50064514.20 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15274.50 | 65469 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50113165.50 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14974.90 | 66778 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 255891.10 | 3908 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18513.50 | 54015 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 148744.40 | 6723 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 148680.40 | 6726 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 236849.30 | 4222 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219233.40 | 4561 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 234713.20 | 4261 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223728.80 | 4470 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 141724.40 | 7056 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 151337.30 | 6608 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113458.38 | 8814 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 249754.20 | 4004 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 299474.60 | 3339 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 319267.40 | 3132 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51053935.70 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51074803.40 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51043869.00 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 116422.30 | 8589 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117518.22 | 8509 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 163419.90 | 6119 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 164140.20 | 6092 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 231093.00 | 4327 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87782.70 | 11392 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 87605.40 | 11415 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 227344.00 | 4399 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 65466.77 | 15275 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 71664.70 | 13954 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79648.60 | 12555 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236319.90 | 4232 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 237956.00 | 4202 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 346481.20 | 2886 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60156940.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60135666.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60040517.70 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 60676.10 | 16481 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64691.60 | 15458 | 0.00 |

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

- **Total benchmark runs:** 1448
- **Build range:** 71 - 102
- **Date range:** 2025-08-22 to 2025-08-22
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*