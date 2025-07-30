Task Scheduler Tutorial
======================

OULY's task scheduler provides a high-performance work-stealing framework for parallel execution. This tutorial covers how to use the scheduler for CPU-intensive tasks, parallel algorithms, and coroutine-based programming.

Scheduler Overview
------------------

The OULY scheduler features:

* **Work-stealing architecture** - Efficient load balancing across threads
* **Workgroup organization** - Logical grouping of related tasks
* **Coroutine support** - Modern C++20 coroutines integration
* **Lock-free design** - Minimal contention and overhead
* **Lifecycle management** - Requires begin/end execution calls

Key considerations:

* Manual NUMA optimization may be required for large systems
* Scheduler requires explicit initialization with begin_execution()
* Workgroups must be created before begin_execution()
* Proper cleanup with end_execution() is required

Basic Task Submission
----------------------

The OULY scheduler requires explicit initialization and workgroup setup before use:

.. code-block:: cpp

   #include <ouly/scheduler/scheduler_v2.hpp>
   #include <iostream>
   #include <chrono>

   int main() {
       // Create scheduler with 4 worker threads
       ouly::scheduler scheduler(4);
       
       // Create workgroups BEFORE beginning execution
       auto main_workgroup = scheduler.create_workgroup();
       auto background_workgroup = scheduler.create_workgroup();
       
       // REQUIRED: Begin execution to start worker threads
       scheduler.begin_execution();
       
       // Now submit tasks
       auto future = scheduler.submit(main_workgroup, []() {
           std::this_thread::sleep_for(std::chrono::milliseconds(100));
           return 42;
       });
       
       // Wait for result
       int result = future.get();
       std::cout << "Task result: " << result << "\n";
       
       // REQUIRED: End execution before shutdown
       scheduler.end_execution();
       scheduler.shutdown();
       return 0;
   }

Task Context and Submission
---------------------------

For efficient task scheduling, use the task context for nested submissions:

.. code-block:: cpp

   #include <ouly/scheduler/scheduler_v2.hpp>
   #include <vector>

   int parallel_sum(const std::vector<int>& data, size_t start, size_t end) {
       if (end - start <= 1000) {
           // Base case: compute directly
           return std::accumulate(data.begin() + start, data.begin() + end, 0);
       }
       
       // Divide and conquer
       size_t mid = start + (end - start) / 2;
       return parallel_sum(data, start, mid) + parallel_sum(data, mid, end);
   }

   void task_based_sum(ouly::task_context& ctx, 
                      const std::vector<int>& data,
                      size_t start, size_t end,
                      std::promise<int>& result) {
       if (end - start <= 1000) {
           // Base case
           int sum = std::accumulate(data.begin() + start, data.begin() + end, 0);
           result.set_value(sum);
           return;
       }
       
       // Create promises for sub-tasks
       std::promise<int> left_promise, right_promise;
       auto left_future = left_promise.get_future();
       auto right_future = right_promise.get_future();
       
       size_t mid = start + (end - start) / 2;
       
       // Submit left half
       ouly::async(ctx, ctx.current_workgroup(), 
           [&data, start, mid, &left_promise](auto& ctx) {
               task_based_sum(ctx, data, start, mid, left_promise);
           });
       
       // Submit right half  
       ouly::async(ctx, ctx.current_workgroup(),
           [&data, mid, end, &right_promise](auto& ctx) {
               task_based_sum(ctx, data, mid, end, right_promise);
           });
       
       // Combine results
       int left_sum = left_future.get();
       int right_sum = right_future.get();
       result.set_value(left_sum + right_sum);
   }

   int main() {
       ouly::scheduler scheduler(std::thread::hardware_concurrency());
       
       // Create workgroup BEFORE begin_execution
       auto workgroup = scheduler.create_workgroup();
       
       // Begin execution to start worker threads
       scheduler.begin_execution();
       
       // Create large dataset
       std::vector<int> data(1000000);
       std::iota(data.begin(), data.end(), 1);
       
       std::promise<int> result_promise;
       auto result_future = result_promise.get_future();
       
       scheduler.submit(workgroup, [&](auto& ctx) {
           task_based_sum(ctx, data, 0, data.size(), result_promise);
       });
       
       int result = result_future.get();
       std::cout << "Parallel sum: " << result << "\n";
       
       scheduler.end_execution();
       scheduler.shutdown();
       return 0;
   }

Workgroups and Organization
---------------------------

Workgroups help organize related tasks and manage dependencies:

