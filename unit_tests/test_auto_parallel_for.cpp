#include "catch2/catch_all.hpp"
#include "ouly/scheduler/auto_parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
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

TEMPLATE_TEST_CASE("Auto Parallel For Basic Functionality", "[auto_parallel_for][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>))
{
  using TestRunner = TestType;

  auto scheduler = TestRunner::setup_scheduler(4);

  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  SECTION("Basic summation with auto_parallel_for")
  {
    constexpr size_t data_size = 1000;
    std::vector<int> data(data_size);
    std::iota(data.begin(), data.end(), 0);

    std::atomic<int64_t> sum{0};

    // Use auto_parallel_for to sum all elements
    ouly::auto_parallel_for(
     [&sum](int& value, auto const& /*context*/)
     {
       sum.fetch_add(value, std::memory_order_relaxed);
     },
     data, main_ctx);

    // Verify the result
    auto    expected_sum = static_cast<int64_t>((data_size - 1) * data_size / 2);
    int64_t actual_sum   = sum.load();

    REQUIRE(actual_sum == expected_sum);

    scheduler.end_execution();
  }

  SECTION("Auto parallel_for with range-based lambda")
  {
    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 1); // 1 to 100

    std::atomic<int64_t> sum{0};

    // Use auto_parallel_for with range-based processing
    ouly::auto_parallel_for(
     [&sum](auto begin, auto end, auto const& /*context*/)
     {
       for (auto it = begin; it != end; ++it)
       {
         sum.fetch_add(*it, std::memory_order_relaxed);
       }
     },
     data, main_ctx);

    // Sum of 1 to 100 = 100 * 101 / 2 = 5050
    int64_t expected_sum = 5050;
    int64_t actual_sum   = sum.load();

    REQUIRE(actual_sum == expected_sum);

    scheduler.end_execution();
  }

  SECTION("Auto parallel_for with small dataset (sequential execution)")
  {
    std::vector<int> small_data{1, 2, 3, 4, 5};
    std::atomic<int> sum{0};

    ouly::auto_parallel_for(
     [&sum](int& value, auto const& /*context*/)
     {
       sum.fetch_add(value, std::memory_order_relaxed);
     },
     small_data, main_ctx);

    REQUIRE(sum.load() == 15); // 1+2+3+4+5 = 15

    scheduler.end_execution();
  }
}

TEST_CASE("range_pool: split_to_fill partitions the range without overlap", "[auto_parallel_for][range_pool]")
{
  using range_t = ouly::subrange<uint32_t>;
  ouly::range_pool<range_t, 8> pool(range_t{0, 1000});
  pool.split_to_fill(3, 1);
  REQUIRE(pool.size() > 1);

  std::vector<uint32_t> hits(1000, 0);
  while (!pool.empty())
  {
    auto r = pool.front();
    REQUIRE(pool.front_depth() <= 3);
    for (auto i = r.begin(); i != r.end(); ++i)
    {
      ++hits[i];
    }
    pool.pop_front();
  }
  for (auto h : hits)
  {
    REQUIRE(h == 1);
  }

  // a range that cannot be split stays as is
  ouly::range_pool<range_t, 8> single(range_t{0, 1});
  single.split_to_fill(3, 1);
  REQUIRE(single.size() == 1);
}

TEMPLATE_TEST_CASE("Auto Parallel For visits every element exactly once", "[auto_parallel_for][template]",
                   (SchedulerTestRunner<ouly::v1::scheduler, ouly::v1::task_context>),
                   (SchedulerTestRunner<ouly::v2::scheduler, ouly::v2::task_context>),
                   (SchedulerTestRunner<ouly::v3::scheduler, ouly::v3::task_context>))
{
  using TestRunner = TestType;

  auto scheduler = TestRunner::setup_scheduler(4);
  scheduler.begin_execution();
  auto const& main_ctx = TestRunner::get_main_context();

  for (int round = 0; round < 20; ++round)
  {
    constexpr uint32_t                 count = 50000;
    std::vector<uint32_t>              data(count);
    std::vector<std::atomic<uint32_t>> visits(count);
    std::iota(data.begin(), data.end(), 0U);

    ouly::auto_parallel_for(
     [&visits](uint32_t& value, auto const& /*context*/)
     {
       // uneven work so that chunks get stolen and split further
       if ((value % 64) == 0)
       {
         std::this_thread::yield();
       }
       visits[value].fetch_add(1, std::memory_order_relaxed);
     },
     data, main_ctx);

    for (auto const& v : visits)
    {
      REQUIRE(v.load(std::memory_order_relaxed) == 1);
    }
  }

  scheduler.end_execution();
}
