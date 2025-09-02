# OULY Performance Tracking

**Generated:** 2025-09-02 22:19:18 UTC

## 📊 Latest Performance Results

**Build Number:** 118
**Commit Hash:** 162df8ac

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 582998.90 | 1715 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 560058.90 | 1786 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 155410.30 | 6435 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 125926.00 | 7941 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 210404.40 | 4753 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 173224.80 | 5773 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50060068.30 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50113981.00 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7288727.80 | 137 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7287721.80 | 137 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 520317.00 | 1922 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 515428.90 | 1940 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 151867.50 | 6585 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 139612.57 | 7163 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 166239.20 | 6015 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 165354.70 | 6048 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50065613.30 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50112020.70 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8615993.00 | 116 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8550732.60 | 117 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.02 | 166112957 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 220.62 | 4532681 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.91 | 255754476 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 193.36 | 5171700 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100074232.70 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41032680.30 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100093366.00 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41041061.50 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100224495.80 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1827392.90 | 547 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100279657.50 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1166220.60 | 857 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50057665.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12687.70 | 78816 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50116622.30 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12669.70 | 78928 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 121494.30 | 8231 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18494.60 | 54070 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100072349.20 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48111910.90 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100089363.70 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48102821.70 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100374104.20 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1782528.30 | 561 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100262162.90 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1163391.30 | 860 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50062589.20 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15279.40 | 65448 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50124070.10 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14979.90 | 66756 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 256578.30 | 3897 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18641.70 | 53643 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 145734.10 | 6862 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 144766.43 | 6908 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 229206.40 | 4363 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88083.90 | 11353 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 88210.10 | 11337 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 227934.10 | 4387 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 65030.90 | 15377 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 69562.80 | 14375 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79183.40 | 12629 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236817.60 | 4223 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 238185.20 | 4198 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 355250.90 | 2815 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60062963.50 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 59955312.30 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59976830.80 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64855.70 | 15419 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64334.70 | 15544 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 157890.10 | 6334 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 151951.00 | 6581 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 246903.10 | 4050 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219111.90 | 4564 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 234398.60 | 4266 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223123.40 | 4482 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 141182.80 | 7083 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 141843.00 | 7050 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 117908.10 | 8481 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 283391.50 | 3529 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 346849.50 | 2883 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 324854.20 | 3078 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51262339.30 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51065997.40 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51080645.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 131567.60 | 7601 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117514.30 | 8510 | 0.00 |

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

- **Total benchmark runs:** 2592
- **Build range:** 71 - 118
- **Date range:** 2025-09-02 to 2025-09-02
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*