.. code-block:: cpp

   #include <ouly/scheduler/scheduler_v2.hpp>

   int main() {
       ouly::scheduler scheduler(4);
       
       // Create workgroups for different phases BEFORE begin_execution
       auto input_group = scheduler.create_workgroup();
       auto processing_group = scheduler.create_workgroup(); 
       auto output_group = scheduler.create_workgroup();
       
       // Begin execution to activate worker threads
       scheduler.begin_execution();
       
       std::vector<int> input_data;
       std::vector<int> processed_data;
       
       // Phase 1: Input tasks
       std::vector<std::future<void>> input_futures;
       for (int i = 0; i < 10; ++i) {
           auto future = scheduler.submit(input_group, [&input_data, i]() {
               std::lock_guard<std::mutex> lock(input_mutex);
               input_data.push_back(i * i);
           });
           input_futures.push_back(std::move(future));
       }
       
       // Wait for input phase to complete
       for (auto& future : input_futures) {
           future.wait();
       }
       
       // Phase 2: Processing tasks
       processed_data.resize(input_data.size());
       std::vector<std::future<void>> process_futures;
       
       for (size_t i = 0; i < input_data.size(); ++i) {
           auto future = scheduler.submit(processing_group, [&, i]() {
               processed_data[i] = input_data[i] * 2 + 1;
           });
           process_futures.push_back(std::move(future));
       }
       
       // Wait for processing
       for (auto& future : process_futures) {
           future.wait();
       }
       
       // Phase 3: Output
       scheduler.submit(output_group, [&]() {
           std::cout << "Processed data: ";
           for (int value : processed_data) {
               std::cout << value << " ";
           }
           std::cout << "\n";
       }).wait();
       
       // End execution before shutdown
       scheduler.end_execution();
       scheduler.shutdown();
       return 0;
   }

Coroutine Integration
---------------------

OULY supports C++20 coroutines for elegant asynchronous programming:

.. code-block:: cpp

   #include <ouly/scheduler/scheduler_v2.hpp>
   #include <ouly/scheduler/co_task.hpp>

   // Coroutine task that performs async computation
   ouly::co_task<int> async_factorial(int n) {
       if (n <= 1) {
           co_return 1;
       }
       
       // Submit recursive call as coroutine
       auto sub_result = co_await async_factorial(n - 1);
       co_return n * sub_result;
   }

   // File I/O simulation with coroutines
   ouly::co_task<std::string> async_read_file(const std::string& filename) {
       // Simulate async file read
       co_await std::suspend_always{};  // Yield execution
       
       // Simulate file content
       co_return "Content of " + filename;
   }

   ouly::co_task<void> process_files(const std::vector<std::string>& filenames) {
       std::vector<ouly::co_task<std::string>> read_tasks;
       
       // Start all reads concurrently
       for (const auto& filename : filenames) {
           read_tasks.push_back(async_read_file(filename));
       }
       
       // Process results as they complete
       for (auto& task : read_tasks) {
           std::string content = co_await task;
           std::cout << "Processed: " << content << "\n";
       }
   }

   int main() {
       ouly::scheduler scheduler(4);
       
       // Create workgroups BEFORE begin_execution
       auto workgroup = scheduler.create_workgroup();
       
       // Begin execution
       scheduler.begin_execution();
       
       // Submit coroutine task
       auto factorial_task = async_factorial(10);
       auto factorial_future = scheduler.submit(workgroup, std::move(factorial_task));
       
       int result = factorial_future.get();
       std::cout << "10! = " << result << "\n";
       
       // File processing example
       std::vector<std::string> files = {"file1.txt", "file2.txt", "file3.txt"};
       auto file_task = process_files(files);
       auto file_future = scheduler.submit(workgroup, std::move(file_task));
       file_future.wait();
       
       scheduler.end_execution();
       scheduler.shutdown();
       return 0;
   }

Parallel Algorithms
-------------------

Implement common parallel patterns using the scheduler:

