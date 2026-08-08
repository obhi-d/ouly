// SPDX-License-Identifier: MIT

#include "catch2/catch_all.hpp"
#include "ouly/scheduler/auto_parallel_for.hpp"
#include "ouly/scheduler/co_task.hpp"
#include "ouly/scheduler/scheduler.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <new>
#include <semaphore>
#include <stdexcept>
#include <vector>

namespace
{

struct test_auto_partitioner_traits : ouly::auto_partitioner_traits
{
  static constexpr uint32_t grain_size           = 2;
  static constexpr uint32_t sequential_threshold = 1;
};

class counting_allocator
{
public:
  auto allocate(std::size_t size) -> void*
  {
    allocations_.fetch_add(1, std::memory_order_relaxed);
    return ::operator new(size);
  }

  void deallocate(void* ptr, std::size_t /*size*/) noexcept
  {
    deallocations_.fetch_add(1, std::memory_order_relaxed);
    ::operator delete(ptr);
  }

  [[nodiscard]] auto balanced() const noexcept -> bool
  {
    return allocations_.load(std::memory_order_relaxed) == deallocations_.load(std::memory_order_relaxed);
  }

private:
  std::atomic<uint32_t> allocations_{0};
  std::atomic<uint32_t> deallocations_{0};
};

auto coroutine_leaf([[maybe_unused]] ouly::scheduler_allocator allocator, int value) -> ouly::co_task<int>
{
  co_return value;
}

auto coroutine_chain([[maybe_unused]] ouly::scheduler_allocator allocator, std::atomic<int>& result,
                     std::binary_semaphore& done) -> ouly::co_task<void>
{
  auto child = coroutine_leaf(allocator, 20);
  result.store(co_await child + 22, std::memory_order_release);
  done.release();
  co_return;
}

auto coroutine_await_task(ouly::task<int> input, std::atomic<int>& result, std::binary_semaphore& done)
  -> ouly::co_task<void>
{
  result.store(co_await input, std::memory_order_release);
  done.release();
  co_return;
}

auto coroutine_task_value(ouly::task<int> input) -> ouly::co_task<int>
{
  co_return co_await input;
}

auto coroutine_failure([[maybe_unused]] ouly::scheduler_allocator allocator) -> ouly::co_task<int>
{
  throw std::runtime_error("expected");
  co_return 0;
}

} // namespace

TEST_CASE("scheduler tasks chain queued continuations", "[scheduler][task][then]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 4);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  auto producer = ouly::submit_task(ctx, []() -> int { return 21; });
  auto doubled  = producer.then(ctx, [](int value) -> int { return value * 2; });
  auto text     = producer.then(ctx, [](int value) -> std::size_t { return static_cast<std::size_t>(value); });

  REQUIRE(doubled.get(ctx) == 42);
  REQUIRE(text.get(ctx) == 21);
  scheduler.end_execution();
}

TEST_CASE("when_all completes after heterogeneous tasks and propagates exceptions", "[scheduler][task][when_all]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 4);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  auto first  = ouly::submit_task(ctx, []() -> int { return 1; });
  auto second = ouly::submit_task(ctx, []() {});
  auto all    = ouly::when_all(ctx, first, second);
  all.get(ctx);

  std::vector<ouly::task<int>> range;
  range.emplace_back(ouly::submit_task(ctx, []() -> int { return 2; }));
  range.emplace_back(ouly::submit_task(ctx, []() -> int { return 3; }));
  ouly::when_all(ctx, range).get(ctx);

  auto failed = ouly::submit_task(ctx,
                                  []() -> int
                                  {
                                    throw std::runtime_error("expected");
                                  });
  auto combined = ouly::when_all(ctx, first, failed);
  REQUIRE_THROWS_AS(combined.get(ctx), std::runtime_error);
  scheduler.end_execution();
}

TEST_CASE("task_scope joins all children before returning", "[scheduler][task][scope]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 4);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  std::atomic<uint32_t> count{0};
  ouly::task_scope      scope;
  for (uint32_t index = 0; index < 1000; ++index)
  {
    scope.run(ctx,
              [&count]()
              {
                count.fetch_add(1, std::memory_order_relaxed);
              });
  }

  scope.join(ctx);
  REQUIRE(scope.is_complete());
  REQUIRE(count.load(std::memory_order_relaxed) == 1000);

  scope.reset();
  scope.run(ctx,
            [&count]()
            {
              count.fetch_add(1, std::memory_order_relaxed);
            });
  scope.join(ctx);
  REQUIRE(count.load(std::memory_order_relaxed) == 1001);

  scope.reset();
  scope.run(ctx,
            []()
            {
              throw std::runtime_error("expected");
            });
  REQUIRE_THROWS_AS(scope.join(ctx), std::runtime_error);
  scheduler.end_execution();
}

TEST_CASE("task_scope cooperative join executes only its own children", "[scheduler][task][scope]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 1);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  std::atomic_bool unrelated{false};
  std::atomic_bool child{false};
  scheduler.submit(ctx,
                   [&unrelated](ouly::task_context const&) noexcept
                   {
                     unrelated.store(true, std::memory_order_release);
                   });

  ouly::task_scope scope;
  scope.run(ctx,
            [&child]() noexcept
            {
              child.store(true, std::memory_order_release);
            });
  scope.join(ctx);

  REQUIRE(child.load(std::memory_order_acquire));
  REQUIRE_FALSE(unrelated.load(std::memory_order_acquire));
  scheduler.wait_for_tasks();
  REQUIRE(unrelated.load(std::memory_order_acquire));
  scheduler.end_execution();
}

