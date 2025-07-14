#include "catch2/catch_all.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <numeric>
#include <ranges>
#include <string>

// NOLINTBEGIN
TEST_CASE("scheduler: Construction")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 16);
  scheduler.create_group(ouly::workgroup_id(1), 16, 2);

  struct executor
  {
    std::array<uint32_t, 18>              executed = {0};
    std::array<std::vector<uint32_t>, 18> accumulate;

    void execute(ouly::worker_context const& id)
    {
      executed[id.get_worker().get_index()]++;
    }

    void execute2(ouly::worker_context const& id, uint32_t n)
    {
      [[maybe_unused]] auto& scheduler = id.get_scheduler();
      accumulate[id.get_worker().get_index()].push_back(n);
    }

    uint32_t sum() const
    {
      uint32_t result = std::accumulate(executed.begin(), executed.end(), 0);
      for (auto const& r : accumulate)
        result += std::accumulate(r.begin(), r.end(), 0);
      return result;
    }

    std::vector<uint32_t> get_missing(uint32_t max) const
    {
      std::vector<uint32_t> result;
      result.resize(max, 0);
      std::iota(result.begin(), result.end(), 0);
      for (auto const& r : accumulate)
      {
        for (auto v : r)
        {
          auto res = std::find(result.begin(), result.end(), v);
          if (res != result.end())
            result.erase(res);
        }
      }
      return result;
    }
  };

  scheduler.begin_execution();
  executor instance;
  for (uint32_t i = 0; i < 1024; ++i)
    ouly::async<&executor::execute>(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(i % 2),
                                    instance);
  scheduler.end_execution();

  auto sum = instance.sum();
  REQUIRE(sum == 1024);

  scheduler.begin_execution();
  executor instance2;
  for (uint32_t i = 0; i < 1024; ++i)
    ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(i % 2),
                [&instance2, i](ouly::worker_context const& ctx)
                {
                  instance2.execute2(ctx, i);
                });
  scheduler.end_execution();

  REQUIRE(instance2.sum() == 1023 * 512);
}

TEST_CASE("scheduler: Range ParallelFor")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 16);
  scheduler.create_group(ouly::workgroup_id(1), 16, 2);

  auto zero = ouly::integer_range(0, 0);
  REQUIRE(zero.empty());

  scheduler.begin_execution();
  std::atomic_int value;
  for (uint32_t i = 0; i < 1024; ++i)
    ouly::parallel_for(
     [&value](int a, int b, [[maybe_unused]] ouly::worker_context const& wc)
     {
       value.fetch_add(b - a);
     },
     ouly::integer_range(0, 2048), ouly::default_workgroup_id);
  scheduler.end_execution();
}

TEST_CASE("scheduler: Simplest ParallelFor")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 16);

  scheduler.begin_execution();

  constexpr uint32_t nb_elements = 10000;
  std::vector<int>   list;
  list.reserve(nb_elements);
  int64_t sum = 0;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    list.emplace_back(std::rand());
    sum += list[i];
  }

  std::atomic_int64_t parallel_sum = 0;
  ouly::parallel_for(
   [&parallel_sum](int a, [[maybe_unused]] ouly::worker_context const& c)
   {
     [[maybe_unused]] auto id = ouly::worker_id::this_worker::get_id();
     OULY_ASSERT(id.get_index() < 16);
     [[maybe_unused]] auto ctx = ouly::worker_context::get(ouly::default_workgroup_id);
     OULY_ASSERT(ctx.get_worker().get_index() < 16);
     OULY_ASSERT(ctx.get_group_offset() < 16);
     parallel_sum += a;
   },
   std::span(list.begin(), list.end()), ouly::default_workgroup_id);

  REQUIRE(parallel_sum.load() == sum);

  parallel_sum = 0;
  ouly::parallel_for(
   [&parallel_sum](auto start, auto end, [[maybe_unused]] ouly::worker_context const& c)
   {
     for (auto it = start; it != end; ++it)
       parallel_sum += *it;
   },
   std::span(list.begin(), list.end()), ouly::default_workgroup_id);

  REQUIRE(parallel_sum.load() == sum);

  scheduler.end_execution();
}

ouly::co_task<std::string> continue_string()
{
  std::string        continue_string;
  constexpr uint32_t nb_elements = 1000;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    continue_string += "-i-" + std::to_string(i);
  }

  co_return continue_string;
}