.. code-block:: cpp

   #include <ouly/scheduler/scheduler_v2.hpp>
   #include <algorithm>

   // Parallel for_each implementation
   template<typename Iterator, typename Function>
   void parallel_for_each(ouly::task_context& ctx,
                         Iterator first, Iterator last,
                         Function func,
                         size_t grain_size = 1000) {
       auto distance = std::distance(first, last);
       if (distance <= grain_size) {
           // Process directly
           std::for_each(first, last, func);
           return;
       }
       
       // Split work
       auto mid = std::next(first, distance / 2);
       
       std::promise<void> left_done;
       auto left_future = left_done.get_future();
       
       // Process left half in parallel
       ouly::async(ctx, ctx.current_workgroup(), 
           [&ctx, first, mid, func, grain_size, &left_done](auto&) {
               parallel_for_each(ctx, first, mid, func, grain_size);
               left_done.set_value();
           });
       
       // Process right half in current task
       parallel_for_each(ctx, mid, last, func, grain_size);
       
       // Wait for left half to complete
       left_future.wait();
   }

   // Parallel reduce implementation
   template<typename Iterator, typename T, typename BinaryOp>
   T parallel_reduce(ouly::task_context& ctx,
                    Iterator first, Iterator last,
                    T init, BinaryOp op,
                    size_t grain_size = 1000) {
       auto distance = std::distance(first, last);
       if (distance <= grain_size) {
           return std::accumulate(first, last, init, op);
       }
       
       auto mid = std::next(first, distance / 2);
       
       std::promise<T> left_result;
       auto left_future = left_result.get_future();
       
       // Process left half
       ouly::async(ctx, ctx.current_workgroup(),
           [&ctx, first, mid, init, op, grain_size, &left_result](auto&) {
               T result = parallel_reduce(ctx, first, mid, init, op, grain_size);
               left_result.set_value(result);
           });
       
       // Process right half
       T right = parallel_reduce(ctx, mid, last, init, op, grain_size);
       T left = left_future.get();
       
       return op(left, right);
   }

   int main() {
       ouly::scheduler scheduler(std::thread::hardware_concurrency());
       
       // Create workgroup BEFORE begin_execution
       auto workgroup = scheduler.create_workgroup();
       
       // Begin execution
       scheduler.begin_execution();
       
       std::vector<int> data(1000000);
       std::iota(data.begin(), data.end(), 1);
       
       // Parallel for_each example
       scheduler.submit(workgroup, [&](auto& ctx) {
           parallel_for_each(ctx, data.begin(), data.end(), [](int& x) {
               x = x * x;  // Square each element
           });
       }).wait();
       
       // Parallel reduce example
       auto sum_future = scheduler.submit(workgroup, [&](auto& ctx) {
           return parallel_reduce(ctx, data.begin(), data.end(), 0LL, std::plus<long long>{});
       });
       
       long long sum = sum_future.get();
       std::cout << "Sum of squares: " << sum << "\n";
       
       scheduler.end_execution();
       scheduler.shutdown();
       return 0;
   }

Producer-Consumer Patterns
--------------------------

Implement efficient producer-consumer patterns:

.. code-block:: cpp

   #include <ouly/scheduler/scheduler_v2.hpp>
   #include <queue>
   #include <mutex>
   #include <condition_variable>

   template<typename T>
   class ThreadSafeQueue {
       std::queue<T> queue_;
       mutable std::mutex mutex_;
       std::condition_variable condition_;
       bool finished_ = false;
       
   public:
       void push(T item) {
           std::lock_guard<std::mutex> lock(mutex_);
           queue_.push(std::move(item));
           condition_.notify_one();
       }
       
       bool pop(T& item) {
           std::unique_lock<std::mutex> lock(mutex_);
           condition_.wait(lock, [this] { return !queue_.empty() || finished_; });
           
           if (queue_.empty()) {
               return false;  // Finished
           }
           
           item = std::move(queue_.front());
           queue_.pop();
           return true;
       }
       
       void finish() {
           std::lock_guard<std::mutex> lock(mutex_);
           finished_ = true;
           condition_.notify_all();
       }
   };

   int main() {
       ouly::scheduler scheduler(4);
       
       // Create workgroups BEFORE begin_execution
       auto producer_group = scheduler.create_workgroup();
       auto consumer_group = scheduler.create_workgroup();
       
       // Begin execution
       scheduler.begin_execution();
       
       ThreadSafeQueue<int> queue;
       
       // Start producers
       std::vector<std::future<void>> producer_futures;
       for (int p = 0; p < 2; ++p) {
           auto future = scheduler.submit(producer_group, [&queue, p]() {
               for (int i = 0; i < 100; ++i) {
                   queue.push(p * 100 + i);
                   std::this_thread::sleep_for(std::chrono::milliseconds(1));
               }
           });
           producer_futures.push_back(std::move(future));
       }
       
       // Start consumers
       std::atomic<int> total_consumed{0};
       std::vector<std::future<void>> consumer_futures;
       
       for (int c = 0; c < 3; ++c) {
           auto future = scheduler.submit(consumer_group, [&queue, &total_consumed, c]() {
               int item;
               int consumed = 0;
               while (queue.pop(item)) {
                   // Process item
                   consumed++;
                   std::this_thread::sleep_for(std::chrono::microseconds(100));
               }
               total_consumed += consumed;
               std::cout << "Consumer " << c << " processed " << consumed << " items\n";
           });
           consumer_futures.push_back(std::move(future));
       }
       
       // Wait for producers to finish
       for (auto& future : producer_futures) {
           future.wait();
       }
       
       // Signal consumers that production is done
       queue.finish();
       
       // Wait for consumers
       for (auto& future : consumer_futures) {
           future.wait();
       }
       
       std::cout << "Total items processed: " << total_consumed.load() << "\n";
       
       scheduler.end_execution();
       scheduler.shutdown();
       return 0;
   }