TEST_CASE("task_scope joins descendants submitted while joining", "[scheduler][task][scope][nested]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 1);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  std::atomic<uint32_t> count{0};
  ouly::task_scope      scope;
  scope.run(ctx,
            [&scope, &count](ouly::task_context const& child_ctx)
            {
              count.fetch_add(1, std::memory_order_relaxed);
              scope.run(child_ctx,
                        [&count]()
                        {
                          count.fetch_add(1, std::memory_order_relaxed);
                        });
            });
  scope.join(ctx);

  REQUIRE(count.load(std::memory_order_relaxed) == 2);
  scheduler.end_execution();
}

TEST_CASE("auto_parallel_for joins only its structured children", "[scheduler][task][scope][parallel_for]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 1);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  std::atomic_bool unrelated{false};
  scheduler.submit(ctx,
                   [&unrelated](ouly::task_context const&) noexcept
                   {
                     unrelated.store(true, std::memory_order_release);
                   });

  std::vector<uint32_t> values(512, 0);
  ouly::auto_parallel_for(
    [](uint32_t& value, ouly::task_context const&) noexcept
    {
      ++value;
    },
    values,
    ctx,
    test_auto_partitioner_traits{});

  REQUIRE(std::ranges::all_of(values, [](uint32_t value) { return value == 1; }));
  REQUIRE_FALSE(unrelated.load(std::memory_order_acquire));
  scheduler.wait_for_tasks();
  REQUIRE(unrelated.load(std::memory_order_acquire));
  scheduler.end_execution();
}

TEST_CASE("task state and continuations use a custom allocator", "[scheduler][task][allocator]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  counting_allocator allocator;
  {
    auto first = ouly::submit_task(ctx, ouly::scheduler_allocator(allocator), []() -> int { return 40; });
    auto last  = first.then(ctx, ctx.get_workgroup(), ouly::scheduler_allocator(allocator),
                           [](int value) -> int { return value + 2; });
    REQUIRE(last.get(ctx) == 42);
  }

  scheduler.wait_for_tasks();
  REQUIRE(allocator.balanced());
  scheduler.end_execution();
}

TEST_CASE("detached coroutine chains use custom allocation", "[scheduler][coroutine][allocator][detached]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  counting_allocator      allocator;
  std::atomic<int>         result{0};
  std::binary_semaphore    done{0};
  auto coroutine = coroutine_chain(ouly::scheduler_allocator(allocator), result, done);
  scheduler.submit(ctx, std::move(coroutine));
  ctx.cooperative_wait(done);
  scheduler.wait_for_tasks();

  REQUIRE(result.load(std::memory_order_acquire) == 42);
  REQUIRE(allocator.balanced());
  scheduler.end_execution();
}

TEST_CASE("borrowed coroutines propagate values", "[scheduler][coroutine][allocator][borrowed][value]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  counting_allocator allocator;
  {
    auto borrowed = coroutine_leaf(ouly::scheduler_allocator(allocator), 42);
    scheduler.submit(ctx, borrowed);
    REQUIRE(borrowed.cooperative_wait(ctx) == 42);
  }
  scheduler.wait_for_tasks();
  REQUIRE(allocator.balanced());
  scheduler.end_execution();
}

TEST_CASE("borrowed coroutines propagate exceptions", "[scheduler][coroutine][allocator][borrowed][exception]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  counting_allocator allocator;
  {
    auto failed = coroutine_failure(ouly::scheduler_allocator(allocator));
    scheduler.submit(ctx, failed);
    REQUIRE_THROWS_AS(failed.cooperative_wait(ctx), std::runtime_error);
  }
  {
    auto failed = coroutine_failure(ouly::scheduler_allocator(allocator));
    REQUIRE_THROWS_AS(failed.wait(), std::runtime_error);
  }
  scheduler.wait_for_tasks();
  REQUIRE(allocator.balanced());
  scheduler.end_execution();
}

TEST_CASE("coroutines await scheduler tasks", "[scheduler][coroutine][task]")
{
  ouly::scheduler scheduler;
  scheduler.create_group(ouly::workgroup_id(0), 0, 2);
  scheduler.begin_execution();
  auto const& ctx = ouly::task_context::this_context::get();

  std::atomic<int>      result{0};
  std::binary_semaphore task_done{0};
  auto input = ouly::submit_task(ctx, []() -> int { return 84; });
  auto awaiter = coroutine_await_task(input, result, task_done);
  scheduler.submit(ctx, std::move(awaiter));
  ctx.cooperative_wait(task_done);
  REQUIRE(result.load(std::memory_order_acquire) == 84);

  auto direct_input = ouly::submit_task(ctx, []() -> int { return 126; });
  auto direct = coroutine_task_value(direct_input);
  REQUIRE(direct.cooperative_wait(ctx) == 126);
  scheduler.end_execution();
}
