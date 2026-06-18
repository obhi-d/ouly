// SPDX-License-Identifier: MIT
//
// Verifies that selecting the v2 scheduler generation through OULY_SCHEDULER_VERSION
// makes it usable purely through the version-agnostic `ouly::scheduler` alias.
#define OULY_SCHEDULER_VERSION v2

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
static_assert(std::is_same_v<ouly::scheduler, ouly::v2::scheduler>);
static_assert(std::is_same_v<ouly::task_context, ouly::v2::task_context>);
static_assert(std::is_same_v<ouly::workgroup, ouly::v2::workgroup>);

TEST_CASE("v2: task submission via ouly::scheduler alias", "[scheduler][version][v2]")
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

TEST_CASE("v2: parallel_for via ouly::scheduler alias", "[scheduler][version][v2]")
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
// NOLINTEND