Performance Considerations
--------------------------

**Task Granularity**

.. code-block:: cpp

   // Too fine-grained (overhead dominates)
   for (int i = 0; i < 1000000; ++i) {
       scheduler.submit(workgroup, [i]() { return i * i; });
   }

   // Better: Batch processing
   constexpr size_t BATCH_SIZE = 1000;
   for (size_t start = 0; start < data.size(); start += BATCH_SIZE) {
       size_t end = std::min(start + BATCH_SIZE, data.size());
       scheduler.submit(workgroup, [&data, start, end]() {
           for (size_t i = start; i < end; ++i) {
               data[i] = data[i] * data[i];
           }
       });
   }

**Memory Access Patterns**

.. code-block:: cpp

   // Cache-friendly: Sequential access
   void process_sequential(ouly::task_context& ctx, std::vector<float>& data) {
       const size_t chunk_size = data.size() / ctx.worker_count();
       
       for (size_t w = 0; w < ctx.worker_count(); ++w) {
           size_t start = w * chunk_size;
           size_t end = (w == ctx.worker_count() - 1) ? data.size() : start + chunk_size;
           
           ouly::async(ctx, ctx.current_workgroup(), [&data, start, end](auto&) {
               for (size_t i = start; i < end; ++i) {
                   data[i] = std::sqrt(data[i]);
               }
           });
       }
   }

**NUMA Awareness**

The OULY scheduler does not have built-in NUMA awareness. For NUMA optimization, you need to manually configure thread affinity:

.. code-block:: cpp

   int main() {
       // Basic scheduler setup - no automatic NUMA configuration
       ouly::scheduler scheduler(std::thread::hardware_concurrency());
       
       // Create workgroups BEFORE begin_execution
       auto workgroup = scheduler.create_workgroup();
       
       // Begin execution
       scheduler.begin_execution();
       
       // For NUMA optimization, you would need to:
       // 1. Manually set thread affinity using platform-specific APIs
       // 2. Allocate memory on appropriate NUMA nodes
       // 3. Partition work based on NUMA topology
       
       // ... use scheduler
       
       scheduler.end_execution();
       scheduler.shutdown();
       return 0;
   }

Best Practices
--------------

1. **Task Design**
   
   * Keep tasks independent when possible
   * Batch small operations for better efficiency
   * Use appropriate grain size for parallel algorithms

2. **Workgroup Organization**
   
   * Create all workgroups before calling ``begin_execution()``
   * Group related tasks in the same workgroup
   * Use separate workgroups for different phases
   * Consider dependencies between workgroups

3. **Scheduler Lifecycle**
   
   * Always call ``begin_execution()`` before submitting tasks
   * Call ``end_execution()`` before ``shutdown()``
   * Create workgroups before beginning execution
   * Handle exceptions properly to ensure cleanup

3. **Memory Management**
   
   * Minimize shared mutable state
   * Use thread-local storage for worker-specific data
   * Consider cache-friendly data layouts

4. **Error Handling**
   
   * Handle exceptions in task functions
   * Use promises/futures for error propagation
   * Implement timeout mechanisms for long-running tasks

5. **Debugging and Profiling**
   
   * Use scheduler statistics for performance analysis
   * Profile task execution times
   * Monitor work-stealing efficiency

Next Steps
----------

* Explore :doc:`ecs_tutorial` for parallel ECS processing patterns
* Learn about :doc:`memory_management` for scheduler-aware allocation
* Check :doc:`../performance/index` for scheduler tuning guide
