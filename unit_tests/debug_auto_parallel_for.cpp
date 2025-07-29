#include "catch2/catch_all.hpp"
#include "ouly/scheduler/auto_parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
#include <iostream>
#include <numeric>
#include <vector>

// Template wrapper for testing both scheduler versions
template <typename SchedulerType, typename TaskContextType>
struct SchedulerTestRunner
{
  using scheduler_type    = SchedulerType;
  using task_context_type = TaskContextType;

  static auto setup_scheduler(uint32_t worker_count = std::thread::hardware_concurrency()) -> SchedulerType
  {
    SchedulerType scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, worker_count);
    return scheduler;
  }

  static auto get_main_context() -> TaskContextType const&
  {
    return task_context_type::this_context::get();
  }
};

TEST_CASE("Debug Auto Parallel For", "[simple_debug_test_with_small_data]")
{
  const int num_elements = 200; // Force parallel execution

  std::cout << "Starting debug test with " << num_elements << " elements\\n";

  std::vector<int> data(num_elements);
  std::iota(data.begin(), data.end(), 0);

  // Create atomic flags to track which elements are processed
  std::vector<std::atomic<bool>> processed(num_elements);
  for (auto& flag : processed)
  {
    flag.store(false);
  }

  std::atomic<int> sum{0};

  auto lambda = [&](int& element, auto const& /*wc*/)
  {
    auto index    = element;
    bool expected = false;
    if (processed[index].compare_exchange_strong(expected, true))
    {
      sum.fetch_add(element);
    }
    else
    {
      std::cout << "ERROR: Element " << element << " processed multiple times!\\n";
    }
  };

  // Test with scheduler v1
  {
    ouly::v1::scheduler scheduler;
    scheduler.create_group(ouly::workgroup_id(0), 0, 4);
    scheduler.begin_execution();
    auto const& ctx = ouly::v1::task_context::this_context::get();
    ouly::auto_parallel_for(lambda, data, ctx);
    scheduler.end_execution();
  }

  std::cout << "Checking processed flags...\\n";

  int expected_sum    = 0;
  int actual_sum      = sum.load();
  int processed_count = 0;

  for (int i = 0; i < num_elements; ++i)
  {
    expected_sum += i;
    if (processed[i].load())
    {
      processed_count++;
    }
    else
    {
      std::cout << "Element " << i << " was NOT processed!\\n";
    }
  }

  std::cout << "Expected sum: " << expected_sum << "\\n";
  std::cout << "Actual sum: " << actual_sum << "\\n";
  std::cout << "Processed " << processed_count << " out of " << num_elements << " elements\\n";

  REQUIRE(actual_sum == expected_sum);
}
