# OULY Performance Tracking

**Generated:** 2025-08-18 21:29:05 UTC

## 📊 Latest Performance Results

**Build Number:** 99
**Commit Hash:** 42b51a1d

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.21 | 237529691 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 207.88 | 4810518 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.31 | 158478605 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.20 | 238095238 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.18 | 4897639 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 584773.60 | 1710 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 571210.10 | 1751 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 138857.71 | 7202 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 115693.78 | 8644 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 212672.20 | 4702 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 152655.60 | 6551 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50064561.90 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50113037.00 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7254865.00 | 138 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7265882.60 | 138 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 518333.70 | 1929 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 514152.60 | 1945 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 147466.80 | 6781 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 142608.70 | 7012 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 157248.10 | 6359 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 153502.10 | 6515 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50060597.80 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50110633.10 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8843807.70 | 113 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8599699.10 | 116 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100074990.90 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 40954032.70 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100090116.10 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41065806.30 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100225836.10 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1844956.80 | 542 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100302618.60 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1167576.10 | 856 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50064665.60 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12691.90 | 78790 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50114995.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12669.80 | 78928 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 121689.70 | 8218 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 19238.20 | 51980 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100075462.00 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48089379.60 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100090762.90 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48135050.10 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100230466.60 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1786164.90 | 560 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100258795.10 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1160875.10 | 861 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50060575.70 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15279.60 | 65447 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50112872.00 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14982.00 | 66747 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 257379.70 | 3885 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18667.90 | 53568 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 151319.00 | 6609 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 151112.60 | 6618 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 221082.90 | 4523 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219419.80 | 4557 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 234600.30 | 4263 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223680.90 | 4471 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 140315.40 | 7127 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 147515.90 | 6779 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 139916.86 | 7147 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 271077.80 | 3689 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 240006.50 | 4167 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 240438.30 | 4159 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51813233.00 | 19 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51056751.20 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51092367.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 196423.70 | 5091 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117646.43 | 8500 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 155051.50 | 6449 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 156477.20 | 6391 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 242079.50 | 4131 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87902.60 | 11376 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 87366.60 | 11446 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 227517.20 | 4395 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64882.50 | 15412 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 69059.40 | 14480 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79590.10 | 12564 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 235833.90 | 4240 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 236857.80 | 4222 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 262935.70 | 3803 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60150003.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60018619.20 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59988270.30 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64612.69 | 15477 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 67527.60 | 14809 | 0.00 |

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

- **Total benchmark runs:** 1184
- **Build range:** 71 - 99
- **Date range:** 2025-08-18 to 2025-08-18
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*