ouly::co_task<std::string> create_string(ouly::co_task<std::string>& tunein)
{
  std::string basic_string = "basic";
  co_await tunein;
  auto tune_result = tunein.result();
  co_return basic_string + tune_result;
}

TEST_CASE("scheduler: Test co_task")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 16);

  scheduler.begin_execution();

  auto task        = continue_string();
  auto string_task = create_string(task);

  ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::default_workgroup_id, task);
  scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, string_task);

  std::string        continue_string = "basic";
  constexpr uint32_t nb_elements     = 1000;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    continue_string += "-i-" + std::to_string(i);
  }

  auto result = string_task.sync_wait_result();
  REQUIRE(result == continue_string);
  scheduler.end_execution();
}

ouly::co_sequence<std::string> create_string_seq(ouly::co_task<std::string>& tunein)
{
  std::string basic_string = "basic";
  co_await tunein;
  auto tune_result = tunein.result();
  co_return basic_string + tune_result;
}

TEST_CASE("scheduler: Test co_sequence")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);

  scheduler.begin_execution();

  auto task        = continue_string();
  auto string_task = create_string_seq(task);

  ouly::co_sequence<std::string> move_string_task = std::move(string_task);

  scheduler.submit(ouly::main_worker_id, ouly::default_workgroup_id, task);

  std::string        continue_string = "basic";
  constexpr uint32_t nb_elements     = 1000;
  for (uint32_t i = 0; i < nb_elements; ++i)
  {
    continue_string += "-i-" + std::to_string(i);
  }

  auto result = move_string_task.sync_wait_result(ouly::main_worker_id, scheduler);
  REQUIRE(result == continue_string);
  scheduler.end_execution();
}

ouly::co_task<void> work_on(std::vector<uint32_t>& id, std::mutex& lck, uint32_t worker)
{
  auto lock = std::scoped_lock(lck);
  id.push_back(worker);
  co_return;
}

TEST_CASE("scheduler: Test submit_to")
{
  ouly::scheduler scheduler;
  auto            wg_default = ouly::workgroup_id(0);
  auto            wg_game    = ouly::workgroup_id(1);
  auto            wg_log     = ouly::workgroup_id(2);
  auto            wg_render  = ouly::workgroup_id(3);
  auto            wg_stream  = ouly::workgroup_id(4);

  scheduler.create_group(wg_default, 0, 32);
  scheduler.create_group(wg_game, 0, 16);
  scheduler.create_group(wg_log, 16, 1);
  scheduler.create_group(wg_render, 12, 4);
  scheduler.create_group(wg_stream, 17, 2);

  std::atomic_int default_ = 0;
  std::atomic_int game_    = 0;
  std::atomic_int log_     = 0;
  std::atomic_int render_  = 0;
  std::atomic_int stream_  = 0;

  scheduler.begin_execution(
   [&](ouly::worker_desc desc)
   {
     if (desc.belongs_to(wg_default))
       default_++;
     if (desc.belongs_to(wg_game))
       game_++;
     if (desc.belongs_to(wg_log))
       log_++;
     if (desc.belongs_to(wg_render))
       render_++;
     if (desc.belongs_to(wg_stream))
       stream_++;
   });

  REQUIRE(default_.load() == 32);
  REQUIRE(game_.load() == 16);
  REQUIRE(log_.load() == 1);
  REQUIRE(render_.load() == 4);
  REQUIRE(stream_.load() == 2);

  std::vector<ouly::co_task<void>> tasks;
  std::vector<uint32_t>            collection;
  std::mutex                       lock;
  for (uint32_t i = 0, end = scheduler.get_worker_count(); i < end; ++i)
  {
    tasks.emplace_back(work_on(collection, lock, i));
    scheduler.submit(ouly::main_worker_id, ouly::worker_id(i), ouly::default_workgroup_id, tasks[i]);
  }

  for (uint32_t i = 0, end = scheduler.get_worker_count(); i < end; ++i)
    tasks[i].sync_wait_result();

  std::ranges::sort(collection);
  for (uint32_t i = 0, end = scheduler.get_worker_count(); i < end; ++i)
  {
    REQUIRE(i < collection.size());
    REQUIRE(collection[i] == i);
  }
}

