# OULY Performance Tracking

**Generated:** 2025-08-30 11:10:57 UTC

## 📊 Latest Performance Results

**Build Number:** 112
**Commit Hash:** 3a10444a

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 589706.90 | 1696 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 563968.80 | 1773 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 148951.30 | 6714 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 122787.50 | 8144 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 219804.70 | 4549 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 172455.20 | 5799 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50065289.50 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50110589.00 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7319045.70 | 137 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7356731.00 | 136 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 516178.20 | 1937 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 514750.50 | 1943 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 150823.10 | 6630 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 139156.20 | 7186 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 166738.80 | 5997 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 155476.70 | 6432 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50065954.90 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50111400.20 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8650486.80 | 116 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8558736.30 | 117 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.91 | 255754476 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 220.21 | 4541120 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.10 | 163934426 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 194.77 | 5134261 | 0.00 |

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 151476.60 | 6602 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 151731.10 | 6591 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 167672.70 | 5964 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 228800.80 | 4371 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 233518.60 | 4282 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 225809.20 | 4429 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 217467.70 | 4598 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 108210.90 | 9241 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113360.50 | 8821 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 251194.60 | 3981 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 324628.50 | 3080 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 342308.50 | 2921 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51023989.70 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51080963.00 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51050880.40 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 130487.80 | 7664 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 118304.00 | 8453 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 146542.60 | 6824 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 147159.80 | 6795 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 235190.20 | 4252 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 87947.30 | 11370 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 88027.40 | 11360 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 228257.30 | 4381 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 64852.10 | 15420 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 68964.80 | 14500 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 79269.10 | 12615 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 248289.70 | 4028 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 238104.60 | 4200 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 297189.00 | 3365 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60482041.50 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60123271.00 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60017581.20 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64880.30 | 15413 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 65095.70 | 15362 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100075096.20 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 40957995.20 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100090489.20 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41095364.40 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100224470.20 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1838337.70 | 544 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100270398.50 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1161571.10 | 861 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50065340.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12690.60 | 78798 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50112502.40 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12669.70 | 78928 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 121828.20 | 8208 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18237.03 | 54834 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100072896.90 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48073649.20 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100090379.30 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48112955.00 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100232768.40 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1782429.50 | 561 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100281010.50 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1158595.00 | 863 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50064547.70 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15280.60 | 65442 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50113300.30 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 14979.00 | 66760 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 255998.60 | 3906 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18316.30 | 54596 | 0.00 |

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

- **Total benchmark runs:** 2196
- **Build range:** 71 - 112
- **Date range:** 2025-08-30 to 2025-08-30
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*