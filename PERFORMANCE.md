# OULY Performance Tracking

**Generated:** 2025-10-14 04:52:26 UTC

## 📊 Latest Performance Results

**Build Number:** 120
**Commit Hash:** 8dd72209

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 204.89 | 4880668 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.91 | 255754476 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 195.06 | 5126628 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100075315.60 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41091409.50 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100093469.90 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41079932.00 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100229103.10 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1828007.60 | 547 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100275968.80 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1166759.10 | 857 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50058197.80 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12689.80 | 78803 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50115303.50 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12671.80 | 78915 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 120262.40 | 8315 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 19232.10 | 51996 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100075588.50 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48081107.10 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100090342.30 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48123074.80 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100233376.30 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1778394.40 | 562 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100262496.40 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1161855.40 | 861 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50065902.80 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15278.70 | 65451 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50115078.20 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14977.20 | 66768 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 271784.10 | 3679 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18558.80 | 53883 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | CoroutineCreation_V1 | 520076.20 | 1923 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 520220.30 | 1922 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 150186.90 | 6658 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 140229.20 | 7131 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 166255.50 | 6015 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 161580.00 | 6189 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50062601.90 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50112099.60 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8751923.70 | 114 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8640624.90 | 116 | 0.00 |
| gcc-14.2 | CoroutineCreation_V1 | 581801.10 | 1719 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 565062.70 | 1770 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 158495.20 | 6309 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 115926.33 | 8626 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 229333.00 | 4360 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 161094.10 | 6208 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50065921.00 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50111811.80 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7365028.60 | 136 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7406442.00 | 135 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 151763.80 | 6589 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 151484.40 | 6601 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 235553.20 | 4245 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 176829.90 | 5655 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 120333.38 | 8310 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 275751.60 | 3626 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 73585.62 | 13590 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 93244.10 | 10725 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 98654.20 | 10136 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 784098.80 | 1275 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 728974.30 | 1372 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 262312.40 | 3812 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60097074.20 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60007132.00 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59970143.10 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 63444.10 | 15762 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64651.30 | 15468 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 134809.60 | 7418 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 134476.00 | 7436 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 192804.30 | 5187 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 273986.50 | 3650 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 302777.40 | 3303 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 259168.80 | 3858 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 168547.50 | 5933 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 168551.90 | 5933 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 150191.50 | 6658 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 384200.00 | 2603 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 561190.70 | 1782 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 320091.90 | 3124 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51156044.20 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 50999500.60 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51087169.20 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 116479.50 | 8585 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117578.50 | 8505 | 0.00 |

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

- **Total benchmark runs:** 2768
- **Build range:** 71 - 120
- **Date range:** 2025-10-14 to 2025-10-14
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*