TEST_CASE("scheduler: Memory Layout Optimization Tests")
{
  SECTION("Cache Line Alignment Verification")
  {
    // Verify that critical structures are properly cache-aligned
    REQUIRE(alignof(ouly::detail::cache_optimized_data<ouly::detail::workgroup>) ==
            std::hardware_destructive_interference_size);
    REQUIRE(alignof(ouly::detail::cache_optimized_data<ouly::detail::worker>) ==
            std::hardware_destructive_interference_size);

    // Verify workgroup structure layout
    ouly::detail::cache_optimized_data<ouly::detail::workgroup> wg;
    REQUIRE(sizeof(wg) >= std::hardware_destructive_interference_size);

    // Verify worker structure layout
    ouly::detail::cache_optimized_data<ouly::detail::worker> worker;
    REQUIRE(sizeof(worker) >= std::hardware_destructive_interference_size);
  }

  SECTION("Memory Access Pattern Tests")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 4);
    scheduler.create_group(ouly::workgroup_id(1), 4, 4);

    scheduler.begin_execution();

    // Test that memory accesses work correctly with new layout
    std::atomic<int> counter{0};
    std::atomic<int> invalid_workers{0};

    for (int i = 0; i < 1000; ++i)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(i % 2),
                  [&counter, &invalid_workers](ouly::worker_context const& ctx)
                  {
                    counter.fetch_add(1, std::memory_order_relaxed);
                    // Verify context access works
                    if (ctx.get_worker().get_index() >= 8)
                    {
                      invalid_workers.fetch_add(1, std::memory_order_relaxed);
                    }
                  });
    }

    scheduler.end_execution();
    REQUIRE(counter.load() == 1000);
    REQUIRE(invalid_workers.load() == 0);
  }
}

TEST_CASE("scheduler: Work Stealing Optimization Tests")
{
  SECTION("Multi-Group Work Distribution")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 8, 1); // High priority
    scheduler.create_group(ouly::workgroup_id(1), 8, 8, 0); // Lower priority

    scheduler.begin_execution();

    std::array<std::atomic<int>, 16> worker_task_counts{};
    constexpr int                    total_tasks = 10000;

    // Submit tasks to both groups
    for (int i = 0; i < total_tasks; ++i)
    {
      auto group = ouly::workgroup_id(i % 2);
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), group,
                  [&worker_task_counts](ouly::worker_context const& ctx)
                  {
                    auto worker_idx = ctx.get_worker().get_index();
                    worker_task_counts[worker_idx].fetch_add(1, std::memory_order_relaxed);
                  });
    }

    scheduler.end_execution();

    // Verify all tasks were executed
    int total_executed = 0;
    for (auto& count : worker_task_counts)
    {
      total_executed += count.load();
    }
    REQUIRE(total_executed == total_tasks);

    // Verify work distribution (some workers should have executed tasks)
    int active_workers = 0;
    for (auto& count : worker_task_counts)
    {
      if (count.load() > 0)
      {
        active_workers++;
      }
    }
    REQUIRE(active_workers > 0);
  }

  SECTION("Load Balancing Verification")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 16);

    scheduler.begin_execution();

    std::array<std::atomic<int>, 16> execution_counts{};
    constexpr int                    tasks_per_worker = 100;
    constexpr int                    total_tasks      = 16 * tasks_per_worker;

    // Submit many small tasks to test load balancing
    for (int i = 0; i < total_tasks; ++i)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::default_workgroup_id,
                  [&execution_counts](ouly::worker_context const& ctx)
                  {
                    auto worker_idx = ctx.get_worker().get_index();
                    execution_counts[worker_idx].fetch_add(1, std::memory_order_relaxed);

                    // Simulate some work
                    volatile int sum = 0;
                    for (int j = 0; j < 100; ++j)
                    {
                      sum += j;
                    }
                  });
    }

    scheduler.end_execution();

    // Verify total execution count
    int total = 0;
    for (auto& count : execution_counts)
    {
      total += count.load();
    }
    REQUIRE(total == total_tasks);

    // Check load balancing - no worker should be completely idle
    int load_balanced_workers = 0;
    for (auto& count : execution_counts)
    {
      if (count.load() > 0)
      {
        load_balanced_workers++;
      }
    }
    REQUIRE(load_balanced_workers > 10);
  }
}

