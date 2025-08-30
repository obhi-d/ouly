# OULY Performance Tracking

**Generated:** 2025-08-30 02:09:54 UTC

## 📊 Latest Performance Results

**Build Number:** 110
**Commit Hash:** c2c47311

### Scheduler Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | TaskSubmission_V1 | 146758.70 | 6814 | 0.00 |
| gcc-14.2 | TaskSubmission_V2 | 146835.29 | 6810 | 0.00 |
| gcc-14.2 | TaskSubmission_TBB | 238991.90 | 4184 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V1 | 236267.80 | 4232 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_V2 | 244864.80 | 4084 | 0.00 |
| gcc-14.2 | ParallelFor_VectorOps_TBB | 236597.30 | 4227 | 0.00 |
| gcc-14.2 | MatrixOps_V1 | 170953.90 | 5850 | 0.00 |
| gcc-14.2 | MatrixOps_V2 | 170848.70 | 5853 | 0.00 |
| gcc-14.2 | MatrixOps_TBB | 113428.60 | 8816 | 0.00 |
| gcc-14.2 | MixedWorkload_V1 | 336161.10 | 2975 | 0.00 |
| gcc-14.2 | MixedWorkload_V2 | 413390.20 | 2419 | 0.00 |
| gcc-14.2 | MixedWorkload_TBB | 325766.70 | 3070 | 0.00 |
| gcc-14.2 | TaskThroughput_V1 | 51263276.70 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_V2 | 51017659.50 | 20 | 0.00 |
| gcc-14.2 | TaskThroughput_TBB | 51085292.00 | 20 | 0.00 |
| gcc-14.2 | NestedParallel_V1 | 131084.50 | 7629 | 0.00 |
| gcc-14.2 | NestedParallel_V2 | 118400.90 | 8446 | 0.00 |
| gcc-4.2 | TaskSubmission_V1 | 140896.50 | 7097 | 0.00 |
| gcc-4.2 | TaskSubmission_V2 | 139846.50 | 7151 | 0.00 |
| gcc-4.2 | TaskSubmission_TBB | 153536.00 | 6513 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V1 | 88014.90 | 11362 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_V2 | 88015.90 | 11362 | 0.00 |
| gcc-4.2 | ParallelFor_VectorOps_TBB | 258772.00 | 3864 | 0.00 |
| gcc-4.2 | MatrixOps_V1 | 97140.90 | 10294 | 0.00 |
| gcc-4.2 | MatrixOps_V2 | 95174.30 | 10507 | 0.00 |
| gcc-4.2 | MatrixOps_TBB | 81363.50 | 12291 | 0.00 |
| gcc-4.2 | MixedWorkload_V1 | 829820.60 | 1205 | 0.00 |
| gcc-4.2 | MixedWorkload_V2 | 431282.30 | 2319 | 0.00 |
| gcc-4.2 | MixedWorkload_TBB | 264203.00 | 3785 | 0.00 |
| gcc-4.2 | TaskThroughput_V1 | 60259294.80 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_V2 | 60157989.10 | 17 | 0.00 |
| gcc-4.2 | TaskThroughput_TBB | 60034451.90 | 17 | 0.00 |
| gcc-4.2 | NestedParallel_V1 | 64984.10 | 15388 | 0.00 |
| gcc-4.2 | NestedParallel_V2 | 64867.80 | 15416 | 0.00 |

