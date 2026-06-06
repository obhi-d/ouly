# OULY Performance Tracking

**Generated:** 2026-06-06 05:47:08 UTC

## 📊 Latest Performance Results

**Build Number:** 128
**Commit Hash:** 14f3cfdf

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.42 | 155763240 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.91 | 255754476 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 205.28 | 4871395 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.12 | 163398693 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.50 | 222222222 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 223.71 | 4470073 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 586733.90 | 1704 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 565037.50 | 1770 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 157069.30 | 6367 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 125288.10 | 7982 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 236022.20 | 4237 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 180168.30 | 5550 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50079279.10 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50156372.20 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7671799.30 | 130 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7580527.30 | 132 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 517362.80 | 1933 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 510176.40 | 1960 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 136373.40 | 7333 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 127700.38 | 7831 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 169424.30 | 5902 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 159520.70 | 6269 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50072829.90 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50127248.80 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8739513.80 | 114 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8725437.60 | 115 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100092950.60 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41175474.50 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100113114.10 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41156330.40 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100247546.20 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1851892.90 | 540 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100287104.40 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1175112.90 | 851 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50079244.80 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12691.60 | 78792 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50158739.40 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12756.70 | 78390 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 124688.00 | 8020 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 19480.30 | 51334 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100085103.80 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48299393.30 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100102916.50 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48363077.20 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100220062.60 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1792116.00 | 558 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100270698.90 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1165047.10 | 858 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50079175.30 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15360.80 | 65101 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50148865.90 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14979.00 | 66760 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 257481.00 | 3884 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18500.12 | 54054 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 130783.30 | 7646 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 153669.90 | 6507 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 242075.40 | 4131 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 269591.50 | 3709 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 333384.20 | 3000 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 322416.90 | 3102 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 270092.40 | 3702 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 169910.90 | 5885 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 204020.60 | 4901 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 476438.40 | 2099 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 459610.10 | 2176 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 321917.90 | 3106 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51798302.90 | 19 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51176445.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51278223.30 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 135401.00 | 7385 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 130923.70 | 7638 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 130619.60 | 7656 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 155397.90 | 6435 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 156802.60 | 6377 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 89850.20 | 11130 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 88475.70 | 11303 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 229711.10 | 4353 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 65222.10 | 15332 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 71512.50 | 13984 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 86650.30 | 11541 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 240942.20 | 4150 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 237403.50 | 4212 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 351669.50 | 2844 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60256891.60 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60177360.40 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60174600.50 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 61213.60 | 16336 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 66711.90 | 14990 | 0.00 |

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

- **Total benchmark runs:** 3384
- **Build range:** 71 - 128
- **Date range:** 2026-06-06 to 2026-06-06
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*