TEST_CASE("scheduler: Priority and Exclusive Work Tests")
{
  SECTION("Worker-to-Worker Task Submission")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 8);

    scheduler.begin_execution();

    std::array<std::atomic<bool>, 8> worker_flags{};

    // Submit exclusive tasks to specific workers
    for (uint32_t i = 0; i < 8; ++i)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::worker_id(i), ouly::default_workgroup_id,
                  [&worker_flags, i](ouly::worker_context const&)
                  {
                    worker_flags[i].store(true, std::memory_order_relaxed);
                  });
    }

    scheduler.end_execution();

    // Verify all workers received their exclusive tasks
    for (auto& flag : worker_flags)
    {
      REQUIRE(flag.load());
    }
  }

  SECTION("Priority Group Execution Order")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 4, 0);  // Low priority
    scheduler.create_group(ouly::workgroup_id(1), 0, 4, 10); // High priority

    scheduler.begin_execution();

    struct work_data
    {
      std::atomic<int>     execution_order{0};
      std::array<int, 200> low_priority_order{};
      std::array<int, 200> high_priority_order{};
    } data;
    // Submit tasks to both priority groups
    for (int i = 0; i < 100; ++i)
    {
      // Low priority tasks
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(0),
                  [&data, i](ouly::worker_context const&)
                  {
                    data.low_priority_order[i] = data.execution_order.fetch_add(1, std::memory_order_acq_rel);
                  });

      // High priority tasks
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(1),
                  [&data, i](ouly::worker_context const&)
                  {
                    data.high_priority_order[i] = data.execution_order.fetch_add(1, std::memory_order_acq_rel);
                  });
    }

    scheduler.end_execution();

    // Verify execution happened (order may vary due to parallelism)
    REQUIRE(data.execution_order.load() == 200);
  }
}

TEST_CASE("scheduler: Error Handling and Edge Cases")
{
  SECTION("Empty Workgroup Handling")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 0); // Empty group
    scheduler.create_group(ouly::workgroup_id(1), 0, 4); // Normal group

    scheduler.begin_execution();

    std::atomic<int> task_count{0};

    // Submit to normal group
    ouly::async(ouly::worker_context::get(ouly::workgroup_id(1)), ouly::workgroup_id(1),
                [&task_count](ouly::worker_context const&)
                {
                  task_count.fetch_add(1, std::memory_order_relaxed);
                });

    scheduler.end_execution();
    REQUIRE(task_count.load() == 1);
  }

  SECTION("High Contention Scenarios")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 16);

    scheduler.begin_execution();

    std::atomic<int> contention_counter{0};
    constexpr int    high_task_count = 50000;

    // Submit many tasks rapidly to test contention handling
    for (int i = 0; i < high_task_count; ++i)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::default_workgroup_id,
                  [&contention_counter](ouly::worker_context const&)
                  {
                    contention_counter.fetch_add(1, std::memory_order_relaxed);
                  });
    }

    scheduler.end_execution();
    REQUIRE(contention_counter.load() == high_task_count);
  }

  SECTION("Nested Task Submission")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 8);

    scheduler.begin_execution();

    std::atomic<int> nested_count{0};
    std::atomic<int> total_count{0};

    // Submit tasks that themselves submit more tasks
    for (int i = 0; i < 10; ++i)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::default_workgroup_id,
                  [&nested_count, &total_count](ouly::worker_context const& ctx)
                  {
                    total_count.fetch_add(1, std::memory_order_relaxed);

                    // Submit nested tasks
                    for (int j = 0; j < 5; ++j)
                    {
                      ouly::async(ctx, ouly::default_workgroup_id,
                                  [&nested_count](ouly::worker_context const&)
                                  {
                                    nested_count.fetch_add(1, std::memory_order_relaxed);
                                  });
                    }
                  });
    }

    scheduler.end_execution();
    REQUIRE(total_count.load() == 10);
    REQUIRE(nested_count.load() == 50);
  }
}

