# OULY Performance Tracking

**Generated:** 2025-08-31 10:43:47 UTC

## 📊 Latest Performance Results

**Build Number:** 115
**Commit Hash:** d263892c

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.91 | 255754476 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 206.09 | 4852249 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.11 | 163666121 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.31 | 232018561 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 196.17 | 5097619 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 582807.90 | 1716 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 579947.50 | 1724 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 151243.10 | 6612 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 109262.80 | 9152 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 219799.90 | 4550 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 155208.40 | 6443 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50058408.50 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50113727.40 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7268948.30 | 138 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7273289.30 | 137 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 518825.50 | 1927 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 514148.70 | 1945 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 275121.10 | 3635 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 124496.10 | 8032 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 167132.60 | 5983 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 157002.71 | 6369 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50064237.90 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50111925.20 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8629522.30 | 116 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8535744.40 | 117 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | TaskSubmission_V1 | 146195.60 | 6840 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 145315.00 | 6882 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 159319.20 | 6277 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87748.70 | 11396 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 87423.00 | 11439 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 227185.60 | 4402 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64855.90 | 15419 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 68441.70 | 14611 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79543.30 | 12572 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 236369.80 | 4231 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 237000.00 | 4219 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 309958.00 | 3226 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60003127.90 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 59985740.30 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60012223.70 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64716.70 | 15452 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64844.69 | 15421 | 0.00 |
| gcc-14.2 | TaskSubmission_V1 | 271757.90 | 3680 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 123323.60 | 8109 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 146113.10 | 6844 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 442883.50 | 2258 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 313762.20 | 3187 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 283925.70 | 3522 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 155214.00 | 6443 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 133804.67 | 7474 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 115209.50 | 8680 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 362627.20 | 2758 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 355058.00 | 2816 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 277186.10 | 3608 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51168006.30 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51063299.40 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51101633.50 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 131212.70 | 7621 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 119292.50 | 8383 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100072881.20 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48138632.10 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100090062.10 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48110016.40 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100223425.00 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1782310.80 | 561 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100281766.60 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1167825.90 | 856 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50063726.40 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15278.50 | 65451 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50112671.30 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14981.00 | 66751 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 256724.80 | 3895 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18434.18 | 54247 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100077424.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 40999457.40 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100090899.60 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41009123.50 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100219362.10 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1814476.70 | 551 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100289931.90 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1168351.50 | 856 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50064565.40 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12691.60 | 78792 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50113442.30 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12666.70 | 78947 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 120959.50 | 8267 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 19252.44 | 51941 | 0.00 |

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

- **Total benchmark runs:** 2328
- **Build range:** 71 - 115
- **Date range:** 2025-08-31 to 2025-08-31
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*