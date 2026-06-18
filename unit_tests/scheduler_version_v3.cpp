// SPDX-License-Identifier: MIT
//
// Verifies that selecting the v3 scheduler generation through OULY_SCHEDULER_VERSION
// makes it usable purely through the version-agnostic `ouly::scheduler` alias.
#define OULY_SCHEDULER_VERSION v3

#include "catch2/catch_all.hpp"
#include "ouly/scheduler/parallel_for.hpp"
#include "ouly/scheduler/scheduler.hpp"
#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <type_traits>
#include <vector>
// NOLINTBEGIN
// The alias must resolve to the selected generation.
static_assert(std::is_same_v<ouly::scheduler, ouly::v3::scheduler>);
static_assert(std::is_same_v<ouly::task_context, ouly::v3::task_context>);
static_assert(std::is_same_v<ouly::workgroup, ouly::v3::workgroup>);

TEST_CASE("v3: task submission via ouly::scheduler alias", "[scheduler][version][v3]")
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

// Regression test: a worker only runs tasks from workgroups it belongs to (fixed
// membership). A submit to a "far" group whose members are all parked must wake one of
// those members promptly. With a notify_one() wake this could rouse an unrelated worker
// that immediately re-parks, leaving the only capable workers asleep until some later
// submit -- so a single round-trip would stall (effectively forever, since wait_for_tasks
// then parks too). We measure many sequential cross-group round-trips and bound the time.
TEST_CASE("v3: cross-group submits are picked up promptly", "[scheduler][version][v3]")
{
  ouly::scheduler scheduler;
  // Four 2-worker groups -> workers 0..7. The main thread is worker 0 (group 0) and is not
  // a member of groups 1..3, so it cannot execute their tasks itself; a far-group worker
  // must wake.
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.create_group(ouly::workgroup_id(1), 2, 2);
  scheduler.create_group(ouly::workgroup_id(2), 4, 2);
  scheduler.create_group(ouly::workgroup_id(3), 6, 2);
  scheduler.begin_execution();

  auto const& main_ctx = ouly::task_context::this_context::get();

  constexpr uint32_t    iterations = 200;
  std::atomic<uint32_t> done{0};

  auto const start = std::chrono::steady_clock::now();
  for (uint32_t i = 0; i < iterations; ++i)
  {
    // Let the far-group workers settle into a parked state before each submit so we
    // exercise the wake path rather than a still-spinning worker.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    auto const target = ouly::workgroup_id(1 + (i % 3));
    scheduler.submit(main_ctx, target,
                     [&done](ouly::task_context const&)
                     {
                       done.fetch_add(1, std::memory_order_relaxed);
                     });

    // Wait for this single task to be picked up and finished. A regression makes this spin
    // far beyond the bound below because the capable worker never wakes.
    while (done.load(std::memory_order_acquire) != (i + 1))
    {
      REQUIRE(std::chrono::steady_clock::now() - start < std::chrono::seconds(30));
      std::this_thread::yield();
    }
  }

  scheduler.end_execution();

  REQUIRE(done.load() == iterations);
}

TEST_CASE("v3: parallel_for via ouly::scheduler alias", "[scheduler][version][v3]")
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