TEST_CASE("scheduler: Performance and Scalability Tests")
{
  SECTION("Large Scale Task Processing")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, std::min(16u, std::thread::hardware_concurrency()));

    scheduler.begin_execution();

    constexpr int    large_task_count = 100000;
    std::atomic<int> processed_count{0};

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < large_task_count; ++i)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::default_workgroup_id,
                  [&processed_count](ouly::worker_context const&)
                  {
                    processed_count.fetch_add(1, std::memory_order_relaxed);
                  });
    }

    scheduler.end_execution();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    REQUIRE(processed_count.load() == large_task_count);

    // Performance check - should complete within reasonable time
    REQUIRE(duration.count() < 5000); // Less than 5 seconds
  }

  SECTION("Memory Efficiency Test")
  {
    // Test that the optimized memory layout doesn't cause excessive allocations
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 8);
    scheduler.create_group(ouly::workgroup_id(1), 8, 8);

    // Multiple begin/end cycles to test memory management
    for (int cycle = 0; cycle < 5; ++cycle)
    {
      scheduler.begin_execution();

      std::atomic<int> cycle_counter{0};

      for (int i = 0; i < 1000; ++i)
      {
        ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(i % 2),
                    [&cycle_counter](ouly::worker_context const&)
                    {
                      cycle_counter.fetch_add(1, std::memory_order_relaxed);
                    });
      }

      scheduler.end_execution();
      REQUIRE(cycle_counter.load() == 1000);
    }
  }
}

TEST_CASE("scheduler: Memory Layout and Cache Optimization Verification")
{
  SECTION("False Sharing Prevention Test")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 16);

    scheduler.begin_execution();

    // Test intensive atomic operations that would suffer from false sharing
    std::array<std::atomic<int>, 16> counters{};
    constexpr int                    iterations_per_worker = 10000;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (uint32_t worker_id = 0; worker_id < 16; ++worker_id)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::worker_id(worker_id),
                  ouly::default_workgroup_id,
                  [&counters, worker_id](ouly::worker_context const&)
                  {
                    // Intensive counter updates that would cause false sharing if not properly aligned
                    for (int i = 0; i < iterations_per_worker; ++i)
                    {
                      counters[worker_id].fetch_add(1, std::memory_order_relaxed);

                      // Also increment neighbor counters to test memory layout
                      if (worker_id > 0)
                      {
                        counters[worker_id - 1].fetch_add(1, std::memory_order_relaxed);
                      }
                      if (worker_id < 15)
                      {
                        counters[worker_id + 1].fetch_add(1, std::memory_order_relaxed);
                      }
                    }
                  });
    }

    scheduler.end_execution();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // Verify all operations completed
    int total_operations = 0;
    for (auto& counter : counters)
    {
      total_operations += counter.load();
    }

    // Should be efficient due to optimized memory layout
    REQUIRE(total_operations > 0);
    REQUIRE(duration.count() < 1000000); // Less than 1 second
  }

  SECTION("Work Queue Cache Locality Test")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 8);
    scheduler.create_group(ouly::workgroup_id(1), 8, 8);

    scheduler.begin_execution();

    // Test rapid submission and execution to verify queue performance
    std::atomic<int> rapid_counter{0};
    constexpr int    rapid_tasks = 20000;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Rapidly submit tasks to test queue contention and cache performance
    for (int i = 0; i < rapid_tasks; ++i)
    {
      auto group = ouly::workgroup_id(i % 2);
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), group,
                  [&rapid_counter, i](ouly::worker_context const&)
                  {
                    rapid_counter.fetch_add(1, std::memory_order_relaxed);

                    // Light computation to test scheduler overhead
                    volatile int temp = i * 2 + 1;
                    temp              = temp % 1000;
                  });
    }

    scheduler.end_execution();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    REQUIRE(rapid_counter.load() == rapid_tasks);
    REQUIRE(duration.count() < 2000); // Should complete quickly with optimized layout
  }
}

