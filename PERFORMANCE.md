# OULY Performance Tracking

**Generated:** 2025-08-18 17:10:16 UTC

## 📊 Latest Performance Results

**Build Number:** 97
**Commit Hash:** f2352b44

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 586729.90 | 1704 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 573127.50 | 1745 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 152345.30 | 6564 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 120804.12 | 8278 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 205242.00 | 4872 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 169633.60 | 5895 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50068420.50 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50136291.00 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7749824.20 | 129 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7741416.50 | 129 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 520303.00 | 1922 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 516079.10 | 1938 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 284592.10 | 3514 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 140805.71 | 7102 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 168703.80 | 5928 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 164095.20 | 6094 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50064182.60 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50110015.70 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8685760.30 | 115 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8627312.00 | 116 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 9.32 | 107296137 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 6.11 | 163666121 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 205.58 | 4864286 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.21 | 161030596 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 217.91 | 4589051 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100082570.30 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41171702.40 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100103120.00 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41096494.50 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100222845.80 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1825495.10 | 548 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100277821.00 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1162672.60 | 860 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50069204.10 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12688.70 | 78810 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50128105.10 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12670.60 | 78923 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 120265.50 | 8315 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 17940.50 | 55740 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100076269.70 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48144448.10 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100092071.10 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48065476.10 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100249605.80 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1787910.30 | 559 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100254454.90 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1158005.90 | 864 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50064512.40 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15279.50 | 65447 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50110769.20 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14980.00 | 66756 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 255769.20 | 3910 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18228.00 | 54861 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 152038.80 | 6577 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 152975.20 | 6537 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 220650.80 | 4532 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 454082.50 | 2202 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 317245.10 | 3152 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 284239.50 | 3518 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 164034.20 | 6096 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 107793.10 | 9277 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 141054.20 | 7089 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 451071.80 | 2217 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 354539.70 | 2821 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 357926.00 | 2794 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51598824.20 | 19 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51102469.30 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51161630.60 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 127735.40 | 7829 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 129511.70 | 7721 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 241155.10 | 4147 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 145122.80 | 6891 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 232435.80 | 4302 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87778.80 | 11392 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 87402.10 | 11441 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 227119.90 | 4403 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 65004.20 | 15384 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 69094.80 | 14473 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79278.80 | 12614 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 235967.90 | 4238 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 238110.00 | 4200 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 349858.30 | 2858 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60279748.00 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60030123.20 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59969611.30 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64802.90 | 15431 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64638.70 | 15471 | 0.00 |

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

- **Total benchmark runs:** 1096
- **Build range:** 71 - 97
- **Date range:** 2025-08-18 to 2025-08-18
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*