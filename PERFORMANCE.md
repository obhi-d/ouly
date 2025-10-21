# OULY Performance Tracking

**Generated:** 2025-10-21 17:13:35 UTC

## 📊 Latest Performance Results

**Build Number:** 121
**Commit Hash:** cbcadcad

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.91 | 255754476 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 218.60 | 4574565 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 207.99 | 4807923 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 583311.90 | 1714 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 564767.30 | 1771 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 166444.70 | 6008 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 126549.30 | 7902 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 195403.80 | 5118 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 157764.50 | 6339 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50066768.20 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50121317.90 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7308086.80 | 137 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7310653.60 | 137 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 520444.30 | 1921 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 515767.50 | 1939 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 275037.60 | 3636 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 146922.80 | 6806 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 147260.30 | 6791 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 146556.10 | 6823 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50064449.20 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50117787.80 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8555320.00 | 117 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8471135.90 | 118 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 150811.50 | 6631 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 149087.30 | 6707 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 244375.90 | 4092 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87788.70 | 11391 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 88189.50 | 11339 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 227186.00 | 4402 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 65039.50 | 15375 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 69279.07 | 14434 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 78917.20 | 12672 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 240824.40 | 4152 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 238610.20 | 4191 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 264507.30 | 3781 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60327522.70 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60010318.20 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59979189.60 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64269.30 | 15560 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64974.70 | 15391 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 144866.00 | 6903 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 145371.20 | 6879 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 232837.50 | 4295 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 244070.50 | 4097 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 272006.30 | 3676 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 252924.80 | 3954 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 146330.90 | 6834 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 159882.20 | 6255 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 154212.60 | 6485 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 607875.80 | 1645 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 509866.00 | 1961 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 443594.40 | 2254 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51063317.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51089465.30 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51123727.00 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 115998.50 | 8621 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117455.50 | 8514 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100078801.10 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41064558.30 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100093524.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41029472.80 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100241534.70 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1838530.80 | 544 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100262327.70 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1158795.60 | 863 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50061792.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12685.70 | 78829 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50117135.70 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12668.60 | 78935 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 119081.30 | 8398 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18221.10 | 54881 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100073600.10 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48100118.30 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100090759.50 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48076172.00 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100226878.30 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1794799.50 | 557 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100276889.50 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1152607.70 | 868 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50064578.20 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15277.30 | 65457 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50117566.80 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14972.80 | 66788 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 257827.80 | 3879 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18528.67 | 53970 | 0.00 |

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

- **Total benchmark runs:** 2856
- **Build range:** 71 - 121
- **Date range:** 2025-10-21 to 2025-10-21
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*