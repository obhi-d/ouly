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
          auto r = std::find(result.begin(), result.end(), v);
          if (r != result.end())
            result.erase(r);
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
     [[maybe_unused]] auto id = ouly::worker_id::get();
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
// NOLINTEND