### Performance Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | ParallelCompute_Coroutines_V1 | 100075530.80 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V1 | 41060761.60 | 24 | 0.00 |
| gcc-14.2 | ParallelCompute_Coroutines_V2 | 100089522.70 | 10 | 0.00 |
| gcc-14.2 | ParallelCompute_RegularTasks_V2 | 41075111.10 | 24 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V1 | 100220381.90 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V1 | 1828312.90 | 547 | 0.00 |
| gcc-14.2 | TaskChaining_Coroutines_V2 | 100253705.50 | 10 | 0.00 |
| gcc-14.2 | TaskChaining_NestedSubmission_V2 | 1158553.50 | 863 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V1 | 50065077.20 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V1 | 12696.70 | 78761 | 0.00 |
| gcc-14.2 | FanOutIn_Coroutines_V2 | 50113308.30 | 20 | 0.00 |
| gcc-14.2 | FanOutIn_ParallelFor_V2 | 12667.60 | 78942 | 0.00 |
| gcc-14.2 | TaskChaining_TBB_Equivalent | 119921.80 | 8339 | 0.00 |
| gcc-14.2 | FanOutIn_TBB_Equivalent | 18393.30 | 54368 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V1 | 100074970.10 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V1 | 48131576.70 | 21 | 0.00 |
| gcc-4.2 | ParallelCompute_Coroutines_V2 | 100089594.20 | 10 | 0.00 |
| gcc-4.2 | ParallelCompute_RegularTasks_V2 | 48180589.60 | 21 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V1 | 100226513.10 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V1 | 1819075.00 | 550 | 0.00 |
| gcc-4.2 | TaskChaining_Coroutines_V2 | 100265493.30 | 10 | 0.00 |
| gcc-4.2 | TaskChaining_NestedSubmission_V2 | 1165936.50 | 858 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V1 | 50060257.00 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V1 | 15277.50 | 65456 | 0.00 |
| gcc-4.2 | FanOutIn_Coroutines_V2 | 50112634.60 | 20 | 0.00 |
| gcc-4.2 | FanOutIn_ParallelFor_V2 | 15051.00 | 66441 | 0.00 |
| gcc-4.2 | TaskChaining_TBB_Equivalent | 256360.40 | 3901 | 0.00 |
| gcc-4.2 | FanOutIn_TBB_Equivalent | 18377.20 | 54415 | 0.00 |

### Allocator Performance

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| clang-18 | ts_shared_linear_single_thread | 6.51 | 153609831 | 0.00 |
| clang-18 | ts_thread_local_single_thread | 3.90 | 256410256 | 0.00 |
| clang-18 | coalescing_arena_alloc_dealloc | 219.51 | 4555601 | 0.00 |
| gcc-14 | ts_shared_linear_single_thread | 6.02 | 166112957 | 0.00 |
| gcc-14 | ts_thread_local_single_thread | 4.30 | 232558140 | 0.00 |
| gcc-14 | coalescing_arena_alloc_dealloc | 204.98 | 4878525 | 0.00 |

### Overhead Comparison

| Compiler | Benchmark | Median Time (ns) | Ops/sec | Error % |
|----------|-----------|------------------|---------|---------|
| gcc-14.2 | CoroutineCreation_V1 | 581723.60 | 1719 | 0.00 |
| gcc-14.2 | CoroutineCreation_V2 | 565268.70 | 1769 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V1 | 155218.60 | 6443 | 0.00 |
| gcc-14.2 | LambdaSubmission_V1 | 127344.60 | 7853 | 0.00 |
| gcc-14.2 | CoroutineSubmission_V2 | 197497.50 | 5063 | 0.00 |
| gcc-14.2 | LambdaSubmission_V2 | 162771.70 | 6144 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V1 | 50064885.10 | 20 | 0.00 |
| gcc-14.2 | SuspensionOverhead_V2 | 50110670.50 | 20 | 0.00 |
| gcc-14.2 | CoroutineMemory_V1 | 7235377.30 | 138 | 0.00 |
| gcc-14.2 | CoroutineMemory_V2 | 7287491.50 | 137 | 0.00 |
| gcc-4.2 | CoroutineCreation_V1 | 516300.50 | 1937 | 0.00 |
| gcc-4.2 | CoroutineCreation_V2 | 516120.30 | 1938 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V1 | 155624.90 | 6426 | 0.00 |
| gcc-4.2 | LambdaSubmission_V1 | 129917.00 | 7697 | 0.00 |
| gcc-4.2 | CoroutineSubmission_V2 | 166840.70 | 5994 | 0.00 |
| gcc-4.2 | LambdaSubmission_V2 | 154602.00 | 6468 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V1 | 50065924.00 | 20 | 0.00 |
| gcc-4.2 | SuspensionOverhead_V2 | 50113603.40 | 20 | 0.00 |
| gcc-4.2 | CoroutineMemory_V1 | 8717149.20 | 115 | 0.00 |
| gcc-4.2 | CoroutineMemory_V2 | 8620492.20 | 116 | 0.00 |

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

- **Total benchmark runs:** 2064
- **Build range:** 71 - 110
- **Date range:** 2025-08-30 to 2025-08-30
- **Compilers tested:** clang-18, gcc-14, gcc-14.2, gcc-4.2
- **Test categories:** allocator_performance, scheduler_comparison, overhead_comparison, performance_comparison

---
*This report is automatically generated from benchmark results stored in the perfo branch.*