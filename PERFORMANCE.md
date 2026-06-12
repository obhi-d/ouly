# OULY Performance Tracking

**Generated:** 2026-06-12 00:15:52 UTC

## 📊 Latest Performance Results

**Build Number:** 129
**Commit Hash:** d35241fc

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 7.51 | 133155792 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 204.71 | 4884959 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 22.07 | 45310376 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 3.68 | 271739130 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 149.61 | 6684045 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 439999.90 | 2273 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 421630.30 | 2372 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 310767.30 | 3218 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 232429.80 | 4302 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 432981.00 | 2310 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 327425.30 | 3054 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50211506.50 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50203506.90 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 8372777.90 | 119 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 8091340.00 | 124 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 554719.30 | 1803 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 549061.80 | 1821 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 217500.60 | 4598 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 204333.70 | 4894 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 317544.20 | 3149 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 274680.70 | 3641 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50118336.30 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50128568.70 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 9894882.20 | 101 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 9675825.50 | 103 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100114197.40 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 44048394.70 | 23 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100106192.70 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 44047791.00 | 23 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100396562.20 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 2200726.90 | 454 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100306942.50 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1245430.20 | 803 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50193068.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12901.90 | 77508 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50194953.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12954.50 | 77193 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 130436.80 | 7667 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 14790.10 | 67613 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100123706.30 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 52887145.30 | 19 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100104397.40 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 53120413.00 | 19 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100360993.20 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1911416.60 | 523 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100287418.10 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1319751.00 | 758 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50118769.60 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 14918.50 | 67031 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50127703.40 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 15096.70 | 66240 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 261157.10 | 3829 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18828.40 | 53111 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 130984.40 | 7634 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 138941.50 | 7197 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 178586.40 | 5600 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 403234.90 | 2480 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 363665.20 | 2750 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 382031.80 | 2618 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 205270.30 | 4872 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 175199.20 | 5708 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 160390.20 | 6235 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 493461.40 | 2027 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 549451.70 | 1820 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 350480.30 | 2853 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 54320794.90 | 18 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 54213708.30 | 18 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 54004193.80 | 19 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 147062.70 | 6800 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 138696.20 | 7210 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 86897.90 | 11508 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 81883.30 | 12213 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 213892.10 | 4675 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 131126.50 | 7626 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 134051.88 | 7460 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 234293.90 | 4268 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 86686.50 | 11536 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 87808.20 | 11388 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 80520.20 | 12419 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 240694.50 | 4155 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 242909.80 | 4117 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 336345.50 | 2973 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 65184001.30 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 65821938.30 | 15 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60117298.50 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 79435.60 | 12589 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 75892.30 | 13177 | 0.00 |

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

- **Total benchmark runs:** 3472
- **Build range:** 71 - 129
- **Date range:** 2026-06-12 to 2026-06-12
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*