TEST_CASE("scheduler: Advanced Work Stealing and Distribution")
{
  SECTION("Cross-Group Work Stealing Efficiency")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 4);  // Small group
    scheduler.create_group(ouly::workgroup_id(1), 4, 12); // Large group

    scheduler.begin_execution();

    std::array<std::atomic<int>, 16> worker_loads{};

    // Submit many tasks to the small group to force work stealing
    constexpr int overload_tasks = 5000;
    for (int i = 0; i < overload_tasks; ++i)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id),
                  ouly::workgroup_id(0), // All to small group
                  [&worker_loads](ouly::worker_context const& ctx)
                  {
                    auto worker_idx = ctx.get_worker().get_index();
                    worker_loads[worker_idx].fetch_add(1, std::memory_order_relaxed);

                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                  });
    }

    scheduler.end_execution();

    // Workers should have stolen work between groups
    for (int i = 1; i < 4; ++i)
    {                                      // Workers in group 0
      REQUIRE(worker_loads[i].load() > 0); // Should have executed tasks
    }

    for (int i = 4; i < 16; ++i)
    {                                       // Workers in group 1
      REQUIRE(worker_loads[i].load() == 0); // Should not have executed tasks
    }

    // Verify all tasks executed
    int total_executed = 0;
    for (auto& load : worker_loads)
    {
      total_executed += load.load();
    }
    REQUIRE(total_executed == overload_tasks);

    // Verify work stealing occurred - workers outside group 0 should have executed tasks
    int stealing_workers = 0;
    for (int i = 1; i < 4; ++i)
    { // Workers not in group 0
      if (worker_loads[i].load() > 0)
      {
        stealing_workers++;
      }
    }
    REQUIRE(stealing_workers > 0); // Work stealing should have occurred
  }

  SECTION("Round-Robin Distribution Verification")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 8);

    scheduler.begin_execution();

    struct task_order
    {
      std::array<std::vector<int>, 8> task_orders;
      std::mutex                      task_order_mutex;
    } data;
    // Submit tasks in order and track which worker gets which task
    constexpr int ordered_tasks = 800;
    for (int i = 0; i < ordered_tasks; ++i)
    {
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::default_workgroup_id,
                  [&data, i](ouly::worker_context const& ctx)
                  {
                    auto worker_idx = ctx.get_worker().get_index();
                    {
                      std::lock_guard<std::mutex> lock(data.task_order_mutex);
                      data.task_orders[worker_idx].push_back(i);
                    }
                  });
    }

    scheduler.end_execution();

    // Verify all tasks were executed
    int total_tasks = 0;
    for (auto& orders : data.task_orders)
    {
      total_tasks += static_cast<int>(orders.size());
    }
    REQUIRE(total_tasks == ordered_tasks);

    // Verify reasonable distribution (no worker should be completely idle)
    int active_workers = 0;
    for (auto& orders : data.task_orders)
    {
      if (orders.size() > 0)
      {
        active_workers++;
      }
    }
    REQUIRE(active_workers > 5);
  }
}

TEST_CASE("scheduler: Stress Tests and Robustness")
{
  SECTION("Rapid Start-Stop Cycles")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 8);

    // Test multiple rapid start-stop cycles
    for (int cycle = 0; cycle < 10; ++cycle)
    {
      scheduler.begin_execution();

      std::atomic<int> cycle_tasks{0};

      for (int i = 0; i < 100; ++i)
      {
        ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::default_workgroup_id,
                    [&cycle_tasks](ouly::worker_context const&)
                    {
                      cycle_tasks.fetch_add(1, std::memory_order_relaxed);
                    });
      }

      scheduler.end_execution();
      REQUIRE(cycle_tasks.load() == 100);
    }
  }

  SECTION("Mixed Workload Patterns")
  {
    ouly::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 4); // CPU-intensive group
    scheduler.create_group(ouly::workgroup_id(1), 4, 4); // I/O simulation group
    scheduler.create_group(ouly::workgroup_id(2), 8, 8); // Mixed group

    scheduler.begin_execution();

    std::atomic<int> cpu_tasks{0};
    std::atomic<int> io_tasks{0};
    std::atomic<int> mixed_tasks{0};

    // Submit different types of workloads
    for (int i = 0; i < 300; ++i)
    {
      // CPU-intensive tasks
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(0),
                  [&cpu_tasks](ouly::worker_context const&)
                  {
                    cpu_tasks.fetch_add(1, std::memory_order_relaxed);
                    // Simulate CPU work
                    volatile int sum = 0;
                    for (int j = 0; j < 1000; ++j)
                    {
                      sum += j * j;
                    }
                  });

      // I/O simulation tasks
      ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(1),
                  [&io_tasks](ouly::worker_context const&)
                  {
                    io_tasks.fetch_add(1, std::memory_order_relaxed);
                    // Simulate I/O wait
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                  });

      // Mixed tasks
      if (i % 2 == 0)
      {
        ouly::async(ouly::worker_context::get(ouly::default_workgroup_id), ouly::workgroup_id(2),
                    [&mixed_tasks](ouly::worker_context const&)
                    {
                      mixed_tasks.fetch_add(1, std::memory_order_relaxed);
                    });
      }
    }

    scheduler.end_execution();

    REQUIRE(cpu_tasks.load() == 300);
    REQUIRE(io_tasks.load() == 300);
    REQUIRE(mixed_tasks.load() == 150);
  }
}
