# OULY Performance Tracking

**Generated:** 2025-08-22 23:02:37 UTC

## 📊 Latest Performance Results

**Build Number:** 105
**Commit Hash:** 8487a0f7

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100085166.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41117327.60 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100100806.10 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41088074.20 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100240707.20 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1844264.00 | 542 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100264548.30 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1152279.90 | 868 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50069046.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12687.70 | 78816 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50124986.70 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12802.89 | 78107 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 123056.40 | 8126 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18224.10 | 54872 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100073853.10 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48082863.70 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100092811.70 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48108714.40 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100243965.60 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1772007.80 | 564 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100259303.40 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1159390.40 | 863 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50065099.00 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15278.60 | 65451 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50112419.50 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14975.00 | 66778 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 256152.20 | 3904 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18465.71 | 54154 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 579609.40 | 1725 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 562169.70 | 1779 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 150641.10 | 6638 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 124391.62 | 8039 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 198167.20 | 5046 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 159953.50 | 6252 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50069086.70 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50141355.90 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7526237.80 | 133 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7601644.20 | 132 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 515732.30 | 1939 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 511396.20 | 1955 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 159852.40 | 6256 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 142609.40 | 7012 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 154052.70 | 6491 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 149384.10 | 6694 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50064789.20 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50109945.10 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8679191.90 | 115 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8568300.50 | 117 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14 | ts_shared_linear_single_thread | 6.01 | 166389351 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 221.81 | 4508363 | 0.00 |
| clang-18 | ts_shared_linear_single_thread | 6.41 | 156006240 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.20 | 238095238 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 209.01 | 4784428 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 143036.10 | 6991 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 141375.38 | 7073 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 228292.40 | 4380 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87859.20 | 11382 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 87441.30 | 11436 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 228201.20 | 4382 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64968.30 | 15392 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 68435.80 | 14612 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 80010.50 | 12498 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 257888.60 | 3878 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 261257.90 | 3828 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 292879.00 | 3414 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60263596.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60112430.00 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 59996378.20 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64702.80 | 15455 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64932.20 | 15401 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 215425.40 | 4642 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 150880.60 | 6628 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 156704.50 | 6381 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 219117.30 | 4564 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 234639.40 | 4262 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 223526.60 | 4474 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 108884.90 | 9184 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 172485.00 | 5798 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 124859.90 | 8009 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 249045.40 | 4015 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 247037.60 | 4048 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 241420.10 | 4142 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51058989.10 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51048040.90 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51089258.10 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 131154.50 | 7625 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 117721.50 | 8495 | 0.00 |

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

- **Total benchmark runs:** 1712
- **Build range:** 71 - 105
- **Date range:** 2025-08-22 to 2025-08-22
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*