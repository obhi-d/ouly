// SPDX-License-Identifier: MIT
//
// Verifies that selecting the v1 scheduler generation through OULY_SCHEDULER_VERSION
// makes it usable purely through the version-agnostic `ouly::scheduler` alias.
#define OULY_SCHEDULER_VERSION v1

#include "catch2/catch_all.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
#include <numeric>
#include <thread>
#include <type_traits>
#include <vector>
// NOLINTBEGIN
// The alias must resolve to the selected generation.
static_assert(std::is_same_v<ouly::scheduler, ouly::v1::scheduler>);
static_assert(std::is_same_v<ouly::task_context, ouly::v1::task_context>);
static_assert(std::is_same_v<ouly::workgroup, ouly::v1::workgroup>);

TEST_CASE("v1: task submission via ouly::scheduler alias", "[scheduler][version][v1]")
{
  std::atomic<uint32_t> count{0};

  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 4);
  scheduler.begin_execution();

  auto const& main_ctx = ouly::task_context::this_context::get();

  for (uint32_t i = 0; i < 1000; ++i)
  {
    scheduler.submit(main_ctx, ouly::workgroup_id(0),
                     [&count](ouly::task_context const&)
                     {
                       count.fetch_add(1, std::memory_order_relaxed);
                     });
  }

  scheduler.end_execution();

  REQUIRE(count.load() == 1000);
}

TEST_CASE("v1: parallel_for via ouly::scheduler alias", "[scheduler][version][v1]")
{
  std::vector<uint32_t> data(10000);
  std::iota(data.begin(), data.end(), 0);

  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 4);
  scheduler.begin_execution();

  auto const& main_ctx = ouly::task_context::this_context::get();

  ouly::parallel_for(
   [](uint32_t& element, ouly::task_context const&)
   {
     element *= 2;
   },
   data, main_ctx);

  scheduler.end_execution();

  for (size_t i = 0; i < data.size(); ++i)
  {
    REQUIRE(data[i] == i * 2);
  }
}

TEST_CASE("v1: task continuations and scopes", "[scheduler][version][v1][task]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  auto value = ouly::submit_task(ctx, []() -> uint32_t { return 40; })
                .then(ctx, [](uint32_t input) -> uint32_t { return input + 2; });
  REQUIRE(value.get(ctx) == 42);

  std::atomic<uint32_t> count{0};
  ouly::task_scope      scope;
  scope.run(ctx, [&count]() { count.fetch_add(1, std::memory_order_relaxed); });
  scope.join(ctx);
  REQUIRE(count.load(std::memory_order_relaxed) == 1);
  scheduler.end_execution();
}
// NOLINTEND
