# OULY Performance Tracking

**Generated:** 2026-05-09 23:46:51 UTC

## 📊 Latest Performance Results

**Build Number:** 127
**Commit Hash:** 9c568419

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 7.51 | 133155792 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.22 | 236702292 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 201.60 | 4960317 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.21 | 161030596 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.51 | 221729490 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 208.69 | 4791796 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 582362.80 | 1717 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 563522.60 | 1775 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 154040.70 | 6492 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 130831.40 | 7643 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 228950.70 | 4368 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 177666.90 | 5629 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50072309.10 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50127644.50 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7303002.90 | 137 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7268721.90 | 138 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 549958.20 | 1818 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 543784.00 | 1839 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 195060.50 | 5127 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 156110.90 | 6406 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 229473.20 | 4358 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 201134.60 | 4972 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50074025.00 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50143544.50 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 10392233.10 | 96 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 10643267.80 | 94 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100086823.30 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41120179.00 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100102232.00 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41136319.90 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100236947.20 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1839750.30 | 544 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100287173.40 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1173748.10 | 852 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50073438.70 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12692.70 | 78785 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50130110.80 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12667.70 | 78941 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 122156.00 | 8186 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 19168.80 | 52168 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100077204.60 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48107555.60 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100102598.80 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 49736734.30 | 20 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100254677.70 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1875679.60 | 533 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100261829.50 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1229351.30 | 813 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50073497.40 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15288.00 | 65411 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50147026.00 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 15095.06 | 66247 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 260469.80 | 3839 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18488.80 | 54087 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 146528.60 | 6825 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 144411.60 | 6925 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 147021.50 | 6802 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 245172.70 | 4079 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 231018.30 | 4329 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 225101.20 | 4442 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 134896.14 | 7413 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 109771.90 | 9110 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113376.70 | 8820 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 251349.20 | 3979 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 364682.80 | 2742 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 344173.60 | 2906 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51174485.50 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51254077.10 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51220863.80 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 132182.90 | 7565 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 119094.40 | 8397 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 90213.70 | 11085 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 133268.00 | 7504 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 153732.20 | 6505 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 89899.82 | 11123 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 109317.50 | 9148 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 233464.20 | 4283 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 68563.10 | 14585 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 78260.00 | 12778 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 81634.80 | 12250 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 229029.50 | 4366 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 231350.00 | 4322 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 323450.60 | 3092 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60305266.00 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60910918.40 | 16 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60087030.40 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 59129.90 | 16912 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 68834.50 | 14528 | 0.00 |

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

- **Total benchmark runs:** 3296
- **Build range:** 71 - 127
- **Date range:** 2026-05-09 to 2026-05-09
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*